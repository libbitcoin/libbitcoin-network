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

BOOST_AUTO_TEST_SUITE(channel_peer_tests)

class mock_channel_peer
  : public channel_peer
{
public:
    using channel_peer::channel_peer;
    using channel_peer::resume;

    // Call must be stranded.
    void subscribe_stop1(result_handler handler) NOEXCEPT
    {
        channel_peer::subscribe_stop(std::move(handler));
    }

    void stop(const code& ec) NOEXCEPT override
    {
        channel_peer::stop(ec);

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

const channel_peer::options_t options{ "test" };

BOOST_AUTO_TEST_CASE(channel_peer__stopped__default__false)
{
    constexpr auto expected = 42u;
    const logger log{};
    threadpool pool(1);
    asio::strand strand(pool.service().get_executor());
    const settings set(bc::system::chain::selection::mainnet);
    network::socket::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto channel_ptr = std::make_shared<channel_peer>(log, socket_ptr, expected, set, options);
    BOOST_REQUIRE(!channel_ptr->stopped());

    BOOST_REQUIRE_NE(channel_ptr->nonce(), zero);
    BOOST_REQUIRE_EQUAL(channel_ptr->identifier(), expected);

    // Stop is asynchronous, threadpool destruct blocks until all complete.
    // Calling stop here sets channel.stopped and prevents destructor assertion.
    channel_ptr->stop(error::invalid_magic);
}

BOOST_AUTO_TEST_CASE(channel_peer__properties__default__expected)
{
    const logger log{};
    threadpool pool(1);
    asio::strand strand(pool.service().get_executor());
    const settings set(bc::system::chain::selection::mainnet);
    network::socket::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto channel_ptr = std::make_shared<channel_peer>(log, socket_ptr, 42, set, options);

    BOOST_REQUIRE(!channel_ptr->address());
    BOOST_REQUIRE_NE(channel_ptr->nonce(), 0u);
    BOOST_REQUIRE_EQUAL(channel_ptr->negotiated_version(), set.protocol_maximum);
    BOOST_REQUIRE(channel_ptr->is_negotiated(messages::peer::level::maximum_protocol));
    BOOST_REQUIRE(!channel_ptr->wants_address_v2());

    // TODO: compare to default instance.
    BOOST_REQUIRE(channel_ptr->peer_version());

    BOOST_REQUIRE_EQUAL(channel_ptr->options().maximum_request, options.maximum_request);
    BOOST_REQUIRE_EQUAL(channel_ptr->settings().identifier, set.identifier);
    BOOST_REQUIRE_EQUAL(channel_ptr->settings().validate_checksum, set.validate_checksum);

    // Stop is asynchronous, threadpool destruct blocks until all complete.
    // Calling stop here sets channel.stopped and prevents destructor assertion.
    channel_ptr->stop(error::invalid_magic);
}

BOOST_AUTO_TEST_CASE(channel_peer__set_wants_address_v2__always__latched)
{
    const logger log{};
    threadpool pool(1);
    asio::strand strand(pool.service().get_executor());
    const settings set(bc::system::chain::selection::mainnet);
    network::socket::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto channel_ptr = std::make_shared<channel_peer>(log, socket_ptr, 42, set, options);

    BOOST_REQUIRE(!channel_ptr->wants_address_v2());
    channel_ptr->set_wants_address_v2();
    BOOST_REQUIRE(channel_ptr->wants_address_v2());

    channel_ptr->stop(error::invalid_magic);
}

BOOST_AUTO_TEST_CASE(channel_peer__set_minimum_fee__always__expected)
{
    const logger log{};
    threadpool pool(1);
    asio::strand strand(pool.service().get_executor());
    const settings set(bc::system::chain::selection::mainnet);
    network::socket::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto channel_ptr = std::make_shared<channel_peer>(log, socket_ptr, 42, set, options);

    BOOST_REQUIRE(is_zero(channel_ptr->minimum_fee()));
    channel_ptr->set_minimum_fee(42);
    BOOST_REQUIRE_EQUAL(channel_ptr->minimum_fee(), 42u);

    channel_ptr->stop(error::invalid_magic);
}

BOOST_AUTO_TEST_CASE(channel_peer__set_pong__unpinged__unchanged)
{
    const logger log{};
    threadpool pool(1);
    asio::strand strand(pool.service().get_executor());
    const settings set(bc::system::chain::selection::mainnet);
    network::socket::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto channel_ptr = std::make_shared<channel_peer>(log, socket_ptr, 42, set, options);

    BOOST_REQUIRE(is_zero(channel_ptr->ping_time().count()));
    BOOST_REQUIRE(is_zero(channel_ptr->minimum_ping_time().count()));
    BOOST_REQUIRE(is_zero(channel_ptr->pending_ping_time().count()));

    channel_ptr->set_pong();
    BOOST_REQUIRE(is_zero(channel_ptr->ping_time().count()));
    BOOST_REQUIRE(is_zero(channel_ptr->minimum_ping_time().count()));
    BOOST_REQUIRE(is_zero(channel_ptr->pending_ping_time().count()));

    channel_ptr->stop(error::invalid_magic);
}

BOOST_AUTO_TEST_CASE(channel_peer__set_ping__unponged__pending_only)
{
    const logger log{};
    threadpool pool(1);
    asio::strand strand(pool.service().get_executor());
    const settings set(bc::system::chain::selection::mainnet);
    network::socket::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto channel_ptr = std::make_shared<channel_peer>(log, socket_ptr, 42, set, options);

    channel_ptr->set_ping();
    BOOST_REQUIRE(is_zero(channel_ptr->ping_time().count()));
    BOOST_REQUIRE(is_zero(channel_ptr->minimum_ping_time().count()));

    channel_ptr->stop(error::invalid_magic);
}

BOOST_AUTO_TEST_CASE(channel_peer__set_pong__pinged__timed_not_pending)
{
    const logger log{};
    threadpool pool(1);
    asio::strand strand(pool.service().get_executor());
    const settings set(bc::system::chain::selection::mainnet);
    network::socket::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto channel_ptr = std::make_shared<channel_peer>(log, socket_ptr, 42, set, options);

    channel_ptr->set_ping();
    channel_ptr->set_pong();
    BOOST_REQUIRE(is_zero(channel_ptr->pending_ping_time().count()));
    BOOST_REQUIRE(channel_ptr->minimum_ping_time() == channel_ptr->ping_time());

    channel_ptr->set_ping();
    channel_ptr->set_pong();
    BOOST_REQUIRE(is_zero(channel_ptr->pending_ping_time().count()));
    BOOST_REQUIRE(channel_ptr->minimum_ping_time() <= channel_ptr->ping_time());

    channel_ptr->stop(error::invalid_magic);
}

BOOST_AUTO_TEST_CASE(channel_peer__subscribe_message__subscribed__expected)
{
    const logger log{};
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    const settings set(bc::system::chain::selection::mainnet);
    network::socket::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto channel_ptr = std::make_shared<channel_peer>(log, socket_ptr, 42, set, options);
    constexpr auto expected_ec = error::invalid_magic;

    auto result = true;
    std::promise<code> message_stopped;
    boost::asio::post(channel_ptr->strand(), [&]() NOEXCEPT
    {
        using namespace messages::peer;
        channel_ptr->subscribe<ping>([&](code ec, ping::cptr ping) NOEXCEPT
            {
                result &= is_null(ping);
                message_stopped.set_value(ec);
                return true;
            });
    });

    BOOST_REQUIRE(!channel_ptr->stopped());

    // Stop is asynchronous, threadpool destruct blocks until all complete.
    // Calling stop here sets channel.stopped and prevents destructor assertion.
    channel_ptr->stop(expected_ec);

    BOOST_REQUIRE_EQUAL(message_stopped.get_future().get(), expected_ec);
    BOOST_REQUIRE(channel_ptr->stopped());
    BOOST_REQUIRE(result);
}

BOOST_AUTO_TEST_CASE(channel_peer__stop__all_subscribed__expected)
{
    const logger log{};
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    const settings set(bc::system::chain::selection::mainnet);
    network::socket::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto channel_ptr = std::make_shared<mock_channel_peer>(log, socket_ptr, 42, set, options);
    constexpr auto expected_ec = error::invalid_magic;

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

        using namespace messages::peer;
        channel_ptr->subscribe<ping>([&](code ec, const ping::cptr& ping) NOEXCEPT
        {
            result &= is_null(ping);
            message_stopped.set_value(ec);
            return true;
        });
    });

    BOOST_REQUIRE(!channel_ptr->stopped());
    BOOST_REQUIRE_EQUAL(stop_subscribed.get_future().get(), error::success);

    // Stop is asynchronous, threadpool destruct blocks until all complete.
    // Calling stop here sets channel.stopped and prevents destructor assertion.
    channel_ptr->stop(expected_ec);

    BOOST_REQUIRE_EQUAL(message_stopped.get_future().get(), expected_ec);
    BOOST_REQUIRE_EQUAL(stop1_stopped.get_future().get(), expected_ec);
    BOOST_REQUIRE_EQUAL(stop2_stopped.get_future().get(), expected_ec);
    BOOST_REQUIRE(channel_ptr->stopped());
    BOOST_REQUIRE(result);
}

BOOST_AUTO_TEST_CASE(channel_peer__send__not_connected__expected)
{
    const logger log{};
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    const settings set(bc::system::chain::selection::mainnet);
    network::socket::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto channel_ptr = std::make_shared<channel_peer>(log, socket_ptr, 42, set, options);

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
        using namespace messages::peer;
        channel_ptr->send<ping>(ping{ 42 }, handler);
    });

    // 10009 (WSAEBADF, invalid file handle) gets mapped to bad_stream.
    BOOST_REQUIRE_EQUAL(promise.get_future().get(), error::bad_stream);
    BOOST_REQUIRE(result);

    // Stop is asynchronous, threadpool destruct blocks until all complete.
    // Calling stop here sets channel.stopped and prevents destructor assertion.
    channel_ptr->stop(error::invalid_magic);
}

BOOST_AUTO_TEST_CASE(channel_peer__send__not_connected_move__expected)
{
    const logger log{};
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    const settings set(bc::system::chain::selection::mainnet);
    network::socket::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto channel_ptr = std::make_shared<channel_peer>(log, socket_ptr, 42, set, options);

    auto result = true;
    std::promise<code> promise;

    BOOST_REQUIRE(!channel_ptr->stopped());
    boost::asio::post(channel_ptr->strand(), [&]() NOEXCEPT
    {
        using namespace messages::peer;
        channel_ptr->send<ping>(ping{ 42 }, [&](code ec)
        {
            result &= channel_ptr->stopped();
            promise.set_value(ec);
        });
    });

    // 10009 (WSAEBADF, invalid file handle) gets mapped to bad_stream.
    BOOST_REQUIRE_EQUAL(promise.get_future().get(), error::bad_stream);
    BOOST_REQUIRE(result);

    // Stop is asynchronous, threadpool destruct blocks until all complete.
    // Calling stop here sets channel.stopped and prevents destructor assertion.
    channel_ptr->stop(error::invalid_magic);
}

BOOST_AUTO_TEST_CASE(channel_peer__stopped__resume_after_read_fail__true)
{
    const logger log{};
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    const settings set(bc::system::chain::selection::mainnet);
    network::socket::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto channel_ptr = std::make_shared<mock_channel_peer>(log, socket_ptr, 42, set, options);

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

BOOST_AUTO_TEST_CASE(channel_peer__set_start_height__value__expected)
{
    const logger log{};
    threadpool pool(1);
    const settings set(bc::system::chain::selection::mainnet);
    network::socket::parameters params{ .maximum_request = 42, .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto channel_ptr = std::make_shared<channel_peer>(log, socket_ptr, 42, set, options);

    BOOST_REQUIRE(is_zero(channel_ptr->start_height()));
    channel_ptr->set_start_height(42);
    BOOST_REQUIRE_EQUAL(channel_ptr->start_height(), 42u);
    BOOST_REQUIRE(channel_ptr->sent_by_message() == channel_peer::counters{});
    BOOST_REQUIRE(channel_ptr->received_by_message() == channel_peer::counters{});

    channel_ptr->stop(error::invalid_magic);
}

BOOST_AUTO_TEST_CASE(channel_peer__set_peer_version__services__peer_services_and_updated_address)
{
    using namespace messages::peer;
    const logger log{};
    threadpool pool(2);
    const settings set(bc::system::chain::selection::mainnet);
    network::socket::parameters params{ .maximum_request = 42, .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto channel_ptr = std::make_shared<channel_peer>(log, socket_ptr, 42, set, options);

    constexpr auto services = service::node_network | service::node_witness;
    const auto peer = system::to_shared(version{ .value = level::maximum_protocol, .services = services });
    std::promise<std::string> states{};
    boost::asio::post(channel_ptr->strand(), [&]() NOEXCEPT
    {
        channel_ptr->set_peer_version(peer);
        channel_ptr->set_current(true);
        states.set_value(std::to_string(channel_ptr->is_peer_service(service::node_witness)) + std::to_string(channel_ptr->is_peer_service(service::node_bloom)) + std::to_string(channel_ptr->current()) + std::to_string(channel_ptr->get_updated_address()->services));
    });

    BOOST_REQUIRE_EQUAL(states.get_future().get(), "1019");
    channel_ptr->stop(error::invalid_magic);
}

// connected

using peer_promise = std::shared_ptr<std::promise<code>>;

static peer_promise peer_make_promise() NOEXCEPT
{
    return std::make_shared<std::promise<code>>();
}

static code peer_await(const peer_promise& promise) NOEXCEPT
{
    using namespace std::chrono_literals;
    auto future = promise->get_future();
    BOOST_REQUIRE(future.wait_for(5s) == std::future_status::ready);
    return future.get();
}

static system::data_chunk peer_frame(const std::string& command, const system::data_chunk& payload) NOEXCEPT
{
    auto frame = system::base16_chunk("f9beb4d9");
    auto name = system::to_chunk(command);
    name.resize(12);
    const auto size = system::to_little_endian(system::possible_narrow_cast<uint32_t>(payload.size()));
    const auto hash = system::bitcoin_hash(payload);
    frame.insert(frame.end(), name.begin(), name.end());
    frame.insert(frame.end(), size.begin(), size.end());
    frame.insert(frame.end(), hash.begin(), std::next(hash.begin(), 4));
    frame.insert(frame.end(), payload.begin(), payload.end());
    return frame;
}

static const settings peer_checked_settings = []() NOEXCEPT
{
    settings value{ bc::system::chain::selection::mainnet };
    value.validate_checksum = true;
    return value;
}();

using peer_responder = std::function<bool(const channel_peer::ptr&, const messages::peer::ping::cptr&)>;

class resumable_channel_peer
  : public channel_peer
{
public:
    using channel_peer::channel_peer;
    using channel_peer::resume;
};

struct peer_loopback_fixture
{
    DELETE_COPY_MOVE(peer_loopback_fixture);

    static constexpr uint16_t port = 65132;

    peer_loopback_fixture() NOEXCEPT
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
            .maximum_request = options.maximum_request,
            .minimum_buffer = options.minimum_buffer,
            .maximum_buffer = options.maximum_buffer
        };

        const auto server = std::make_shared<network::socket>(log, pool_.service(), std::move(params));
        const auto accepted = peer_make_promise();
        server->accept(acceptor_, [=](const code& accept_ec) NOEXCEPT
        {
            accepted->set_value(accept_ec);
        });

        client.connect(local, ec);
        BOOST_REQUIRE(!ec);
        BOOST_REQUIRE_EQUAL(peer_await(accepted), error::success);

        channel = std::make_shared<resumable_channel_peer>(log, server, 42, peer_checked_settings, options);
        const auto promise = stopped;
        channel->subscribe_stop([=](const code& stop_ec) NOEXCEPT
        {
            promise->set_value(stop_ec);
        }, [](const code&) NOEXCEPT {});
    }

    ~peer_loopback_fixture() NOEXCEPT
    {
        boost_code ignore{};
        client.close(ignore);
        channel->stop(error::service_stopped);
        pool_.stop();
        BOOST_REQUIRE(pool_.join());
    }

    void start(const peer_responder& respond, bool current=false) NOEXCEPT
    {
        const auto self = channel;
        const auto started = peer_make_promise();
        boost::asio::post(channel->strand(), [=]() NOEXCEPT
        {
            using namespace messages::peer;
            self->subscribe<ping>([=](const code& ec, const ping::cptr& message) NOEXCEPT
            {
                return !ec && respond(self, message);
            });

            self->set_current(current);
            self->resume();
            started->set_value(error::success);
        });

        BOOST_REQUIRE_EQUAL(peer_await(started), error::success);
    }

    void send(const system::data_chunk& data) NOEXCEPT
    {
        boost_code ec{};
        boost::asio::write(client, boost::asio::buffer(data), ec);
        BOOST_REQUIRE(!ec);
    }

    const logger log{};

private:
    threadpool pool_;
    asio::strand strand_;
    asio::acceptor acceptor_;
    asio::context client_service_{};

public:
    asio::socket client;
    const peer_promise stopped{ peer_make_promise() };
    std::shared_ptr<resumable_channel_peer> channel{};
};

static bool peer_keep(const channel_peer::ptr&, const messages::peer::ping::cptr&) NOEXCEPT
{
    return true;
}

BOOST_FIXTURE_TEST_CASE(channel_peer__resume__zero_magic__invalid_magic, peer_loopback_fixture)
{
    start(peer_keep);
    send(system::base16_chunk("00000000" "70696e670000000000000000" "08000000" "00000000"));
    BOOST_REQUIRE_EQUAL(peer_await(stopped), error::invalid_magic);
}

BOOST_FIXTURE_TEST_CASE(channel_peer__resume__http_request__invalid_magic, peer_loopback_fixture)
{
    start(peer_keep);
    send(system::to_chunk(std::string{ "GET / HTTP/1.1\r\nHost: example.com\r\n\r\n" }));
    BOOST_REQUIRE_EQUAL(peer_await(stopped), error::invalid_magic);
}

BOOST_FIXTURE_TEST_CASE(channel_peer__resume__maximum_payload_size__oversized_payload, peer_loopback_fixture)
{
    start(peer_keep);
    send(system::base16_chunk("f9beb4d9" "70696e670000000000000000" "ffffffff" "00000000"));
    BOOST_REQUIRE_EQUAL(peer_await(stopped), error::oversized_payload);
}

BOOST_FIXTURE_TEST_CASE(channel_peer__resume__bad_checksum__invalid_checksum, peer_loopback_fixture)
{
    start(peer_keep);
    send(system::base16_chunk("f9beb4d9" "70696e670000000000000000" "08000000" "00000000" "0102030405060708"));
    BOOST_REQUIRE_EQUAL(peer_await(stopped), error::invalid_checksum);
}

BOOST_FIXTURE_TEST_CASE(channel_peer__resume__ping_without_nonce__invalid_message, peer_loopback_fixture)
{
    start(peer_keep);
    send(system::base16_chunk("f9beb4d9" "70696e670000000000000000" "00000000" "5df6e0e2"));
    BOOST_REQUIRE_EQUAL(peer_await(stopped), error::invalid_message);
}

BOOST_FIXTURE_TEST_CASE(channel_peer__resume__ping__dispatched_and_counted, peer_loopback_fixture)
{
    const auto nonce = std::make_shared<std::promise<uint64_t>>();
    start([=](const channel_peer::ptr&, const messages::peer::ping::cptr& message) NOEXCEPT
    {
        nonce->set_value(message->nonce);
        return false;
    });

    send(peer_frame("ping", system::base16_chunk("0807060504030201")));
    BOOST_REQUIRE_EQUAL(nonce->get_future().get(), 0x0102030405060708u);
}

BOOST_FIXTURE_TEST_CASE(channel_peer__resume__current_large_payload__dispatched, peer_loopback_fixture)
{
    const auto nonce = std::make_shared<std::promise<uint64_t>>();
    start([=](const channel_peer::ptr&, const messages::peer::ping::cptr& message) NOEXCEPT
    {
        nonce->set_value(message->nonce);
        return false;
    }, true);

    auto inventory = system::base16_chunk("96");
    inventory.resize(add1(150u * 36u));
    send(peer_frame("inv", inventory));
    send(peer_frame("ping", system::base16_chunk("0100000000000000")));
    BOOST_REQUIRE_EQUAL(nonce->get_future().get(), 1u);
}

BOOST_FIXTURE_TEST_CASE(channel_peer__gate__retained_peer_close__peer_disconnect, peer_loopback_fixture)
{
    const auto retained = std::make_shared<channel::gate_t::ptr>();
    start([=](const channel_peer::ptr& self, const messages::peer::ping::cptr&) NOEXCEPT
    {
        *retained = self->gate();
        return true;
    });

    send(peer_frame("ping", system::base16_chunk("0100000000000000")));

    boost_code ignore{};
    client.shutdown(asio::socket::shutdown_send, ignore);
    BOOST_REQUIRE_EQUAL(peer_await(stopped), error::peer_disconnect);
}

// p2ps

using namespace std::chrono_literals;

static const network::p2ps::context p2ps_configuration{ 0xd9b4bef9 };

struct p2ps_loopback_fixture
{
    DELETE_COPY_MOVE(p2ps_loopback_fixture);

    static constexpr uint16_t port = 65133;

    p2ps_loopback_fixture() NOEXCEPT
      : pool_(2), strand_(pool_.service().get_executor()), acceptor_(strand_),
        client(client_service)
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
            .maximum_request = options.maximum_request,
            .minimum_buffer = options.minimum_buffer,
            .maximum_buffer = options.maximum_buffer,
            .context = network::socket::context{ std::cref(p2ps_configuration) }
        };

        server_ = std::make_shared<network::socket>(log, pool_.service(), std::move(params));
        const auto promise = accepted_;
        server_->accept(acceptor_, [=](const code& accept_ec) NOEXCEPT
        {
            promise->set_value(accept_ec);
        });

        client.connect(local, ec);
        BOOST_REQUIRE(!ec);
    }

    ~p2ps_loopback_fixture() NOEXCEPT
    {
        boost_code ignore{};
        client.close(ignore);
        channel ? channel->stop(error::service_stopped) : server_->stop();
        boost::asio::post(strand_, [this]() NOEXCEPT
        {
            boost_code cancel{};
            acceptor_.cancel(cancel);
        });

        pool_.stop();
        BOOST_REQUIRE(pool_.join());
    }

    void accept() NOEXCEPT
    {
        BOOST_REQUIRE_EQUAL(peer_await(accepted_), error::success);
        channel = std::make_shared<resumable_channel_peer>(log, server_, 42, peer_checked_settings, options);
        const auto promise = stopped;
        channel->subscribe_stop([=](const code& stop_ec) NOEXCEPT
        {
            promise->set_value(stop_ec);
        }, [](const code&) NOEXCEPT {});
    }

    bool encrypted() NOEXCEPT
    {
        const auto self = channel;
        const auto result = std::make_shared<std::promise<bool>>();
        boost::asio::post(channel->strand(), [=]() NOEXCEPT
        {
            result->set_value(self->encrypted());
        });

        return result->get_future().get();
    }

    void start(const peer_responder& respond) NOEXCEPT
    {
        const auto self = channel;
        const auto started = peer_make_promise();
        boost::asio::post(channel->strand(), [=]() NOEXCEPT
        {
            using namespace messages::peer;
            self->subscribe<ping>([=](const code& ec, const ping::cptr& message) NOEXCEPT
            {
                return !ec && respond(self, message);
            });

            self->resume();
            started->set_value(error::success);
        });

        BOOST_REQUIRE_EQUAL(peer_await(started), error::success);
    }

    const logger log{};

private:
    threadpool pool_;
    asio::strand strand_;
    asio::acceptor acceptor_;
    const peer_promise accepted_{ peer_make_promise() };
    network::socket::ptr server_{};

public:
    asio::context client_service{};
    asio::socket client;
    const peer_promise stopped{ peer_make_promise() };
    std::shared_ptr<resumable_channel_peer> channel{};
};

BOOST_FIXTURE_TEST_CASE(channel_peer__resume__p2ps_v1_version_prefix__v1_served, p2ps_loopback_fixture)
{
    boost_code ec{};
    boost::asio::write(client, boost::asio::buffer(system::base16_chunk("f9beb4d9" "76657273696f6e0000000000" "00000000" "5df6e0e2")), ec);
    BOOST_REQUIRE(!ec);

    accept();
    start(peer_keep);
    BOOST_REQUIRE(!encrypted());
    BOOST_REQUIRE_EQUAL(peer_await(stopped), error::invalid_message);
}

BOOST_FIXTURE_TEST_CASE(channel_peer__resume__p2ps_v2_initiator__encrypted_ping_exchange, p2ps_loopback_fixture)
{
    using namespace messages::peer;
    boost_code handshake{ boost::asio::error::would_block };
    network::p2ps::stream initiator{ std::move(client), p2ps_configuration };
    initiator.async_handshake([&](const boost_code& shake_ec) NOEXCEPT
    {
        handshake = shake_ec;
    });

    client_service.run_for(5s);
    client_service.restart();
    BOOST_REQUIRE(!handshake);

    accept();
    BOOST_REQUIRE(encrypted());

    const auto nonce = std::make_shared<std::promise<uint64_t>>();
    start([=](const channel_peer::ptr& self, const ping::cptr& message) NOEXCEPT
    {
        nonce->set_value(message->nonce);
        self->send<ping>(ping{ 7 }, [](const code&) NOEXCEPT {});
        return false;
    });

    boost_code sent{ boost::asio::error::would_block };
    initiator.async_write_message(identifiers::ping, "", system::to_shared(system::base16_chunk("0100000000000000")), [&](const boost_code& write_ec, size_t) NOEXCEPT
    {
        sent = write_ec;
    });

    client_service.run_for(5s);
    client_service.restart();
    BOOST_REQUIRE(!sent);
    BOOST_REQUIRE_EQUAL(nonce->get_future().get(), 1u);

    uint8_t identifier{};
    system::data_chunk payload{};
    system::data_chunk buffer{};
    boost_code received{ boost::asio::error::would_block };
    initiator.async_read_message(buffer, network::p2ps::cipher::maximum_content, [&](const boost_code& read_ec, uint8_t id, const std::string&, const network::p2ps::stream::payload_t& data) NOEXCEPT
    {
        received = read_ec;
        identifier = id;
        payload.assign(data.begin(), data.end());
    });

    client_service.run_for(5s);
    client_service.restart();
    BOOST_REQUIRE(!received);
    BOOST_REQUIRE_EQUAL(identifier, identifiers::ping);
    BOOST_REQUIRE_EQUAL(payload, system::base16_chunk("0700000000000000"));
}

BOOST_FIXTURE_TEST_CASE(channel_peer__resume__p2ps_v2_initiator_closed__peer_disconnect, p2ps_loopback_fixture)
{
    boost_code handshake{ boost::asio::error::would_block };
    auto initiator = std::make_unique<network::p2ps::stream>(std::move(client), p2ps_configuration);
    initiator->async_handshake([&](const boost_code& shake_ec) NOEXCEPT
    {
        handshake = shake_ec;
    });

    client_service.run_for(5s);
    client_service.restart();
    BOOST_REQUIRE(!handshake);

    accept();
    start(peer_keep);
    initiator.reset();
    BOOST_REQUIRE_EQUAL(peer_await(stopped), error::peer_disconnect);
}

BOOST_AUTO_TEST_SUITE_END()
