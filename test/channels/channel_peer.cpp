/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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

BOOST_AUTO_TEST_CASE(channel_peer__stopped__default__false)
{
    constexpr auto expected_identifier = 42u;
    default_memory memory{};
    const logger log{};
    threadpool pool(1);
    asio::strand strand(pool.service().get_executor());
    const settings set(bc::system::chain::selection::mainnet);
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service());
    auto channel_ptr = std::make_shared<channel_peer>(memory, log, socket_ptr, set, expected_identifier);
    BOOST_REQUIRE(!channel_ptr->stopped());

    BOOST_REQUIRE_NE(channel_ptr->nonce(), zero);
    BOOST_REQUIRE_EQUAL(channel_ptr->identifier(), expected_identifier);

    // Stop is asynchronous, threadpool destruct blocks until all complete.
    // Calling stop here sets channel.stopped and prevents destructor assertion.
    channel_ptr->stop(error::invalid_magic);
}

BOOST_AUTO_TEST_CASE(channel_peer__properties__default__expected)
{
    default_memory memory{};
    const logger log{};
    threadpool pool(1);
    asio::strand strand(pool.service().get_executor());
    const settings set(bc::system::chain::selection::mainnet);
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service());
    auto channel_ptr = std::make_shared<channel_peer>(memory, log, socket_ptr, set, 42);

    const auto payload_maximum = [](const settings& settings) NOEXCEPT
    {
        using namespace messages::peer;
        return heading::maximum_payload(settings.protocol_maximum,
            to_bool(settings.services_maximum & service::node_witness));
    };

    BOOST_REQUIRE(!channel_ptr->address());
    BOOST_REQUIRE_NE(channel_ptr->nonce(), 0u);
    BOOST_REQUIRE_EQUAL(channel_ptr->negotiated_version(), set.protocol_maximum);
    BOOST_REQUIRE(channel_ptr->is_negotiated(messages::peer::level::maximum_protocol));

    // TODO: compare to default instance.
    BOOST_REQUIRE(channel_ptr->peer_version());

    BOOST_REQUIRE_EQUAL(channel_ptr->settings().maximum_payload(), payload_maximum(set));
    BOOST_REQUIRE_EQUAL(channel_ptr->settings().identifier, set.identifier);
    BOOST_REQUIRE_EQUAL(channel_ptr->settings().validate_checksum, set.validate_checksum);
    BOOST_REQUIRE_EQUAL(channel_ptr->settings().minimum_buffer, set.minimum_buffer);

    // Stop is asynchronous, threadpool destruct blocks until all complete.
    // Calling stop here sets channel.stopped and prevents destructor assertion.
    channel_ptr->stop(error::invalid_magic);
}

BOOST_AUTO_TEST_CASE(channel_peer__subscribe_message__subscribed__expected)
{
    default_memory memory{};
    const logger log{};
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    const settings set(bc::system::chain::selection::mainnet);
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service());
    auto channel_ptr = std::make_shared<channel_peer>(memory, log, socket_ptr, set, 42);
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
    default_memory memory{};
    const logger log{};
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    const settings set(bc::system::chain::selection::mainnet);
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service());
    auto channel_ptr = std::make_shared<mock_channel_peer>(memory, log, socket_ptr, set, 42);
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
    default_memory memory{};
    const logger log{};
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    const settings set(bc::system::chain::selection::mainnet);
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service());
    auto channel_ptr = std::make_shared<channel_peer>(memory, log, socket_ptr, set, 42);

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
    default_memory memory{};
    const logger log{};
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    const settings set(bc::system::chain::selection::mainnet);
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service());
    auto channel_ptr = std::make_shared<channel_peer>(memory, log, socket_ptr, set, 42);

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

BOOST_AUTO_TEST_CASE(channel_peer__paused__resume_after_read_fail__true)
{
    default_memory memory{};
    const logger log{};
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    const settings set(bc::system::chain::selection::mainnet);
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service());
    auto channel_ptr = std::make_shared<mock_channel_peer>(memory, log, socket_ptr, set, 42);

    std::promise<bool> paused_after_resume;
    boost::asio::post(channel_ptr->strand(), [=, &paused_after_resume]() NOEXCEPT
    {
        // Resume queues up a (failing) read that will invoke stopped.
        channel_ptr->resume();
        paused_after_resume.set_value(channel_ptr->paused());
    });

    BOOST_REQUIRE(!paused_after_resume.get_future().get());
    BOOST_REQUIRE(channel_ptr->require_stopped());

    std::promise<bool> paused_after_read_fail;
    boost::asio::post(channel_ptr->strand(), [=, &paused_after_read_fail]() NOEXCEPT
    {
        // paused() requires strand.
        paused_after_read_fail.set_value(channel_ptr->paused());
    });

    BOOST_REQUIRE(paused_after_read_fail.get_future().get());

    // Stop is asynchronous, threadpool destruct blocks until all complete.
    // Calling stop here sets channel.stopped and prevents destructor assertion.
    channel_ptr->stop(error::invalid_magic);
}

BOOST_AUTO_TEST_SUITE_END()
