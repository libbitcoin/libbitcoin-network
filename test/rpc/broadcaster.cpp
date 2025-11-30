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

BOOST_AUTO_TEST_SUITE(broadcaster_tests)

using namespace rpc;
using namespace system;
using namespace messages::peer;

// uses desubscriber<> (void handler, requires broadcaster and interface::key).
struct mock_desubscriber
{
    using key = uint64_t;

    static constexpr std::tuple methods
    {
        method<"ping0", ping::cptr, size_t>{},
        method<"ping1", ping::cptr, string_t>{ "message", "value" },
        method<"ping2", size_t, nullable<ping::cptr>>{},
        method<"ping3", size_t, nullable<ping::cptr>>{ "value", "message" }
    };

    template <typename... Args>
    using subscriber = network::desubscriber<key, Args...>;

    template <size_t Index>
    using at = method_at<methods, Index>;

    using ping0 = at<0>;
    using ping1 = at<1>;
    using ping2 = at<2>;
    using ping3 = at<3>;
};

using mock_desubscriber_interface = publish<mock_desubscriber>;
using mock_broadcaster = broadcaster<mock_desubscriber_interface>;

BOOST_AUTO_TEST_CASE(broadcaster__notify__native_positional__expected)
{
    mock_broadcaster instance{};
    constexpr uint64_t expected_nonce = 42;
    constexpr size_t expected_value = 42;
    constexpr mock_broadcaster::key_t channel_id = 17;
    constexpr auto expected_ec = network::error::invalid_magic;
    bool called{};
    bool result{};
    code stop_ec{};

    BOOST_REQUIRE(!instance.subscribe(
        [&](const code& ec, ping::cptr ping, size_t value)
        {
            // Handle stop notification (unavoidable test condition).
            if (called)
            {
                stop_ec = ec;
                return true;
            }

            // Handle message notification.
            result = (!ec);
            result &= (ping->nonce == expected_nonce);
            result &= (value == expected_value);
            called = true;
            return true;
        }, channel_id));

    const auto message = emplace_shared<const ping>(expected_nonce);
    BOOST_REQUIRE(!instance.notify(request_t
    {
        .method = "ping0",
        .params = { array_t{ any_t{ message }, expected_value } }
    }, channel_id));

    instance.stop(expected_ec);
    BOOST_REQUIRE_EQUAL(stop_ec, expected_ec);
    BOOST_REQUIRE(result);
}

BOOST_AUTO_TEST_CASE(broadcaster__notify__native_named__expected)
{
    mock_broadcaster instance{};
    constexpr uint64_t expected_nonce = 42;
    const std::string expected_value{ "42" };
    constexpr mock_broadcaster::key_t channel_id = 17;
    constexpr auto expected_ec = network::error::invalid_magic;
    bool called{};
    bool result{};
    code stop_ec{};

    BOOST_REQUIRE(!instance.subscribe(
        [&](const code& ec, const ping::cptr& ping, const std::string& value)
        {
            // Handle stop notification (unavoidable test condition).
            if (called)
            {
                stop_ec = ec;
                return true;
            }

            // Handle message notification.
            result = (!ec);
            result &= (ping->nonce == expected_nonce);
            result &= (value == expected_value);
            called = true;
            return true;
        }, channel_id));

    const auto message = emplace_shared<const ping>(expected_nonce);
    BOOST_REQUIRE(!instance.notify(request_t
    {
        .method = "ping1",
        .params = { object_t{ { "message", any_t{ message } }, { "value", expected_value } } }
    }, channel_id));

    instance.stop(expected_ec);
    BOOST_REQUIRE_EQUAL(stop_ec, expected_ec);
    BOOST_REQUIRE(result);
}

BOOST_AUTO_TEST_CASE(broadcaster__notify__non_native_nullable_positional__expected)
{
    mock_broadcaster instance{};
    using tag = mock_desubscriber::ping2;
    constexpr uint64_t expected_nonce = 42;
    constexpr size_t expected_value = 42;
    constexpr mock_broadcaster::key_t channel_id = 17;
    constexpr auto expected_ec = network::error::invalid_magic;
    bool called{};
    bool result{};
    code stop_ec{};

    BOOST_REQUIRE(!instance.subscribe(
        [&](const code& ec, tag, size_t value, std::optional<ping::cptr> ping)
        {
            // Handle stop notification (unavoidable test condition).
            if (called)
            {
                stop_ec = ec;
                return true;
            }

            // Handle message notification.
            result = (!ec);
            result &= ping.has_value();
            result &= (ping.value()->nonce == expected_nonce);
            result &= (value == expected_value);
            called = true;
            return true;
        }, channel_id));

    const auto message = emplace_shared<const ping>(expected_nonce);
    instance.notify(request_t
    {
        .method = "ping2",
        .params = { array_t{ expected_value, any_t{ message } } }
    }, channel_id);

    instance.stop(expected_ec);
    BOOST_REQUIRE_EQUAL(stop_ec, expected_ec);
    BOOST_REQUIRE(result);
}

BOOST_AUTO_TEST_CASE(broadcaster__notify__non_native_nullable_named__expected)
{
    mock_broadcaster instance{};
    using tag = mock_desubscriber::ping3;
    constexpr uint64_t expected_nonce = 42;
    constexpr size_t expected_value = 42;
    constexpr mock_broadcaster::key_t channel_id = 17;
    constexpr auto expected_ec = network::error::invalid_magic;
    bool called{};
    bool result{};
    code stop_ec{};

    BOOST_REQUIRE(!instance.subscribe(
        [&](const code& ec, tag, size_t value, std::optional<ping::cptr> ping)
        {
            // Handle stop notification (unavoidable test condition).
            if (called)
            {
                stop_ec = ec;
                return true;
            }

            // Handle message notification.
            result = (!ec);
            result &= ping.has_value();
            result &= (ping.value()->nonce == expected_nonce);
            result &= (value == expected_value);
            called = true;
            return true;
        }, channel_id));

    const auto message = emplace_shared<const ping>(expected_nonce);
    instance.notify(request_t
    {
        .method = "ping3",
        .params = { object_t{ { "value", expected_value }, { "message", any_t{ message } } } }
    }, channel_id);

    instance.stop(expected_ec);
    BOOST_REQUIRE_EQUAL(stop_ec, expected_ec);
    BOOST_REQUIRE(result);
}

BOOST_AUTO_TEST_CASE(broadcaster__subscribe__peer_broadcaster_stop__expected)
{
    using peer_broadcaster = broadcaster<interface::peer::broadcast>;

    peer_broadcaster instance{};
    constexpr auto expected_ec = network::error::invalid_magic;
    bool result{};

    BOOST_REQUIRE(!instance.subscribe(
        [&](const code& ec, const ping::cptr& ping, peer_broadcaster::key_t id)
        {
            // Stop notification has nullptr message, zero id, and specified code.
            result = (ec == expected_ec);
            result &= is_null(ping);
            result &= is_zero(id);
            return true;
        }, 17));

    instance.stop(expected_ec);
    BOOST_REQUIRE(result);
}

BOOST_AUTO_TEST_SUITE_END()
