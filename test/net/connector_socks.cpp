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

BC_AUTO_TEST_SUITE(connector_socks_tests)

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
        REQUIRE_SUCCESS(acceptor, open, io::tcp::v4());
        REQUIRE_SUCCESS(acceptor, bind, test::loopback());
        REQUIRE_SUCCESS(acceptor, listen);
        socks_settings.socks = { acceptor.local_endpoint() };
    }

    ~socks_setup_fixture()
    {
        if (instance)
            io::post(strand, [this]() NOEXCEPT { instance->stop(); });

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
        io::post(strand, [this, target]() NOEXCEPT
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
        io::post(strand, [this, target]() NOEXCEPT
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
        REQUIRE_SUCCESS(acceptor, accept, proxy);
    }

    system::data_chunk read(size_t size)
    {
        system::data_chunk buffer(size);
        test::read(proxy, io::buffer(buffer));
        return buffer;
    }

    void write(const system::data_chunk& data)
    {
        test::write(proxy, io::buffer(data));
    }

    // Point the proxy at an endpoint that refuses connections.
    void refuse()
    {
        std::promise<bool> pointed{};
        io::post(strand, [this, &pointed]() NOEXCEPT
        {
            socks_settings.socks = { test::refuser() };
            pointed.set_value(true);
        });

        pointed.get_future().get();
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
    io::strand strand;
    settings::socks5 socks_settings{};
    connector_socks::ptr instance{};
    io::context context{};
    io::acceptor acceptor;
    io::socket proxy;
    std::promise<code> connected{};
};

BC_AUTO_TEST_CASE(connector_socks__construct__unproxied__expected)
{
    logger log{};
    log.stop();
    threadpool pool(1);
    std::atomic_bool suspended{ false };
    io::strand strand(pool.service().get_executor());
    const settings::socks5 socks{};
    connector::parameters params
    {
        .connect_timeout = seconds(10), 
        .maximum_request = 42 
    };
    auto instance = std::make_shared<connector_socks>(log, strand, pool.service(), suspended, std::move(params), socks);
    BC_REQUIRE(!socks.proxied());

    io::post(strand, [&]() NOEXCEPT { instance->stop(); });
    pool.stop();
    BC_REQUIRE(pool.join());
}

BC_FIXTURE_TEST_CASE(connector_socks__connect__clear_greeting_and_ipv4_reply__success, socks_setup_fixture)
{
    create();
    auto result = connect(config::endpoint{ SOCKS_TARGET_ENDPOINT });
    accept();

    const auto greeting = greet(socks_method_clear);
    BC_REQUIRE_EQUAL(greeting, system::data_chunk({ socks_version, 0x01, socks_method_clear }));

    const auto request = read(4);
    BC_REQUIRE_EQUAL(request, system::data_chunk({ socks_version, 0x01, socks_reserved, socks_address_fqdn }));

    const auto length = read(1);
    read(length.front() + 2u);
    write({ socks_version, 0x00, socks_reserved, socks_address_ipv4, 0, 0, 0, 0, 0, 0 });

    BC_REQUIRE_EQUAL(result.get(), error::success);
}

BC_FIXTURE_TEST_CASE(connector_socks__connect__ipv6_reply__success, socks_setup_fixture)
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

    BC_REQUIRE_EQUAL(result.get(), error::success);
}

BC_FIXTURE_TEST_CASE(connector_socks__connect__fqdn_reply__success, socks_setup_fixture)
{
    create();
    auto result = connect(config::endpoint{ SOCKS_TARGET_ENDPOINT });
    accept();

    greet(socks_method_clear);
    read(4);
    const auto length = read(1);
    read(length.front() + 2u);

    write({ socks_version, 0x00, socks_reserved, socks_address_fqdn, 0x02, 'h', 'i', 0, 0 });

    BC_REQUIRE_EQUAL(result.get(), error::success);
}

BC_FIXTURE_TEST_CASE(connector_socks__connect__unsupported_reply_address__socks_response_invalid, socks_setup_fixture)
{
    create();
    auto result = connect(config::endpoint{ SOCKS_TARGET_ENDPOINT });
    accept();

    greet(socks_method_clear);
    read(4);
    const auto length = read(1);
    read(length.front() + 2u);
    write({ socks_version, 0x00, socks_reserved, 0x42 });

    BC_REQUIRE_EQUAL(result.get(), error::socks_response_invalid);
}

BC_FIXTURE_TEST_CASE(connector_socks__connect__invalid_reply_version__socks_response_invalid, socks_setup_fixture)
{
    create();
    auto result = connect(config::endpoint{ SOCKS_TARGET_ENDPOINT });
    accept();

    greet(socks_method_clear);
    read(4);
    const auto length = read(1);
    read(length.front() + 2u);
    write({ 0x42, 0x00, socks_reserved, socks_address_ipv4 });

    BC_REQUIRE_EQUAL(result.get(), error::socks_response_invalid);
}

BC_FIXTURE_TEST_CASE(connector_socks__connect__reply_failure__socks_failure, socks_setup_fixture)
{
    create();
    auto result = connect(config::endpoint{ SOCKS_TARGET_ENDPOINT });
    accept();

    greet(socks_method_clear);
    read(4);
    const auto length = read(1);
    read(length.front() + 2u);
    write({ socks_version, 0x01, socks_reserved, socks_address_ipv4 });

    BC_REQUIRE_EQUAL(result.get(), error::socks_failure);
}

BC_FIXTURE_TEST_CASE(connector_socks__connect__reply_unassigned__socks_unassigned_failure, socks_setup_fixture)
{
    create();
    auto result = connect(config::endpoint{ SOCKS_TARGET_ENDPOINT });
    accept();

    greet(socks_method_clear);
    read(4);
    const auto length = read(1);
    read(length.front() + 2u);
    write({ socks_version, 0x42, socks_reserved, socks_address_ipv4 });

    BC_REQUIRE_EQUAL(result.get(), error::socks_unassigned_failure);
}

BC_FIXTURE_TEST_CASE(connector_socks__connect__unoffered_method__socks_method, socks_setup_fixture)
{
    create();
    auto result = connect(config::endpoint{ SOCKS_TARGET_ENDPOINT });
    accept();

    read(3);
    write({ socks_version, socks_method_basic });

    BC_REQUIRE_EQUAL(result.get(), error::socks_method);
}

BC_FIXTURE_TEST_CASE(connector_socks__connect__authenticated__success, socks_setup_fixture)
{
    socks_settings.username = "user";
    socks_settings.password = "pass";
    create();
    auto result = connect(config::endpoint{ SOCKS_TARGET_ENDPOINT });
    accept();

    const auto greeting = greet(socks_method_basic);
    BC_REQUIRE_EQUAL(greeting, system::data_chunk({ socks_version, 0x01, socks_method_basic }));

    const auto authenticator = read(11);
    BC_REQUIRE_EQUAL(authenticator, system::data_chunk({ socks_basic_version, 0x04, 'u', 's', 'e', 'r', 0x04, 'p', 'a', 's', 's' }));
    write({ socks_basic_version, 0x00 });

    read(4);
    const auto length = read(1);
    read(length.front() + 2u);
    write({ socks_version, 0x00, socks_reserved, socks_address_ipv4, 0, 0, 0, 0, 0, 0 });

    BC_REQUIRE_EQUAL(result.get(), error::success);
}

BC_FIXTURE_TEST_CASE(connector_socks__connect__authentication_rejected__socks_authentication, socks_setup_fixture)
{
    socks_settings.username = "user";
    socks_settings.password = "pass";
    create();
    auto result = connect(config::endpoint{ SOCKS_TARGET_ENDPOINT });
    accept();

    greet(socks_method_basic);
    read(11);
    write({ socks_basic_version, 0x01 });

    BC_REQUIRE_EQUAL(result.get(), error::socks_authentication);
}

BC_FIXTURE_TEST_CASE(connector_socks__connect__ipv4_address__ipv4_request, socks_setup_fixture)
{
    create();
    auto result = connect(config::address{ "1.2.3.4:65012" });
    accept();

    greet(socks_method_clear);
    const auto request = read(4);
    BC_REQUIRE_EQUAL(request, system::data_chunk({ socks_version, 0x01, socks_reserved, socks_address_ipv4 }));

    const auto host = read(4u + 2u);
    BC_REQUIRE_EQUAL(host, system::data_chunk({ 0x01, 0x02, 0x03, 0x04, 0xfd, 0xf4 }));
    write({ socks_version, 0x00, socks_reserved, socks_address_ipv4, 0, 0, 0, 0, 0, 0 });

    BC_REQUIRE_EQUAL(result.get(), error::success);
}

BC_FIXTURE_TEST_CASE(connector_socks__connect__ipv6_address__ipv6_request, socks_setup_fixture)
{
    create();
    auto result = connect(config::address{ "[2020:db8::1]:65012" });
    accept();

    greet(socks_method_clear);
    const auto request = read(4);
    BC_REQUIRE_EQUAL(request, system::data_chunk({ socks_version, 0x01, socks_reserved, socks_address_ipv6 }));

    read(16u + 2u);
    write({ socks_version, 0x00, socks_reserved, socks_address_ipv4, 0, 0, 0, 0, 0, 0 });

    BC_REQUIRE_EQUAL(result.get(), error::success);
}

BC_FIXTURE_TEST_CASE(connector_socks__connect__unproxied__direct_success, socks_setup_fixture)
{
    socks_settings.socks = {};
    create();
    auto result = connect(config::endpoint{ acceptor.local_endpoint() });
    accept();

    BC_REQUIRE_EQUAL(result.get(), error::success);
}

BC_FIXTURE_TEST_CASE(connector_socks__connect__proxy_refused__connection_refused, socks_setup_fixture)
{
    refuse();
    create();
    BC_REQUIRE_EQUAL(connect(config::endpoint{ SOCKS_TARGET_ENDPOINT }).get(), error::connection_refused);
}

BC_FIXTURE_TEST_CASE(connector_socks__connect__greeting_closed__peer_disconnect, socks_setup_fixture)
{
    create();
    auto result = connect(config::endpoint{ SOCKS_TARGET_ENDPOINT });
    accept();

    read(3);
    proxy.close();

    BC_REQUIRE_EQUAL(result.get(), error::peer_disconnect);
}

BC_FIXTURE_TEST_CASE(connector_socks__connect__username_overflow__socks_username, socks_setup_fixture)
{
    socks_settings.username = std::string(256, 'u');
    socks_settings.password = "pass";
    create();
    auto result = connect(config::endpoint{ SOCKS_TARGET_ENDPOINT });
    accept();

    greet(socks_method_basic);

    BC_REQUIRE_EQUAL(result.get(), error::socks_username);
}

BC_FIXTURE_TEST_CASE(connector_socks__connect__password_overflow__socks_password, socks_setup_fixture)
{
    socks_settings.username = "user";
    socks_settings.password = std::string(256, 'p');
    create();
    auto result = connect(config::endpoint{ SOCKS_TARGET_ENDPOINT });
    accept();

    greet(socks_method_basic);

    BC_REQUIRE_EQUAL(result.get(), error::socks_password);
}

BC_FIXTURE_TEST_CASE(connector_socks__connect__maximum_credentials__expected_authenticator, socks_setup_fixture)
{
    socks_settings.username = std::string(255, 'u');
    socks_settings.password = std::string(255, 'p');
    create();
    auto result = connect(config::endpoint{ SOCKS_TARGET_ENDPOINT });
    accept();

    greet(socks_method_basic);
    const auto authenticator = read(1u + 1u + 255u + 1u + 255u);
    BC_REQUIRE_EQUAL(authenticator.at(1), 255u);
    BC_REQUIRE_EQUAL(authenticator.at(257), 255u);
    write({ socks_basic_version, 0x01 });

    BC_REQUIRE_EQUAL(result.get(), error::socks_authentication);
}

BC_FIXTURE_TEST_CASE(connector_socks__connect__authentication_closed__peer_disconnect, socks_setup_fixture)
{
    socks_settings.username = "user";
    socks_settings.password = "pass";
    create();
    auto result = connect(config::endpoint{ SOCKS_TARGET_ENDPOINT });
    accept();

    greet(socks_method_basic);
    read(11);
    proxy.close();

    BC_REQUIRE_EQUAL(result.get(), error::peer_disconnect);
}

BC_FIXTURE_TEST_CASE(connector_socks__connect__host_name_overflow__socks_host_name, socks_setup_fixture)
{
    create();
    auto result = connect(config::endpoint{ std::string(255, 'h'), 42 });
    accept();

    greet(socks_method_clear);

    BC_REQUIRE_EQUAL(result.get(), error::socks_host_name);
}

BC_FIXTURE_TEST_CASE(connector_socks__connect__maximum_host_name__expected_request, socks_setup_fixture)
{
    create();
    auto result = connect(config::endpoint{ std::string(254, 'h'), 42 });
    accept();

    greet(socks_method_clear);
    const auto request = read(4u + 1u + 254u + 2u);
    BC_REQUIRE_EQUAL(request.at(3), socks_address_fqdn);
    BC_REQUIRE_EQUAL(request.at(4), 254u);
    BC_REQUIRE_EQUAL(request.at(259), 0x00);
    BC_REQUIRE_EQUAL(request.at(260), 42u);
    write({ socks_version, 0x05, socks_reserved, socks_address_ipv4 });

    BC_REQUIRE_EQUAL(result.get(), error::socks_connection_refused);
}

BC_FIXTURE_TEST_CASE(connector_socks__connect__request_closed__peer_disconnect, socks_setup_fixture)
{
    create();
    auto result = connect(config::endpoint{ SOCKS_TARGET_ENDPOINT });
    accept();

    greet(socks_method_clear);
    read(4);
    const auto length = read(1);
    read(length.front() + 2u);
    proxy.close();

    BC_REQUIRE_EQUAL(result.get(), error::peer_disconnect);
}

BC_FIXTURE_TEST_CASE(connector_socks__connect__reply_length_closed__peer_disconnect, socks_setup_fixture)
{
    create();
    auto result = connect(config::endpoint{ SOCKS_TARGET_ENDPOINT });
    accept();

    greet(socks_method_clear);
    read(4);
    const auto length = read(1);
    read(length.front() + 2u);
    write({ socks_version, 0x00, socks_reserved, socks_address_fqdn });
    proxy.close();

    BC_REQUIRE_EQUAL(result.get(), error::peer_disconnect);
}

BC_FIXTURE_TEST_CASE(connector_socks__connect__reply_address_truncated__peer_disconnect, socks_setup_fixture)
{
    create();
    auto result = connect(config::endpoint{ SOCKS_TARGET_ENDPOINT });
    accept();

    greet(socks_method_clear);
    read(4);
    const auto length = read(1);
    read(length.front() + 2u);
    write({ socks_version, 0x00, socks_reserved, socks_address_ipv4, 0, 0 });
    proxy.close();

    BC_REQUIRE_EQUAL(result.get(), error::peer_disconnect);
}

BC_AUTO_TEST_CASE(connector_socks__socks_response__reply_codes__expected)
{
    BC_REQUIRE_EQUAL(accessor::socks_response(0x00), error::success);
    BC_REQUIRE_EQUAL(accessor::socks_response(0x01), error::socks_failure);
    BC_REQUIRE_EQUAL(accessor::socks_response(0x02), error::socks_disallowed);
    BC_REQUIRE_EQUAL(accessor::socks_response(0x03), error::socks_net_unreachable);
    BC_REQUIRE_EQUAL(accessor::socks_response(0x04), error::socks_host_unreachable);
    BC_REQUIRE_EQUAL(accessor::socks_response(0x05), error::socks_connection_refused);
    BC_REQUIRE_EQUAL(accessor::socks_response(0x06), error::socks_connection_expired);
    BC_REQUIRE_EQUAL(accessor::socks_response(0x07), error::socks_unsupported_command);
    BC_REQUIRE_EQUAL(accessor::socks_response(0x08), error::socks_unsupported_address);
    BC_REQUIRE_EQUAL(accessor::socks_response(0x09), error::socks_unassigned_failure);
    BC_REQUIRE_EQUAL(accessor::socks_response(0xff), error::socks_unassigned_failure);
}

BC_AUTO_TEST_SUITE_END()
