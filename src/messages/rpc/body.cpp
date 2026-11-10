/**
 * Copyright (c) 2011-2026 libbitcoin developers
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

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

using namespace system;
using namespace network::error;
using json::is_json_whitespace;

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

// Convert the parsed model to a request message and validate. Tolerated
// jrpc violations are reported as lax flags (the channel validates policy).
static void to_request(rpc::request& value, code& ec) NOEXCEPT
{
    value.lax_params = false;
    value.lax_named = false;
    value.lax_batch = false;

    try
    {
        value.message = value_to<rpc::request_t>(value.model);
        value.model.emplace_null();
    }
    catch (const system::json::exception& e)
    {
        // Primary exception type for conversion operations.
        ec = e.code();
        return;
    }
    catch (...)
    {
        ec = code{ error::jsonrpc_reader_exception };
        return;
    }

    // Set version default.
    if (value.message.jsonrpc == version::undefined)
        value.message.jsonrpc = version::v1;

    // Post-parse semantic validation.
    if (value.message.jsonrpc == version::invalid)
    {
        ec = code{ error::jsonrpc_invalid_version };
    }
    else if (value.message.method.empty())
    {
        ec = code{ error::jsonrpc_requires_method };
    }
    else if (value.message.jsonrpc == version::v1)
    {
        if (!value.message.params.has_value())
            ec = code{ error::jsonrpc_v1_requires_params };
        else if (!value.message.id.has_value())
            ec = code{ error::jsonrpc_v1_requires_id };
        else if (std::holds_alternative<object_t>(
            value.message.params.value()))
            value.lax_named = true;
        else if (!std::holds_alternative<array_t>(
            value.message.params.value()))
            ec = code{ error::jsonrpc_v1_requires_array_params };
    }
    else if (value.message.params.has_value() &&
        std::holds_alternative<value_t>(value.message.params.value()))
    {
        // Tolerated: Electrum non-standard single value params (converted).
        value.lax_params = true;
        value.message.params.emplace(array_t
        {
            std::get<value_t>(std::move(value.message.params.value()))
        });
    }
}

// rpc::body<>::reader
// ----------------------------------------------------------------------------
// A batch is never materialized. Each read yields one element (message), with
// batch open riding along on the first element read (value_.changed), and
// batch close completing a read without a message (value_.changed with
// value_.batch). The caller provides current state (value_.batch), and the
// parse is always lax: batch framing is always recognized, with tolerated
// jrpc violations reported (lax flags) for channel policy. Delimiters are
// consumed here and never surface to the parser, so each element parses as
// its own document.
//
// Delivery is self-selecting on observed framing. A batched read delivers
// each message from put by pausing the parse (need_buffer) at the element
// boundary and re-arming for the next element, uniformly across framings: a
// http parse pauses natively, a stream read completes on the pause
// with buffer residue carried to the next read; for batched reads finish
// only validates message-end state. A singleton delivers classically from
// finish, compatible with one-shot body reads.

template <typename Message>
size_t body<Message>::reader::
put(const buffer_type& buffer, code& ec) NOEXCEPT
{
    if constexpr (is_same_type<Message, request_t>)
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
            return terminated_ &&
                (closed_ || (!this->value_.batch && !opened_));
        };

        // A completed element is re-fed (http parser) only with its trailer.
        const auto completed = started_ && base::reader::done();

        // Prologue: consume whitespace and batch delimiters until element
        // start, batch close, or buffer exhaustion.
        if (!started_ && !closed_)
        {
            for (; index < size; ++index)
            {
                const auto character = data[index];
                if (is_json_whitespace(character))
                    continue;

                if (character == '[' && !opened_ && !this->value_.batch)
                {
                    // Batch open rides along with the first element read.
                    opened_ = true;
                    this->value_.changed = true;
                    continue;
                }

                if (character == ',')
                {
                    // Element separator (between batched elements only).
                    if ((!this->value_.batch && !opened_) || separated_ ||
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
                    if ((!this->value_.batch && !opened_) || separated_)
                    {
                        ec = code{ error::jsonrpc_batch_malformed };
                        return index;
                    }

                    // Batch close completes this read without a message.
                    closed_ = true;
                    this->value_.changed = true;
                    ++index;
                    break;
                }

                // Nested batch open, or batched element without separator.
                if (character == '[' ||
                    ((this->value_.batch || opened_) &&
                        !separated_ && !(opened_ && !delivered_)))
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

            // The parser consumes whitespace, and leaves any subsequent chars
            // unparsed, so a terminator it consumed must be in the parsed
            // segment.
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

        // Trailer: consume whitespace trailing a non-terminated singleton
        // (http), other characters start the next message (stream) unless
        // re-fed.
        if (!terminated_ && !opened_ && !this->value_.batch && started_ &&
            base::reader::done())
        {
            for (; index < size; ++index)
            {
                if (is_json_whitespace(data[index]))
                    continue;

                if (!completed)
                    break;

                ec = code{ error::jsonrpc_reader_stall };
                return index;
            }
        }

        // Terminator: consume whitespace through the terminator, allowing
        // padded termination (electrum traffic shaping) and split reads.
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

        // Delivery: batched messages are delivered from put by pausing (a
        // singleton delivers classically, from finish following completion).
        if ((closed_ || opened_ || this->value_.batch) &&
            (!requires_terminator() || has_terminator_))
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

                to_request(this->value_, ec);
                if (ec) return index;

                // Tolerated: v1 has no batch (btcd), noted for the channel.
                this->value_.lax_batch =
                    (this->value_.message.jsonrpc == version::v1);

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
    else
    {
        BC_ASSERT(false);
        return {};
    }
}

template <typename Message>
bool body<Message>::reader::
done() const NOEXCEPT
{
    if constexpr (is_same_type<Message, request_t>)
    {
        // Parser may be done but with terminator still outstanding.
        const auto complete = closed_ || base::reader::done();
        const auto required = terminated_ &&
            (closed_ || (!this->value_.batch && !opened_));

        return complete && (!required || has_terminator_);
    }
    else
    {
        BC_ASSERT(false);
        return {};
    }
}

template <typename Message>
void body<Message>::reader::
finish(code& ec) NOEXCEPT
{
    if constexpr (is_same_type<Message, request_t>)
    {
        // Batched messages are delivered from put, so finish only validates
        // message-end state (delivered close, otherwise incomplete).
        if ((closed_ || opened_ || this->value_.batch) && !started_)
        {
            ec = (closed_ && signaled_) ? code{ error::success } :
                to_http_code(http_error_t::need_more);
            return;
        }

        // See notes in base reader.
        base::reader::finish(ec);
        if (ec) return;

        to_request(this->value_, ec);
    }
    else
    {
        BC_ASSERT(false);
    }
}

// rpc::body<>::writer
// ----------------------------------------------------------------------------
// A batch is never materialized. Each write emits one part, with framing
// derived from caller-assigned batch state (value_.batch is the state before
// the part, value_.changed indicates transition). The open part is prefixed
// with '[', continuation parts with ',', and the close part is framing and
// termination only (no message), mirroring the reader's message-less close.
// Termination applies to singleton and batch close parts only.

template <typename Message>
void body<Message>::writer::
init(code& ec) NOEXCEPT
{
    if constexpr (is_same_type<Message, request_t>)
    {
        if (is_zero(this->value_.size_hint))
            this->value_.size_hint = to_size(this->value_.message);

        this->terminator_ = this->value_.terminate ? '\n' : char{};
        base::writer::init(ec);
        if (ec) return;

        try
        {
            system::json::value_from(this->value_.message, this->value_.model);
        }
        catch (const system::json::exception& e)
        {
            // Primary exception type for conversion operations.
            ec = e.code();
            return;
        }
        catch (...)
        {
            ec = code{ error::jsonrpc_writer_exception };
            return;
        }

        this->serializer_.reset(&this->value_.model);
    }
    else
    {
        set_terminator_ = false;
        set_prefix_ = false;
        set_close_ = false;

        // Batch close part carries no message.
        if (closes_batch(this->value_))
        {
            ec.clear();
            return;
        }

        // A notification has no response, so its response object is empty.
        auto& message = this->value_.message;
        if ((message.jsonrpc == version::v2) && !message.id.has_value())
            message = { .jsonrpc = version::v2 };

        if (is_zero(this->value_.size_hint))
            this->value_.size_hint = to_size(this->value_.message);

        // Batched parts are terminated by the batch close part (not per part).
        const auto terminated = this->value_.terminate &&
            !opens_batch(this->value_) && !continues_batch(this->value_);

        this->terminator_ = terminated ? '\n' : char{};
        base::writer::init(ec);
        if (ec) return;

        try
        {
            system::json::value_from(this->value_.message, this->value_.model);
        }
        catch (const system::json::exception& e)
        {
            // Primary exception type for conversion operations.
            ec = e.code();
            return;
        }
        catch (...)
        {
            ec = code{ error::jsonrpc_writer_exception };
            return;
        }

        this->serializer_.reset(&this->value_.model);
    }
}

template <typename Message>
typename body<Message>::writer::out_buffer
body<Message>::writer::
get(code& ec) NOEXCEPT
{
    if constexpr (is_same_type<Message, request_t>)
    {
        return base::writer::done() ? out_buffer{} : base::writer::get(ec);
    }
    else
    {
        static constexpr auto open = '[';
        static constexpr auto comma = ',';
        static constexpr auto close = ']';
        static constexpr auto line = '\n';

        // Buffer a single static framing character.
        const auto single = [](const char& character, bool more) NOEXCEPT
        {
            return out_buffer
            {
                std::make_pair(io::buffer(&character, one), more)
            };
        };

        // Emit batch framing (open or separator) preceding the part.
        if (!set_prefix_)
        {
            set_prefix_ = true;

            if (opens_batch(this->value_))
                return single(open, true);

            if (continues_batch(this->value_))
                return single(comma, true);
        }

        // Batch close part is framing and termination only (no message).
        if (closes_batch(this->value_))
        {
            if (!set_close_)
            {
                set_close_ = true;
                return single(close, this->value_.terminate);
            }

            set_terminator_ = true;
            return single(line, false);
        }

        return base::writer::done() ? out_buffer{} : base::writer::get(ec);
    }
}

template <typename Message>
bool body<Message>::writer::
done() const NOEXCEPT
{
    if constexpr (is_same_type<Message, request_t>)
    {
        // Done is redundant with !out.second, but provides a cleaner interface.
        return base::writer::done();
    }
    else
    {
        // Batch close part is done when framing and termination are written.
        if (closes_batch(this->value_))
            return set_close_ && (!this->value_.terminate || set_terminator_);

        // Done is redundant with !out.second, but provides a cleaner interface.
        return base::writer::done();
    }
}

template struct BC_INSTANCE(BCT_API) body<request_t>;
template struct BC_INSTANCE(BCT_API) body<response_t>;

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace rpc
} // namespace network
} // namespace libbitcoin
