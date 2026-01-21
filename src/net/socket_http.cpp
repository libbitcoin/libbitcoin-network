/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/net/socket.hpp>

#include <memory>
#include <utility>
#include <variant>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {
    
using namespace system;
using namespace std::placeholders;
namespace beast = boost::beast;

// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// HTTP Read.
// ----------------------------------------------------------------------------

void socket::http_read(http::flat_buffer& buffer,
    http::request& request, count_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_http_read, shared_from_this(),
            std::ref(buffer), std::ref(request), std::move(handler)));
}

void socket::do_http_read(ref<http::flat_buffer> buffer,
    const ref<http::request>& request,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (is_websocket())
    {
        handler(error::service_stopped, {});
        return;
    }

    try
    {
        // Explicit parser override gives access to limits.
        auto parser = to_shared<http_parser>();

        // Causes http::error::body_limit on completion.
        parser->body_limit(maximum_);

        // Causes http::error::header_limit on completion.
        parser->header_limit(limit<uint32_t>(maximum_));

        VARIANT_DISPATCH_FUNCTION(beast::http::async_read,
            get_tcp(), buffer.get(), *parser,
                std::bind(&socket::handle_http_read,
                    shared_from_this(), _1, _2, request, parser, handler));
    }
    catch (const std::exception& e)
    {
        LOGF("Exception @ do_http_read: " << e.what());
        handler(error::operation_failed, {});
    }
}

void socket::handle_http_read(const boost_code& ec, size_t size,
    const ref<http::request>& request, const http_parser_ptr& parser,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (error::asio_is_canceled(ec))
    {
        handler(error::channel_stopped, size);
        return;
    }

    if (!ec && beast::websocket::is_upgrade(parser->get()))
    {
        handler(set_websocket(parser->get()), size);
        return;
    }

    if (!ec)
    {
        request.get() = parser->release();
    }

    const auto code = error::http_to_error_code(ec);
    if (code == error::unknown) logx("http", ec);
    handler(code, size);
}

// HTTP Write.
// ----------------------------------------------------------------------------

void socket::http_write(http::response& response,
    count_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_http_write, shared_from_this(),
            std::ref(response), std::move(handler)));
}

void socket::do_http_write(const ref<http::response>& response,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (is_websocket())
    {
        handler(error::service_stopped, {});
        return;
    }

    try
    {
        VARIANT_DISPATCH_FUNCTION(beast::http::async_write,
            get_tcp(), response.get(),
                std::bind(&socket::handle_http_write,
                    shared_from_this(), _1, _2, handler));
    }
    catch (const std::exception& e)
    {
        LOGF("Exception @ do_http_write: " << e.what());
        handler(error::operation_failed, {});
    }
}

void socket::handle_http_write(const boost_code& ec, size_t size,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (error::asio_is_canceled(ec))
    {
        handler(error::channel_stopped, size);
        return;
    }

    const auto code = error::http_to_error_code(ec);
    if (code == error::unknown) logx("http", ec);
    handler(code, size);
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
