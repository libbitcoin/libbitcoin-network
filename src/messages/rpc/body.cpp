/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <bitcoin/network/messages/rpc/body.hpp>

#include <memory>
#include <utility>
#include <variant>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

using namespace system;
using namespace network::error;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
BC_PUSH_WARNING(NO_UNGUARDED_POINTERS)
BC_PUSH_WARNING(NO_POINTER_ARITHMETIC)

constexpr bool is_rpc_terminator(char character) NOEXCEPT
{
    // Document terminal characters (object always, array when batching).
    return character == '}' || character == ']';
}

constexpr bool is_electrum_terminator(char character) NOEXCEPT
{
    // Electrum rpc messages require a trailing newline.
    return character == '\n';
}

constexpr bool is_json_whitespace(char character) NOEXCEPT
{
    return character == ' ' || character == '\t' ||
        character == '\r' || character == '\n';
}

// rpc::body<request_t>::reader
// ----------------------------------------------------------------------------
// A batch is never materialized. Each read yields one element (message), with
// batch open riding along on the first element read (value_.changed), and
// batch close completing a read without a message (value_.changed with
// value_.batch). The caller provides current state (value_.batch) and enables
// delimiter recognition (value_.batchable). Delimiters are consumed here and
// never surface to the parser, so each element parses as its own document.

template <>
size_t body<rpc::request_t>::reader::
put(const buffer_type& buffer, boost_code& ec) NOEXCEPT
{
    // Null and empty guarded in base reader.
    if (is_zero(buffer.size()) || is_null(buffer.data()))
        return base::reader::put(buffer, ec);

    const auto data = pointer_cast<const char>(buffer.data());
    const auto size = buffer.size();
    size_t index{};
    ec.clear();

    // Termination applies to singleton and batch close reads only.
    const auto requires_terminator = [this]() NOEXCEPT
    {
        return terminated_ && (closed_ || (!value_.batch && !opened_));
    };

    // Prologue: consume whitespace and batch delimiters until element start,
    // batch close, or buffer exhaustion.
    if (!started_ && !closed_ && value_.batchable)
    {
        for (; index < size; ++index)
        {
            const auto character = data[index];
            if (is_json_whitespace(character))
                continue;

            if (character == '[' && !opened_ && !value_.batch)
            {
                // Batch open rides along with the first element read.
                opened_ = true;
                value_.changed = true;
                continue;
            }

            if (character == ',')
            {
                // Element separator (between batched elements only).
                if (!value_.batch || separated_)
                {
                    ec = code{ error::jsonrpc_batch_malformed };
                    return index;
                }

                separated_ = true;
                continue;
            }

            if (character == ']')
            {
                // Batch open immediately closed (no elements).
                if (opened_)
                {
                    ec = code{ error::jsonrpc_batch_empty };
                    return index;
                }

                // Batch close (only where a separator could occur).
                if (!value_.batch || separated_)
                {
                    ec = code{ error::jsonrpc_batch_malformed };
                    return index;
                }

                // Batch close completes this read without a message.
                closed_ = true;
                value_.changed = true;
                ++index;
                break;
            }

            // Nested batch open, or batched element without separator.
            if (character == '[' || (value_.batch && !separated_))
            {
                ec = code{ error::jsonrpc_batch_malformed };
                return index;
            }

            // Element start (this character is the parser's).
            started_ = true;
            break;
        }

        // Buffer exhausted within prologue.
        if (!started_ && !closed_)
            return index;
    }
    else if (!started_ && !closed_)
    {
        // Batch disallowed, buffer is element data (array is a parse).
        started_ = true;
    }

    // Element: pass buffer remainder to the json parser.
    if (started_ && !base::reader::done())
    {
        const auto start = index;
        const buffer_type remainder{ &data[index], size - index };
        const auto parsed = base::reader::put(remainder, ec);

        index += parsed;
        if (ec || !base::reader::done())
            return index;

        // Batched element reads complete without termination.
        if (!requires_terminator())
            return index;

        // boost::json consumes whitespace, and leaves any subsequent chars
        // unparsed, so a terminator it consumed must be in the parsed segment.
        for (auto scan = index; scan > start; --scan)
        {
            const auto character = data[sub1(scan)];
            if (is_electrum_terminator(character))
            {
                has_terminator_ = true;
                return index;
            }

            if (is_rpc_terminator(character))
                break;
        }
    }

    // Terminator: consume whitespace through the terminator, allowing padded
    // termination (electrum traffic shaping) and split reads.
    if (requires_terminator() && !has_terminator_)
    {
        for (; index < size; ++index)
        {
            const auto character = data[index];
            if (is_electrum_terminator(character))
            {
                has_terminator_ = true;
                return add1(index);
            }

            if (!is_json_whitespace(character))
            {
                ec = code{ error::jsonrpc_reader_stall };
                return index;
            }
        }
    }

    return index;
}

template <>
bool body<rpc::request_t>::reader::
done() const NOEXCEPT
{
    // Parser may be done but with terminator still outstanding.
    const auto complete = closed_ || base::reader::done();
    const auto required = terminated_ &&
        (closed_ || (!value_.batch && !opened_));

    return complete && (!required || has_terminator_);
}

template <>
void body<rpc::request_t>::reader::
finish(boost_code& ec) NOEXCEPT
{
    // Batch close completes a read without a message.
    if (closed_)
    {
        ec = done() ? boost_code{} : to_http_code(http_error_t::need_more);
        return;
    }

    // See notes in base reader.
    base::reader::finish(ec);
    if (ec) return;

    try
    {
        value_.message = value_to<rpc::request_t>(value_.model);
        value_.model.emplace_null();
    }
    catch (const boost::system::system_error& e)
    {
        // Primary exception type for parsing operations.
        ec = e.code();
    }
    catch (...)
    {
        ec = code{ error::jsonrpc_reader_exception };
    }

    // Set version default.
    if (value_.message.jsonrpc == version::undefined)
        value_.message.jsonrpc = version::v1;

    // Post-parse semantic validation.
    if (value_.message.method.empty())
    {
        ec = code{ error::jsonrpc_requires_method };
    }
    else if (value_.message.jsonrpc == version::v1)
    {
        if (!value_.message.params.has_value())
            ec = code{ error::jsonrpc_v1_requires_params };
        else if (!value_.message.id.has_value())
            ec = code{ error::jsonrpc_v1_requires_id };
        else if (!std::holds_alternative<array_t>(
            value_.message.params.value()))
            ec = code{ error::jsonrpc_v1_requires_array_params };
    }
    else if (value_.message.params.has_value() &&
            std::holds_alternative<value_t>(value_.message.params.value()))
    {
        if (!value_.strict)
        {
            // Convert non-standard rpc, required for Electrum compat.
            value_.message.params.emplace(array_t
            {
                std::get<value_t>(std::move(value_.message.params.value()))
            });
        }
        else
        {
            ec = code{ error::jsonrpc_params_not_collection };
        }
    }
}

// rpc::body<response_t>::reader (unused)
// ----------------------------------------------------------------------------

template <>
size_t body<rpc::response_t>::reader::
put(const buffer_type&, boost_code&) NOEXCEPT
{
    BC_ASSERT(false);
    return {};
}

template <>
void body<rpc::response_t>::reader::
finish(boost_code&) NOEXCEPT
{
    BC_ASSERT(false);
}

template <>
bool body<rpc::response_t>::reader::
done() const NOEXCEPT
{
    BC_ASSERT(false);
    return {};
}

// rpc::body<response_t>::writer
// ----------------------------------------------------------------------------

template <>
void body<rpc::response_t>::writer::
init(boost_code& ec) NOEXCEPT
{
    base::writer::init(ec);
    if (ec) return;

    try
    {
        boost::json::value_from(value_.message, value_.model);
    }
    catch (const boost::system::system_error& e)
    {
        // Primary exception type for parsing operations.
        ec = e.code();
        return;
    }
    catch (...)
    {
        ec = code{ error::jsonrpc_writer_exception };
        return;
    }

    set_terminator_ = false;
    serializer_.reset(&value_.model);
}

template <>
body<rpc::response_t>::writer::out_buffer
body<rpc::response_t>::writer::
get(boost_code& ec) NOEXCEPT
{
    auto out = base::writer::done() ? out_buffer{} : base::writer::get(ec);
    if (ec) return out;

    // Override json reader !more so terminator can be added.
    if (out.has_value())
    {
        out.value().second = true;
        return out;
    }

    // Add terminator and signal done.
    set_terminator_ = true;
    using namespace boost::asio;
    static constexpr auto line = '\n';
    return out_buffer{ std::make_pair(buffer(&line, sizeof(line)), false) };
}

template <>
bool body<rpc::response_t>::writer::
done() const NOEXCEPT
{
    // Done is redundant with !out.second, but provides a cleaner interface.
    return base::writer::done() && (!terminate_ || set_terminator_);
}

// rpc::body<request_t>::writer
// ----------------------------------------------------------------------------

template <>
void body<rpc::request_t>::writer::
init(boost_code& ec) NOEXCEPT
{
    base::writer::init(ec);
    if (ec) return;

    try
    {
        boost::json::value_from(value_.message, value_.model);
    }
    catch (const boost::system::system_error& e)
    {
        // Primary exception type for parsing operations.
        ec = e.code();
        return;
    }
    catch (...)
    {
        ec = code{ error::jsonrpc_writer_exception };
        return;
    }

    set_terminator_ = false;
    serializer_.reset(&value_.model);
}

template <>
body<rpc::request_t>::writer::out_buffer
body<rpc::request_t>::writer::
get(boost_code& ec) NOEXCEPT
{
    auto out = base::writer::done() ? out_buffer{} : base::writer::get(ec);
    if (ec) return out;

    // Override json reader !more so terminator can be added.
    if (out.has_value())
    {
        out.value().second = true;
        return out;
    }

    // Add terminator and signal done.
    set_terminator_ = true;
    using namespace boost::asio;
    static constexpr auto line = '\n';
    return out_buffer{ std::make_pair(buffer(&line, sizeof(line)), false) };
}

template <>
bool body<rpc::request_t>::writer::
done() const NOEXCEPT
{
    // Done is redundant with !out.second, but provides a cleaner interface.
    return base::writer::done() && (!terminate_ || set_terminator_);
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace rpc
} // namespace network
} // namespace libbitcoin
