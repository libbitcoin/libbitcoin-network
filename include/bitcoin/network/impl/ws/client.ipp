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
#ifndef LIBBITCOIN_NETWORK_IMPL_WS_CLIENT_IPP
#define LIBBITCOIN_NETWORK_IMPL_WS_CLIENT_IPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/http/bodies.hpp>
#include <bitcoin/network/messages/http/fields.hpp>
#include <bitcoin/network/messages/http/operations.hpp>

namespace libbitcoin {
namespace network {
namespace ws {

#define TEMPLATE template <typename Stream>
#define CLASS client<Stream>

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// The size of the handshake key nonce (rfc6455 4.1).
constexpr size_t nonce_size = 16;

TEMPLATE
CLASS::client(Stream& stream, size_t maximum) NOEXCEPT
  : stream_(stream), reader_(maximum, false)
{
}

TEMPLATE
code CLASS::handshake(response& out, const std::string& host,
    const std::string& target, const http::fields& extra) NOEXCEPT
{
    using namespace http;
    message<true, empty_body> request{ verb::get, target, version_1_1 };
    system::data_array<nonce_size> nonce{};
    system::maybe_random::fill(nonce);
    request.set(field::host, host);
    request.set(field::upgrade, "websocket");
    request.set(field::connection, "Upgrade");
    request.set(field::sec_websocket_key, system::encode_base64(nonce));
    request.set(field::sec_websocket_version, "13");
    for (const auto& item: extra)
        request.set(item.name, item.value);

    code ec{};
    http::write(stream_, request, ec);
    if (ec)
        return ec;

    http::read(stream_, input_, out, ec);
    if (ec)
        return ec;

    if (out.result() != status::switching_protocols)
        return error::upgrade_declined;

    if (out[field::sec_websocket_accept] != to_websocket_accept(request))
        return error::bad_sec_accept;

    return error::success;
}

TEMPLATE
code CLASS::handshake(const std::string& host,
    const std::string& target) NOEXCEPT
{
    response ignore{};
    return handshake(ignore, host, target);
}

TEMPLATE
code CLASS::write(const std::string_view& data, bool binary) NOEXCEPT
{
    return send(binary ? opcode::binary : opcode::text, data);
}

TEMPLATE
code CLASS::ping(const std::string_view& payload) NOEXCEPT
{
    return send(opcode::ping, payload);
}

TEMPLATE
code CLASS::pong(const std::string_view& payload) NOEXCEPT
{
    return send(opcode::pong, payload);
}

TEMPLATE
code CLASS::close(close_code value) NOEXCEPT
{
    const auto payload = close_payload(value);
    closing_ = true;
    if (const auto ec = send(opcode::close, { system::pointer_cast<const char>(
        payload.data()), payload.size() }))
        return ec;

    http::flat_buffer discard{};
    while (true)
    {
        const auto ec = read(discard);
        if (ec == error::websocket_closed)
            return error::success;

        if (ec)
            return ec;

        discard.clear();
    }
}

TEMPLATE
code CLASS::read(http::flat_buffer& out) NOEXCEPT
{
    while (true)
    {
        reader::event event{};
        code ec{};
        const auto data = input_.data();
        const auto used = reader_.put({ system::pointer_cast<const uint8_t>(
            data.data()), data.size() }, out, event, ec);

        input_.consume(used);
        if (ec == error::need_more)
        {
            if (const auto failure = receive())
                return failure;

            continue;
        }

        if (ec)
            return ec;

        const auto& control = reader_.control();
        const std::string_view payload{ system::pointer_cast<const char>(
            control.data()), control.size() };

        switch (event)
        {
            case reader::event::message:
                return error::success;
            case reader::event::ping:
                if (const auto failure = send(opcode::pong, payload))
                    return failure;

                continue;
            case reader::event::close:
                if (!closing_)
                {
                    // The close is answered with the peer's close code.
                    closing_ = true;
                    send(opcode::close, payload.substr(zero, two));
                }

                return error::websocket_closed;
            case reader::event::pong:
            case reader::event::none:
            default:
                continue;
        }
    }
}

TEMPLATE
bool CLASS::binary() const NOEXCEPT
{
    return reader_.binary();
}

// private
TEMPLATE
code CLASS::send(opcode kind, const std::string_view& payload) NOEXCEPT
{
    mask key{};
    system::maybe_random::fill(key);
    auto frame = serialize_header(kind, true,
        system::possible_wide_cast<uint64_t>(payload.size()), key);

    const auto start = frame.size();
    frame.insert(frame.end(), payload.begin(), payload.end());
    apply_mask(std::span<uint8_t>{ frame }.subspan(start), key, zero);

    network::code ec{};
    io::write(stream_, io::buffer(frame), ec);
    return ec;
}

// private
TEMPLATE
code CLASS::receive() NOEXCEPT
{
    code ec{};
    const auto size = stream_.read_some(input_.prepare(http::transfer_size),
        ec);

    input_.commit(size);
    return ec;
}

BC_POP_WARNING()

#undef CLASS
#undef TEMPLATE

} // namespace ws
} // namespace network
} // namespace libbitcoin

#endif
