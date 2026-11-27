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
#include <bitcoin/network/channels/channel_sv2.hpp>

#include <bitcoin/network/channels/channel_http.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/messages/sv2/registry.hpp>

namespace libbitcoin {
namespace network {

using namespace system;
using namespace messages::sv2;
using namespace std::placeholders;

// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// Start/stop/resume (started upon create).
// ----------------------------------------------------------------------------

// This should not be called internally.
void channel_sv2::stopping(const code& ec) NOEXCEPT
{
    BC_ASSERT(stranded());
    channel_http::stopping(ec);
    dispatcher_.stop(ec);
}

// Properties.
// ----------------------------------------------------------------------------

bool channel_sv2::handshaked() const NOEXCEPT
{
    BC_ASSERT(stranded());
    return handshaked_;
}

void channel_sv2::set_handshaked() NOEXCEPT
{
    BC_ASSERT(stranded());
    handshaked_ = true;
}

http::body::value_type channel_sv2::default_body() const NOEXCEPT
{
    // There is no forwarding constructor so assign and move.
    http::body::value_type value{};
    value = http::sv2_value
    {
        .maximum = options().maximum_request,
        .exact = true
    };

    return value;
}

// Read cycle.
// ----------------------------------------------------------------------------

void channel_sv2::handle_receive(const code& ec, size_t bytes,
    const http::request_ptr& request) NOEXCEPT
{
    BC_ASSERT(stranded());

    // The frame carries parse fault detail (the read code is generic).
    const auto& body = request->body();
    if (ec && (websocket() || downgraded()) && !stopped() &&
        body.contains<http::sv2_value>())
    {
        const auto& fault = body.get<http::sv2_value>().fault;
        LOGF("Stratum read failure [" << endpoint() << "] "
            << (fault ? fault : ec).message());

        stop(fault ? fault : ec);
        return;
    }

    channel_http::handle_receive(ec, bytes, request);
}

void channel_sv2::dispatch(const http::request_ptr& request) NOEXCEPT
{
    BC_ASSERT(stranded());

    // The service is websocket or tcp, a request without upgrade is refused.
    if (!websocket() && !downgraded())
    {
        http::response out{ http::status::upgrade_required, request->version() };
        out.keep_alive(false);
        out.prepare_payload();
        channel_http::send(std::move(out),
            std::bind(&channel_sv2::handle_refused,
                shared_from_base<channel_sv2>(), _1));
        return;
    }

    const auto& body = request->body();
    if (!body.contains<http::sv2_value>())
    {
        stop(error::invalid_message);
        return;
    }

    const auto& in = body.get<http::sv2_value>();
    const auto index = in.head.index();
    if (index == registry::unknown)
    {
        // A core protocol message that is not registered is not served.
        if (is_zero(in.head.extension()))
        {
            stop(error::unknown_message);
            return;
        }

        // An extension is not negotiated, so its message is discarded.
        open_gate();
        close_gate();
        return;
    }

    // The handshake precedes all other messages and is not repeated.
    if ((index < registry::handshakes) == handshaked_)
    {
        stop(error::protocol_violation);
        return;
    }

    LOGX("Recv " << registry::commands().at(index) << " from ["
        << endpoint() << "] (" << in.head.message_length << " bytes)");

    open_gate();

    // Notify subscribers of the new message.
    const auto code = dispatcher_.notify(rpc::request_t
    {
        .method = std::string{ registry::commands().at(index) },
        .params = { rpc::array_t{ in.payload } }
    });

    close_gate();

    if (code)
        stop(code);
}

// private
void channel_sv2::handle_refused(const code& ec) NOEXCEPT
{
    BC_ASSERT(stranded());
    stop(ec ? ec : code{ error::channel_dropped });
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
