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

#include <future>

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

    const config::endpoint& get_endpoint() const NOEXCEPT
    {
        return endpoint_;
    }

    const config::address& get_address() const NOEXCEPT
    {
        return address_;
    }

    size_t get_maximum_request() const NOEXCEPT
    {
        return maximum_;
    }

    // Call must be stranded.
    bool is_base1() const NOEXCEPT
    {
        return is_base();
    }

    // Call must be stranded.
    void async_read_some1(const asio::mutable_buffer& buffer, const count_handler& handler) NOEXCEPT
    {
        async_read_some(buffer, handler);
    }
};

BOOST_AUTO_TEST_CASE(socket__construct__default__closed_not_stopped_expected)
{
    const logger log{};
    threadpool pool(1);
    constexpr auto maximum = 42u;
    connector::parameters params
    {
        .maximum_request = maximum,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer
    };

    const auto instance = std::make_shared<socket_accessor>(log, pool.service(), std::move(params));

    BOOST_REQUIRE(!instance->stranded());
    BOOST_REQUIRE(&instance->get_strand() == &instance->strand());
    BOOST_REQUIRE(instance->get_endpoint() == instance->endpoint());
    BOOST_REQUIRE(!instance->get_endpoint().is_address());
    BOOST_REQUIRE(instance->get_address() == instance->address());
    BOOST_REQUIRE(instance->get_address() == config::address{});
    BOOST_REQUIRE_EQUAL(instance->get_maximum_request(), maximum);
    BOOST_REQUIRE(instance->binding() == config::endpoint{});
    BOOST_REQUIRE_EQUAL(instance->slot(), max_size_t);
    instance->stop();
}

BOOST_AUTO_TEST_CASE(socket__accept__cancel_acceptor__channel_stopped)
{
    const logger log{};
    threadpool pool(2);
    connector::parameters params
    {
        .maximum_request = 42u,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer
    };

    const auto instance = std::make_shared<socket_accessor>(log, pool.service(), std::move(params));
    asio::strand strand(pool.service().get_executor());
    asio::acceptor acceptor(strand);

    boost_code ec;
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
        BOOST_REQUIRE(!instance->get_endpoint().is_address());
    });

    // Stopping the socket does not cancel the acceptor but precludes assertion.
    instance->stop();

    // Acceptor must be canceled to release/invoke the accept handler.
    // This has the same effect as network::acceptor::stop.
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        boost_code ignore;
        acceptor.cancel(ignore);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
}

////// Test is a race condition, periodically fails.
////BOOST_AUTO_TEST_CASE(socket__connect__invalid__error)
////{
////    const logger log{};
////    threadpool pool(2);
////    connector::parameters params
////    {
////        .maximum_request = 42u,
////        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer
////    };
////
////    const auto instance = std::make_shared<socket_accessor>(log, pool.service(), std::move(params));
////    asio::strand strand(pool.service().get_executor());
////
////    const asio::endpoint endpoint(asio::tcp::v6(), 42);
////    asio::endpoints endpoints;
////    endpoints.create(endpoint, "bogus.xxx", "service");
////
////    instance->connect(endpoints, [instance](const code& ec)
////    {
////        // Socket cancellation sets channel_stopped and default ipv6 authority.
////        // TODO: 3 (ERROR_PATH_NOT_FOUND) code gets mapped to unknown.
////        ////BOOST_REQUIRE(ec == error::unknown || ec == error::channel_stopped);
////
////        // gcc/ubuntu (one time in CI):
////        // fatal error: in "socket_tests/socket__connect__invalid__error":
////        // std::length_error: basic_string::_M_create
////        BOOST_REQUIRE(ec);
////
////        // Default authority string inconsistent due to context.
////        ////BOOST_REQUIRE_EQUAL(instance->get_authority().to_string(), "[::ffff:0:0]");
////        ////BOOST_REQUIRE_EQUAL(instance->get_authority().to_string(), "0.0.0.0");
////    });
////
////    // Test race.
////    std::this_thread::sleep_for(microseconds(1));
////
////    // Stopping the socket cancels connection attempt, but should fail first.
////    // Delay above increases chance that connect fail will win consistently.
////    instance->stop();
////
////    pool.stop();
////    BOOST_REQUIRE(pool.join());
////}
////
////BOOST_AUTO_TEST_CASE(socket__read_some__disconnected__error)
////{
////    const logger log{};
////    threadpool pool(2);
////    connector::parameters params
////    {
////        .maximum_request = 42u,
////        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer
////    };
////
////    const auto instance = std::make_shared<socket_accessor>(log, pool.service(), std::move(params));
////
////    system::data_array<42> data{};
////    instance->read_some(asio::mutable_buffer{ data.data(), data.size() },
////        [instance](const code& ec, size_t size)
////        {
////            // 10009 (WSAEBADF, invalid file handle) gets mapped to bad_stream.
////            BOOST_REQUIRE_EQUAL(ec, error::bad_stream);
////            BOOST_REQUIRE_EQUAL(size, zero);
////        });
////
////    // Test race.
////    std::this_thread::sleep_for(microseconds(1));
////
////    // Stopping the socket precludes assertion.
////    instance->stop();
////
////    pool.stop();
////    BOOST_REQUIRE(pool.join());
////}

BOOST_AUTO_TEST_CASE(socket__read__disconnected__error)
{
    const logger log{};
    threadpool pool(2);
    connector::parameters params
    {
        .maximum_request = 42u,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer
    };

    const auto instance = std::make_shared<socket_accessor>(log, pool.service(), std::move(params));

    system::data_array<42> data;
    instance->tcp_read({ data.data(), data.size() },
        [instance](const code& ec, size_t size)
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
    connector::parameters params
    {
        .maximum_request = 42u,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer
    };

    const auto instance = std::make_shared<socket_accessor>(log, pool.service(), std::move(params));

    system::data_array<42> data;
    instance->tcp_write({ data.data(), data.size() },
        [instance](const code& ec, size_t size)
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

BOOST_AUTO_TEST_CASE(socket__connect__bound_loopback__binding_expected)
{
    using namespace std::chrono_literals;

    const logger log{};
    threadpool pool(2);
    connector::parameters params
    {
        .maximum_request = 42u,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer
    };

    // Bind a loopback acceptor on an ephemeral port.
    asio::strand accept_strand(pool.service().get_executor());
    asio::acceptor acceptor(accept_strand);
    boost_code ec{};
    const asio::endpoint bind_endpoint(asio::ipv4::loopback(), 0);

    acceptor.open(bind_endpoint.protocol(), ec);
    BOOST_REQUIRE(!ec);
    acceptor.bind(bind_endpoint, ec);
    BOOST_REQUIRE(!ec);
    acceptor.listen(1, ec);
    BOOST_REQUIRE(!ec);
    const auto port = acceptor.local_endpoint().port();

    const auto server = std::make_shared<socket_accessor>(log, pool.service(), params);
    const auto accept_result = std::make_shared<std::promise<code>>();
    auto accept_future = accept_result->get_future();
    server->accept(acceptor, [=](const code& accept_ec) NOEXCEPT
    {
        accept_result->set_value(accept_ec);
    });

    params.bind = { asio::ipv4::loopback(), 0 };
    const auto client = std::make_shared<socket_accessor>(log, pool.service(), params, config::address{}, config::endpoint{}, false);
    const asio::endpoint peer(asio::ipv4::loopback(), port);
    const auto range = asio::endpoints::create(peer, "127.0.0.1", std::to_string(port));

    const auto connect_result = std::make_shared<std::promise<code>>();
    auto connect_future = connect_result->get_future();
    client->connect(range, [=](const code& connect_ec) NOEXCEPT
    {
        connect_result->set_value(connect_ec);
    });

    BOOST_REQUIRE(connect_future.wait_for(2s) == std::future_status::ready);
    BOOST_REQUIRE_EQUAL(connect_future.get(), error::success);
    BOOST_REQUIRE(accept_future.wait_for(2s) == std::future_status::ready);
    BOOST_REQUIRE_EQUAL(accept_future.get(), error::success);
    BOOST_REQUIRE_EQUAL(client->binding().host(), "127.0.0.1");
    BOOST_REQUIRE_NE(client->binding().port(), 0u);
    BOOST_REQUIRE_EQUAL(client->get_address().to_host(), "127.0.0.1");

    client->stop();
    server->stop();
    pool.stop();
    BOOST_REQUIRE(pool.join());
}

BOOST_AUTO_TEST_CASE(socket__connect__bound_family_mismatch__resolve_failed)
{
    using namespace std::chrono_literals;

    const logger log{};
    threadpool pool(2);
    connector::parameters params
    {
        .maximum_request = 42u,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer,
        .bind = { asio::ipv4::loopback(), 0 }
    };

    const auto client = std::make_shared<socket_accessor>(log, pool.service(), params, config::address{}, config::endpoint{}, false);
    const asio::endpoint peer(asio::ipv6::loopback(), 42);
    const auto range = asio::endpoints::create(peer, "::1", "42");

    const auto connect_result = std::make_shared<std::promise<code>>();
    auto connect_future = connect_result->get_future();
    client->connect(range, [=](const code& connect_ec) NOEXCEPT
    {
        connect_result->set_value(connect_ec);
    });

    BOOST_REQUIRE(connect_future.wait_for(2s) == std::future_status::ready);
    BOOST_REQUIRE_EQUAL(connect_future.get(), error::resolve_failed);

    client->stop();
    pool.stop();
    BOOST_REQUIRE(pool.join());
}

BOOST_AUTO_TEST_CASE(socket__connect__bound_unavailable__net_unreachable)
{
    using namespace std::chrono_literals;

    const logger log{};
    threadpool pool(2);
    connector::parameters params
    {
        .maximum_request = 42u,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer,
        .bind = { boost::asio::ip::make_address_v4("192.0.2.1"), 0 }
    };

    const auto client = std::make_shared<socket_accessor>(log, pool.service(), params, config::address{}, config::endpoint{}, false);
    const asio::endpoint peer(asio::ipv4::loopback(), 42);
    const auto range = asio::endpoints::create(peer, "127.0.0.1", "42");

    const auto connect_result = std::make_shared<std::promise<code>>();
    auto connect_future = connect_result->get_future();
    client->connect(range, [=](const code& connect_ec) NOEXCEPT
    {
        connect_result->set_value(connect_ec);
    });

    BOOST_REQUIRE(connect_future.wait_for(2s) == std::future_status::ready);
    BOOST_REQUIRE_EQUAL(connect_future.get(), error::net_unreachable);

    client->stop();
    pool.stop();
    BOOST_REQUIRE(pool.join());
}

// Regression test for a fix to socket::async_write: it used to write every
// body_write() chunk as its own whole websocket message (finish always
// true), splitting one logical response into N independent messages. The
// fix passes finish = !out->more, so only the final chunk closes the
// message. A real (non-library) websocket client is used as ground truth:
// beast's blocking read() returns only once a full message (finish) is
// received, so a single read() call assembling the entire multi-chunk body
// proves the chunks were sent as one multi-frame message rather than as
// several.
BOOST_AUTO_TEST_CASE(socket__body_write__websocket_multiple_chunks__single_message)
{
    using namespace std::chrono_literals;

    const logger log{};
    threadpool pool(2);
    connector::parameters params
    {
        .maximum_request = 1'000'000u,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer
    };

    // Bind a loopback acceptor on an ephemeral port.
    asio::strand accept_strand(pool.service().get_executor());
    asio::acceptor acceptor(accept_strand);
    boost_code ec{};
    const asio::endpoint bind_endpoint(asio::ipv4::loopback(), 0);

    acceptor.open(bind_endpoint.protocol(), ec);
    BOOST_REQUIRE(!ec);
    acceptor.set_option(asio::reuse_address(true), ec);
    BOOST_REQUIRE(!ec);
    acceptor.bind(bind_endpoint, ec);
    BOOST_REQUIRE(!ec);
    acceptor.listen(1, ec);
    BOOST_REQUIRE(!ec);
    const auto port = acceptor.local_endpoint().port();

    const auto server = std::make_shared<socket_accessor>(log, pool.service(), std::move(params));
    const auto buffer = std::make_shared<http::flat_buffer>();
    const auto request = std::make_shared<http::request>();

    // Small size_hint relative to the payload forces writer.get() across
    // many passes, i.e. many socket::async_write calls for one logical body.
    json::body<>::value_type content{};
    content.model = boost::json::object{ { "key", std::string(4000, 'x') } };
    content.size_hint = 32;
    const auto expected = boost::json::serialize(content.model);

    const auto response = std::make_shared<http::response>();
    response->result(boost::beast::http::status::ok);
    response->body() = std::move(content);

    const auto accept_result = std::make_shared<std::promise<code>>();
    const auto upgrade_result = std::make_shared<std::promise<code>>();
    const auto switch_result = std::make_shared<std::promise<code>>();
    const auto write_result = std::make_shared<std::promise<code>>();
    auto accept_future = accept_result->get_future();
    auto upgrade_future = upgrade_result->get_future();
    auto switch_future = switch_result->get_future();
    auto write_future = write_result->get_future();

    // Each stage records unconditionally, results asserted after join.
    server->accept(acceptor,
        [=](const code& accept_ec) mutable
        {
            accept_result->set_value(accept_ec);
            server->http_read(*buffer, *request,
                [=](const code& read_ec, size_t) mutable
                {
                    // The upgrade request is published, accept sends the 101.
                    upgrade_result->set_value(read_ec);
                    switch_result->set_value(server->accept_websocket(*request));
                    server->body_write(std::move(*response),
                        [=](const code& write_ec, size_t) NOEXCEPT
                        {
                            write_result->set_value(write_ec);
                        });
                });
        });

    // Real (blocking) websocket client, independent of the code under test.
    asio::context client_service;
    asio::socket raw(client_service);
    boost_code client_ec{};
    raw.connect({ asio::ipv4::loopback(), port }, client_ec);
    BOOST_REQUIRE(!client_ec);

    ws::socket client(std::move(raw));
    client.handshake("127.0.0.1", "/", client_ec);
    BOOST_REQUIRE(!client_ec);

    // One blocking read of a "complete message" must assemble every chunk.
    http::flat_buffer read_buffer{};
    client.read(read_buffer, client_ec);
    BOOST_REQUIRE(!client_ec);
    BOOST_REQUIRE(client.got_text());

    const auto received = boost::beast::buffers_to_string(read_buffer.data());
    BOOST_REQUIRE_EQUAL(received, expected);

    BOOST_REQUIRE(accept_future.wait_for(2s) == std::future_status::ready);
    BOOST_REQUIRE_EQUAL(accept_future.get(), error::success);
    BOOST_REQUIRE(upgrade_future.wait_for(2s) == std::future_status::ready);
    BOOST_REQUIRE_EQUAL(upgrade_future.get(), error::upgrade);
    BOOST_REQUIRE(switch_future.wait_for(2s) == std::future_status::ready);
    BOOST_REQUIRE_EQUAL(switch_future.get(), error::success);
    BOOST_REQUIRE(write_future.wait_for(2s) == std::future_status::ready);
    BOOST_REQUIRE_EQUAL(write_future.get(), error::success);

    server->stop();
    pool.stop();
    BOOST_REQUIRE(pool.join());
}

BOOST_AUTO_TEST_CASE(socket__http_write__json_body__serialized_body_received)
{
    using namespace std::chrono_literals;

    const logger log{};
    threadpool pool(2);
    connector::parameters params
    {
        .maximum_request = 1'000'000u,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer
    };

    // Bind a loopback acceptor on an ephemeral port.
    asio::strand accept_strand(pool.service().get_executor());
    asio::acceptor acceptor(accept_strand);
    boost_code ec{};
    const asio::endpoint bind_endpoint(asio::ipv4::loopback(), 0);

    acceptor.open(bind_endpoint.protocol(), ec);
    BOOST_REQUIRE(!ec);
    acceptor.set_option(asio::reuse_address(true), ec);
    BOOST_REQUIRE(!ec);
    acceptor.bind(bind_endpoint, ec);
    BOOST_REQUIRE(!ec);
    acceptor.listen(1, ec);
    BOOST_REQUIRE(!ec);
    const auto port = acceptor.local_endpoint().port();

    const auto server = std::make_shared<socket_accessor>(log, pool.service(), std::move(params));
    const auto buffer = std::make_shared<http::flat_buffer>();
    const auto request = std::make_shared<http::request>();

    // The payload exceeds the write buffer, so the body is write chunked.
    json::body<>::value_type content{};
    content.model = boost::json::object{ { "key", std::string(100'000, 'x') } };
    const auto expected = boost::json::serialize(content.model);

    const auto response = std::make_shared<http::response>();
    response->result(boost::beast::http::status::ok);
    response->body() = std::move(content);
    response->prepare_payload();

    const auto accept_result = std::make_shared<std::promise<code>>();
    const auto read_result = std::make_shared<std::promise<code>>();
    const auto write_result = std::make_shared<std::promise<code>>();
    auto accept_future = accept_result->get_future();
    auto read_future = read_result->get_future();
    auto write_future = write_result->get_future();

    // Each stage records unconditionally, results asserted after join.
    server->accept(acceptor,
        [=](const code& accept_ec) mutable
        {
            accept_result->set_value(accept_ec);
            server->http_read(*buffer, *request,
                [=](const code& read_ec, size_t) mutable
                {
                    read_result->set_value(read_ec);
                    server->http_write(std::move(*response),
                        [=](const code& write_ec, size_t) NOEXCEPT
                        {
                            write_result->set_value(write_ec);
                        });
                });
        });

    // Real (blocking) http client, independent of the code under test.
    asio::context client_service;
    asio::socket client(client_service);
    boost_code client_ec{};
    client.connect({ asio::ipv4::loopback(), port }, client_ec);
    BOOST_REQUIRE(!client_ec);

    boost::beast::http::request<boost::beast::http::empty_body> get{ boost::beast::http::verb::get, "/", 11 };
    get.set(boost::beast::http::field::host, "127.0.0.1");
    get.prepare_payload();
    boost::beast::http::write(client, get, client_ec);
    BOOST_REQUIRE(!client_ec);

    // One blocking read must assemble every chunk of the json body.
    http::flat_buffer read_buffer{};
    boost::beast::http::response<boost::beast::http::string_body> received{};
    boost::beast::http::read(client, read_buffer, received, client_ec);
    BOOST_REQUIRE(!client_ec);
    BOOST_REQUIRE_EQUAL(received.result_int(), 200u);
    BOOST_REQUIRE_EQUAL(received.body(), expected);

    BOOST_REQUIRE(accept_future.wait_for(2s) == std::future_status::ready);
    BOOST_REQUIRE_EQUAL(accept_future.get(), error::success);
    BOOST_REQUIRE(read_future.wait_for(2s) == std::future_status::ready);
    BOOST_REQUIRE_EQUAL(read_future.get(), error::success);
    BOOST_REQUIRE(write_future.wait_for(2s) == std::future_status::ready);
    BOOST_REQUIRE_EQUAL(write_future.get(), error::success);

    server->stop();
    pool.stop();
    BOOST_REQUIRE(pool.join());
}

// loopback
// ----------------------------------------------------------------------------

static const asio::endpoint open_endpoint{ asio::ipv4::loopback(), 65121 };
static const asio::endpoint closed_endpoint{ asio::ipv4::loopback(), 65122 };
static const asio::endpoint raw_endpoint{ asio::ipv4::loopback(), 65123 };
static const p2ps::context mainnet_context{ 0xd9b4bef9 };

template <typename Type>
class awaiter
{
public:
    void set(const Type& value) const
    {
        promise_->set_value(value);
    }

    Type get() const
    {
        using namespace std::chrono_literals;
        BOOST_REQUIRE(future_.wait_for(10s) == std::future_status::ready);
        return future_.get();
    }

private:
    std::shared_ptr<std::promise<Type>> promise_{ std::make_shared<std::promise<Type>>() };
    std::shared_future<Type> future_{ promise_->get_future().share() };
};

struct acceptor_fixture
{
    DELETE_COPY_MOVE(acceptor_fixture);

    acceptor_fixture()
    {
        boost_code ec{};
        acceptor.open(open_endpoint.protocol(), ec);
        BOOST_REQUIRE(!ec);
        acceptor.set_option(asio::reuse_address(true), ec);
        BOOST_REQUIRE(!ec);
        acceptor.bind(open_endpoint, ec);
        BOOST_REQUIRE(!ec);
        acceptor.listen(1, ec);
        BOOST_REQUIRE(!ec);
    }

    ~acceptor_fixture()
    {
        pool.stop();
        BOOST_REQUIRE(pool.join());
    }

    std::shared_ptr<socket_accessor> accept(const socket::parameters& params, const awaiter<code>& accepted, bool proxied=false)
    {
        const auto instance = std::make_shared<socket_accessor>(log, pool.service(), params, proxied);
        instance->accept(acceptor, [=](const code& ec) NOEXCEPT
        {
            accepted.set(ec);
        });

        boost_code ec{};
        client.connect(open_endpoint, ec);
        BOOST_REQUIRE(!ec);
        return instance;
    }

    std::shared_ptr<socket_accessor> connect(const socket::parameters& params, const std::vector<asio::endpoint>& peers, const awaiter<code>& connected)
    {
        const auto instance = std::make_shared<socket_accessor>(log, pool.service(), params, config::address{}, config::endpoint{}, false);
        const auto range = asio::endpoints::create(peers.begin(), peers.end(), "127.0.0.1", "0");
        instance->connect(range, [=](const code& ec) NOEXCEPT
        {
            connected.set(ec);
        });

        return instance;
    }

    const logger log{};
    threadpool pool{ 2 };
    asio::strand strand{ pool.service().get_executor() };
    asio::acceptor acceptor{ strand };
    asio::context service{};
    asio::socket client{ service };
    const socket::parameters clear
    {
        .maximum_request = 42u,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer
    };
};

BOOST_FIXTURE_TEST_CASE(socket__service__default__expected, acceptor_fixture)
{
    const auto instance = std::make_shared<socket_accessor>(log, pool.service(), clear);
    BOOST_REQUIRE(&instance->service() == &pool.service());
    instance->stop();
}

BOOST_FIXTURE_TEST_CASE(socket__set_address__stranded__address_and_endpoint_set, acceptor_fixture)
{
    const auto instance = std::make_shared<socket_accessor>(log, pool.service(), clear);
    const config::address expected{ asio::endpoint{ boost::asio::ip::make_address_v4("1.2.3.4"), 42 } };
    const awaiter<bool> set{};
    boost::asio::post(instance->strand(), [=]() NOEXCEPT
    {
        instance->set_address(expected);
        set.set(true);
    });

    BOOST_REQUIRE(set.get());
    BOOST_REQUIRE(instance->address() == expected);
    BOOST_REQUIRE_EQUAL(instance->endpoint().port(), 42u);
    BOOST_REQUIRE_EQUAL(instance->endpoint().host(), "1.2.3.4");
    instance->stop();
}

BOOST_FIXTURE_TEST_CASE(socket__accept__proxied__success_endpoint_set, acceptor_fixture)
{
    const awaiter<code> accepted{};
    const auto instance = accept(clear, accepted, true);

    BOOST_REQUIRE_EQUAL(accepted.get(), error::success);
    BOOST_REQUIRE_EQUAL(instance->endpoint().port(), client.local_endpoint().port());
    BOOST_REQUIRE_EQUAL(instance->binding().port(), open_endpoint.port());
    instance->stop();
}

BOOST_FIXTURE_TEST_CASE(socket__accept__not_listening__invalid_configuration, acceptor_fixture)
{
    asio::acceptor unlistened{ strand };
    boost_code ec{};
    unlistened.open(raw_endpoint.protocol(), ec);
    BOOST_REQUIRE(!ec);
    unlistened.bind(raw_endpoint, ec);
    BOOST_REQUIRE(!ec);

    const awaiter<code> accepted{};
    const auto instance = std::make_shared<socket_accessor>(log, pool.service(), clear);
    instance->accept(unlistened, [=](const code& accept_ec) NOEXCEPT
    {
        accepted.set(accept_ec);
    });

    BOOST_REQUIRE_EQUAL(accepted.get(), error::invalid_configuration);
    instance->stop();
}

BOOST_FIXTURE_TEST_CASE(socket__async_read_some__client_sent__base_expected, acceptor_fixture)
{
    const awaiter<code> accepted{};
    const auto instance = accept(clear, accepted);
    BOOST_REQUIRE_EQUAL(accepted.get(), error::success);

    char byte{};
    const awaiter<bool> base{};
    const awaiter<code> read{};
    boost::asio::post(instance->strand(), [=, &byte]() NOEXCEPT
    {
        base.set(instance->is_base1());
        instance->async_read_some1({ &byte, 1 }, [=](const code& ec, size_t) NOEXCEPT
        {
            read.set(ec);
        });
    });

    boost_code ec{};
    boost::asio::write(client, boost::asio::buffer("x", 1), ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(base.get());
    BOOST_REQUIRE_EQUAL(read.get(), error::success);
    BOOST_REQUIRE_EQUAL(byte, 'x');
    instance->stop();
}

BOOST_FIXTURE_TEST_CASE(socket__connect__refused__connection_refused, acceptor_fixture)
{
    const awaiter<code> connected{};
    const auto instance = connect(clear, { closed_endpoint }, connected);

    BOOST_REQUIRE_EQUAL(connected.get(), error::connection_refused);
    instance->stop();
}

BOOST_FIXTURE_TEST_CASE(socket__connect__bound_first_refused__second_connected, acceptor_fixture)
{
    auto params = clear;
    params.bind = { asio::ipv4::loopback(), 0 };
    const awaiter<code> connected{};
    const auto instance = connect(params, { closed_endpoint, open_endpoint }, connected);

    BOOST_REQUIRE_EQUAL(connected.get(), error::success);
    BOOST_REQUIRE_EQUAL(instance->binding().host(), "127.0.0.1");
    BOOST_REQUIRE_EQUAL(instance->address().port(), open_endpoint.port());
    instance->stop();
}

BOOST_FIXTURE_TEST_CASE(socket__accept__p2ps_v1_prefix__success_unencrypted, acceptor_fixture)
{
    static const system::data_chunk prefix{ 0xf9, 0xbe, 0xb4, 0xd9, 'v', 'e', 'r', 's', 'i', 'o', 'n', 0x00, 0x00, 0x00, 0x00, 0x00 };
    auto params = clear;
    params.context = std::cref(mainnet_context);
    const awaiter<code> accepted{};
    const auto instance = accept(params, accepted);

    boost_code ec{};
    boost::asio::write(client, boost::asio::buffer(prefix), ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(accepted.get(), error::success);

    const awaiter<bool> encrypted{};
    boost::asio::post(instance->strand(), [=]() NOEXCEPT
    {
        encrypted.set(instance->encrypted());
    });

    BOOST_REQUIRE(!encrypted.get());
    instance->stop();
}

BOOST_FIXTURE_TEST_CASE(socket__accept__p2ps_v2_prefix_disconnected__peer_disconnect, acceptor_fixture)
{
    static const system::data_chunk prefix(p2ps::stream::detection_size, 0x42);
    auto params = clear;
    params.context = std::cref(mainnet_context);
    const awaiter<code> accepted{};
    const auto instance = accept(params, accepted);

    boost_code ec{};
    boost::asio::write(client, boost::asio::buffer(prefix), ec);
    BOOST_REQUIRE(!ec);
    client.shutdown(asio::socket::shutdown_send, ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(accepted.get(), error::peer_disconnect);
    instance->stop();
}

BOOST_FIXTURE_TEST_CASE(socket__accept__p2ps_handshake_timeout__stopped, acceptor_fixture)
{
    auto params = clear;
    params.context = std::cref(mainnet_context);
    params.connect_timeout = milliseconds(10);
    const awaiter<code> accepted{};
    const auto instance = accept(params, accepted);

    // The stop's shutdown may complete the pending read with end of file.
    const auto ec = accepted.get();
    BOOST_REQUIRE(ec == error::operation_canceled || ec == error::peer_disconnect);
    BOOST_REQUIRE(instance->stopped());
}

BOOST_FIXTURE_TEST_CASE(socket__connect__p2ps_responder_disconnected__peer_disconnect, acceptor_fixture)
{
    asio::acceptor raw{ service };
    boost_code ec{};
    raw.open(raw_endpoint.protocol(), ec);
    BOOST_REQUIRE(!ec);
    raw.set_option(asio::reuse_address(true), ec);
    BOOST_REQUIRE(!ec);
    raw.bind(raw_endpoint, ec);
    BOOST_REQUIRE(!ec);
    raw.listen(1, ec);
    BOOST_REQUIRE(!ec);

    auto params = clear;
    params.context = std::cref(mainnet_context);
    const awaiter<code> connected{};
    const auto instance = connect(params, { raw_endpoint }, connected);

    asio::socket responder{ service };
    raw.accept(responder, ec);
    BOOST_REQUIRE(!ec);

    system::data_array<64> key{};
    boost::asio::read(responder, boost::asio::buffer(key), ec);
    BOOST_REQUIRE(!ec);
    responder.shutdown(asio::socket::shutdown_send, ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(connected.get(), error::peer_disconnect);
    instance->stop();
}

BOOST_FIXTURE_TEST_CASE(socket__lazy_stop__clear__stopped_client_end_of_file, acceptor_fixture)
{
    const awaiter<code> accepted{};
    const auto instance = accept(clear, accepted);
    BOOST_REQUIRE_EQUAL(accepted.get(), error::success);

    instance->lazy_stop();
    BOOST_REQUIRE(instance->stopped());

    char byte{};
    boost_code ec{};
    client.read_some(boost::asio::buffer(&byte, 1), ec);
    BOOST_REQUIRE(ec == boost::asio::error::eof);
}

BOOST_AUTO_TEST_SUITE_END()
