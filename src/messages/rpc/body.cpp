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
//
// A batchable read delivers each message from put by pausing the parse
// (need_buffer) at the element boundary and re-arming for the next element,
// uniformly across framings: a beast (http) parse pauses natively, a stream
// read completes on the pause with buffer residue carried to the next read.
// For batchable reads finish only validates message-end state.

// Convert the parsed model to a request message and validate.
static void to_request(rpc::request& value, boost_code& ec) NOEXCEPT
{
    try
    {
        value.message = value_to<rpc::request_t>(value.model);
        value.model.emplace_null();
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
    if (value.message.jsonrpc == version::undefined)
        value.message.jsonrpc = version::v1;

    // Post-parse semantic validation.
    if (value.message.method.empty())
    {
        ec = code{ error::jsonrpc_requires_method };
    }
    else if (value.message.jsonrpc == version::v1)
    {
        if (!value.message.params.has_value())
            ec = code{ error::jsonrpc_v1_requires_params };
        else if (!value.message.id.has_value())
            ec = code{ error::jsonrpc_v1_requires_id };
        else if (!std::holds_alternative<array_t>(
            value.message.params.value()))
            ec = code{ error::jsonrpc_v1_requires_array_params };
    }
    else if (value.message.params.has_value() &&
        std::holds_alternative<value_t>(value.message.params.value()))
    {
        if (!value.strict)
        {
            // Convert non-standard rpc, required for Electrum compat.
            value.message.params.emplace(array_t
            {
                std::get<value_t>(std::move(value.message.params.value()))
            });
        }
        else
        {
            ec = code{ error::jsonrpc_params_not_collection };
        }
    }
}

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
                if ((!value_.batch && !opened_) || separated_ ||
                    (opened_ && !delivered_))
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
                if (opened_ && !delivered_)
                {
                    ec = code{ error::jsonrpc_batch_empty };
                    return index;
                }

                // Batch close (only where a separator could occur).
                if ((!value_.batch && !opened_) || separated_)
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

            // Nested batch open, batched element without separator, or a
            // second document following a delivered (unbatched) singleton.
            if (character == '[' ||
                ((value_.batch || opened_) &&
                    !separated_ && !(opened_ && !delivered_)) ||
                (delivered_ && !opened_ && !value_.batch))
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

    // Epilogue: consume whitespace trailing a delivered batch close.
    if (closed_ && signaled_)
    {
        for (; index < size; ++index)
        {
            if (!is_json_whitespace(data[index]))
            {
                ec = code{ error::jsonrpc_batch_malformed };
                return index;
            }
        }
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

        // boost::json consumes whitespace, and leaves any subsequent chars
        // unparsed, so a terminator it consumed must be in the parsed segment.
        if (requires_terminator())
            for (auto scan = index; scan > start; --scan)
            {
                const auto character = data[sub1(scan)];
                if (is_electrum_terminator(character))
                {
                    has_terminator_ = true;
                    break;
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
                ++index;
                break;
            }

            if (!is_json_whitespace(character))
            {
                ec = code{ error::jsonrpc_reader_stall };
                return index;
            }
        }
    }

    // Delivery: batchable messages are delivered from put by pausing.
    if (value_.batchable && (!requires_terminator() || has_terminator_))
    {
        if (closed_ && !signaled_)
        {
            // Batch close is delivered without a message.
            signaled_ = true;
            ec = to_http_code(http_error_t::need_buffer);
        }
        else if (started_ && base::reader::done())
        {
            // Convert and validate the parsed element (releases the model).
            base::reader::finish(ec);
            if (ec) return index;

            to_request(value_, ec);
            if (ec) return index;

            // Deliver the message and re-arm for the next element.
            delivered_ = true;
            started_ = false;
            separated_ = false;
            has_terminator_ = false;
            ec = to_http_code(http_error_t::need_buffer);
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
    // Batchable messages are delivered from put, so finish only validates
    // message-end state: delivered close or delivered singleton (no batch).
    if (value_.batchable && !started_)
    {
        ec = ((closed_ && signaled_) ||
            (!closed_ && delivered_ && !opened_)) ? boost_code{} :
                to_http_code(http_error_t::need_more);
        return;
    }

    // See notes in base reader.
    base::reader::finish(ec);
    if (ec) return;

    to_request(value_, ec);
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
// A batch is never materialized. Each write emits one part, with framing
// derived from caller-assigned batch state (value_.batch is the state before
// the part, value_.changed indicates transition). The open part is prefixed
// with '[', continuation parts with ',', and the close part is framing and
// termination only (no message), mirroring the reader's message-less close.
// Termination applies to singleton and batch close parts only.

template <>
void body<rpc::response_t>::writer::
init(boost_code& ec) NOEXCEPT
{
    set_terminator_ = false;
    set_prefix_ = false;
    set_close_ = false;

    // Batch close part carries no message.
    if (closes_batch(value_))
    {
        ec.clear();
        return;
    }

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

    serializer_.reset(&value_.model);
}

template <>
body<rpc::response_t>::writer::out_buffer
body<rpc::response_t>::writer::
get(boost_code& ec) NOEXCEPT
{
    using namespace boost::asio;
    static constexpr auto open = '[';
    static constexpr auto comma = ',';
    static constexpr auto close = ']';
    static constexpr auto line = '\n';

    // Buffer a single static framing character.
    const auto single = [](const char& character, bool more) NOEXCEPT
    {
        return out_buffer{ std::make_pair(buffer(&character, one), more) };
    };

    // Emit batch framing (open or separator) preceding the part.
    if (!set_prefix_)
    {
        set_prefix_ = true;

        if (opens_batch(value_))
            return single(open, true);

        if (continues_batch(value_))
            return single(comma, true);
    }

    // Batch close part is framing and termination only (no message).
    if (closes_batch(value_))
    {
        if (!set_close_)
        {
            set_close_ = true;
            return single(close, true);
        }

        set_terminator_ = true;
        return single(line, false);
    }

    auto out = base::writer::done() ? out_buffer{} : base::writer::get(ec);

    // Batched parts are terminated by the batch close part (not per part).
    if (ec || opens_batch(value_) || continues_batch(value_))
        return out;

    // Override json reader !more so terminator can be added.
    if (out.has_value())
    {
        out.value().second = true;
        return out;
    }

    // Add terminator and signal done.
    set_terminator_ = true;
    return single(line, false);
}

template <>
bool body<rpc::response_t>::writer::
done() const NOEXCEPT
{
    // Batch close part is done when framing and termination are written.
    if (closes_batch(value_))
        return set_close_ && set_terminator_;

    // Batched parts are done on serialization (terminator rides on close).
    // Done is redundant with !out.second, but provides a cleaner interface.
    return base::writer::done() && (opens_batch(value_) ||
        continues_batch(value_) || !terminate_ || set_terminator_);
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
