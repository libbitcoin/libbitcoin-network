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
#include "../test.hpp"

BC_AUTO_TEST_SUITE(io_socket_tests)

namespace ip = system::ip;
using system::data_chunk;

static const io::endpoint loopback{ ip::address{ ip::ipv4{ { 127, 0, 0, 1 } } }, 0 };

// A loopback acceptor on an ephemeral port.
struct loopback_fixture
{
    loopback_fixture()
      : acceptor(context, loopback), client(context), server(context)
    {
        BC_REQUIRE(acceptor.is_open());
        port = acceptor.local_endpoint().port();
        BC_REQUIRE(is_nonzero(port));
    }

    io::endpoint target() const
    {
        return { loopback.address(), port };
    }

    // Connect the client (async) and accept the server (async).
    void connect()
    {
        code accepted{ error::unknown };
        code connected{ error::unknown };
        acceptor.async_accept(server, [&](const code& ec) NOEXCEPT { accepted = ec; });
        client.async_connect(target(), [&](const code& ec) NOEXCEPT { connected = ec; });
        context.run();
        context.restart();
        BC_REQUIRE_EQUAL(accepted, error::success);
        BC_REQUIRE_EQUAL(connected, error::success);
    }

    io::context context{};
    io::acceptor acceptor;
    io::socket client;
    io::socket server;
    uint16_t port{};
};

// acceptor

BC_AUTO_TEST_CASE(io_acceptor__construct__unbindable__closed)
{
    io::context context{};
    const io::acceptor first{ context, loopback };
    BC_REQUIRE(first.is_open());
    const io::endpoint taken{ loopback.address(), first.local_endpoint().port() };

    io::acceptor second{ context };
    code ec{};
    second.open(io::tcp::v4(), ec);
    BC_REQUIRE_EQUAL(ec, error::success);
    second.bind(taken, ec);
    BC_REQUIRE(ec);
}

BC_FIXTURE_TEST_CASE(io_acceptor__async_accept__cancel__operation_canceled, loopback_fixture)
{
    code accepted{};
    acceptor.async_accept(server, [&](const code& ec) NOEXCEPT { accepted = ec; });
    code ec{};
    acceptor.cancel(ec);
    BC_REQUIRE_EQUAL(ec, error::success);
    context.run();
    BC_REQUIRE_EQUAL(accepted, error::operation_canceled);
    BC_REQUIRE(!server.is_open());
}

BC_FIXTURE_TEST_CASE(io_acceptor__async_accept__close__operation_canceled, loopback_fixture)
{
    code accepted{};
    acceptor.async_accept(server, [&](const code& ec) NOEXCEPT { accepted = ec; });
    code ec{};
    acceptor.close(ec);
    context.run();
    BC_REQUIRE_EQUAL(accepted, error::operation_canceled);
    BC_REQUIRE(!acceptor.is_open());
}

BC_FIXTURE_TEST_CASE(io_acceptor__async_accept__open_peer__connect_failed, loopback_fixture)
{
    code ec{};
    server.open(io::tcp::v4(), ec);
    code accepted{};
    acceptor.async_accept(server, [&](const code& ec) NOEXCEPT { accepted = ec; });
    context.run();
    BC_REQUIRE_EQUAL(accepted, error::connect_failed);
}

// connect

BC_FIXTURE_TEST_CASE(io_socket__async_connect__accepted__endpoints_match, loopback_fixture)
{
    connect();
    BC_REQUIRE(client.is_open());
    BC_REQUIRE(server.is_open());
    BC_REQUIRE_EQUAL(client.remote_endpoint().port(), port);
    BC_REQUIRE(server.remote_endpoint() == client.local_endpoint());
}

BC_FIXTURE_TEST_CASE(io_socket__async_connect__refused__connection_refused, loopback_fixture)
{
    code ec{};
    acceptor.close(ec);
    code connected{};
    client.async_connect(target(), [&](const code& ec) NOEXCEPT { connected = ec; });
    context.run();
    BC_REQUIRE_EQUAL(connected, error::connection_refused);
}

BC_FIXTURE_TEST_CASE(io_socket__connect__sync__accepted, loopback_fixture)
{
    code ec{};
    client.connect(target(), ec);
    BC_REQUIRE_EQUAL(ec, error::success);
    acceptor.accept(server, ec);
    BC_REQUIRE_EQUAL(ec, error::success);
    BC_REQUIRE(server.remote_endpoint() == client.local_endpoint());
}

BC_FIXTURE_TEST_CASE(io_async_connect__range__first_refused_second_accepted, loopback_fixture)
{
    io::acceptor closed{ context, loopback };
    const io::endpoint refused{ loopback.address(), closed.local_endpoint().port() };
    code ec{};
    closed.close(ec);

    code accepted{ error::unknown };
    code connected{ error::unknown };
    io::endpoint connected_to{};
    acceptor.async_accept(server, [&](const code& ec) NOEXCEPT { accepted = ec; });
    io::async_connect(client, { refused, target() }, [&](const code& ec, const io::endpoint& point) NOEXCEPT
    {
        connected = ec;
        connected_to = point;
    });

    context.run();
    BC_REQUIRE_EQUAL(connected, error::success);
    BC_REQUIRE_EQUAL(accepted, error::success);
    BC_REQUIRE(connected_to == target());
}

BC_FIXTURE_TEST_CASE(io_async_connect__empty_range__resolve_failed, loopback_fixture)
{
    code connected{};
    io::async_connect(client, {}, [&](const code& ec, const io::endpoint&) NOEXCEPT { connected = ec; });
    context.run();
    BC_REQUIRE_EQUAL(connected, error::resolve_failed);
}

// read/write

BC_FIXTURE_TEST_CASE(io_socket__async_write_read__exchanged, loopback_fixture)
{
    connect();
    const std::string sent{ "hello world" };
    std::string received(sent.size(), '\0');
    code wrote{ error::unknown };
    code read{ error::unknown };
    size_t read_size{};
    io::async_write(client, io::buffer(sent), [&](const code& ec, size_t) NOEXCEPT { wrote = ec; });
    io::async_read(server, io::buffer(received), [&](const code& ec, size_t size) NOEXCEPT
    {
        read = ec;
        read_size = size;
    });

    context.run();
    BC_REQUIRE_EQUAL(wrote, error::success);
    BC_REQUIRE_EQUAL(read, error::success);
    BC_REQUIRE_EQUAL(read_size, sent.size());
    BC_REQUIRE_EQUAL(received, sent);
}

BC_FIXTURE_TEST_CASE(io_socket__async_write__gathered_large__received_whole, loopback_fixture)
{
    connect();
    data_chunk head(100'000);
    data_chunk tail(150'000);
    std::iota(head.begin(), head.end(), uint8_t{ 1 });
    std::iota(tail.begin(), tail.end(), uint8_t{ 7 });
    const std::array<io::const_buffer, 2> buffers{ io::buffer(std::as_const(head)), io::buffer(std::as_const(tail)) };
    data_chunk received(head.size() + tail.size());
    size_t written{};
    size_t read{};
    io::async_write(client, buffers, [&](const code&, size_t size) NOEXCEPT { written = size; });
    io::async_read(server, io::buffer(received), [&](const code&, size_t size) NOEXCEPT { read = size; });
    context.run();

    data_chunk expected{ head };
    expected.insert(expected.end(), tail.begin(), tail.end());
    BC_REQUIRE_EQUAL(written, expected.size());
    BC_REQUIRE_EQUAL(read, expected.size());
    BC_REQUIRE(received == expected);
}

BC_FIXTURE_TEST_CASE(io_socket__async_read_some__peer_closed__peer_disconnect, loopback_fixture)
{
    connect();
    code ec{};
    client.close(ec);
    data_chunk buffer(10);
    code read{};
    server.async_read_some(io::buffer(buffer), [&](const code& ec, size_t) NOEXCEPT { read = ec; });
    context.run();
    BC_REQUIRE_EQUAL(read, error::peer_disconnect);
}

BC_FIXTURE_TEST_CASE(io_socket__async_read_some__peer_shutdown_send__peer_disconnect, loopback_fixture)
{
    connect();
    code ec{};
    client.shutdown(io::socket::shutdown_type::send, ec);
    BC_REQUIRE_EQUAL(ec, error::success);
    data_chunk buffer(10);
    code read{};
    server.async_read_some(io::buffer(buffer), [&](const code& ec, size_t) NOEXCEPT { read = ec; });
    context.run();
    BC_REQUIRE_EQUAL(read, error::peer_disconnect);
}

BC_FIXTURE_TEST_CASE(io_socket__async_read_some__cancel__operation_canceled, loopback_fixture)
{
    connect();
    data_chunk buffer(10);
    code read{};
    server.async_read_some(io::buffer(buffer), [&](const code& ec, size_t) NOEXCEPT { read = ec; });
    code ec{};
    server.cancel(ec);
    BC_REQUIRE_EQUAL(ec, error::success);
    context.run();
    BC_REQUIRE_EQUAL(read, error::operation_canceled);
    BC_REQUIRE(server.is_open());
}

BC_FIXTURE_TEST_CASE(io_socket__async_read_some__close__operation_canceled, loopback_fixture)
{
    connect();
    data_chunk buffer(10);
    code read{};
    server.async_read_some(io::buffer(buffer), [&](const code& ec, size_t) NOEXCEPT { read = ec; });
    code ec{};
    server.close(ec);
    context.run();
    BC_REQUIRE_EQUAL(read, error::operation_canceled);
}

BC_FIXTURE_TEST_CASE(io_socket__async_read_some__empty_buffer__success_zero, loopback_fixture)
{
    connect();
    code read{ error::unknown };
    size_t read_size{ 42 };
    server.async_read_some(io::mutable_buffer{}, [&](const code& ec, size_t size) NOEXCEPT
    {
        read = ec;
        read_size = size;
    });

    context.run();
    BC_REQUIRE_EQUAL(read, error::success);
    BC_REQUIRE_EQUAL(read_size, 0u);
}

BC_FIXTURE_TEST_CASE(io_socket__async_read_some__closed_socket__bad_stream, loopback_fixture)
{
    data_chunk buffer(10);
    code read{};
    client.async_read_some(io::buffer(buffer), [&](const code& ec, size_t) NOEXCEPT { read = ec; });
    context.run();
    BC_REQUIRE_EQUAL(read, error::bad_stream);
}

BC_FIXTURE_TEST_CASE(io_socket__write_read__sync__exchanged, loopback_fixture)
{
    connect();
    const std::string sent{ "line one\r\nline two" };
    code ec{};
    BC_REQUIRE_EQUAL(io::write(client, io::buffer(sent), ec), sent.size());
    BC_REQUIRE_EQUAL(ec, error::success);

    std::string text{};
    const auto size = io::read_until(server, text, "\r\n", ec);
    BC_REQUIRE_EQUAL(ec, error::success);
    BC_REQUIRE_EQUAL(text.substr(0, size), "line one\r\n");
}

BC_FIXTURE_TEST_CASE(io_socket__read_some__non_blocking_no_data__would_block, loopback_fixture)
{
    connect();
    code ec{};
    server.non_blocking(true, ec);
    BC_REQUIRE_EQUAL(ec, error::success);
    BC_REQUIRE(server.non_blocking());
    data_chunk buffer(10);
    BC_REQUIRE_EQUAL(server.read_some(io::buffer(buffer), ec), 0u);
    BC_REQUIRE_EQUAL(ec, error::would_block);
}

BC_FIXTURE_TEST_CASE(io_socket__move__open_state_moved, loopback_fixture)
{
    connect();
    io::socket moved{ std::move(server) };
    BC_REQUIRE(moved.is_open());
    BC_REQUIRE(!server.is_open());

    const std::string sent{ "after move" };
    std::string received(sent.size(), '\0');
    io::async_write(client, io::buffer(sent), [](const code&, size_t) NOEXCEPT {});
    io::async_read(moved, io::buffer(received), [](const code&, size_t) NOEXCEPT {});
    context.run();
    BC_REQUIRE_EQUAL(received, sent);
}

BC_FIXTURE_TEST_CASE(io_socket__strand__completes_in_strand, loopback_fixture)
{
    connect();
    const io::strand strand{ context };
    io::socket stranded{ strand };
    code ec{};
    stranded.connect(target(), ec);
    BC_REQUIRE_EQUAL(ec, error::success);
    io::socket peer{ context };
    acceptor.accept(peer, ec);
    BC_REQUIRE_EQUAL(ec, error::success);

    auto in_strand = false;
    const std::string sent{ "x" };
    std::string received(1, '\0');
    io::write(peer, io::buffer(sent), ec);
    stranded.async_read_some(io::buffer(received), [&](const code&, size_t) NOEXCEPT { in_strand = strand.running_in_this_thread(); });
    context.run();
    BC_REQUIRE(in_strand);
    BC_REQUIRE_EQUAL(received, sent);
}

// half_closed

struct connected_pair
{
    connected_pair() NOEXCEPT
      : acceptor(service), accepted(service), connected(service)
    {
        code ec{};
        acceptor.open(io::tcp::v4(), ec);
        acceptor.bind(test::loopback(), ec);
        acceptor.listen(ec);
        connected.connect(acceptor.local_endpoint(ec), ec);
        acceptor.accept(accepted, ec);
        ok = !ec;
    }

    io::context service{};
    io::acceptor acceptor;
    io::socket accepted;
    io::socket connected;
    bool ok{};
};

// The FIN is delivered by the stack, not the caller.
static bool half_closed_within(io::socket& sock, size_t milliseconds) NOEXCEPT
{
    for (size_t time{}; time < milliseconds; time += 10)
    {
        if (sock.half_closed())
            return true;

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return sock.half_closed();
}

BC_AUTO_TEST_CASE(io_socket__half_closed__unopened__false)
{
    io::context service{};
    io::socket sock(service);
    BC_REQUIRE(!sock.half_closed());
}

BC_AUTO_TEST_CASE(io_socket__half_closed__connected__false)
{
    connected_pair connection{};
    BC_REQUIRE(connection.ok);
    BC_REQUIRE(!connection.accepted.half_closed());
    BC_REQUIRE(!connection.connected.half_closed());
}

BC_AUTO_TEST_CASE(io_socket__half_closed__peer_shutdown__true)
{
    connected_pair connection{};
    BC_REQUIRE(connection.ok);

    code ec{};
    connection.connected.shutdown(io::socket::shutdown_type::send, ec);
    BC_REQUIRE(!ec);

    BC_REQUIRE(half_closed_within(connection.accepted, 1000));
}

BC_AUTO_TEST_CASE(io_socket__half_closed__peer_close__true)
{
    connected_pair connection{};
    BC_REQUIRE(connection.ok);

    code ec{};
    connection.connected.close(ec);
    BC_REQUIRE(!ec);

    BC_REQUIRE(half_closed_within(connection.accepted, 1000));
}

BC_AUTO_TEST_CASE(io_socket__half_closed__peer_shutdown_with_unread_data__true)
{
    connected_pair connection{};
    BC_REQUIRE(connection.ok);

    code ec{};
    const std::string text{ "unconsumed" };
    connection.connected.write_some(io::buffer(text), ec);
    BC_REQUIRE(!ec);

    connection.connected.shutdown(io::socket::shutdown_type::send, ec);
    BC_REQUIRE(!ec);

    BC_REQUIRE(half_closed_within(connection.accepted, 1000));
    BC_REQUIRE(is_nonzero(connection.accepted.available(ec)));
    BC_REQUIRE(!ec);
}

BC_AUTO_TEST_CASE(io_socket__half_closed__opened_unconnected__false)
{
    io::context service{};
    io::socket sock(service);
    code ec{};
    sock.open(io::tcp::v4(), ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(!sock.half_closed());
}

// resolver

BC_AUTO_TEST_CASE(io_resolver__async_resolve__numeric__endpoint)
{
    io::context context{};
    io::resolver resolver{ context };
    code resolved{ error::unknown };
    io::endpoints range{};
    resolver.async_resolve("127.0.0.1", "8333", [&](const code& ec, const io::endpoints& result) NOEXCEPT
    {
        resolved = ec;
        range = result;
    });

    context.run();
    BC_REQUIRE_EQUAL(resolved, error::success);
    BC_REQUIRE_EQUAL(range.size(), 1u);
    BC_REQUIRE(range.front() == io::endpoint(ip::address{ ip::ipv4{ { 127, 0, 0, 1 } } }, 8333));
}

BC_AUTO_TEST_CASE(io_resolver__cancel__operation_canceled)
{
    io::context context{};
    io::resolver resolver{ context };
    code resolved{};
    resolver.async_resolve("127.0.0.1", "8333", [&](const code& ec, const io::endpoints&) NOEXCEPT { resolved = ec; });
    resolver.cancel();
    context.run();
    BC_REQUIRE_EQUAL(resolved, error::operation_canceled);
}

BC_AUTO_TEST_SUITE_END()
