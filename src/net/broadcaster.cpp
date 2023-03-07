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
#include <bitcoin/network/net/broadcaster.hpp>

#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {

using namespace system;

#define SUBSCRIBER(name) name##_subscriber_
#define MAKE_SUBSCRIBER(name) SUBSCRIBER(name)(strand)
#define STOP_SUBSCRIBER(name) SUBSCRIBER(name).stop_default(ec)
#define UNSUBSCRIBER(name) SUBSCRIBER(name) \
    .notify_one(subscriber, error::desubscribed, nullptr, subscriber)

broadcaster::broadcaster(asio::strand& strand) NOEXCEPT
  : MAKE_SUBSCRIBER(address),
    MAKE_SUBSCRIBER(alert),
    MAKE_SUBSCRIBER(block),
    MAKE_SUBSCRIBER(bloom_filter_add),
    MAKE_SUBSCRIBER(bloom_filter_clear),
    MAKE_SUBSCRIBER(bloom_filter_load),
    MAKE_SUBSCRIBER(client_filter),
    MAKE_SUBSCRIBER(client_filter_checkpoint),
    MAKE_SUBSCRIBER(client_filter_headers),
    MAKE_SUBSCRIBER(compact_block),
    MAKE_SUBSCRIBER(compact_transactions),
    MAKE_SUBSCRIBER(fee_filter),
    MAKE_SUBSCRIBER(get_address),
    MAKE_SUBSCRIBER(get_blocks),
    MAKE_SUBSCRIBER(get_client_filter_checkpoint),
    MAKE_SUBSCRIBER(get_client_filter_headers),
    MAKE_SUBSCRIBER(get_client_filters),
    MAKE_SUBSCRIBER(get_compact_transactions),
    MAKE_SUBSCRIBER(get_data),
    MAKE_SUBSCRIBER(get_headers),
    MAKE_SUBSCRIBER(headers),
    MAKE_SUBSCRIBER(inventory),
    MAKE_SUBSCRIBER(memory_pool),
    MAKE_SUBSCRIBER(merkle_block),
    MAKE_SUBSCRIBER(not_found),
    MAKE_SUBSCRIBER(ping),
    MAKE_SUBSCRIBER(pong),
    MAKE_SUBSCRIBER(reject),
    MAKE_SUBSCRIBER(send_compact),
    MAKE_SUBSCRIBER(send_headers),
    MAKE_SUBSCRIBER(transaction),
    MAKE_SUBSCRIBER(version),
    MAKE_SUBSCRIBER(version_acknowledge)
{
}

void broadcaster::unsubscribe(channel_id subscriber) NOEXCEPT
{
    UNSUBSCRIBER(address);
    UNSUBSCRIBER(alert);
    UNSUBSCRIBER(block);
    UNSUBSCRIBER(bloom_filter_add);
    UNSUBSCRIBER(bloom_filter_clear);
    UNSUBSCRIBER(bloom_filter_load);
    UNSUBSCRIBER(client_filter);
    UNSUBSCRIBER(client_filter_checkpoint);
    UNSUBSCRIBER(client_filter_headers);
    UNSUBSCRIBER(compact_block);
    UNSUBSCRIBER(compact_transactions);
    UNSUBSCRIBER(fee_filter);
    UNSUBSCRIBER(get_address);
    UNSUBSCRIBER(get_blocks);
    UNSUBSCRIBER(get_client_filter_checkpoint);
    UNSUBSCRIBER(get_client_filter_headers);
    UNSUBSCRIBER(get_client_filters);
    UNSUBSCRIBER(get_compact_transactions);
    UNSUBSCRIBER(get_data);
    UNSUBSCRIBER(get_headers);
    UNSUBSCRIBER(headers);
    UNSUBSCRIBER(inventory);
    UNSUBSCRIBER(memory_pool);
    UNSUBSCRIBER(merkle_block);
    UNSUBSCRIBER(not_found);
    UNSUBSCRIBER(ping);
    UNSUBSCRIBER(pong);
    UNSUBSCRIBER(reject);
    UNSUBSCRIBER(send_compact);
    UNSUBSCRIBER(send_headers);
    UNSUBSCRIBER(transaction);
    UNSUBSCRIBER(version);
    UNSUBSCRIBER(version_acknowledge);
}

void broadcaster::stop(const code& ec) NOEXCEPT
{
    STOP_SUBSCRIBER(address);
    STOP_SUBSCRIBER(alert);
    STOP_SUBSCRIBER(block);
    STOP_SUBSCRIBER(bloom_filter_add);
    STOP_SUBSCRIBER(bloom_filter_clear);
    STOP_SUBSCRIBER(bloom_filter_load);
    STOP_SUBSCRIBER(client_filter);
    STOP_SUBSCRIBER(client_filter_checkpoint);
    STOP_SUBSCRIBER(client_filter_headers);
    STOP_SUBSCRIBER(compact_block);
    STOP_SUBSCRIBER(compact_transactions);
    STOP_SUBSCRIBER(fee_filter);
    STOP_SUBSCRIBER(get_address);
    STOP_SUBSCRIBER(get_blocks);
    STOP_SUBSCRIBER(get_client_filter_checkpoint);
    STOP_SUBSCRIBER(get_client_filter_headers);
    STOP_SUBSCRIBER(get_client_filters);
    STOP_SUBSCRIBER(get_compact_transactions);
    STOP_SUBSCRIBER(get_data);
    STOP_SUBSCRIBER(get_headers);
    STOP_SUBSCRIBER(headers);
    STOP_SUBSCRIBER(inventory);
    STOP_SUBSCRIBER(memory_pool);
    STOP_SUBSCRIBER(merkle_block);
    STOP_SUBSCRIBER(not_found);
    STOP_SUBSCRIBER(ping);
    STOP_SUBSCRIBER(pong);
    STOP_SUBSCRIBER(reject);
    STOP_SUBSCRIBER(send_compact);
    STOP_SUBSCRIBER(send_headers);
    STOP_SUBSCRIBER(transaction);
    STOP_SUBSCRIBER(version);
    STOP_SUBSCRIBER(version_acknowledge);
}

#undef SUBSCRIBER
#undef MAKE_SUBSCRIBER
#undef STOP_SUBSCRIBER

} // namespace network
} // namespace libbitcoin
