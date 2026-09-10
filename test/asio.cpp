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
#include "test.hpp"

BOOST_AUTO_TEST_SUITE(asio_tests)

struct pair
{
    pair() NOEXCEPT
      : acceptor(service), accepted(service), connected(service)
    {
        boost_code ec{};
        acceptor.open(asio::tcp::v4(), ec);
        acceptor.bind({ asio::ipv4::loopback(), 0 }, ec);
        acceptor.listen(1, ec);
        connected.connect(acceptor.local_endpoint(ec), ec);
        acceptor.accept(accepted, ec);
        ok = !ec;
    }

    asio::context service{};
    asio::acceptor acceptor;
    asio::socket accepted;
    asio::socket connected;
    bool ok{};
};

// The FIN is delivered by the stack, not the caller.
static bool half_closed_within(asio::socket& sock, size_t milliseconds) NOEXCEPT
{
    for (size_t time{}; time < milliseconds; time += 10)
    {
        if (asio::half_closed(sock))
            return true;

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return asio::half_closed(sock);
}

BOOST_AUTO_TEST_CASE(asio__half_closed__unopened__false)
{
    asio::context service{};
    asio::socket sock(service);
    BOOST_REQUIRE(!asio::half_closed(sock));
}

BOOST_AUTO_TEST_CASE(asio__half_closed__connected__false)
{
    pair connection{};
    BOOST_REQUIRE(connection.ok);
    BOOST_REQUIRE(!asio::half_closed(connection.accepted));
    BOOST_REQUIRE(!asio::half_closed(connection.connected));
}

BOOST_AUTO_TEST_CASE(asio__half_closed__peer_shutdown__true)
{
    pair connection{};
    BOOST_REQUIRE(connection.ok);

    boost_code ec{};
    connection.connected.shutdown(asio::socket::shutdown_send, ec);
    BOOST_REQUIRE(!ec);

    BOOST_REQUIRE(half_closed_within(connection.accepted, 1000));
}

BOOST_AUTO_TEST_CASE(asio__half_closed__peer_close__true)
{
    pair connection{};
    BOOST_REQUIRE(connection.ok);

    boost_code ec{};
    connection.connected.close(ec);
    BOOST_REQUIRE(!ec);

    BOOST_REQUIRE(half_closed_within(connection.accepted, 1000));
}

BOOST_AUTO_TEST_CASE(asio__half_closed__peer_shutdown_with_unread_data__true)
{
    pair connection{};
    BOOST_REQUIRE(connection.ok);

    boost_code ec{};
    const std::string text{ "unconsumed" };
    connection.connected.write_some(boost::asio::buffer(text), ec);
    BOOST_REQUIRE(!ec);

    connection.connected.shutdown(asio::socket::shutdown_send, ec);
    BOOST_REQUIRE(!ec);

    BOOST_REQUIRE(half_closed_within(connection.accepted, 1000));
    BOOST_REQUIRE(is_nonzero(connection.accepted.available(ec)));
    BOOST_REQUIRE(!ec);
}

BOOST_AUTO_TEST_SUITE_END()
