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
};

BOOST_AUTO_TEST_CASE(socket__construct__default__closed_not_stopped_expected)
{
    const logger log{};
    threadpool pool(1);
    constexpr auto maximum = 42u;
    connector::parameters params{.maximum_request = maximum };
    const auto instance = std::make_shared<socket_accessor>(log, pool.service(), std::move(params));

    BOOST_REQUIRE(!instance->stranded());
    BOOST_REQUIRE(&instance->get_strand() == &instance->strand());
    BOOST_REQUIRE(instance->get_endpoint() == instance->endpoint());
    BOOST_REQUIRE(!instance->get_endpoint().is_address());
    BOOST_REQUIRE(instance->get_address() == instance->address());
    BOOST_REQUIRE(instance->get_address() == config::address{});
    BOOST_REQUIRE_EQUAL(instance->get_maximum_request(), maximum);
    instance->stop();
}

BOOST_AUTO_TEST_CASE(socket__accept__cancel_acceptor__channel_stopped)
{
    const logger log{};
    threadpool pool(2);
    connector::parameters params{ .maximum_request = 42u };
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
////    connector::parameters params{ .maximum_request = 42u };
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
////    connector::parameters params{ .maximum_request = 42u };
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
    connector::parameters params{ .maximum_request = 42u };
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
    connector::parameters params{ .maximum_request = 42u };
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
    connector::parameters params{ .maximum_request = 1'000'000u };

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

BOOST_AUTO_TEST_SUITE_END()
