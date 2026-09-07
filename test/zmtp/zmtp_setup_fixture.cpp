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
#include "zmtp_setup_fixture.hpp"

#include <cstdlib>
#include <future>
#include "../test.hpp"

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

using system::data_chunk;
using system::data_stack;

// Wire (peer side).
// ----------------------------------------------------------------------------

data_chunk command(const std::string& name, const data_chunk& content)
{
    data_chunk body{};
    const auto size = system::possible_narrow_cast<uint8_t>(name.size());
    body.push_back(size);
    body.insert(body.end(), name.begin(), name.end());
    body.insert(body.end(), content.begin(), content.end());
    return zmtp_stream::frame_encode(body, true, false);
}

data_chunk property(const std::string& key, const std::string& value)
{
    const auto key_size = system::possible_narrow_cast<uint8_t>(key.size());
    const auto size = system::possible_narrow_cast<uint32_t>(value.size());
    const auto value_size = system::to_big_endian(size);

    data_chunk meta{};
    meta.push_back(key_size);
    meta.insert(meta.end(), key.begin(), key.end());
    meta.insert(meta.end(), value_size.begin(), value_size.end());
    meta.insert(meta.end(), value.begin(), value.end());
    return meta;
}

data_chunk ready(const std::string& type, const std::string& identity)
{
    auto meta = property("Socket-Type", type);
    if (!identity.empty())
    {
        const auto named = property("Identity", identity);
        meta.insert(meta.end(), named.begin(), named.end());
    }

    return command("READY", meta);
}

void parse_command(const data_chunk& body, std::string& name, data_chunk& content)
{
    std::span<const uint8_t> rest{};
    const std::span<const uint8_t> frame{ body };
    BOOST_REQUIRE(zmtp_stream::command_name(name, rest, frame));
    content.assign(rest.begin(), rest.end());
}

void peer_write(peer_socket& peer, const data_chunk& data)
{
    const boost::asio::const_buffer out{ data.data(), data.size() };
    boost::asio::write(peer, out);
}

void peer_read_frame(peer_socket& peer, uint8_t& flags, data_chunk& body)
{
    system::data_array<2> head{};
    const boost::asio::mutable_buffer in{ head.data(), head.size() };
    boost::asio::read(peer, in);
    flags = head.front();
    BOOST_REQUIRE(is_zero(flags & zmtp_stream::flag_long));

    body.assign(head.back(), 0x00);
    if (body.empty())
        return;

    const boost::asio::mutable_buffer rest{ body.data(), body.size() };
    boost::asio::read(peer, rest);
}

data_stack peer_read_message(peer_socket& peer)
{
    data_stack parts{};
    auto more = true;
    while (more)
    {
        uint8_t flags{};
        data_chunk body{};
        peer_read_frame(peer, flags, body);
        parts.push_back(body);
        more = !is_zero(flags & zmtp_stream::flag_more);
    }

    return parts;
}

std::string peer_read_command(peer_socket& peer, data_chunk& content)
{
    uint8_t flags{};
    data_chunk body{};
    peer_read_frame(peer, flags, body);
    BOOST_REQUIRE(!is_zero(flags & zmtp_stream::flag_command));

    std::string name{};
    parse_command(body, name, content);
    return name;
}

std::string peer_handshake(peer_socket& peer, const std::string& type, const std::string& identity, uint8_t minor)
{
    auto greeting = zmtp_stream::make_greeting(false, false);
    greeting.at(11) = minor;
    peer_write(peer, greeting);

    data_chunk theirs(zmtp_stream::greeting_size, 0x00);
    const boost::asio::mutable_buffer in{ theirs.data(), theirs.size() };
    boost::asio::read(peer, in);
    uint8_t their_minor{};
    bool curve{};
    bool as_server{};
    BOOST_REQUIRE(zmtp_stream::parse_greeting(theirs, their_minor, curve, as_server));

    peer_write(peer, ready(type, identity));
    data_chunk content{};
    return peer_read_command(peer, content);
}

// Rpc (message side).
// ----------------------------------------------------------------------------

data_chunk param_of(const rpc::request& request, size_t index)
{
    const auto& params = std::get<rpc::array_t>(*request.message.params);
    const auto& any = std::get<rpc::any_t>(params.at(index).value());
    return *any.as<const data_chunk>();
}

size_t params_of(const rpc::request& request)
{
    return std::get<rpc::array_t>(*request.message.params).size();
}

data_chunk prefix_of(const rpc::request& request)
{
    return param_of(request, 0);
}

bool stop_of(const rpc::request& request)
{
    const auto& params = std::get<rpc::array_t>(*request.message.params);
    return std::get<rpc::boolean_t>(params.at(1).value());
}

data_chunk chunk(const std::string& text)
{
    return system::to_chunk(text);
}

// socket_setup_fixture
// ----------------------------------------------------------------------------

socket_setup_fixture::socket_setup_fixture(zmtp_role value, const std::string& type, size_t maximum, const std::string& identity)
  : pool(1),
    params
    {
        .maximum_request = maximum,
        .context = socket::context{ std::cref(configuration) },
        .role = value
    },
    sock(std::make_shared<network::socket>(log, pool.service(), params)),
    strand(pool.service().get_executor()),
    acceptor(strand),
    peer(peer_context)
{
    boost_code ec{};
    acceptor.open(asio::tcp::v4(), ec);
    BOOST_REQUIRE(!ec);
    acceptor.set_option(asio::reuse_address(true), ec);
    BOOST_REQUIRE(!ec);
    acceptor.bind({ boost::asio::ip::address_v4::loopback(), 0 }, ec);
    BOOST_REQUIRE(!ec);
    acceptor.listen(1, ec);
    BOOST_REQUIRE(!ec);

    // The accept completes with the handshake, which the peer drives.
    std::promise<code> promised{};
    sock->accept(acceptor, [&](const code& result) NOEXCEPT
    {
        promised.set_value(result);
    });

    peer.connect(acceptor.local_endpoint());
    first = peer_handshake(peer, type, identity);
    accepted = promised.get_future().get();
}

// The peer is closed first, so the socket sees an ordered end of stream, and
// the pool is joined before any member is destroyed.
socket_setup_fixture::~socket_setup_fixture()
{
    peer.close();
    sock->stop();
    pool.stop();
    BOOST_REQUIRE(pool.join());
}

code socket_setup_fixture::read(rpc::request& request)
{
    std::promise<code> promised{};
    sock->rpc_read(buffer, request, [&](const code& ec, size_t) NOEXCEPT
    {
        promised.set_value(ec);
    });

    return promised.get_future().get();
}

code socket_setup_fixture::notify(rpc::request&& notification)
{
    std::promise<code> promised{};
    sock->rpc_notify(std::move(notification), [&](const code& ec, size_t) NOEXCEPT
    {
        promised.set_value(ec);
    });

    return promised.get_future().get();
}

code socket_setup_fixture::respond(rpc::response&& response)
{
    std::promise<code> promised{};
    sock->rpc_write(std::move(response), [&](const code& ec, size_t) NOEXCEPT
    {
        promised.set_value(ec);
    });

    return promised.get_future().get();
}

// publisher_setup_fixture
// ----------------------------------------------------------------------------

publisher_setup_fixture::publisher_setup_fixture()
  : pool(1),
    params
    {
        .maximum_request = 4096,
        .context = socket::context{ std::cref(configuration) },
        .role = zmtp_role::publisher
    },
    sock(std::make_shared<network::socket>(log, pool.service(), params)),
    prx(std::make_shared<mock_proxy>(sock)),
    strand(pool.service().get_executor()),
    acceptor(strand),
    peer(peer_context)
{
    boost_code ec{};
    acceptor.open(asio::tcp::v4(), ec);
    BOOST_REQUIRE(!ec);
    acceptor.set_option(asio::reuse_address(true), ec);
    BOOST_REQUIRE(!ec);
    acceptor.bind({ boost::asio::ip::address_v4::loopback(), 0 }, ec);
    BOOST_REQUIRE(!ec);
    acceptor.listen(1, ec);
    BOOST_REQUIRE(!ec);

    std::promise<code> promised{};
    sock->accept(acceptor, [&](const code& result) NOEXCEPT
    {
        promised.set_value(result);
    });

    peer.connect(acceptor.local_endpoint());
    BOOST_REQUIRE_EQUAL(peer_handshake(peer, "SUB"), "READY");
    BOOST_REQUIRE_EQUAL(promised.get_future().get(), error::success);
}

publisher_setup_fixture::~publisher_setup_fixture()
{
    peer.close();
    prx->stop(error::channel_stopped);
    pool.stop();
    BOOST_REQUIRE(pool.join());
}

// The promise is a member, as the read is armed by one call and awaited by
// another, and the pool is joined before it is destroyed.
void publisher_setup_fixture::arm_read(rpc::request& request)
{
    armed = {};
    std::promise<bool> posted{};
    boost::asio::post(prx->strand(), [&]() NOEXCEPT
    {
        prx->read1(buffer, request, [this](const code& ec, size_t) NOEXCEPT
        {
            armed.set_value(ec);
        });

        posted.set_value(true);
    });

    // The read is armed before the peer writes, so control is absorbed.
    BOOST_REQUIRE(posted.get_future().get());
}

code publisher_setup_fixture::await_read()
{
    return armed.get_future().get();
}

code publisher_setup_fixture::notify(rpc::request&& notification)
{
    std::promise<code> promised{};
    prx->write1(std::move(notification), [&](const code& ec, size_t) NOEXCEPT
    {
        promised.set_value(ec);
    });

    return promised.get_future().get();
}

code publisher_setup_fixture::respond(rpc::response&& response)
{
    std::promise<code> promised{};
    prx->write1(std::move(response), [&](const code& ec, size_t) NOEXCEPT
    {
        promised.set_value(ec);
    });

    return promised.get_future().get();
}

// role_setup_fixture
// ----------------------------------------------------------------------------

role_setup_fixture::role_setup_fixture(zmtp_role value, uint16_t port)
  : enabled(!is_null(std::getenv("ZMTP_HARNESS"))),
    pool(1),
    params
    {
        .maximum_request = 4096,
        .context = socket::context{ std::cref(configuration) },
        .role = value
    },
    sock(std::make_shared<network::socket>(log, pool.service(), params)),
    strand(pool.service().get_executor()),
    acceptor(strand)
{
    // Without the harness there is no peer, so nothing is bound or armed.
    if (!enabled)
        return;

    boost_code ec{};
    acceptor.open(asio::tcp::v4(), ec);
    BOOST_REQUIRE(!ec);
    acceptor.set_option(asio::reuse_address(true), ec);
    BOOST_REQUIRE(!ec);
    acceptor.bind({ boost::asio::ip::address_v4::loopback(), port }, ec);
    BOOST_REQUIRE(!ec);
    acceptor.listen(1, ec);
    BOOST_REQUIRE(!ec);

    std::promise<code> promised{};
    sock->accept(acceptor, [&](const code& result) NOEXCEPT
    {
        promised.set_value(result);
    });

    BOOST_REQUIRE_EQUAL(promised.get_future().get(), error::success);
}

role_setup_fixture::~role_setup_fixture()
{
    sock->stop();
    pool.stop();
    BOOST_REQUIRE(pool.join());
}

code role_setup_fixture::read(rpc::request& request)
{
    std::promise<code> promised{};
    sock->rpc_read(buffer, request, [&](const code& ec, size_t) NOEXCEPT
    {
        promised.set_value(ec);
    });

    return promised.get_future().get();
}

code role_setup_fixture::notify(rpc::request&& notification)
{
    std::promise<code> promised{};
    sock->rpc_notify(std::move(notification), [&](const code& ec, size_t) NOEXCEPT
    {
        promised.set_value(ec);
    });

    return promised.get_future().get();
}

code role_setup_fixture::respond(rpc::response&& response)
{
    std::promise<code> promised{};
    sock->rpc_write(std::move(response), [&](const code& ec, size_t) NOEXCEPT
    {
        promised.set_value(ec);
    });

    return promised.get_future().get();
}

BC_POP_WARNING()
