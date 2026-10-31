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
#ifndef LIBBITCOIN_NETWORK_IMPL_MESSAGES_HTTP_PARSER_IPP
#define LIBBITCOIN_NETWORK_IMPL_MESSAGES_HTTP_PARSER_IPP

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace http {

#define TEMPLATE template <bool IsRequest, class Body>
#define CLASS parser<IsRequest, Body>

constexpr std::string_view line_end{ "\r\n" };
constexpr std::string_view section_end{ "\r\n\r\n" };

TEMPLATE
CLASS::parser() NOEXCEPT
  : parser(message_type{})
{
}

TEMPLATE
CLASS::parser(message_type&& message) NOEXCEPT
  : message_(std::move(message)),
    reader_(message_.base(), message_.body())
{
}

TEMPLATE
void CLASS::header_limit(size_t value) NOEXCEPT
{
    header_limit_ = value;
}

TEMPLATE
void CLASS::body_limit(size_t value) NOEXCEPT
{
    body_limit_ = value;
}

TEMPLATE
size_t CLASS::put(const asio::const_buffer& buffer, boost_code& ec) NOEXCEPT
{
    using namespace system;
    using namespace network::error;
    const std::string_view data{ pointer_cast<const char>(buffer.data()),
        buffer.size() };

    size_t used{};
    received_ = received_ || !data.empty();
    ec.clear();
    while (state_ != state::done && !ec)
    {
        const auto rest = data.substr(used);
        const auto before = state_;
        size_t step{};
        switch (state_)
        {
            case state::start_line:
                step = put_start_line(rest, ec);
                break;
            case state::fields:
                step = put_fields(rest, ec);

                // The parse pauses once the header is complete.
                if (!ec && state_ != state::fields)
                    return used + step;

                break;
            case state::body:
            case state::body_to_eof:
            case state::chunk_body:
                step = put_body(rest, ec);
                break;
            case state::chunk_header:
                step = put_chunk_header(rest, ec);
                break;
            case state::trailer:
                step = put_trailer(rest, ec);
                break;
            case state::done:
                break;
        }

        used += step;
        if (!ec && is_zero(step) && state_ == before)
            ec = to_http_code(http_error_t::need_more);
    }

    return used;
}

TEMPLATE
void CLASS::put_eof(boost_code& ec) NOEXCEPT
{
    using namespace network::error;
    if (state_ == state::body_to_eof)
    {
        finish(ec);
        return;
    }

    if (state_ == state::done)
    {
        ec.clear();
        return;
    }

    ec = to_http_code(received_ ? http_error_t::partial_message :
        http_error_t::end_of_stream);
}

TEMPLATE
bool CLASS::is_header_done() const NOEXCEPT
{
    return state_ != state::start_line && state_ != state::fields;
}

TEMPLATE
bool CLASS::is_done() const NOEXCEPT
{
    return state_ == state::done;
}

TEMPLATE
void CLASS::skip(bool value) NOEXCEPT
{
    skip_ = value;
}

TEMPLATE
typename CLASS::message_type& CLASS::get() NOEXCEPT
{
    return message_;
}

TEMPLATE
typename CLASS::message_type CLASS::release() NOEXCEPT
{
    return std::move(message_);
}

// private
// ----------------------------------------------------------------------------

// The header is parsed within the bytes remaining to the header limit.
TEMPLATE
std::string_view CLASS::header_window(
    const std::string_view& data) const NOEXCEPT
{
    if (is_zero(header_limit_))
        return data;

    return data.substr(0, header_limit_ - std::min(header_limit_, header_));
}

// An incomplete header element that fills the window exceeds the limit.
TEMPLATE
void CLASS::header_limited(const std::string_view& data,
    boost_code& ec) const NOEXCEPT
{
    using namespace network::error;
    if (ec == http_error_t::need_more && !is_zero(header_limit_) &&
        data.size() >= header_limit_ - std::min(header_limit_, header_))
        ec = to_http_code(http_error_t::header_limit);
}

TEMPLATE
size_t CLASS::put_start_line(const std::string_view& data,
    boost_code& ec) NOEXCEPT
{
    if (data.empty())
        return {};

    const auto used = parse_start_line(message_.base(), header_window(data),
        ec);

    header_limited(data, ec);
    if (ec)
        return {};

    header_ += used;
    state_ = state::fields;
    return used;
}

TEMPLATE
size_t CLASS::put_fields(const std::string_view& data,
    boost_code& ec) NOEXCEPT
{
    size_t used{};
    bool end{};
    while (!end)
    {
        const auto rest = data.substr(used);
        const auto step = parse_field(&message_.base(), frame_, end,
            header_window(rest), ec);

        header_limited(rest, ec);
        if (ec)
            return used;

        header_ += step;
        used += step;
    }

    finish_header(ec);
    return used;
}

TEMPLATE
void CLASS::finish_header(boost_code& ec) NOEXCEPT
{
    using namespace network::error;
    bool bodiless{ skip_ };
    if constexpr (!IsRequest)
        bodiless = bodiless || message_.bodiless();

    if (bodiless)
    {
        finish(ec);
        return;
    }

    if (frame_.length.has_value())
    {
        remaining_ = frame_.length.value();
        if (is_zero(remaining_))
        {
            finish(ec);
            return;
        }

        if (!is_zero(body_limit_) && remaining_ > body_limit_)
        {
            ec = to_http_code(http_error_t::body_limit);
            return;
        }

        state_ = state::body;
        reader_.init(frame_.length, ec);
        return;
    }

    if (frame_.chunked)
    {
        state_ = state::chunk_header;
        reader_.init({}, ec);
        return;
    }

    // A response so encoded is read to eof (rfc9112 6.3).
    if (frame_.encoded)
    {
        if constexpr (IsRequest)
        {
            ec = to_http_code(http_error_t::bad_transfer_encoding);
        }
        else
        {
            state_ = state::body_to_eof;
            reader_.init({}, ec);
        }

        return;
    }

    // A request without framing has no body.
    if constexpr (IsRequest)
    {
        finish(ec);
    }
    else
    {
        state_ = state::body_to_eof;
        reader_.init({}, ec);
    }
}

TEMPLATE
size_t CLASS::put_body(const std::string_view& data, boost_code& ec) NOEXCEPT
{
    using namespace system;
    using namespace network::error;
    const auto delimited = state_ != state::body_to_eof;

    // A body exhausted at a reader pause completes upon resumption.
    if (delimited && is_zero(remaining_))
    {
        complete_body(ec);
        return {};
    }

    auto size = data.size();
    if (delimited)
        size = limit<size_t>(std::min<uint64_t>(size, remaining_));

    if (!delimited && !is_zero(body_limit_) && total_ + size > body_limit_)
    {
        ec = to_http_code(http_error_t::body_limit);
        return {};
    }

    if (is_zero(size))
        return {};

    const auto used = reader_.put({ data.data(), size }, ec);
    total_ += used;
    if (delimited)
        remaining_ -= used;

    if (!ec && delimited && is_zero(remaining_))
        complete_body(ec);

    return used;
}

TEMPLATE
void CLASS::complete_body(boost_code& ec) NOEXCEPT
{
    if (state_ == state::chunk_body)
        state_ = state::chunk_header;
    else
        finish(ec);
}

// The line end following chunk data is parsed with the next chunk header. A
// chunk line is bounded by the header limit.
TEMPLATE
size_t CLASS::put_chunk_header(const std::string_view& data,
    boost_code& ec) NOEXCEPT
{
    using namespace network::error;
    if (data.size() < line_end.size())
        return {};

    size_t index{};
    if (expect_crlf_)
    {
        if (!data.starts_with(line_end))
        {
            ec = to_http_code(http_error_t::bad_chunk);
            return {};
        }

        index = line_end.size();
    }

    size_t text{};
    const auto line_size = find_chunk_line(text, data.substr(index), ec);
    if (ec)
    {
        if (ec == http_error_t::need_more)
        {
            ec.clear();
            if (!is_zero(header_limit_) && data.size() >= header_limit_)
                ec = to_http_code(http_error_t::bad_chunk_extension);
        }

        return {};
    }

    const auto line = data.substr(index, text);
    uint64_t size{};
    const auto digits = parse_chunk_size(size, line, ec);
    if (ec)
        return {};

    if (!is_zero(body_limit_) && size > body_limit_ - std::min<uint64_t>(
        body_limit_, total_))
    {
        ec = to_http_code(http_error_t::body_limit);
        return {};
    }

    if (!valid_chunk_extensions(line, digits))
    {
        ec = to_http_code(http_error_t::bad_chunk_extension);
        return {};
    }

    remaining_ = size;
    expect_crlf_ = true;
    state_ = is_zero(size) ? state::trailer : state::chunk_body;

    // The trailer (and its final empty line) is parsed within the header
    // limit, plus the final line end.
    if (is_zero(size))
        header_ = system::floored_subtract(header_, line_end.size());

    return index + line_size;
}

// Trailer fields frame like header fields, but are not retained.
TEMPLATE
size_t CLASS::put_trailer(const std::string_view& data,
    boost_code& ec) NOEXCEPT
{
    size_t used{};
    bool end{};
    while (!end)
    {
        const auto rest = data.substr(used);
        const auto step = parse_field(nullptr, frame_, end,
            header_window(rest), ec);

        header_limited(rest, ec);
        if (ec)
            return used;

        header_ += step;
        used += step;
    }

    finish(ec);
    return used;
}

TEMPLATE
void CLASS::finish(boost_code& ec) NOEXCEPT
{
    reader_.finish(ec);
    state_ = state::done;
}

#undef CLASS
#undef TEMPLATE

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
