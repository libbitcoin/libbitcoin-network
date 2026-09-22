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
#include <future>
#include "../test.hpp"

BOOST_AUTO_TEST_SUITE(connector_socks_tests)

#define SOCKS_PROXY_ENDPOINT "127.0.0.1:65011"
#define SOCKS_TARGET_ENDPOINT "127.0.0.1:65012"

class accessor
  : public connector_socks
{
public:
    using connector_socks::connector_socks;
    using connector_socks::socks_response;
};

// datatracker.ietf.org/doc/html/rfc1928
constexpr uint8_t socks_version = 0x05;
constexpr uint8_t socks_reserved = 0x00;
constexpr uint8_t socks_method_clear = 0x00;
constexpr uint8_t socks_method_basic = 0x02;
constexpr uint8_t socks_basic_version = 0x01;
constexpr uint8_t socks_address_ipv4 = 0x01;
constexpr uint8_t socks_address_fqdn = 0x03;
constexpr uint8_t socks_address_ipv6 = 0x04;

// The connector runs on the fixture pool and the proxy is a plain socket
// driven synchronously from the test thread, so each exchange is ordered by
// the proxy's own reads and writes.
struct socks_setup_fixture
{
    DELETE_COPY_MOVE(socks_setup_fixture);

    socks_setup_fixture()
      : pool(2), strand(pool.service().get_executor()),
        acceptor(context), proxy(context)
    {
        log.stop();
        socks_settings.socks = { SOCKS_PROXY_ENDPOINT };
        const config::authority listen{ SOCKS_PROXY_ENDPOINT };
        acceptor.open(boost::asio::ip::tcp::v4());
        acceptor.set_option(boost::asio::socket_base::reuse_address(true));
        acceptor.bind(listen.to_endpoint());
        acceptor.listen();
    }

    ~socks_setup_fixture()
    {
        if (instance)
            boost::asio::post(strand, [this]() NOEXCEPT { instance->stop(); });

        proxy.close();
        acceptor.close();
        pool.stop();
        pool.join();
    }

    // Construct the connector, after any settings adjustment.
    void create()
    {
        connector::parameters params
        {
            .connect_timeout = seconds(10),
            .maximum_request = 42
        };
        instance = std::make_shared<connector_socks>(log, strand,
            pool.service(), suspended, std::move(params), socks_settings);
    }

    // Start a connect to the target, returns the future connect result.
    std::future<code> connect(const config::endpoint& target)
    {
        boost::asio::post(strand, [this, target]() NOEXCEPT
        {
            instance->connect(target,
                [this](const code& ec, const socket::ptr& socket) NOEXCEPT
                {
                    if (socket)
                        socket->stop();

                    connected.set_value(ec);
                });
        });

        return connected.get_future();
    }

    // Start a connect to the address, returns the future connect result.
    std::future<code> connect(const config::address& target)
    {
        boost::asio::post(strand, [this, target]() NOEXCEPT
        {
            instance->connect(target,
                [this](const code& ec, const socket::ptr& socket) NOEXCEPT
                {
                    if (socket)
                        socket->stop();

                    connected.set_value(ec);
                });
        });

        return connected.get_future();
    }

    // Accept the connector's connection to the proxy.
    void accept()
    {
        acceptor.accept(proxy);
    }

    system::data_chunk read(size_t size)
    {
        system::data_chunk buffer(size);
        boost::asio::read(proxy, boost::asio::buffer(buffer));
        return buffer;
    }

    void write(const system::data_chunk& data)
    {
        boost::asio::write(proxy, boost::asio::buffer(data));
    }

    // Read the greeting and answer with the given method.
    system::data_chunk greet(uint8_t method)
    {
        const auto greeting = read(3);
        write({ socks_version, method });
        return greeting;
    }

    logger log{};
    threadpool pool;
    std::atomic_bool suspended{ false };
    asio::strand strand;
    settings::socks5 socks_settings{};
    connector_socks::ptr instance{};
    boost::asio::io_context context{};
    boost::asio::ip::tcp::acceptor acceptor;
    boost::asio::ip::tcp::socket proxy;
    std::promise<code> connected{};
};

BOOST_AUTO_TEST_CASE(connector_socks__construct__unproxied__expected)
{
    logger log{};
    log.stop();
    threadpool pool(1);
    std::atomic_bool suspended{ false };
    asio::strand strand(pool.service().get_executor());
    const settings::socks5 socks{};
    connector::parameters params
    {
        .connect_timeout = seconds(10), 
        .maximum_request = 42 
    };
    auto instance = std::make_shared<connector_socks>(log, strand, pool.service(), suspended, std::move(params), socks);
    BOOST_REQUIRE(!socks.proxied());

    boost::asio::post(strand, [&]() NOEXCEPT { instance->stop(); });
    pool.stop();
    BOOST_REQUIRE(pool.join());
}

BOOST_FIXTURE_TEST_CASE(connector_socks__connect__clear_greeting_and_ipv4_reply__success, socks_setup_fixture)
{
    create();
    auto result = connect(config::endpoint{ SOCKS_TARGET_ENDPOINT });
    accept();

    const auto greeting = greet(socks_method_clear);
    BOOST_REQUIRE_EQUAL(greeting, system::data_chunk({ socks_version, 0x01, socks_method_clear }));

    const auto request = read(4);
    BOOST_REQUIRE_EQUAL(request, system::data_chunk({ socks_version, 0x01, socks_reserved, socks_address_fqdn }));

    const auto length = read(1);
    read(length.front() + 2u);
    write({ socks_version, 0x00, socks_reserved, socks_address_ipv4, 0, 0, 0, 0, 0, 0 });

    BOOST_REQUIRE_EQUAL(result.get(), error::success);
}

BOOST_FIXTURE_TEST_CASE(connector_socks__connect__ipv6_reply__success, socks_setup_fixture)
{
    create();
    auto result = connect(config::endpoint{ SOCKS_TARGET_ENDPOINT });
    accept();

    greet(socks_method_clear);
    read(4);
    const auto length = read(1);
    read(length.front() + 2u);

    system::data_chunk reply{ socks_version, 0x00, socks_reserved, socks_address_ipv6 };
    reply.resize(reply.size() + 16u + 2u);
    write(reply);

    BOOST_REQUIRE_EQUAL(result.get(), error::success);
}

BOOST_FIXTURE_TEST_CASE(connector_socks__connect__fqdn_reply__success, socks_setup_fixture)
{
    create();
    auto result = connect(config::endpoint{ SOCKS_TARGET_ENDPOINT });
    accept();

    greet(socks_method_clear);
    read(4);
    const auto length = read(1);
    read(length.front() + 2u);

    write({ socks_version, 0x00, socks_reserved, socks_address_fqdn, 0x02, 'h', 'i', 0, 0 });

    BOOST_REQUIRE_EQUAL(result.get(), error::success);
}

BOOST_FIXTURE_TEST_CASE(connector_socks__connect__unsupported_reply_address__socks_response_invalid, socks_setup_fixture)
{
    create();
    auto result = connect(config::endpoint{ SOCKS_TARGET_ENDPOINT });
    accept();

    greet(socks_method_clear);
    read(4);
    const auto length = read(1);
    read(length.front() + 2u);
    write({ socks_version, 0x00, socks_reserved, 0x42 });

    BOOST_REQUIRE_EQUAL(result.get(), error::socks_response_invalid);
}

BOOST_FIXTURE_TEST_CASE(connector_socks__connect__invalid_reply_version__socks_response_invalid, socks_setup_fixture)
{
    create();
    auto result = connect(config::endpoint{ SOCKS_TARGET_ENDPOINT });
    accept();

    greet(socks_method_clear);
    read(4);
    const auto length = read(1);
    read(length.front() + 2u);
    write({ 0x42, 0x00, socks_reserved, socks_address_ipv4 });

    BOOST_REQUIRE_EQUAL(result.get(), error::socks_response_invalid);
}

BOOST_FIXTURE_TEST_CASE(connector_socks__connect__reply_failure__socks_failure, socks_setup_fixture)
{
    create();
    auto result = connect(config::endpoint{ SOCKS_TARGET_ENDPOINT });
    accept();

    greet(socks_method_clear);
    read(4);
    const auto length = read(1);
    read(length.front() + 2u);
    write({ socks_version, 0x01, socks_reserved, socks_address_ipv4 });

    BOOST_REQUIRE_EQUAL(result.get(), error::socks_failure);
}

BOOST_FIXTURE_TEST_CASE(connector_socks__connect__reply_unassigned__socks_unassigned_failure, socks_setup_fixture)
{
    create();
    auto result = connect(config::endpoint{ SOCKS_TARGET_ENDPOINT });
    accept();

    greet(socks_method_clear);
    read(4);
    const auto length = read(1);
    read(length.front() + 2u);
    write({ socks_version, 0x42, socks_reserved, socks_address_ipv4 });

    BOOST_REQUIRE_EQUAL(result.get(), error::socks_unassigned_failure);
}

BOOST_FIXTURE_TEST_CASE(connector_socks__connect__unoffered_method__socks_method, socks_setup_fixture)
{
    create();
    auto result = connect(config::endpoint{ SOCKS_TARGET_ENDPOINT });
    accept();

    read(3);
    write({ socks_version, socks_method_basic });

    BOOST_REQUIRE_EQUAL(result.get(), error::socks_method);
}

BOOST_FIXTURE_TEST_CASE(connector_socks__connect__authenticated__success, socks_setup_fixture)
{
    socks_settings.username = "user";
    socks_settings.password = "pass";
    create();
    auto result = connect(config::endpoint{ SOCKS_TARGET_ENDPOINT });
    accept();

    const auto greeting = greet(socks_method_basic);
    BOOST_REQUIRE_EQUAL(greeting, system::data_chunk({ socks_version, 0x01, socks_method_basic }));

    const auto authenticator = read(11);
    BOOST_REQUIRE_EQUAL(authenticator, system::data_chunk({ socks_basic_version, 0x04, 'u', 's', 'e', 'r', 0x04, 'p', 'a', 's', 's' }));
    write({ socks_basic_version, 0x00 });

    read(4);
    const auto length = read(1);
    read(length.front() + 2u);
    write({ socks_version, 0x00, socks_reserved, socks_address_ipv4, 0, 0, 0, 0, 0, 0 });

    BOOST_REQUIRE_EQUAL(result.get(), error::success);
}

BOOST_FIXTURE_TEST_CASE(connector_socks__connect__authentication_rejected__socks_authentication, socks_setup_fixture)
{
    socks_settings.username = "user";
    socks_settings.password = "pass";
    create();
    auto result = connect(config::endpoint{ SOCKS_TARGET_ENDPOINT });
    accept();

    greet(socks_method_basic);
    read(11);
    write({ socks_basic_version, 0x01 });

    BOOST_REQUIRE_EQUAL(result.get(), error::socks_authentication);
}

BOOST_FIXTURE_TEST_CASE(connector_socks__connect__ipv4_address__ipv4_request, socks_setup_fixture)
{
    create();
    auto result = connect(config::address{ "1.2.3.4:65012" });
    accept();

    greet(socks_method_clear);
    const auto request = read(4);
    BOOST_REQUIRE_EQUAL(request, system::data_chunk({ socks_version, 0x01, socks_reserved, socks_address_ipv4 }));

    const auto host = read(4u + 2u);
    BOOST_REQUIRE_EQUAL(host, system::data_chunk({ 0x01, 0x02, 0x03, 0x04, 0xfd, 0xf4 }));
    write({ socks_version, 0x00, socks_reserved, socks_address_ipv4, 0, 0, 0, 0, 0, 0 });

    BOOST_REQUIRE_EQUAL(result.get(), error::success);
}

BOOST_FIXTURE_TEST_CASE(connector_socks__connect__ipv6_address__ipv6_request, socks_setup_fixture)
{
    create();
    auto result = connect(config::address{ "[2020:db8::1]:65012" });
    accept();

    greet(socks_method_clear);
    const auto request = read(4);
    BOOST_REQUIRE_EQUAL(request, system::data_chunk({ socks_version, 0x01, socks_reserved, socks_address_ipv6 }));

    read(16u + 2u);
    write({ socks_version, 0x00, socks_reserved, socks_address_ipv4, 0, 0, 0, 0, 0, 0 });

    BOOST_REQUIRE_EQUAL(result.get(), error::success);
}

BOOST_AUTO_TEST_CASE(connector_socks__socks_response__reply_codes__expected)
{
    BOOST_REQUIRE_EQUAL(accessor::socks_response(0x00), error::success);
    BOOST_REQUIRE_EQUAL(accessor::socks_response(0x01), error::socks_failure);
    BOOST_REQUIRE_EQUAL(accessor::socks_response(0x02), error::socks_disallowed);
    BOOST_REQUIRE_EQUAL(accessor::socks_response(0x03), error::socks_net_unreachable);
    BOOST_REQUIRE_EQUAL(accessor::socks_response(0x04), error::socks_host_unreachable);
    BOOST_REQUIRE_EQUAL(accessor::socks_response(0x05), error::socks_connection_refused);
    BOOST_REQUIRE_EQUAL(accessor::socks_response(0x06), error::socks_connection_expired);
    BOOST_REQUIRE_EQUAL(accessor::socks_response(0x07), error::socks_unsupported_command);
    BOOST_REQUIRE_EQUAL(accessor::socks_response(0x08), error::socks_unsupported_address);
    BOOST_REQUIRE_EQUAL(accessor::socks_response(0x09), error::socks_unassigned_failure);
    BOOST_REQUIRE_EQUAL(accessor::socks_response(0xff), error::socks_unassigned_failure);
}

BOOST_AUTO_TEST_SUITE_END()
