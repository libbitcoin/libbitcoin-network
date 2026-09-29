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
#include <sstream>

using namespace std::chrono_literals;

using http_promise = std::shared_ptr<std::promise<code>>;

static http_promise http_make_promise() NOEXCEPT
{
    return std::make_shared<std::promise<code>>();
}

static code http_await(const http_promise& promise) NOEXCEPT
{
    auto future = promise->get_future();
    BOOST_REQUIRE(future.wait_for(5s) == std::future_status::ready);
    return future.get();
}

static count_handler http_complete(const http_promise& promise) NOEXCEPT
{
    return [=](const code& ec, size_t) NOEXCEPT
    {
        promise->set_value(ec);
    };
}

static std::string http_chunk(const std::string& data) NOEXCEPT
{
    std::ostringstream out{};
    out << std::hex << data.size() << "\r\n" << data << "\r\n";
    return out.str();
}

static std::string http_post(const std::string& body) NOEXCEPT
{
    return "POST / HTTP/1.1\r\nHost: example.com\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
}

static const std::string http_upgrade_request
{
    "GET /chat HTTP/1.1\r\n"
    "Host: server.example.com\r\n"
    "Upgrade: websocket\r\n"
    "Connection: Upgrade\r\n"
    "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
    "Sec-WebSocket-Version: 13\r\n"
    "\r\n"
};

static const std::string http_keyless_upgrade_request
{
    "GET /chat HTTP/1.1\r\n"
    "Host: server.example.com\r\n"
    "Upgrade: websocket\r\n"
    "Connection: Upgrade\r\n"
    "Sec-WebSocket-Version: 13\r\n"
    "\r\n"
};

static const std::string http_unknown_method_request
{
    "FOO / HTTP/1.1\r\n"
    "Host: example.com\r\n"
    "\r\n"
};

static const std::string http_get_request
{
    "GET /index HTTP/1.1\r\n"
    "Host: example.com\r\n"
    "Accept: application/json, text/plain\r\n"
    "\r\n"
};

static const std::string http_authorized_get_request
{
    "GET /index HTTP/1.1\r\n"
    "Host: example.com\r\n"
    "Authorization: Basic dGVzdDoxMjPCow==\r\n"
    "\r\n"
};

using client_response = boost::beast::http::response<boost::beast::http::string_body>;

struct http_loopback_fixture
{
    DELETE_COPY_MOVE(http_loopback_fixture);

    static constexpr uint16_t port = 65131;

    http_loopback_fixture() NOEXCEPT
      : pool_(2), strand_(pool_.service().get_executor()), acceptor_(strand_),
        client(client_service_)
    {
        const asio::endpoint local(asio::ipv4::loopback(), port);
        boost_code ec{};
        acceptor_.open(local.protocol(), ec);
        BOOST_REQUIRE(!ec);
        acceptor_.set_option(asio::reuse_address(true), ec);
        BOOST_REQUIRE(!ec);
        acceptor_.bind(local, ec);
        BOOST_REQUIRE(!ec);
        acceptor_.listen(1, ec);
        BOOST_REQUIRE(!ec);

        network::socket::parameters params
        {
            .maximum_request = 1'000'000,
            .minimum_buffer = settings::tcp_server{ "test" }.minimum_buffer,
            .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer
        };

        server = std::make_shared<network::socket>(log, pool_.service(), std::move(params));
        const auto accepted = http_make_promise();
        server->accept(acceptor_, [=](const code& accept_ec) NOEXCEPT
        {
            accepted->set_value(accept_ec);
        });

        client.connect(local, ec);
        BOOST_REQUIRE(!ec);
        BOOST_REQUIRE_EQUAL(http_await(accepted), error::success);
    }

    ~http_loopback_fixture() NOEXCEPT
    {
        boost_code ignore{};
        client.close(ignore);
        server->stop();
        pool_.stop();
        BOOST_REQUIRE(pool_.join());
    }

    void send(const std::string& text) NOEXCEPT
    {
        boost_code ec{};
        boost::asio::write(client, boost::asio::buffer(text), ec);
        BOOST_REQUIRE(!ec);
    }

    client_response receive() NOEXCEPT
    {
        boost_code ec{};
        http::flat_buffer buffer{};
        client_response response{};
        boost::beast::http::read(client, buffer, response, ec);
        BOOST_REQUIRE(!ec);
        return response;
    }

    ws::socket upgrade() NOEXCEPT
    {
        const auto socket = server;
        const auto buffer = std::make_shared<http::flat_buffer>();
        const auto request = std::make_shared<http::request>();
        const auto accepted = http_make_promise();
        server->http_read(*buffer, *request, [=](const code&, size_t) NOEXCEPT
        {
            accepted->set_value(socket->accept_websocket(*request));
        });

        boost_code ec{};
        ws::socket websocket{ std::move(client) };
        websocket.handshake("127.0.0.1", "/", ec);
        BOOST_REQUIRE(!ec);
        BOOST_REQUIRE_EQUAL(http_await(accepted), error::success);
        return websocket;
    }

    const logger log{};

private:
    threadpool pool_;
    asio::strand strand_;
    asio::acceptor acceptor_;
    asio::context client_service_{};

public:
    asio::socket client;
    network::socket::ptr server{};
};

BOOST_AUTO_TEST_SUITE(socket_http_tests)

// detect

BOOST_FIXTURE_TEST_CASE(socket__detect__json_object__downgraded, http_loopback_fixture)
{
    http::flat_buffer buffer{ 4096 };
    const auto result = http_make_promise();
    server->detect(buffer, http_complete(result));

    send(R"({"jsonrpc":"2.0","id":1,"method":"echo","params":[]})" "\n");
    BOOST_REQUIRE_EQUAL(http_await(result), error::success);
    BOOST_REQUIRE(server->detected());
    BOOST_REQUIRE(server->downgraded());
}

BOOST_FIXTURE_TEST_CASE(socket__detect__leading_line_breaks_json_array__downgraded, http_loopback_fixture)
{
    http::flat_buffer buffer{ 4096 };
    const auto result = http_make_promise();
    server->detect(buffer, http_complete(result));

    send("\r\n\n[");
    BOOST_REQUIRE_EQUAL(http_await(result), error::success);
    BOOST_REQUIRE(server->detected());
    BOOST_REQUIRE(server->downgraded());
}

BOOST_FIXTURE_TEST_CASE(socket__detect__http_request__not_downgraded, http_loopback_fixture)
{
    http::flat_buffer buffer{ 4096 };
    const auto result = http_make_promise();
    server->detect(buffer, http_complete(result));

    send(http_get_request);
    BOOST_REQUIRE_EQUAL(http_await(result), error::success);
    BOOST_REQUIRE(server->detected());
    BOOST_REQUIRE(!server->downgraded());
}

BOOST_FIXTURE_TEST_CASE(socket__detect__peer_close__peer_disconnect, http_loopback_fixture)
{
    http::flat_buffer buffer{ 4096 };
    const auto result = http_make_promise();
    server->detect(buffer, http_complete(result));

    boost_code ignore{};
    client.shutdown(asio::socket::shutdown_send, ignore);
    BOOST_REQUIRE_EQUAL(http_await(result), error::peer_disconnect);
    BOOST_REQUIRE(!server->detected());
}

// http_read

BOOST_FIXTURE_TEST_CASE(socket__http_read__unknown_method__bad_method, http_loopback_fixture)
{
    http::flat_buffer buffer{ 4096 };
    http::request request{};
    const auto result = http_make_promise();
    server->http_read(buffer, request, http_complete(result));

    send(http_unknown_method_request);
    BOOST_REQUIRE_EQUAL(http_await(result), error::bad_method);
}

BOOST_FIXTURE_TEST_CASE(socket__http_read__stop__channel_stopped, http_loopback_fixture)
{
    http::flat_buffer buffer{ 4096 };
    http::request request{};
    const auto result = http_make_promise();
    server->http_read(buffer, request, http_complete(result));

    server->stop();
    BOOST_REQUIRE_EQUAL(http_await(result), error::channel_stopped);
}

// http_read_header

BOOST_FIXTURE_TEST_CASE(socket__http_read_header__upgrade__upgrade, http_loopback_fixture)
{
    http::flat_buffer buffer{ 4096 };
    network::socket::http_parser parser{};
    const auto result = http_make_promise();
    server->http_read_header(buffer, parser, http_complete(result));

    send(http_upgrade_request);
    BOOST_REQUIRE_EQUAL(http_await(result), error::upgrade);
    BOOST_REQUIRE_EQUAL(parser.get().target(), "/chat");
}

BOOST_FIXTURE_TEST_CASE(socket__http_read_header__unknown_method__bad_method, http_loopback_fixture)
{
    http::flat_buffer buffer{ 4096 };
    network::socket::http_parser parser{};
    const auto result = http_make_promise();
    server->http_read_header(buffer, parser, http_complete(result));

    send(http_unknown_method_request);
    BOOST_REQUIRE_EQUAL(http_await(result), error::bad_method);
}

BOOST_FIXTURE_TEST_CASE(socket__http_read_header__stop__channel_stopped, http_loopback_fixture)
{
    http::flat_buffer buffer{ 4096 };
    network::socket::http_parser parser{};
    const auto result = http_make_promise();
    server->http_read_header(buffer, parser, http_complete(result));

    server->stop();
    BOOST_REQUIRE_EQUAL(http_await(result), error::channel_stopped);
}

// http_read_body/http_read_some

BOOST_FIXTURE_TEST_CASE(socket__http_read_body__content_length_json_rpc__done, http_loopback_fixture)
{
    http::flat_buffer buffer{ 4096 };
    network::socket::http_parser parser{};
    const auto header = http_make_promise();
    server->http_read_header(buffer, parser, http_complete(header));

    send(http_post(R"({"jsonrpc":"2.0","id":1,"method":"echo","params":["hello"]})"));
    BOOST_REQUIRE_EQUAL(http_await(header), error::success);
    BOOST_REQUIRE(parser.is_header_done());

    const auto result = http_make_promise();
    server->http_read_body(buffer, parser, http_complete(result));
    BOOST_REQUIRE_EQUAL(http_await(result), error::success);
    BOOST_REQUIRE(parser.is_done());
    BOOST_REQUIRE(parser.get().body().contains<rpc::request>());
    BOOST_REQUIRE_EQUAL(parser.get().body().get<rpc::request>().message.method, "echo");
}

BOOST_FIXTURE_TEST_CASE(socket__http_read_body__chunked_json_rpc__done, http_loopback_fixture)
{
    http::flat_buffer buffer{ 4096 };
    network::socket::http_parser parser{};
    const auto header = http_make_promise();
    server->http_read_header(buffer, parser, http_complete(header));

    send("POST / HTTP/1.1\r\nHost: example.com\r\nContent-Type: application/json\r\nTransfer-Encoding: chunked\r\n\r\n" + http_chunk(R"({"jsonrpc":"2.0","id)") + http_chunk(R"(":1,"method":"echo","params":["hello"]})") + "0\r\n\r\n");
    BOOST_REQUIRE_EQUAL(http_await(header), error::success);
    BOOST_REQUIRE(parser.chunked());

    const auto result = http_make_promise();
    server->http_read_body(buffer, parser, http_complete(result));
    BOOST_REQUIRE_EQUAL(http_await(result), error::success);
    BOOST_REQUIRE(parser.is_done());
    BOOST_REQUIRE_EQUAL(parser.get().body().get<rpc::request>().message.method, "echo");
}

BOOST_FIXTURE_TEST_CASE(socket__http_read_some__json_rpc_batch__need_buffer_first_element, http_loopback_fixture)
{
    http::flat_buffer buffer{ 4096 };
    network::socket::http_parser parser{};
    const auto header = http_make_promise();
    server->http_read_header(buffer, parser, http_complete(header));

    send(http_post(R"([{"jsonrpc":"2.0","id":1,"method":"a","params":[]},{"jsonrpc":"2.0","id":2,"method":"b","params":[]}])"));
    BOOST_REQUIRE_EQUAL(http_await(header), error::success);

    const auto result = http_make_promise();
    server->http_read_some(buffer, parser, http_complete(result));
    BOOST_REQUIRE_EQUAL(http_await(result), error::need_buffer);
    BOOST_REQUIRE(!parser.is_done());
    BOOST_REQUIRE(parser.get().body().get<rpc::request>().changed);
    BOOST_REQUIRE_EQUAL(parser.get().body().get<rpc::request>().message.method, "a");
}

BOOST_FIXTURE_TEST_CASE(socket__http_read_some__stop__channel_stopped, http_loopback_fixture)
{
    http::flat_buffer buffer{ 4096 };
    network::socket::http_parser parser{};
    const auto header = http_make_promise();
    server->http_read_header(buffer, parser, http_complete(header));

    send("POST / HTTP/1.1\r\nHost: example.com\r\nContent-Type: application/json\r\nContent-Length: 100\r\n\r\n");
    BOOST_REQUIRE_EQUAL(http_await(header), error::success);

    const auto result = http_make_promise();
    server->http_read_some(buffer, parser, http_complete(result));
    server->stop();
    BOOST_REQUIRE_EQUAL(http_await(result), error::channel_stopped);
}

// http_write_header/rpc_write_chunk

BOOST_FIXTURE_TEST_CASE(socket__rpc_write_chunk__batch_parts__chunked_json_array, http_loopback_fixture)
{
    http::response response{ http::status::ok, 11 };
    response.body() = http::empty_value{};
    response.chunked(true);
    const auto header = http_make_promise();
    server->http_write_header(std::move(response), http_complete(header));
    BOOST_REQUIRE_EQUAL(http_await(header), error::success);

    rpc::response open{};
    open.changed = true;
    open.message = { .jsonrpc = rpc::version::v2, .id = rpc::code_t{ 1 }, .result = rpc::value_t{ rpc::string_t{ "a" } } };
    const auto first = http_make_promise();
    server->rpc_write_chunk(std::move(open), http_complete(first));
    BOOST_REQUIRE_EQUAL(http_await(first), error::success);

    rpc::response close{};
    close.batch = true;
    close.changed = true;
    const auto last = http_make_promise();
    server->rpc_write_chunk(std::move(close), http_complete(last));
    BOOST_REQUIRE_EQUAL(http_await(last), error::success);

    const auto received = receive();
    BOOST_REQUIRE_EQUAL(received.result_int(), 200u);
    BOOST_REQUIRE(received.chunked());

    const auto array = boost::json::parse(received.body()).as_array();
    BOOST_REQUIRE_EQUAL(array.size(), 1u);
    BOOST_REQUIRE_EQUAL(array.at(0).at("id").to_number<int64_t>(), 1);
    BOOST_REQUIRE_EQUAL(array.at(0).at("result").as_string(), "a");
}

// websocket

BOOST_FIXTURE_TEST_CASE(socket__ws_read__text_after_control_frames__expected, http_loopback_fixture)
{
    auto websocket = upgrade();
    BOOST_REQUIRE(server->websocket());

    http::flat_buffer buffer{ 4096 };
    const auto result = http_make_promise();
    server->ws_read(buffer, http_complete(result));

    boost_code ec{};
    websocket.ping("ping", ec);
    BOOST_REQUIRE(!ec);
    websocket.pong("pong", ec);
    BOOST_REQUIRE(!ec);
    websocket.write(boost::asio::buffer(std::string{ "hello" }), ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(http_await(result), error::success);
    BOOST_REQUIRE_EQUAL(boost::beast::buffers_to_string(buffer.data()), "hello");
}

BOOST_FIXTURE_TEST_CASE(socket__ws_read__close_frame__websocket_closed, http_loopback_fixture)
{
    auto websocket = upgrade();
    http::flat_buffer buffer{ 4096 };
    const auto result = http_make_promise();
    server->ws_read(buffer, http_complete(result));

    boost_code ec{};
    websocket.close(boost::beast::websocket::close_code::normal, ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(http_await(result), error::websocket_closed);
}

BOOST_FIXTURE_TEST_CASE(socket__http_read_header__websocket__operation_failed, http_loopback_fixture)
{
    auto websocket = upgrade();
    http::flat_buffer buffer{ 4096 };
    network::socket::http_parser parser{};
    const auto result = http_make_promise();
    server->http_read_header(buffer, parser, http_complete(result));
    BOOST_REQUIRE_EQUAL(http_await(result), error::operation_failed);
}

BOOST_FIXTURE_TEST_CASE(socket__http_read_body__websocket__operation_failed, http_loopback_fixture)
{
    auto websocket = upgrade();
    http::flat_buffer buffer{ 4096 };
    network::socket::http_parser parser{};
    const auto result = http_make_promise();
    server->http_read_body(buffer, parser, http_complete(result));
    BOOST_REQUIRE_EQUAL(http_await(result), error::operation_failed);
}

BOOST_FIXTURE_TEST_CASE(socket__http_read_some__websocket__operation_failed, http_loopback_fixture)
{
    auto websocket = upgrade();
    http::flat_buffer buffer{ 4096 };
    network::socket::http_parser parser{};
    const auto result = http_make_promise();
    server->http_read_some(buffer, parser, http_complete(result));
    BOOST_REQUIRE_EQUAL(http_await(result), error::operation_failed);
}

BOOST_FIXTURE_TEST_CASE(socket__http_write_header__websocket__operation_failed, http_loopback_fixture)
{
    auto websocket = upgrade();
    const auto result = http_make_promise();
    server->http_write_header(http::response{ http::status::ok, 11 }, http_complete(result));
    BOOST_REQUIRE_EQUAL(http_await(result), error::operation_failed);
}

BOOST_FIXTURE_TEST_CASE(socket__rpc_write_chunk__websocket__operation_failed, http_loopback_fixture)
{
    auto websocket = upgrade();
    const auto result = http_make_promise();
    server->rpc_write_chunk(rpc::response{}, http_complete(result));
    BOOST_REQUIRE_EQUAL(http_await(result), error::operation_failed);
}

BOOST_FIXTURE_TEST_CASE(socket__accept_websocket__missing_key__operation_failed_bad_request, http_loopback_fixture)
{
    const auto socket = server;
    const auto buffer = std::make_shared<http::flat_buffer>();
    const auto request = std::make_shared<http::request>();
    const auto read = http_make_promise();
    const auto accepted = http_make_promise();
    server->http_read(*buffer, *request, [=](const code& ec, size_t) NOEXCEPT
    {
        read->set_value(ec);
        accepted->set_value(socket->accept_websocket(*request));
    });

    send(http_keyless_upgrade_request);
    BOOST_REQUIRE_EQUAL(http_await(read), error::upgrade);
    BOOST_REQUIRE_EQUAL(http_await(accepted), error::operation_failed);
    BOOST_REQUIRE(!server->websocket());
    BOOST_REQUIRE_EQUAL(receive().result_int(), 400u);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(channel_http_tests)

class mock_channel_http
  : public channel_http
{
public:
    using channel_http::channel_http;

    // Call must be stranded.
    void subscribe_stop1(result_handler handler) NOEXCEPT
    {
        channel_http::subscribe_stop(std::move(handler));
    }

    void stop(const code& ec) NOEXCEPT override
    {
        channel_http::stop(ec);

        if (!stop_)
        {
            stop_ = true;
            stopped_.set_value(ec);
        }
    }

    code require_stopped() const NOEXCEPT
    {
        return stopped_.get_future().get();
    }

private:
    mutable bool stop_{ false };
    mutable std::promise<code> stopped_;
};

using namespace http;
const channel_http::options_t options{ "test" };

BOOST_AUTO_TEST_CASE(channel_http__stopped__default__false)
{
    constexpr auto expected = 42u;
    const logger log{};
    threadpool pool(1);
    asio::strand strand(pool.service().get_executor());
    const settings set(system::chain::selection::mainnet);
    network::socket::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto channel_ptr = std::make_shared<channel_http>(log, socket_ptr, expected, set, options);
    BOOST_REQUIRE(!channel_ptr->stopped());

    BOOST_REQUIRE_NE(channel_ptr->nonce(), zero);
    BOOST_REQUIRE_EQUAL(channel_ptr->identifier(), expected);

    // Stop is asynchronous, threadpool destruct blocks until all complete.
    // Calling stop here sets channel.stopped and prevents destructor assertion.
    channel_ptr->stop(error::invalid_magic);
}

BOOST_AUTO_TEST_CASE(channel_http__properties__default__expected)
{
    const logger log{};
    threadpool pool(1);
    asio::strand strand(pool.service().get_executor());
    const settings set(system::chain::selection::mainnet);
    network::socket::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto channel_ptr = std::make_shared<channel_http>(log, socket_ptr, 42, set, options);

    BOOST_REQUIRE(!channel_ptr->address());
    BOOST_REQUIRE_NE(channel_ptr->nonce(), 0u);

    // Stop is asynchronous, threadpool destruct blocks until all complete.
    // Calling stop here sets channel.stopped and prevents destructor assertion.
    channel_ptr->stop(error::invalid_magic);
}

BOOST_AUTO_TEST_CASE(channel_http__subscribe_message__subscribed__expected)
{
    const logger log{};
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    const settings set(system::chain::selection::mainnet);
    network::socket::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto channel_ptr = std::make_shared<channel_http>(log, socket_ptr, 42, set, options);
    constexpr auto expected_ec = error::invalid_magic;

    auto result = true;
    std::promise<bool> subscribed{};
    std::promise<code> message_stopped{};
    boost::asio::post(channel_ptr->strand(), [&]() NOEXCEPT
    {
        channel_ptr->subscribe<method::get>(
            [&](code ec, method::get::cptr request) NOEXCEPT
            {
                result &= !request;
                message_stopped.set_value(ec);
                return true;
            });

        subscribed.set_value(true);
    });

    BOOST_REQUIRE(subscribed.get_future().get());
    BOOST_REQUIRE(!channel_ptr->stopped());

    // Stop is asynchronous, threadpool destruct blocks until all complete.
    // Calling stop here sets channel.stopped and prevents destructor assertion.
    channel_ptr->stop(expected_ec);

    BOOST_REQUIRE(channel_ptr->stopped());
    BOOST_REQUIRE_EQUAL(message_stopped.get_future().get(), expected_ec);
    BOOST_REQUIRE(result);
}

BOOST_AUTO_TEST_CASE(channel_http__stop__all_subscribed__expected)
{
    const logger log{};
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    const settings set(system::chain::selection::mainnet);
    network::socket::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto channel_ptr = std::make_shared<mock_channel_http>(log, socket_ptr, 42, set, options);
    constexpr auto expected_ec = error::invalid_magic;

    std::promise<bool> subscribed{};
    std::promise<code> stop2_stopped;
    std::promise<code> stop_subscribed;
    channel_ptr->subscribe_stop(
        [=, &stop2_stopped](code ec) NOEXCEPT
        {
            stop2_stopped.set_value(ec);
        },
        [=, &stop_subscribed](code ec) NOEXCEPT
        {
            stop_subscribed.set_value(ec);
        });

    auto result = true;
    std::promise<code> stop1_stopped;
    std::promise<code> message_stopped;
    boost::asio::post(channel_ptr->strand(), [&]() NOEXCEPT
    {
        channel_ptr->subscribe_stop1([=, &stop1_stopped](code ec) NOEXCEPT
        {
            stop1_stopped.set_value(ec);
        });

        channel_ptr->subscribe<method::post>(
            [&](code ec, const method::post::cptr& request) NOEXCEPT
            {
                result &= !request;
                message_stopped.set_value(ec);
                return true;
            });

        subscribed.set_value(true);
    });

    BOOST_REQUIRE(subscribed.get_future().get());
    BOOST_REQUIRE(!channel_ptr->stopped());
    BOOST_REQUIRE_EQUAL(stop_subscribed.get_future().get(), error::success);

    // Stop is asynchronous, threadpool destruct blocks until all complete.
    // Calling stop here sets channel.stopped and prevents destructor assertion.
    channel_ptr->stop(expected_ec);

    BOOST_REQUIRE(channel_ptr->stopped());
    BOOST_REQUIRE_EQUAL(message_stopped.get_future().get(), expected_ec);
    BOOST_REQUIRE_EQUAL(stop1_stopped.get_future().get(), expected_ec);
    BOOST_REQUIRE_EQUAL(stop2_stopped.get_future().get(), expected_ec);
    BOOST_REQUIRE(result);
}

BOOST_AUTO_TEST_CASE(channel_http__send__not_connected__expected)
{
    const logger log{};
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    const settings set(system::chain::selection::mainnet);
    network::socket::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto channel_ptr = std::make_shared<channel_http>(log, socket_ptr, 42, set, options);

    auto result = true;
    std::promise<code> promise;
    const auto handler = [&](code ec) NOEXCEPT
    {
        result &= channel_ptr->stopped();
        promise.set_value(ec);
    };

    BOOST_REQUIRE(!channel_ptr->stopped());
    boost::asio::post(channel_ptr->strand(), [&]() NOEXCEPT
    {
        channel_ptr->send({}, handler);
    });

    // 10009 (WSAEBADF, invalid file handle) gets mapped to bad_stream.
    BOOST_REQUIRE_EQUAL(promise.get_future().get().value(), error::bad_stream);
    BOOST_REQUIRE(result);

    // Stop is asynchronous, threadpool destruct blocks until all complete.
    // Calling stop here sets channel.stopped and prevents destructor assertion.
    channel_ptr->stop(error::invalid_magic);
}

BOOST_AUTO_TEST_CASE(channel_http__send__not_connected_move__expected)
{
    const logger log{};
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    const settings set(system::chain::selection::mainnet);
    network::socket::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto channel_ptr = std::make_shared<channel_http>(log, socket_ptr, 42, set, options);

    auto result = true;
    std::promise<code> promise;

    BOOST_REQUIRE(!channel_ptr->stopped());
    boost::asio::post(channel_ptr->strand(), [&]() NOEXCEPT
    {
        channel_ptr->send(http::response{}, [&](code ec)
        {
            result &= channel_ptr->stopped();
            promise.set_value(ec);
        });
    });

    // 10009 (WSAEBADF, invalid file handle) gets mapped to bad_stream.
    BOOST_REQUIRE_EQUAL(promise.get_future().get().value(), error::bad_stream);
    BOOST_REQUIRE(result);

    // Stop is asynchronous, threadpool destruct blocks until all complete.
    // Calling stop here sets channel.stopped and prevents destructor assertion.
    channel_ptr->stop(error::invalid_magic);
}

BOOST_AUTO_TEST_CASE(channel_http__stopped__resume_after_read_fail__true)
{
    const logger log{};
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    const settings set(system::chain::selection::mainnet);
    network::socket::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto channel_ptr = std::make_shared<mock_channel_http>(log, socket_ptr, 42, set, options);

    std::promise<bool> stopped_after_resume;
    boost::asio::post(channel_ptr->strand(), [=, &stopped_after_resume]() NOEXCEPT
    {
        // Resume queues up a (failing) read that will invoke stopped.
        channel_ptr->resume();
        stopped_after_resume.set_value(channel_ptr->stopped());
    });

    BOOST_REQUIRE(!stopped_after_resume.get_future().get());
    BOOST_REQUIRE(channel_ptr->require_stopped());

    std::promise<bool> stopped_after_read_fail;
    boost::asio::post(channel_ptr->strand(), [=, &stopped_after_read_fail]() NOEXCEPT
    {
        stopped_after_read_fail.set_value(channel_ptr->stopped());
    });

    BOOST_REQUIRE(stopped_after_read_fail.get_future().get());

    // Stop is asynchronous, threadpool destruct blocks until all complete.
    // Calling stop here sets channel.stopped and prevents destructor assertion.
    channel_ptr->stop(error::invalid_magic);
}

// connected

struct credentialed_options
  : channel_http::options_t
{
    credentialed_options() NOEXCEPT
      : channel_http::options_t("test")
    {
        credentials.emplace_back("test:123\xC2\xA3:getinfo");
    }
};

static const credentialed_options credentialed{};
static const network::settings http_settings{ system::chain::selection::mainnet };
static const std::string rfc7617_authorization{ "Basic dGVzdDoxMjPCow==" };

using http_responder = std::function<void(const channel_http::ptr&, const http::request&)>;

static void http_ok(const channel_http::ptr& self, const http::request& request) NOEXCEPT
{
    response out{ status::ok, request.version() };
    out.content_length(zero);
    self->send(std::move(out), [](const code&) NOEXCEPT {});
}

static response json_response(const boost::json::object& model) NOEXCEPT
{
    response out{ status::ok, 11 };
    json_value body{};
    body.model = model;
    out.body() = std::move(body);
    return out;
}

struct http_channel_fixture
  : http_loopback_fixture
{
    http_channel_fixture(const channel_http::options_t& options_=options) NOEXCEPT
      : channel(std::make_shared<channel_http>(log, server, 42, http_settings, options_))
    {
        const auto promise = stopped;
        channel->subscribe_stop([=](const code& ec) NOEXCEPT
        {
            promise->set_value(ec);
        }, [](const code&) NOEXCEPT {});
    }

    ~http_channel_fixture() NOEXCEPT
    {
        channel->stop(error::service_stopped);
    }

    void start(const http_responder& respond) NOEXCEPT
    {
        const auto self = channel;
        const auto started = http_make_promise();
        boost::asio::post(channel->strand(), [=]() NOEXCEPT
        {
            self->subscribe<method::get>([=](const code& ec, const method::get::cptr& request) NOEXCEPT
            {
                if (!ec) respond(self, *request);
            });

            self->subscribe<method::post>([=](const code& ec, const method::post::cptr& request) NOEXCEPT
            {
                if (!ec) respond(self, *request);
            });

            self->subscribe<method::unknown>([=](const code& ec, const method::unknown::cptr& request) NOEXCEPT
            {
                if (!ec) respond(self, *request);
            });

            self->resume();
            started->set_value(error::success);
        });

        BOOST_REQUIRE_EQUAL(http_await(started), error::success);
    }

    const http_promise stopped{ http_make_promise() };
    channel_http::ptr channel;
};

struct credentialed_channel_fixture
  : http_channel_fixture
{
    credentialed_channel_fixture() NOEXCEPT
      : http_channel_fixture(credentialed)
    {
    }
};

BOOST_FIXTURE_TEST_CASE(channel_http__resume__get__dispatched_and_response_sent, http_channel_fixture)
{
    const auto target = std::make_shared<std::promise<std::string>>();
    start([=](const channel_http::ptr& self, const http::request& request) NOEXCEPT
    {
        target->set_value(std::string{ request.target() });
        http_ok(self, request);
    });

    send(http_get_request);
    BOOST_REQUIRE_EQUAL(receive().result_int(), 200u);
    BOOST_REQUIRE_EQUAL(target->get_future().get(), "/index");
}

BOOST_FIXTURE_TEST_CASE(channel_http__resume__large_post_then_get__both_dispatched, http_channel_fixture)
{
    start(http_ok);

    const std::string body(8 * kilobyte, 'x');
    send("POST / HTTP/1.1\r\nHost: example.com\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body);
    BOOST_REQUIRE_EQUAL(receive().result_int(), 200u);

    send(http_get_request);
    BOOST_REQUIRE_EQUAL(receive().result_int(), 200u);
}

BOOST_FIXTURE_TEST_CASE(channel_http__resume__lax_params__jsonrpc_params_not_collection, http_channel_fixture)
{
    start(http_ok);
    send(http_post(R"({"jsonrpc":"2.0","id":1,"method":"echo","params":"hello"})"));
    BOOST_REQUIRE_EQUAL(http_await(stopped), error::jsonrpc_params_not_collection);
}

BOOST_FIXTURE_TEST_CASE(channel_http__stop__reading__stopped_with_code, http_channel_fixture)
{
    start(http_ok);
    channel->stop(error::invalid_magic);
    BOOST_REQUIRE_EQUAL(http_await(stopped), error::invalid_magic);
}

BOOST_FIXTURE_TEST_CASE(channel_http__resume__unauthorized_get__unauthorized, credentialed_channel_fixture)
{
    start(http_ok);
    send(http_get_request);
    BOOST_REQUIRE_EQUAL(receive().result_int(), 401u);
    BOOST_REQUIRE_EQUAL(http_await(stopped), error::unauthorized);
}

BOOST_FIXTURE_TEST_CASE(channel_http__resume__authorized_get__permitted_methods, credentialed_channel_fixture)
{
    const auto permissions = std::make_shared<std::promise<std::string>>();
    start([=](const channel_http::ptr& self, const http::request& request) NOEXCEPT
    {
        permissions->set_value(std::to_string(self->authorized()) + std::to_string(self->permitted("getinfo")) + std::to_string(self->permitted("stop")));
        http_ok(self, request);
    });

    send(http_authorized_get_request);
    BOOST_REQUIRE_EQUAL(receive().result_int(), 200u);
    BOOST_REQUIRE_EQUAL(permissions->get_future().get(), "110");
}

BOOST_FIXTURE_TEST_CASE(channel_http__resume__upgrade__websocket_message_dispatched_and_answered, http_channel_fixture)
{
    start([](const channel_http::ptr& self, const http::request&) NOEXCEPT
    {
        self->send(json_response({ { "b", 2 } }), [](const code&) NOEXCEPT {});
    });

    boost_code ec{};
    ws::socket websocket{ std::move(client) };
    websocket.handshake("127.0.0.1", "/", ec);
    BOOST_REQUIRE(!ec);

    websocket.write(boost::asio::buffer(std::string{ R"({"a":1})" }), ec);
    BOOST_REQUIRE(!ec);

    http::flat_buffer buffer{};
    websocket.read(buffer, ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(channel->websocket());
    BOOST_REQUIRE_EQUAL(boost::json::parse(boost::beast::buffers_to_string(buffer.data())).at("b").to_number<int64_t>(), 2);
}

BOOST_FIXTURE_TEST_CASE(channel_http__notify__websocket__message_sent, http_channel_fixture)
{
    start([](const channel_http::ptr& self, const http::request&) NOEXCEPT
    {
        self->notify(json_response({ { "n", 1 } }), [](const code&) NOEXCEPT {});
    });

    boost_code ec{};
    ws::socket websocket{ std::move(client) };
    websocket.handshake("127.0.0.1", "/", ec);
    BOOST_REQUIRE(!ec);

    websocket.write(boost::asio::buffer(std::string{ "{}" }), ec);
    BOOST_REQUIRE(!ec);

    http::flat_buffer buffer{};
    websocket.read(buffer, ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(boost::json::parse(boost::beast::buffers_to_string(buffer.data())).at("n").to_number<int64_t>(), 1);
}

BOOST_FIXTURE_TEST_CASE(channel_http__resume__unauthorized_upgrade__unauthorized, credentialed_channel_fixture)
{
    start(http_ok);

    boost_code ec{};
    boost::beast::websocket::response_type response{};
    ws::socket websocket{ std::move(client) };
    websocket.handshake(response, "127.0.0.1", "/", ec);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE_EQUAL(response.result_int(), 401u);
    BOOST_REQUIRE_EQUAL(http_await(stopped), error::unauthorized);
}

BOOST_FIXTURE_TEST_CASE(channel_http__resume__keyless_upgrade__operation_failed, http_channel_fixture)
{
    start(http_ok);
    send(http_keyless_upgrade_request);
    BOOST_REQUIRE_EQUAL(receive().result_int(), 400u);
    BOOST_REQUIRE_EQUAL(http_await(stopped), error::operation_failed);
}

// strand properties

BOOST_AUTO_TEST_CASE(channel_http__register_methods__tokens__accumulated_once)
{
    const logger log{};
    threadpool pool(2);
    network::socket::parameters params{ .maximum_request = 42, .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto channel_ptr = std::make_shared<channel_http>(log, socket_ptr, 42, http_settings, options);

    std::promise<std::string> methods{};
    boost::asio::post(channel_ptr->strand(), [&]() NOEXCEPT
    {
        channel_ptr->register_methods("abc a");
        channel_ptr->register_methods(" b  a abc ");
        methods.set_value(channel_ptr->methods());
    });

    BOOST_REQUIRE_EQUAL(methods.get_future().get(), "abc a b");
    channel_ptr->stop(error::invalid_magic);
}

BOOST_AUTO_TEST_CASE(channel_http__set_claimed__default__latched)
{
    const logger log{};
    threadpool pool(2);
    network::socket::parameters params{ .maximum_request = 42, .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto channel_ptr = std::make_shared<channel_http>(log, socket_ptr, 42, http_settings, options);

    std::promise<std::string> claims{};
    boost::asio::post(channel_ptr->strand(), [&]() NOEXCEPT
    {
        const auto before = channel_ptr->claimed();
        channel_ptr->set_claimed();
        claims.set_value(std::to_string(before) + std::to_string(channel_ptr->claimed()));
    });

    BOOST_REQUIRE_EQUAL(claims.get_future().get(), "01");
    channel_ptr->stop(error::invalid_magic);
}

BOOST_AUTO_TEST_CASE(channel_http__permitted__no_credentials__authorized_and_permitted)
{
    const logger log{};
    threadpool pool(2);
    network::socket::parameters params{ .maximum_request = 42, .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto channel_ptr = std::make_shared<channel_http>(log, socket_ptr, 42, http_settings, options);

    BOOST_REQUIRE(channel_ptr->authorized());
    BOOST_REQUIRE(channel_ptr->permitted("stop"));
    channel_ptr->stop(error::invalid_magic);
}

BOOST_AUTO_TEST_CASE(channel_http__set_authorized__rfc7617_digest__authorized_for_credential_methods)
{
    const logger log{};
    threadpool pool(2);
    network::socket::parameters params{ .maximum_request = 42, .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto channel_ptr = std::make_shared<channel_http>(log, socket_ptr, 42, http_settings, credentialed);

    std::promise<std::string> states{};
    boost::asio::post(channel_ptr->strand(), [&]() NOEXCEPT
    {
        const auto initial = channel_ptr->authorized();
        channel_ptr->set_authorized(system::sha256_hash(std::string{ "Basic wrong" }));
        const auto wrong = channel_ptr->authorized();
        channel_ptr->set_authorized(system::sha256_hash(rfc7617_authorization));
        states.set_value(std::to_string(initial) + std::to_string(wrong) + std::to_string(channel_ptr->authorized()) + std::to_string(channel_ptr->permitted("getinfo")) + std::to_string(channel_ptr->permitted("stop")));
    });

    BOOST_REQUIRE_EQUAL(states.get_future().get(), "00110");
    channel_ptr->stop(error::invalid_magic);
}

BOOST_AUTO_TEST_SUITE_END()
