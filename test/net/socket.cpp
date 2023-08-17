/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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

    const asio::strand& get_strand() const NOEXCEPT
    {
        return strand_;
    }

    const asio::socket& get_socket() const NOEXCEPT
    {
        return socket_;
    }

    const config::authority& get_authority() const NOEXCEPT
    {
        return authority_;
    }
};

class acceptor_accessor
  : public acceptor
{
public:
    using acceptor::acceptor;

    const settings& get_settings() const NOEXCEPT
    {
        return settings_;
    }

    const asio::io_context& get_service() const NOEXCEPT
    {
        return service_;
    }

    const asio::strand& get_strand() const NOEXCEPT
    {
        return strand_;
    }

    asio::acceptor& get_acceptor() NOEXCEPT
    {
        return acceptor_;
    }
};

BOOST_AUTO_TEST_CASE(socket__construct__default__closed_not_stopped_expected)
{
    const logger log{};
    threadpool pool(1);
    const auto instance = std::make_shared<socket_accessor>(log, pool.service());

    BOOST_REQUIRE(!instance->stranded());
    BOOST_REQUIRE(!instance->get_socket().is_open());
    BOOST_REQUIRE(&instance->get_strand() == &instance->strand());
    BOOST_REQUIRE(instance->get_authority() == instance->authority());
    BOOST_REQUIRE(instance->get_authority().ip().is_unspecified());
    instance->stop();
}

BOOST_AUTO_TEST_CASE(socket__accept__cancel_acceptor__channel_stopped)
{
    const logger log{};
    threadpool pool(2);
    const auto instance = std::make_shared<socket_accessor>(log, pool.service());
    asio::strand strand(pool.service().get_executor());
    asio::acceptor acceptor(strand);

    error::boost_code ec;
    const asio::endpoint endpoint(asio::tcp::v6(), 42);

    acceptor.open(endpoint.protocol(), ec);
    BOOST_REQUIRE(!ec);

    acceptor.set_option(asio::acceptor::reuse_address(true), ec);
    BOOST_REQUIRE(!ec);

    // Result codes inconsistent due to context.
    acceptor.bind(endpoint, ec);
    ////BOOST_REQUIRE(!ec);

    // Result codes inconsistent due to context.
    acceptor.listen(1, ec);
    ////BOOST_REQUIRE(!ec);

    instance->accept(acceptor, [instance](const code& ec)
    {
        // Acceptor cancellation sets channel_stopped and unspecified address.
        BOOST_REQUIRE_EQUAL(ec, error::operation_canceled);
        BOOST_REQUIRE(instance->get_authority().ip().is_unspecified());
    });

    // Stopping the socket does not cancel the acceptor but precludes assertion.
    instance->stop();

    // Acceptor must be canceled to release/invoke the accept handler.
    // This has the same effect as network::acceptor::stop.
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        error::boost_code ignore;
        acceptor.cancel(ignore);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
}

BOOST_AUTO_TEST_CASE(socket__connect__invalid__error)
{
    const logger log{};
    threadpool pool(2);
    const auto instance = std::make_shared<socket_accessor>(log, pool.service());
    asio::strand strand(pool.service().get_executor());

    const asio::endpoint endpoint(asio::tcp::v6(), 42);
    asio::endpoints endpoints;
    endpoints.create(endpoint, "bogus.xxx", "service");

    instance->connect(endpoints, [instance](const code& ec)
    {
        // Socket cancellation sets channel_stopped and default ipv6 authority.
        // TODO: 3 (ERROR_PATH_NOT_FOUND) code gets mapped to unknown.
        BOOST_REQUIRE(ec == error::unknown || ec == error::channel_stopped);

        // Default authority string inconsistent due to context.
        ////BOOST_REQUIRE_EQUAL(instance->get_authority().to_string(), "[::ffff:0:0]");
        ////BOOST_REQUIRE_EQUAL(instance->get_authority().to_string(), "0.0.0.0");
    });

    // Test race.
    std::this_thread::sleep_for(microseconds(1));

    // Stopping the socket cancels connection attempt, but should fail first.
    // Delay above increases chance that connect fail will win consistently.
    instance->stop();

    pool.stop();
    BOOST_REQUIRE(pool.join());
}

BOOST_AUTO_TEST_CASE(socket__read__disconnected__error)
{
    const logger log{};
    threadpool pool(2);
    const auto instance = std::make_shared<socket_accessor>(log, pool.service());

    system::data_array<42> data;
    instance->read({ data }, [instance](const code& ec, size_t size)
    {
        // 10009 (WSAEBADF, invalid file handle) gets mapped to bad_stream.
        BOOST_REQUIRE_EQUAL(ec, error::bad_stream);
        BOOST_REQUIRE_EQUAL(size, zero);
    });

    // Test race.
    std::this_thread::sleep_for(microseconds(1));

    // Stopping the socket precludes assertion.
    instance->stop();

    pool.stop();
    BOOST_REQUIRE(pool.join());
}

BOOST_AUTO_TEST_CASE(socket__write__disconnected__bad_stream)
{
    const logger log{};
    threadpool pool(2);
    const auto instance = std::make_shared<socket_accessor>(log, pool.service());

    system::data_array<42> data;
    instance->write({ data }, [instance](const code& ec, size_t size)
    {
        // 10009 (WSAEBADF, invalid file handle) gets mapped to bad_stream.
        BOOST_REQUIRE_EQUAL(ec, error::bad_stream);
        BOOST_REQUIRE_EQUAL(size, zero);
    });

    // Test race.
    std::this_thread::sleep_for(microseconds(1));

    // Stopping the socket precludes assertion.
    instance->stop();

    pool.stop();
    BOOST_REQUIRE(pool.join());
}

BOOST_AUTO_TEST_SUITE_END()
