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

using namespace std::chrono_literals;

using rpc_promise = std::shared_ptr<std::promise<code>>;

static rpc_promise rpc_make_promise() NOEXCEPT
{
    return std::make_shared<std::promise<code>>();
}

static code rpc_await(const rpc_promise& promise) NOEXCEPT
{
    auto future = promise->get_future();
    BOOST_REQUIRE(future.wait_for(5s) == std::future_status::ready);
    return future.get();
}

static count_handler rpc_complete(const rpc_promise& promise) NOEXCEPT
{
    return [=](const code& ec, size_t) NOEXCEPT
    {
        promise->set_value(ec);
    };
}

struct rpc_loopback_fixture
{
    DELETE_COPY_MOVE(rpc_loopback_fixture);

    static constexpr uint16_t port = 65130;

    rpc_loopback_fixture(size_t maximum=1'000'000) NOEXCEPT
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
            .maximum_request = maximum,
            .minimum_buffer = settings::tcp_server{ "test" }.minimum_buffer,
            .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer
        };

        server = std::make_shared<network::socket>(log, pool_.service(), std::move(params));
        const auto accepted = rpc_make_promise();
        server->accept(acceptor_, [=](const code& accept_ec) NOEXCEPT
        {
            accepted->set_value(accept_ec);
        });

        client.connect(local, ec);
        BOOST_REQUIRE(!ec);
        BOOST_REQUIRE_EQUAL(rpc_await(accepted), error::success);
    }

    ~rpc_loopback_fixture() NOEXCEPT
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

    std::string receive_line() NOEXCEPT
    {
        boost_code ec{};
        std::string line{};
        boost::asio::read_until(client, boost::asio::dynamic_buffer(line), '\n', ec);
        BOOST_REQUIRE(!ec);
        return line;
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

struct rpc_small_fixture
  : rpc_loopback_fixture
{
    rpc_small_fixture() NOEXCEPT
      : rpc_loopback_fixture(16)
    {
    }
};

BOOST_AUTO_TEST_SUITE(socket_rpc_tests)

BOOST_FIXTURE_TEST_CASE(socket__rpc_read__request__expected, rpc_loopback_fixture)
{
    http::flat_buffer buffer{ 4096 };
    rpc::request request{};
    const auto result = rpc_make_promise();
    server->rpc_read(buffer, request, rpc_complete(result));

    send(R"({"jsonrpc":"2.0","id":42,"method":"echo","params":["hello"]})" "\n");
    BOOST_REQUIRE_EQUAL(rpc_await(result), error::success);
    BOOST_REQUIRE(request.message.jsonrpc == rpc::version::v2);
    BOOST_REQUIRE_EQUAL(request.message.method, "echo");
    BOOST_REQUIRE(request.message.id.has_value());
    BOOST_REQUIRE_EQUAL(std::get<rpc::code_t>(request.message.id.value()), 42);
    BOOST_REQUIRE(!request.changed);
    BOOST_REQUIRE(!request.lax_params);
}

BOOST_FIXTURE_TEST_CASE(socket__rpc_read__batch__elements_delivered, rpc_loopback_fixture)
{
    http::flat_buffer buffer{ 4096 };
    rpc::request first{};
    const auto result1 = rpc_make_promise();
    server->rpc_read(buffer, first, rpc_complete(result1));

    send(R"([{"jsonrpc":"2.0","id":1,"method":"a","params":[]},{"jsonrpc":"2.0","id":2,"method":"b","params":[]}])");
    BOOST_REQUIRE_EQUAL(rpc_await(result1), error::success);
    BOOST_REQUIRE_EQUAL(first.message.method, "a");
    BOOST_REQUIRE(first.changed);

    rpc::request second{};
    second.batch = true;
    const auto result2 = rpc_make_promise();
    server->rpc_read(buffer, second, rpc_complete(result2));
    BOOST_REQUIRE_EQUAL(rpc_await(result2), error::success);
    BOOST_REQUIRE_EQUAL(second.message.method, "b");
    BOOST_REQUIRE(!second.changed);
}

BOOST_FIXTURE_TEST_CASE(socket__rpc_read__trailing_comma__syntax, rpc_loopback_fixture)
{
    http::flat_buffer buffer{ 4096 };
    rpc::request request{};
    const auto result = rpc_make_promise();
    server->rpc_read(buffer, request, rpc_complete(result));

    send(R"({"jsonrpc":"2.0","id":1,"method":"echo",})" "\n");
    BOOST_REQUIRE_EQUAL(rpc_await(result), error::syntax);
}

BOOST_FIXTURE_TEST_CASE(socket__rpc_read__missing_method__failure, rpc_loopback_fixture)
{
    http::flat_buffer buffer{ 4096 };
    rpc::request request{};
    const auto result = rpc_make_promise();
    server->rpc_read(buffer, request, rpc_complete(result));

    send(R"({"jsonrpc":"2.0","id":1,"params":[]})" "\n");
    BOOST_REQUIRE(rpc_await(result));
}

BOOST_FIXTURE_TEST_CASE(socket__rpc_read__exceeds_maximum__message_overflow, rpc_small_fixture)
{
    http::flat_buffer buffer{ 4096 };
    rpc::request request{};
    const auto result = rpc_make_promise();
    server->rpc_read(buffer, request, rpc_complete(result));

    send(R"({"jsonrpc":"2.0","id":1,"method":"echo","params":["0123456789abcdef)");
    BOOST_REQUIRE_EQUAL(rpc_await(result), error::message_overflow);
}

BOOST_FIXTURE_TEST_CASE(socket__rpc_read__peer_close__peer_disconnect, rpc_loopback_fixture)
{
    http::flat_buffer buffer{ 4096 };
    rpc::request request{};
    const auto result = rpc_make_promise();
    server->rpc_read(buffer, request, rpc_complete(result));

    boost_code ignore{};
    client.shutdown(asio::socket::shutdown_send, ignore);
    BOOST_REQUIRE_EQUAL(rpc_await(result), error::peer_disconnect);
}

BOOST_FIXTURE_TEST_CASE(socket__rpc_read__stop__channel_stopped, rpc_loopback_fixture)
{
    http::flat_buffer buffer{ 4096 };
    rpc::request request{};
    const auto result = rpc_make_promise();
    server->rpc_read(buffer, request, rpc_complete(result));

    server->stop();
    BOOST_REQUIRE_EQUAL(rpc_await(result), error::channel_stopped);
}

BOOST_FIXTURE_TEST_CASE(socket__rpc_write__response__newline_terminated_json, rpc_loopback_fixture)
{
    rpc::response response{};
    response.message = { .jsonrpc = rpc::version::v2, .id = rpc::code_t{ 42 }, .result = rpc::value_t{ rpc::string_t{ "hello" } } };
    const auto result = rpc_make_promise();
    server->rpc_write(std::move(response), rpc_complete(result));

    const auto line = receive_line();
    BOOST_REQUIRE_EQUAL(rpc_await(result), error::success);
    BOOST_REQUIRE_EQUAL(line.back(), '\n');

    const auto object = boost::json::parse(line).as_object();
    BOOST_REQUIRE_EQUAL(object.at("jsonrpc").as_string(), "2.0");
    BOOST_REQUIRE_EQUAL(object.at("id").to_number<int64_t>(), 42);
    BOOST_REQUIRE_EQUAL(object.at("result").as_string(), "hello");
    BOOST_REQUIRE(!object.contains("error"));
}

BOOST_FIXTURE_TEST_CASE(socket__rpc_notify__notification__newline_terminated_json_without_id, rpc_loopback_fixture)
{
    rpc::request notification{};
    notification.message = { .jsonrpc = rpc::version::v2, .method = "note", .params = rpc::array_t{ rpc::value_t{ rpc::string_t{ "x" } } } };
    const auto result = rpc_make_promise();
    server->rpc_notify(std::move(notification), rpc_complete(result));

    const auto line = receive_line();
    BOOST_REQUIRE_EQUAL(rpc_await(result), error::success);
    BOOST_REQUIRE_EQUAL(line.back(), '\n');

    const auto object = boost::json::parse(line).as_object();
    BOOST_REQUIRE_EQUAL(object.at("jsonrpc").as_string(), "2.0");
    BOOST_REQUIRE_EQUAL(object.at("method").as_string(), "note");
    BOOST_REQUIRE_EQUAL(object.at("params").as_array().at(0).as_string(), "x");
    BOOST_REQUIRE(!object.contains("id"));
}

BOOST_FIXTURE_TEST_CASE(socket__body_read__unreadable_body__end_of_stream, rpc_loopback_fixture)
{
    http::flat_buffer buffer{ 4096 };
    http::request request{};
    request.body() = rpc::response{};
    const auto result = rpc_make_promise();
    server->body_read(buffer, request, rpc_complete(result));
    BOOST_REQUIRE_EQUAL(rpc_await(result), error::end_of_stream);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(channel_rpc_tests)

struct echo_methods
{
    static constexpr std::tuple methods
    {
        rpc::method<"echo", std::string>{ "text" }
    };

    template <typename... Args>
    using subscriber = network::unsubscriber<Args...>;

    template <size_t Index>
    using at = rpc::method_at<methods, Index>;

    using echo = at<0>;
};

using echo_interface = rpc::publish<echo_methods>;
using echo_channel = channel_rpc<echo_interface>;
using echo_responder = std::function<void(const echo_channel::ptr&, const std::string&)>;

static const network::settings echo_settings{ system::chain::selection::mainnet };
static const echo_channel::options_t echo_options{ "test" };

struct rpc_channel_fixture
  : rpc_loopback_fixture
{
    rpc_channel_fixture() NOEXCEPT
      : channel(std::make_shared<echo_channel>(log, server, 42, echo_settings, echo_options))
    {
        const auto promise = stopped;
        channel->subscribe_stop([=](const code& ec) NOEXCEPT
        {
            promise->set_value(ec);
        }, [](const code&) NOEXCEPT {});
    }

    ~rpc_channel_fixture() NOEXCEPT
    {
        channel->stop(error::service_stopped);
    }

    void start(const echo_responder& respond) NOEXCEPT
    {
        const auto self = channel;
        const auto started = rpc_make_promise();
        boost::asio::post(channel->strand(), [=]() NOEXCEPT
        {
            self->subscribe<echo_interface::echo>([=](const code& ec, echo_interface::echo, std::string text) NOEXCEPT
            {
                if (ec)
                    return false;

                respond(self, text);
                return true;
            });

            self->resume();
            started->set_value(error::success);
        });

        BOOST_REQUIRE_EQUAL(rpc_await(started), error::success);
    }

    const rpc_promise stopped{ rpc_make_promise() };
    echo_channel::ptr channel;
};

static void ignore_echo(const echo_channel::ptr&, const std::string&) NOEXCEPT
{
}

BOOST_FIXTURE_TEST_CASE(channel_rpc__resume__request__dispatched_and_result_sent, rpc_channel_fixture)
{
    start([](const echo_channel::ptr& self, const std::string& text) NOEXCEPT
    {
        self->send_result(rpc::value_t{ rpc::string_t{ text } }, [](const code&) NOEXCEPT {});
    });

    send(R"({"jsonrpc":"2.0","id":7,"method":"echo","params":["hello"]})" "\n");

    const auto object = boost::json::parse(receive_line()).as_object();
    BOOST_REQUIRE_EQUAL(object.at("jsonrpc").as_string(), "2.0");
    BOOST_REQUIRE_EQUAL(object.at("id").to_number<int64_t>(), 7);
    BOOST_REQUIRE_EQUAL(object.at("result").as_string(), "hello");
}

BOOST_FIXTURE_TEST_CASE(channel_rpc__send_code__request__error_response, rpc_channel_fixture)
{
    start([](const echo_channel::ptr& self, const std::string&) NOEXCEPT
    {
        self->send_code(error::not_found, [](const code&) NOEXCEPT {});
    });

    send(R"({"jsonrpc":"2.0","id":"abc","method":"echo","params":["hello"]})" "\n");

    const auto object = boost::json::parse(receive_line()).as_object();
    BOOST_REQUIRE_EQUAL(object.at("id").as_string(), "abc");
    BOOST_REQUIRE(!object.contains("result"));
    BOOST_REQUIRE_EQUAL(object.at("error").at("code").to_number<int64_t>(), code{ error::not_found }.value());
    BOOST_REQUIRE_EQUAL(object.at("error").at("message").as_string(), code{ error::not_found }.message());
}

BOOST_FIXTURE_TEST_CASE(channel_rpc__send_notification__request__notification_sent, rpc_channel_fixture)
{
    start([](const echo_channel::ptr& self, const std::string& text) NOEXCEPT
    {
        self->send_notification("note", rpc::array_t{ rpc::value_t{ rpc::string_t{ text } } }, [](const code&) NOEXCEPT {});
    });

    send(R"({"jsonrpc":"2.0","id":1,"method":"echo","params":["hello"]})" "\n");

    const auto object = boost::json::parse(receive_line()).as_object();
    BOOST_REQUIRE_EQUAL(object.at("method").as_string(), "note");
    BOOST_REQUIRE_EQUAL(object.at("params").as_array().at(0).as_string(), "hello");
    BOOST_REQUIRE(!object.contains("id"));
}

BOOST_FIXTURE_TEST_CASE(channel_rpc__receive__unknown_method__unexpected_method, rpc_channel_fixture)
{
    start(ignore_echo);
    send(R"({"jsonrpc":"2.0","id":1,"method":"unknown","params":[]})" "\n");
    BOOST_REQUIRE_EQUAL(rpc_await(stopped), error::unexpected_method);
}

BOOST_FIXTURE_TEST_CASE(channel_rpc__receive__v1_batch_element__jsonrpc_batch_requires_v2, rpc_channel_fixture)
{
    start(ignore_echo);
    send(R"([{"id":1,"method":"echo","params":["hello"]}])");
    BOOST_REQUIRE_EQUAL(rpc_await(stopped), error::jsonrpc_batch_requires_v2);
}

BOOST_FIXTURE_TEST_CASE(channel_rpc__receive__peer_close__peer_disconnect, rpc_channel_fixture)
{
    start(ignore_echo);

    boost_code ignore{};
    client.shutdown(asio::socket::shutdown_send, ignore);
    BOOST_REQUIRE_EQUAL(rpc_await(stopped), error::peer_disconnect);
}

BOOST_AUTO_TEST_SUITE_END()
