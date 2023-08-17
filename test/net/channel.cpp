/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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

BOOST_AUTO_TEST_SUITE(channel_tests)

class channel_accessor
  : public channel
{
public:
    using channel::channel;

    size_t maximum_payload() const NOEXCEPT override
    {
        return channel::maximum_payload();
    }

    uint32_t protocol_magic() const NOEXCEPT override
    {
        return channel::protocol_magic();
    }

    bool validate_checksum() const NOEXCEPT override
    {
        return channel::validate_checksum();
    }

    uint32_t version() const NOEXCEPT override
    {
        return channel::version();
    }
};

BOOST_AUTO_TEST_CASE(channel__stopped__default__false)
{
    const logger log{};
    threadpool pool(1);
    asio::strand strand(pool.service().get_executor());
    const settings set(bc::system::chain::selection::mainnet);
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service());
    auto channel_ptr = std::make_shared<channel>(log, socket_ptr, set, 42);
    BOOST_REQUIRE(!channel_ptr->stopped());

    // Stop completion is asynchronous.
    channel_ptr->stop(error::invalid_magic);
    channel_ptr.reset();
}

inline size_t payload_maximum(const settings& settings)
{
    return messages::heading::maximum_payload(settings.protocol_maximum,
        to_bool(settings.services_maximum & messages::service::node_witness));
}

BOOST_AUTO_TEST_CASE(channel__properties__default__expected)
{
    const logger log{};
    threadpool pool(1);
    asio::strand strand(pool.service().get_executor());
    const settings set(bc::system::chain::selection::mainnet);
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service());
    auto channel_ptr = std::make_shared<channel_accessor>(log, socket_ptr, set, 42);

    BOOST_REQUIRE(!channel_ptr->address());
    BOOST_REQUIRE_NE(channel_ptr->nonce(), 0u);
    BOOST_REQUIRE_EQUAL(channel_ptr->negotiated_version(), set.protocol_maximum);

    // TODO: compare to default instance.
    BOOST_REQUIRE(channel_ptr->peer_version());

    BOOST_REQUIRE_EQUAL(channel_ptr->maximum_payload(), payload_maximum(set));
    BOOST_REQUIRE_EQUAL(channel_ptr->protocol_magic(), set.identifier);
    BOOST_REQUIRE_EQUAL(channel_ptr->validate_checksum(), set.validate_checksum);
    BOOST_REQUIRE_EQUAL(channel_ptr->version(), set.protocol_maximum);

    channel_ptr->stop(error::invalid_magic);
    channel_ptr.reset();
}

BOOST_AUTO_TEST_SUITE_END()
