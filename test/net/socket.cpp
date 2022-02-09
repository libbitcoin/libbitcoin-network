/**
 * Copyright (c) 2011-2021 libbitcoin developers (see AUTHORS)
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

BOOST_AUTO_TEST_SUITE(socket_tests)

class socket_accessor
  : public network::socket
{
public:
    using socket::socket;

    const asio::strand& get_strand() const
    {
        return strand_;
    }

    const asio::socket& get_socket() const
    {
        return socket_;
    }

    const config::authority& get_authority() const
    {
        return authority_;
    }
};

class acceptor_accessor
  : public acceptor
{
public:
    using acceptor::acceptor;

    const settings& get_settings() const
    {
        return settings_;
    }

    const asio::io_context& get_service() const
    {
        return service_;
    }

    const asio::strand& get_strand() const
    {
        return strand_;
    }

    bool get_stopped() const
    {
        return stopped_;
    }

    deadline::ptr get_timer()
    {
        return timer_;
    }

    asio::acceptor& get_acceptor()
    {
        return acceptor_;
    }
};

BOOST_AUTO_TEST_CASE(socket__construct__default__closed_not_stopped_expected)
{
    threadpool pool(1);
    const auto instance = std::make_shared<socket_accessor>(pool.service());

    BOOST_REQUIRE(!instance->stopped());
    BOOST_REQUIRE(!instance->stranded());
    BOOST_REQUIRE(!instance->get_socket().is_open());
    BOOST_REQUIRE(&instance->get_strand() == &instance->strand());
    BOOST_REQUIRE(instance->get_authority() == instance->authority());
    BOOST_REQUIRE_EQUAL(instance->get_authority().to_string(), "[::]");
}

BOOST_AUTO_TEST_CASE(socket__accept__cancel_acceptor__channel_stopped)
{
    threadpool pool(2);
    const auto instance = std::make_shared<socket_accessor>(pool.service());
    asio::strand strand(pool.service().get_executor());
    asio::acceptor acceptor(strand);

    // This is hardwired to listen on IPv6.
    asio::endpoint endpoint(asio::tcp::v6(), 42);
    error::boost_code ec;

    acceptor.open(endpoint.protocol(), ec);
    BOOST_REQUIRE(!ec);

    acceptor.set_option(asio::acceptor::reuse_address(true), ec);
    BOOST_REQUIRE(!ec);

    acceptor.bind(endpoint, ec);
    BOOST_REQUIRE(!ec);

    acceptor.listen(1, ec);
    BOOST_REQUIRE(!ec);

    instance->accept(acceptor, [instance](const code& ec)
    {
        // Acceptor cancelation sets channel_stopped and default authority.
        BOOST_REQUIRE_EQUAL(ec, error::channel_stopped);
        BOOST_REQUIRE_EQUAL(instance->get_authority().to_string(), "[::]");
    });

    // Stopping the socket does not cancel the acceptor.
    ////instance->stop();

    // Acceptor must be canceled to release/invoke the acecpt handler.
    // This has the same effect as network::acceptor::stop|timeout.
    boost::asio::post(strand, [&]()
    {
        error::boost_code ignore;
        acceptor.cancel(ignore);
    });

    pool.stop();
    pool.join();
}

BOOST_AUTO_TEST_SUITE_END()
