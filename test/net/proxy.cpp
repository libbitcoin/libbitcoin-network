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

BOOST_AUTO_TEST_SUITE(proxy_tests)

class mock_proxy
  : public proxy
{
public:
    using proxy::watch;
    using proxy::unwatch;
    using proxy::read;
    using proxy::accept_websocket;

    void write_ws(const asio::const_buffer& in, bool binary, count_handler&& handler) NOEXCEPT
    {
        proxy::write(in, binary, std::move(handler));
    }

    void write_rpc(rpc::response&& response, count_handler&& handler) NOEXCEPT
    {
        proxy::write(std::move(response), std::move(handler));
    }

    void notify_rpc(rpc::request&& notification, count_handler&& handler) NOEXCEPT
    {
        proxy::notify(std::move(notification), std::move(handler));
    }

    void write_http(http::response&& response, count_handler&& handler) NOEXCEPT
    {
        proxy::write(std::move(response), std::move(handler));
    }

    void notify_http(http::response&& notification, count_handler&& handler) NOEXCEPT
    {
        proxy::notify(std::move(notification), std::move(handler));
    }

    // Call must be stranded.
    void subscribe_stop1(result_handler handler) NOEXCEPT
    {
        proxy::subscribe_stop(std::move(handler));
    }

    // Call must be stranded.
    steady_clock::duration unconsumed1(size_t bytes,
        const steady_clock::time_point& start) const NOEXCEPT
    {
        return proxy::unconsumed(bytes, start);
    }

    // Call must be stranded.
    void write1(const asio::const_buffer& in,
        count_handler&& handler) NOEXCEPT
    {
        proxy::write(in, std::move(handler));
    }

    // Access protected constructor.
    mock_proxy(const socket::ptr& socket, uint32_t rate_limit=0,
        size_t maximum_backlog=settings::tcp_server{ "mock" }.maximum_backlog) NOEXCEPT
      : proxy(socket, rate_limit, maximum_backlog)
    {
    }
};

class mock_zmtp_socket
  : public network::socket
{
public:
    using socket::socket;

    bool zeromq() const NOEXCEPT override
    {
        return true;
    }
};

// Obtain the deferral for a send of bytes that started at the given offset.
static milliseconds get_unconsumed(uint32_t rate_limit, size_t bytes,
    const steady_clock::duration& elapsed) NOEXCEPT
{
    const logger log{};
    threadpool pool(1);
    socket::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr, rate_limit);

    std::promise<milliseconds> deferral;
    boost::asio::post(proxy_ptr->strand(), [=, &deferral]() NOEXCEPT
    {
        deferral.set_value(std::chrono::duration_cast<milliseconds>(
            proxy_ptr->unconsumed1(bytes, steady_clock::now() - elapsed)));
    });

    const auto result = deferral.get_future().get();
    proxy_ptr->stop(error::invalid_magic);
    pool.stop();
    return result;
}

BOOST_AUTO_TEST_CASE(proxy__unconsumed__unlimited__zero)
{
    BOOST_REQUIRE_EQUAL(get_unconsumed(0, 1000, seconds(0)), milliseconds(0));
}

BOOST_AUTO_TEST_CASE(proxy__unconsumed__untransmitted__full_allocation)
{
    // 1000 bytes at 1000 bytes/second is allocated one second, unconsumed.
    const auto deferral = get_unconsumed(1000, 1000, seconds(0));
    BOOST_REQUIRE_GT(deferral, milliseconds(900));
    BOOST_REQUIRE_LE(deferral, milliseconds(1000));
}

BOOST_AUTO_TEST_CASE(proxy__unconsumed__partly_transmitted__remainder)
{
    // Transmission consumed 250ms of the 1000ms allocation.
    const auto deferral = get_unconsumed(1000, 1000, milliseconds(250));
    BOOST_REQUIRE_GT(deferral, milliseconds(650));
    BOOST_REQUIRE_LE(deferral, milliseconds(750));
}

BOOST_AUTO_TEST_CASE(proxy__unconsumed__fully_transmitted__zero)
{
    // Transmission was slower than the rate limit, so there is nothing to add.
    BOOST_REQUIRE_EQUAL(get_unconsumed(1000, 1000, seconds(2)),
        milliseconds(0));
}

BOOST_AUTO_TEST_CASE(proxy__unconsumed__stopped__zero)
{
    const logger log{};
    threadpool pool(1);
    socket::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr, 1000);
    proxy_ptr->stop(error::invalid_magic);

    std::promise<steady_clock::duration::rep> deferral;
    boost::asio::post(proxy_ptr->strand(), [=, &deferral]() NOEXCEPT
    {
        deferral.set_value(
            proxy_ptr->unconsumed1(1000, steady_clock::now()).count());
    });

    BOOST_REQUIRE_EQUAL(deferral.get_future().get(), 0);
}

BOOST_AUTO_TEST_CASE(proxy__stopped__default__false)
{
    const logger log{};
    threadpool pool(2);
    socket::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr);
    BOOST_REQUIRE(!proxy_ptr->stopped());

    proxy_ptr->stop(error::invalid_magic);
}

BOOST_AUTO_TEST_CASE(proxy__stranded__default__false)
{
    const logger log{};
    threadpool pool(2);
    socket::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr);
    BOOST_REQUIRE(!proxy_ptr->stranded());

    proxy_ptr->stop(error::invalid_magic);
}

BOOST_AUTO_TEST_CASE(proxy__authority__default__expected)
{
    const logger log{};
    threadpool pool(2);
    const config::endpoint default_endpoint{};
    socket::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr);
    BOOST_REQUIRE(proxy_ptr->endpoint() == default_endpoint);

    proxy_ptr->stop(error::invalid_magic);
}

BOOST_AUTO_TEST_CASE(proxy__subscribe_stop__subscribed__expected)
{
    const logger log{};
    threadpool pool(2);
    socket::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr);
    constexpr auto expected_ec = error::invalid_magic;

    std::promise<code> stop2_stopped;
    std::promise<code> stop_subscribed;
    proxy_ptr->subscribe_stop(
        [=, &stop2_stopped](code ec) NOEXCEPT
        {
            stop2_stopped.set_value(ec);
        },
        [=, &stop_subscribed](code ec) NOEXCEPT
        {
            stop_subscribed.set_value(ec);
        });

    BOOST_REQUIRE(!proxy_ptr->stopped());
    BOOST_REQUIRE_EQUAL(stop_subscribed.get_future().get(), error::success);

    proxy_ptr->stop(expected_ec);
    BOOST_REQUIRE_EQUAL(stop2_stopped.get_future().get(), expected_ec);
    BOOST_REQUIRE(proxy_ptr->stopped());
}

BOOST_AUTO_TEST_CASE(proxy__do_subscribe_stop__subscribed__expected)
{
    const logger log{};
    threadpool pool(2);
    socket::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr);
    constexpr auto expected_ec = error::invalid_magic;

    std::promise<code> stop1_stopped;
    boost::asio::post(proxy_ptr->strand(), [&]() NOEXCEPT
    {
        proxy_ptr->subscribe_stop1([=, &stop1_stopped](code ec) NOEXCEPT
        {
            stop1_stopped.set_value(ec);
        });
    });

    BOOST_REQUIRE(!proxy_ptr->stopped());

    proxy_ptr->stop(expected_ec);
    BOOST_REQUIRE_EQUAL(stop1_stopped.get_future().get(), expected_ec);
    BOOST_REQUIRE(proxy_ptr->stopped());
}

BOOST_AUTO_TEST_CASE(proxy__write__within_backlog__not_stopped)
{
    const logger log{};
    threadpool pool(2);
    socket::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr, 0, 1'000'000);

    const std::string text(1024, 'x');
    const asio::const_buffer buffer{ text.data(), text.size() };

    // Both writes are queued inline, as dispatch to the current strand.
    std::promise<bool> queued;
    boost::asio::post(proxy_ptr->strand(), [&]() NOEXCEPT
    {
        proxy_ptr->write1(buffer, [](const code&, size_t) NOEXCEPT {});
        proxy_ptr->write1(buffer, [](const code&, size_t) NOEXCEPT {});
        queued.set_value(true);
    });

    BOOST_REQUIRE(queued.get_future().get());
    BOOST_REQUIRE(!proxy_ptr->stopped());

    proxy_ptr->stop(error::service_stopped);
    pool.stop();
    BOOST_REQUIRE(pool.join());
}

BOOST_AUTO_TEST_CASE(proxy__write__exceeds_backlog__channel_backlog)
{
    const logger log{};
    threadpool pool(2);
    socket::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));

    // The first message is admitted to the idle queue, the second exceeds.
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr, 0, 512);

    std::promise<code> stopped;
    std::promise<code> subscribed;
    proxy_ptr->subscribe_stop([&](code ec) NOEXCEPT
    {
        stopped.set_value(ec);
    },
    [&](code ec) NOEXCEPT
    {
        subscribed.set_value(ec);
    });

    // The stop subscription is posted, so the writes await its completion.
    BOOST_REQUIRE_EQUAL(subscribed.get_future().get(), error::success);

    const std::string text(1024, 'x');
    const asio::const_buffer buffer{ text.data(), text.size() };

    // Both writes are queued inline, so the first cannot complete between.
    boost::asio::post(proxy_ptr->strand(), [&]() NOEXCEPT
    {
        proxy_ptr->write1(buffer, [](const code&, size_t) NOEXCEPT {});
        proxy_ptr->write1(buffer, [](const code&, size_t) NOEXCEPT {});
    });

    BOOST_REQUIRE_EQUAL(stopped.get_future().get(), error::channel_backlog);
    pool.stop();
    BOOST_REQUIRE(pool.join());
}

BOOST_AUTO_TEST_CASE(proxy__write__exceeds_backlog_zeromq__message_dropped)
{
    const logger log{};
    threadpool pool(2);
    socket::parameters params
    {
        .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer
    };
    auto socket_ptr = std::make_shared<mock_zmtp_socket>(log, pool.service(), std::move(params));

    // The first message is admitted to the idle queue, the second exceeds.
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr, 0, 512);

    const std::string text(1024, 'x');
    const asio::const_buffer buffer{ text.data(), text.size() };

    // Both writes are queued inline, so the first cannot complete between.
    std::promise<code> dropped;
    boost::asio::post(proxy_ptr->strand(), [&]() NOEXCEPT
    {
        proxy_ptr->write1(buffer, [](const code&, size_t) NOEXCEPT {});
        proxy_ptr->write1(buffer, [&](const code& ec, size_t) NOEXCEPT
        {
            dropped.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(dropped.get_future().get(), error::message_dropped);
    BOOST_REQUIRE(!proxy_ptr->stopped());

    proxy_ptr->stop(error::service_stopped);
    pool.stop();
    BOOST_REQUIRE(pool.join());
}

// loopback
// ----------------------------------------------------------------------------

static constexpr uint16_t loopback_port = 65120;
static const std::string response1{ R"({"jsonrpc":"2.0","id":1,"result":true})" };
static const std::string response2{ R"({"jsonrpc":"2.0","id":2,"result":true})" };
static const std::string request1{ R"({"jsonrpc":"2.0","id":1,"method":"a"})" };
static const std::string request2{ R"({"jsonrpc":"2.0","id":2,"method":"b"})" };
static const std::string request3{ R"({"jsonrpc":"2.0","id":3,"method":"c"})" };

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

    bool pending() const
    {
        using namespace std::chrono_literals;
        return future_.wait_for(50ms) == std::future_status::timeout;
    }

private:
    std::shared_ptr<std::promise<Type>> promise_{ std::make_shared<std::promise<Type>>() };
    std::shared_future<Type> future_{ promise_->get_future().share() };
};

static count_handler complete(const awaiter<code>& done)
{
    return [=](const code& ec, size_t) NOEXCEPT
    {
        done.set(ec);
    };
}

static rpc::response to_response(rpc::code_t id)
{
    rpc::response out{};
    out.message = rpc::response_t{ rpc::version::v2, rpc::identity_t{ id }, {}, rpc::value_t{ true } };
    return out;
}

static rpc::request to_notification(const std::string& method)
{
    rpc::request out{};
    out.message = rpc::request_t{ rpc::version::v2, {}, method, {} };
    return out;
}

template <typename Body>
static http::response to_http(Body&& body)
{
    http::response out{};
    out.result(boost::beast::http::status::ok);
    out.body() = std::forward<Body>(body);
    out.prepare_payload();
    return out;
}

static std::string to_post(const std::string& body)
{
    return "POST / HTTP/1.1\r\nHost: localhost\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
}

static std::string to_method(const std::string& json)
{
    return std::string{ boost::json::parse(json).as_object().at("method").as_string() };
}

struct loopback_fixture
{
    DELETE_COPY_MOVE(loopback_fixture);

    loopback_fixture(uint32_t rate_limit=0)
    {
        const socket::parameters params
        {
            .maximum_request = 1'000'000,
            .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer
        };

        const auto server = std::make_shared<network::socket>(log, pool.service(), params);
        channel = std::make_shared<mock_proxy>(server, rate_limit, 1'000'000);

        const asio::endpoint endpoint{ asio::ipv4::loopback(), loopback_port };
        asio::strand strand{ pool.service().get_executor() };
        asio::acceptor acceptor{ strand };
        boost_code ec{};
        acceptor.open(endpoint.protocol(), ec);
        BOOST_REQUIRE(!ec);
        acceptor.set_option(asio::reuse_address(true), ec);
        BOOST_REQUIRE(!ec);
        acceptor.bind(endpoint, ec);
        BOOST_REQUIRE(!ec);
        acceptor.listen(1, ec);
        BOOST_REQUIRE(!ec);

        const awaiter<code> accepted{};
        server->accept(acceptor, [=](const code& accept_ec) NOEXCEPT
        {
            accepted.set(accept_ec);
        });

        client.connect(endpoint, ec);
        BOOST_REQUIRE(!ec);
        BOOST_REQUIRE_EQUAL(accepted.get(), error::success);
    }

    ~loopback_fixture()
    {
        channel->stop(error::service_stopped);
        pool.stop();
        BOOST_REQUIRE(pool.join());
    }

    awaiter<code> subscribe()
    {
        const awaiter<code> stopped{};
        const awaiter<code> subscribed{};
        channel->subscribe_stop([=](const code& ec) NOEXCEPT
        {
            stopped.set(ec);
        },
        [=](const code& ec) NOEXCEPT
        {
            subscribed.set(ec);
        });

        BOOST_REQUIRE_EQUAL(subscribed.get(), error::success);
        return stopped;
    }

    void read_rpc(const awaiter<code>& done)
    {
        boost::asio::post(channel->strand(), [=, this]() NOEXCEPT
        {
            channel->read(buffer, rpc_request, complete(done));
        });
    }

    void read_http(const awaiter<code>& done)
    {
        boost::asio::post(channel->strand(), [=, this]() NOEXCEPT
        {
            channel->read(buffer, http_request, complete(done));
        });
    }

    void send(const std::string& text)
    {
        boost_code ec{};
        boost::asio::write(client, boost::asio::buffer(text), ec);
        BOOST_REQUIRE(!ec);
    }

    std::string receive(size_t size)
    {
        std::string text(size, '\0');
        boost_code ec{};
        boost::asio::read(client, boost::asio::buffer(text), ec);
        BOOST_REQUIRE(!ec);
        return text;
    }

    std::string receive_line()
    {
        boost_code ec{};
        const auto size = boost::asio::read_until(client, boost::asio::dynamic_buffer(pending), '\n', ec);
        BOOST_REQUIRE(!ec);
        const auto line = pending.substr(0, sub1(size));
        pending.erase(0, size);
        return line;
    }

    boost::beast::http::response<boost::beast::http::string_body> receive_http()
    {
        boost_code ec{};
        boost::beast::http::response<boost::beast::http::string_body> out{};
        boost::beast::http::read(client, client_buffer, out, ec);
        BOOST_REQUIRE(!ec);
        return out;
    }

    const logger log{};
    threadpool pool{ 2 };
    asio::context service{};
    asio::socket client{ service };
    http::flat_buffer client_buffer{};
    std::string pending{};
    http::flat_buffer buffer{ 64 * 1024 };
    rpc::request rpc_request{};
    http::request http_request{};
    system::data_array<4> bytes{};
    std::shared_ptr<mock_proxy> channel{};
};

struct throttled_fixture
  : loopback_fixture
{
    throttled_fixture()
      : loopback_fixture(1000)
    {
    }
};

struct websocket_fixture
  : loopback_fixture
{
    websocket_fixture()
      : stream{ std::move(client) }
    {
        const awaiter<code> upgraded{};
        const awaiter<code> accepted{};
        boost::asio::post(channel->strand(), [=, this]() NOEXCEPT
        {
            channel->read(buffer, http_request, [=, this](const code& ec, size_t) NOEXCEPT
            {
                upgraded.set(ec);
                accepted.set(channel->accept_websocket(http_request));
            });
        });

        boost_code ec{};
        stream.handshake("localhost", "/", ec);
        BOOST_REQUIRE(!ec);
        upgrade_result = upgraded.get();
        accept_result = accepted.get();
    }

    ws::socket stream;
    code upgrade_result{};
    code accept_result{};
};

// properties

BOOST_FIXTURE_TEST_CASE(proxy__properties__accepted__expected, loopback_fixture)
{
    BOOST_REQUIRE(&channel->service() == &pool.service());
    BOOST_REQUIRE(channel->inbound());
    BOOST_REQUIRE(!channel->detected());
    BOOST_REQUIRE(!channel->downgraded());
    BOOST_REQUIRE(!channel->websocket());
    BOOST_REQUIRE_EQUAL(channel->slot(), max_size_t);
    BOOST_REQUIRE_EQUAL(channel->binding().port(), loopback_port);
    BOOST_REQUIRE_EQUAL(channel->binding().host(), "127.0.0.1");
    BOOST_REQUIRE_EQUAL(channel->endpoint().port(), client.local_endpoint().port());
}

// watch

BOOST_FIXTURE_TEST_CASE(proxy__watch__client_shutdown__peer_disconnect, loopback_fixture)
{
    const awaiter<code> watched{};
    boost::asio::post(channel->strand(), [=, this]() NOEXCEPT
    {
        channel->watch([=](const code& ec) NOEXCEPT
        {
            watched.set(ec);
        });
    });

    boost_code ec{};
    client.shutdown(asio::socket::shutdown_send, ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(watched.get(), error::peer_disconnect);
}

BOOST_FIXTURE_TEST_CASE(proxy__watch__connected__pending, loopback_fixture)
{
    const awaiter<code> watched{};
    boost::asio::post(channel->strand(), [=, this]() NOEXCEPT
    {
        channel->watch([=](const code& ec) NOEXCEPT
        {
            watched.set(ec);
        });
    });

    BOOST_REQUIRE(watched.pending());
    channel->stop(error::service_stopped);
    BOOST_REQUIRE_EQUAL(watched.get(), error::success);
}

BOOST_FIXTURE_TEST_CASE(proxy__read_ws__tcp_full_buffer__buffer_overflow, loopback_fixture)
{
    static const std::string text{ "abcd" };
    http::flat_buffer full{ text.size() };
    full.commit(boost::asio::buffer_copy(full.prepare(text.size()), boost::asio::buffer(text)));
    const awaiter<code> read{};
    boost::asio::post(channel->strand(), [=, this, &full]() NOEXCEPT
    {
        channel->read(full, complete(read));
    });

    BOOST_REQUIRE_EQUAL(read.get(), error::buffer_overflow);
}

BOOST_FIXTURE_TEST_CASE(proxy__unwatch__watching__success, loopback_fixture)
{
    const awaiter<code> watched{};
    boost::asio::post(channel->strand(), [=, this]() NOEXCEPT
    {
        channel->watch([=](const code& ec) NOEXCEPT
        {
            watched.set(ec);
        });

        channel->unwatch();
    });

    BOOST_REQUIRE_EQUAL(watched.get(), error::success);
}

// tcp

BOOST_FIXTURE_TEST_CASE(proxy__read_tcp__client_sent__expected_counted, loopback_fixture)
{
    const awaiter<code> read{};
    boost::asio::post(channel->strand(), [=, this]() NOEXCEPT
    {
        channel->read(asio::mutable_buffer{ bytes.data(), bytes.size() }, complete(read));
    });

    send("abcd");
    BOOST_REQUIRE_EQUAL(read.get(), error::success);
    BOOST_REQUIRE_EQUAL(std::string(bytes.begin(), bytes.end()), "abcd");
    BOOST_REQUIRE_EQUAL(channel->received(), 4u);
}

BOOST_FIXTURE_TEST_CASE(proxy__read_tcp__stopped__channel_stopped, loopback_fixture)
{
    const awaiter<code> read{};
    boost::asio::post(channel->strand(), [=, this]() NOEXCEPT
    {
        channel->read(asio::mutable_buffer{ bytes.data(), bytes.size() }, complete(read));
    });

    channel->stop(error::service_stopped);
    const auto ec = read.get();
    BOOST_REQUIRE(ec == error::channel_stopped || ec == error::peer_disconnect);
}

BOOST_FIXTURE_TEST_CASE(proxy__read_ws__tcp_client_sent__prepared_uncommitted, loopback_fixture)
{
    const awaiter<size_t> read{};
    boost::asio::post(channel->strand(), [=, this]() NOEXCEPT
    {
        channel->read(buffer, [=](const code&, size_t size) NOEXCEPT
        {
            read.set(size);
        });
    });

    send("abcd");
    BOOST_REQUIRE_EQUAL(read.get(), 4u);
    BOOST_REQUIRE_EQUAL(buffer.size(), 0u);
    buffer.commit(4);
    BOOST_REQUIRE_EQUAL(boost::beast::buffers_to_string(buffer.data()), "abcd");
}

BOOST_FIXTURE_TEST_CASE(proxy__write_tcp__buffer__client_receives_counted, loopback_fixture)
{
    static const std::string text{ "wxyz" };
    const awaiter<code> written{};
    channel->write1({ text.data(), text.size() }, complete(written));

    BOOST_REQUIRE_EQUAL(receive(text.size()), text);
    BOOST_REQUIRE_EQUAL(written.get(), error::success);
    BOOST_REQUIRE_EQUAL(channel->sent(), text.size());
}

BOOST_FIXTURE_TEST_CASE(proxy__write_tcp__rate_limited__deferred_success, throttled_fixture)
{
    static const std::string text{ "wxyz" };
    const awaiter<code> written{};
    channel->write1({ text.data(), text.size() }, complete(written));

    BOOST_REQUIRE_EQUAL(receive(text.size()), text);
    BOOST_REQUIRE_EQUAL(written.get(), error::success);
}

// rpc (tcp)

BOOST_FIXTURE_TEST_CASE(proxy__read_rpc__single__expected_unbatched, loopback_fixture)
{
    const awaiter<code> read{};
    send(request1 + "\n");
    read_rpc(read);

    BOOST_REQUIRE_EQUAL(read.get(), error::success);
    BOOST_REQUIRE_EQUAL(rpc_request.message.method, "a");
    BOOST_REQUIRE(!rpc_request.batch);
    BOOST_REQUIRE(!rpc_request.changed);
}

BOOST_FIXTURE_TEST_CASE(proxy__write_rpc__response__client_receives_terminated, loopback_fixture)
{
    const awaiter<code> written{};
    channel->write_rpc(to_response(1), complete(written));

    BOOST_REQUIRE_EQUAL(receive_line(), response1);
    BOOST_REQUIRE_EQUAL(written.get(), error::success);
}

BOOST_FIXTURE_TEST_CASE(proxy__read_rpc__batch__parts_framed_close_absorbed_read_rearmed, loopback_fixture)
{
    const awaiter<code> read1{};
    const awaiter<code> read2{};
    const awaiter<code> read3{};
    const awaiter<code> written1{};
    const awaiter<code> written2{};
    send("[" + request1 + "," + request2 + "]\n");

    read_rpc(read1);
    BOOST_REQUIRE_EQUAL(read1.get(), error::success);
    BOOST_REQUIRE_EQUAL(rpc_request.message.method, "a");
    channel->write_rpc(to_response(1), complete(written1));
    BOOST_REQUIRE_EQUAL(written1.get(), error::success);

    read_rpc(read2);
    BOOST_REQUIRE_EQUAL(read2.get(), error::success);
    BOOST_REQUIRE_EQUAL(rpc_request.message.method, "b");
    channel->write_rpc(to_response(2), complete(written2));
    BOOST_REQUIRE_EQUAL(written2.get(), error::success);

    read_rpc(read3);
    BOOST_REQUIRE_EQUAL(receive_line(), "[" + response1 + "," + response2 + "]");

    send(request3 + "\n");
    BOOST_REQUIRE_EQUAL(read3.get(), error::success);
    BOOST_REQUIRE_EQUAL(rpc_request.message.method, "c");
}

BOOST_FIXTURE_TEST_CASE(proxy__notify_rpc__batch_open__deferred_until_close, loopback_fixture)
{
    const awaiter<code> read1{};
    const awaiter<code> read2{};
    const awaiter<code> read3{};
    const awaiter<code> written1{};
    const awaiter<code> written2{};
    const awaiter<code> notified{};
    send("[" + request1 + "," + request2 + "]\n");

    read_rpc(read1);
    BOOST_REQUIRE_EQUAL(read1.get(), error::success);
    channel->notify_rpc(to_notification("note"), complete(notified));
    channel->write_rpc(to_response(1), complete(written1));
    BOOST_REQUIRE_EQUAL(written1.get(), error::success);

    read_rpc(read2);
    BOOST_REQUIRE_EQUAL(read2.get(), error::success);
    channel->write_rpc(to_response(2), complete(written2));
    BOOST_REQUIRE_EQUAL(written2.get(), error::success);

    read_rpc(read3);
    BOOST_REQUIRE_EQUAL(receive_line(), "[" + response1 + "," + response2 + "]");
    BOOST_REQUIRE_EQUAL(to_method(receive_line()), "note");
    BOOST_REQUIRE_EQUAL(notified.get(), error::success);
}

BOOST_FIXTURE_TEST_CASE(proxy__stop__batch_parted__close_part_written, loopback_fixture)
{
    const auto stopped = subscribe();
    const awaiter<code> read{};
    const awaiter<code> written{};
    send("[" + request1 + "," + request2 + "]\n");

    read_rpc(read);
    BOOST_REQUIRE_EQUAL(read.get(), error::success);
    channel->write_rpc(to_response(1), complete(written));
    BOOST_REQUIRE_EQUAL(written.get(), error::success);

    boost::asio::post(channel->strand(), [=, this]() NOEXCEPT
    {
        channel->stop(error::invalid_magic);
    });

    BOOST_REQUIRE_EQUAL(receive_line(), "[" + response1 + "]");
    BOOST_REQUIRE_EQUAL(stopped.get(), error::invalid_magic);
}

// stop

BOOST_FIXTURE_TEST_CASE(proxy__stop__channel_expired__client_end_of_file, loopback_fixture)
{
    const auto stopped = subscribe();
    channel->stop(error::channel_expired);
    BOOST_REQUIRE_EQUAL(stopped.get(), error::channel_expired);

    char byte{};
    boost_code ec{};
    client.read_some(boost::asio::buffer(&byte, 1), ec);
    BOOST_REQUIRE(ec == boost::asio::error::eof);
}

// http

BOOST_FIXTURE_TEST_CASE(proxy__read_http__json_rpc_post__not_downgraded_singleton, loopback_fixture)
{
    const awaiter<code> read{};
    http_request.body() = rpc::request{};
    send(to_post(request1));
    read_http(read);

    BOOST_REQUIRE_EQUAL(read.get(), error::success);
    BOOST_REQUIRE(channel->detected());
    BOOST_REQUIRE(!channel->downgraded());
    BOOST_REQUIRE(http_request.method() == http::verb::post);
    BOOST_REQUIRE(http_request.body().contains<rpc::request>());
    BOOST_REQUIRE_EQUAL(http_request.body().get<rpc::request>().message.method, "a");
}

BOOST_FIXTURE_TEST_CASE(proxy__read_http__detect_client_shutdown__peer_disconnect, loopback_fixture)
{
    const awaiter<code> read{};
    http_request.body() = rpc::request{};
    read_http(read);

    boost_code ec{};
    client.shutdown(asio::socket::shutdown_send, ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(read.get(), error::peer_disconnect);
}

BOOST_FIXTURE_TEST_CASE(proxy__read_http__truncated_body__partial_message, loopback_fixture)
{
    const awaiter<code> read{};
    http_request.body() = rpc::request{};
    const auto post = to_post(request1);
    send(post.substr(0, sub1(post.size())));
    read_http(read);

    boost_code ec{};
    client.shutdown(asio::socket::shutdown_send, ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(read.get(), error::partial_message);
}

BOOST_FIXTURE_TEST_CASE(proxy__read_http__batch_post__chunked_parts_close_absorbed_read_rearmed, loopback_fixture)
{
    const awaiter<code> read1{};
    const awaiter<code> read2{};
    const awaiter<code> read3{};
    const awaiter<code> written1{};
    const awaiter<code> written2{};
    http_request.body() = rpc::request{};
    send(to_post("[" + request1 + "," + request2 + "]"));

    read_http(read1);
    BOOST_REQUIRE_EQUAL(read1.get(), error::success);
    BOOST_REQUIRE_EQUAL(http_request.body().get<rpc::request>().message.method, "a");
    channel->write_http(to_http(to_response(1)), complete(written1));
    BOOST_REQUIRE_EQUAL(written1.get(), error::success);

    read_http(read2);
    BOOST_REQUIRE_EQUAL(read2.get(), error::success);
    BOOST_REQUIRE_EQUAL(http_request.body().get<rpc::request>().message.method, "b");
    channel->write_http(to_http(to_response(2)), complete(written2));
    BOOST_REQUIRE_EQUAL(written2.get(), error::success);

    read_http(read3);
    const auto response = receive_http();
    BOOST_REQUIRE_EQUAL(response.result_int(), 200u);
    BOOST_REQUIRE_EQUAL(response.body(), "[" + response1 + "," + response2 + "]");

    send(to_post(request3));
    BOOST_REQUIRE_EQUAL(read3.get(), error::success);
    BOOST_REQUIRE_EQUAL(http_request.body().get<rpc::request>().message.method, "c");
}

BOOST_FIXTURE_TEST_CASE(proxy__write_http__batch_open_string_body__bad_stream, loopback_fixture)
{
    const awaiter<code> read{};
    const awaiter<code> written{};
    http_request.body() = rpc::request{};
    send(to_post("[" + request1 + "," + request2 + "]"));

    read_http(read);
    BOOST_REQUIRE_EQUAL(read.get(), error::success);
    channel->write_http(to_http(http::string_value{ "text" }), complete(written));
    BOOST_REQUIRE_EQUAL(written.get(), error::bad_stream);
}

BOOST_FIXTURE_TEST_CASE(proxy__write_http__rpc_response__client_receives_body, loopback_fixture)
{
    const awaiter<code> written{};
    channel->write_http(to_http(to_response(1)), complete(written));

    const auto response = receive_http();
    BOOST_REQUIRE_EQUAL(response.result_int(), 200u);
    BOOST_REQUIRE_EQUAL(response.body(), response1);
    BOOST_REQUIRE_EQUAL(written.get(), error::success);
}

BOOST_FIXTURE_TEST_CASE(proxy__write_http__rpc_request__client_receives_body, loopback_fixture)
{
    const awaiter<code> written{};
    channel->write_http(to_http(to_notification("note")), complete(written));

    BOOST_REQUIRE_EQUAL(to_method(receive_http().body()), "note");
    BOOST_REQUIRE_EQUAL(written.get(), error::success);
}

BOOST_FIXTURE_TEST_CASE(proxy__notify_http__string_body__client_receives_body, loopback_fixture)
{
    const awaiter<code> written{};
    channel->notify_http(to_http(http::string_value{ "text" }), complete(written));

    BOOST_REQUIRE_EQUAL(receive_http().body(), "text");
    BOOST_REQUIRE_EQUAL(written.get(), error::success);
}

BOOST_FIXTURE_TEST_CASE(proxy__write_http__data_body__client_receives_body, loopback_fixture)
{
    const awaiter<code> written{};
    channel->write_http(to_http(http::data_value{ 'a', 'b', 'c' }), complete(written));

    BOOST_REQUIRE_EQUAL(receive_http().body(), "abc");
    BOOST_REQUIRE_EQUAL(written.get(), error::success);
}

BOOST_FIXTURE_TEST_CASE(proxy__write_http__span_body__client_receives_body, loopback_fixture)
{
    static system::data_chunk data{ 'a', 'b', 'c' };
    const awaiter<code> written{};
    channel->write_http(to_http(http::span_value{ data.data(), data.size() }), complete(written));

    BOOST_REQUIRE_EQUAL(receive_http().body(), "abc");
    BOOST_REQUIRE_EQUAL(written.get(), error::success);
}

BOOST_FIXTURE_TEST_CASE(proxy__write_http__buffer_body__client_receives_body, loopback_fixture)
{
    static system::data_chunk data{ 'a', 'b', 'c' };
    const awaiter<code> written{};
    channel->write_http(to_http(http::buffer_value{ data.data(), data.size(), false }), complete(written));

    BOOST_REQUIRE_EQUAL(receive_http().body(), "abc");
    BOOST_REQUIRE_EQUAL(written.get(), error::success);
}

BOOST_FIXTURE_TEST_CASE(proxy__write_http__stopped_peer_body__handler_not_invoked, loopback_fixture)
{
    auto invoked = false;
    const awaiter<bool> drained{};
    channel->stop(error::service_stopped);
    channel->write_http(to_http(http::peer_value{}), [&](const code&, size_t) NOEXCEPT
    {
        invoked = true;
    });

    boost::asio::post(channel->strand(), [=]() NOEXCEPT
    {
        drained.set(true);
    });

    BOOST_REQUIRE(drained.get());
    BOOST_REQUIRE(!invoked);
}

// http (downgraded)

BOOST_FIXTURE_TEST_CASE(proxy__read_http__json_rpc_stream__downgraded, loopback_fixture)
{
    const awaiter<code> read{};
    http_request.body() = rpc::request{};
    send(request1 + "\n");
    read_http(read);

    BOOST_REQUIRE_EQUAL(read.get(), error::success);
    BOOST_REQUIRE(channel->detected());
    BOOST_REQUIRE(channel->downgraded());
    BOOST_REQUIRE_EQUAL(std::string{ http_request.method_string() }, "stream");
    BOOST_REQUIRE_EQUAL(http_request.body().get<rpc::request>().message.method, "a");
}

BOOST_FIXTURE_TEST_CASE(proxy__read_http__downgraded_empty_body__rpc_request, loopback_fixture)
{
    const awaiter<code> read1{};
    const awaiter<code> read2{};
    http_request.body() = rpc::request{};
    send(request1 + "\n" + request2 + "\n");

    read_http(read1);
    BOOST_REQUIRE_EQUAL(read1.get(), error::success);
    http_request = http::request{};

    read_http(read2);
    BOOST_REQUIRE_EQUAL(read2.get(), error::success);
    BOOST_REQUIRE_EQUAL(std::string{ http_request.method_string() }, "stream");
    BOOST_REQUIRE_EQUAL(http_request.body().get<rpc::request>().message.method, "b");
}

BOOST_FIXTURE_TEST_CASE(proxy__write_http__downgraded_rpc_response__client_receives_line, loopback_fixture)
{
    const awaiter<code> read{};
    const awaiter<code> written{};
    http_request.body() = rpc::request{};
    send(request1 + "\n");
    read_http(read);
    BOOST_REQUIRE_EQUAL(read.get(), error::success);

    channel->write_http(to_http(to_response(1)), complete(written));
    BOOST_REQUIRE_EQUAL(receive_line(), response1);
    BOOST_REQUIRE_EQUAL(written.get(), error::success);
}

BOOST_FIXTURE_TEST_CASE(proxy__notify_http__downgraded_rpc_request__client_receives_line, loopback_fixture)
{
    const awaiter<code> read{};
    const awaiter<code> written{};
    http_request.body() = rpc::request{};
    send(request1 + "\n");
    read_http(read);
    BOOST_REQUIRE_EQUAL(read.get(), error::success);

    channel->notify_http(to_http(to_notification("note")), complete(written));
    BOOST_REQUIRE_EQUAL(to_method(receive_line()), "note");
    BOOST_REQUIRE_EQUAL(written.get(), error::success);
}

BOOST_FIXTURE_TEST_CASE(proxy__write_http__downgraded_string_body__bad_stream, loopback_fixture)
{
    const awaiter<code> read{};
    const awaiter<code> written{};
    http_request.body() = rpc::request{};
    send(request1 + "\n");
    read_http(read);
    BOOST_REQUIRE_EQUAL(read.get(), error::success);

    channel->write_http(to_http(http::string_value{ "text" }), complete(written));
    BOOST_REQUIRE_EQUAL(written.get(), error::bad_stream);
}

// websocket

BOOST_FIXTURE_TEST_CASE(proxy__read_http__websocket_upgrade__upgrade_published_accepted, websocket_fixture)
{
    BOOST_REQUIRE_EQUAL(upgrade_result, error::upgrade);
    BOOST_REQUIRE_EQUAL(accept_result, error::success);
    BOOST_REQUIRE(boost::beast::websocket::is_upgrade(http_request));
    BOOST_REQUIRE(channel->websocket());
}

BOOST_FIXTURE_TEST_CASE(proxy__write_ws__text__client_receives_text_message, websocket_fixture)
{
    static const std::string text{ "hello" };
    const awaiter<code> written{};
    channel->write_ws({ text.data(), text.size() }, false, complete(written));

    boost_code ec{};
    http::flat_buffer in{};
    stream.read(in, ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(stream.got_text());
    BOOST_REQUIRE_EQUAL(boost::beast::buffers_to_string(in.data()), text);
    BOOST_REQUIRE_EQUAL(written.get(), error::success);
}

BOOST_FIXTURE_TEST_CASE(proxy__read_ws__client_message__expected, websocket_fixture)
{
    static const std::string text{ "hello" };
    const awaiter<code> read{};
    boost::asio::post(channel->strand(), [=, this]() NOEXCEPT
    {
        channel->read(buffer, complete(read));
    });

    boost_code ec{};
    stream.write(boost::asio::buffer(text), ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(read.get(), error::success);
    BOOST_REQUIRE_EQUAL(boost::beast::buffers_to_string(buffer.data()), text);
}

BOOST_FIXTURE_TEST_CASE(proxy__read_tcp__websocket__operation_failed, websocket_fixture)
{
    const awaiter<code> read{};
    boost::asio::post(channel->strand(), [=, this]() NOEXCEPT
    {
        channel->read(asio::mutable_buffer{ bytes.data(), bytes.size() }, complete(read));
    });

    BOOST_REQUIRE_EQUAL(read.get(), error::operation_failed);
}

BOOST_FIXTURE_TEST_CASE(proxy__read_http__websocket_rpc_body__expected, websocket_fixture)
{
    const awaiter<code> read{};
    http_request = http::request{};
    http_request.body() = rpc::request{};
    read_http(read);

    boost_code ec{};
    stream.text(true);
    stream.write(boost::asio::buffer(request1), ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(read.get(), error::success);
    BOOST_REQUIRE_EQUAL(http_request.body().get<rpc::request>().message.method, "a");
}

BOOST_FIXTURE_TEST_CASE(proxy__write_http__websocket_json_body__client_receives_body_only, websocket_fixture)
{
    const awaiter<code> written{};
    http::json_value content{};
    content.model = boost::json::object{ { "key", "value" } };
    channel->write_http(to_http(std::move(content)), complete(written));

    boost_code ec{};
    http::flat_buffer in{};
    stream.read(in, ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(boost::beast::buffers_to_string(in.data()), R"({"key":"value"})");
    BOOST_REQUIRE_EQUAL(written.get(), error::success);
}

BOOST_FIXTURE_TEST_CASE(proxy__stop__websocket_closed__client_receives_close, websocket_fixture)
{
    const auto stopped = subscribe();
    channel->stop(error::websocket_closed);

    boost_code ec{};
    http::flat_buffer in{};
    stream.read(in, ec);
    BOOST_REQUIRE(ec == boost::beast::websocket::error::closed);
    BOOST_REQUIRE_EQUAL(stopped.get(), error::websocket_closed);
}

BOOST_AUTO_TEST_SUITE_END()
