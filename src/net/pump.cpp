/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/net/pump.hpp>

#include <memory>
#include <string>
#include <bitcoin/system.hpp>

#define INITIALIZE_SUBSCRIBER(pool, name) \
    name##_subscriber_(std::make_shared<name##_subscriber_type>( \
        pool, #name "_sub"))

#define RELAY_CODE(code, name) \
    name##_subscriber_->relay(code, {})

// This allows us to block the peer while handling the message.
#define CASE_HANDLE_MESSAGE(reader, version, name) \
    case message_type::name: \
        return handle<messages::name>(reader, version, name##_subscriber_)

#define CASE_RELAY_MESSAGE(reader, version, name) \
    case message_type::name: \
        return relay<messages::name>(reader, version, name##_subscriber_)

#define START_SUBSCRIBER(name) \
    name##_subscriber_->start()

#define STOP_SUBSCRIBER(name) \
    name##_subscriber_->stop()

namespace libbitcoin {
namespace network {

using namespace bc::system;
using namespace bc::system::messages;

pump::pump(threadpool& pool)
  : INITIALIZE_SUBSCRIBER(pool, address),
    INITIALIZE_SUBSCRIBER(pool, alert),
    INITIALIZE_SUBSCRIBER(pool, block),
    INITIALIZE_SUBSCRIBER(pool, block_transactions),
    INITIALIZE_SUBSCRIBER(pool, compact_block),
    INITIALIZE_SUBSCRIBER(pool, compact_filter),
    INITIALIZE_SUBSCRIBER(pool, compact_filter_checkpoint),
    INITIALIZE_SUBSCRIBER(pool, compact_filter_headers),
    INITIALIZE_SUBSCRIBER(pool, fee_filter),
    INITIALIZE_SUBSCRIBER(pool, filter_add),
    INITIALIZE_SUBSCRIBER(pool, filter_clear),
    INITIALIZE_SUBSCRIBER(pool, filter_load),
    INITIALIZE_SUBSCRIBER(pool, get_address),
    INITIALIZE_SUBSCRIBER(pool, get_blocks),
    INITIALIZE_SUBSCRIBER(pool, get_block_transactions),
    INITIALIZE_SUBSCRIBER(pool, get_compact_filter_checkpoint),
    INITIALIZE_SUBSCRIBER(pool, get_compact_filter_headers),
    INITIALIZE_SUBSCRIBER(pool, get_compact_filters),
    INITIALIZE_SUBSCRIBER(pool, get_data),
    INITIALIZE_SUBSCRIBER(pool, get_headers),
    INITIALIZE_SUBSCRIBER(pool, headers),
    INITIALIZE_SUBSCRIBER(pool, inventory),
    INITIALIZE_SUBSCRIBER(pool, memory_pool),
    INITIALIZE_SUBSCRIBER(pool, merkle_block),
    INITIALIZE_SUBSCRIBER(pool, not_found),
    INITIALIZE_SUBSCRIBER(pool, ping),
    INITIALIZE_SUBSCRIBER(pool, pong),
    INITIALIZE_SUBSCRIBER(pool, reject),
    INITIALIZE_SUBSCRIBER(pool, send_compact),
    INITIALIZE_SUBSCRIBER(pool, send_headers),
    INITIALIZE_SUBSCRIBER(pool, transaction),
    INITIALIZE_SUBSCRIBER(pool, verack),
    INITIALIZE_SUBSCRIBER(pool, version)
{
}

void pump::broadcast(const code& ec)
{
    RELAY_CODE(ec, address);
    RELAY_CODE(ec, alert);
    RELAY_CODE(ec, block);
    RELAY_CODE(ec, block_transactions);
    RELAY_CODE(ec, compact_block);
    RELAY_CODE(ec, compact_filter);
    RELAY_CODE(ec, compact_filter_checkpoint);
    RELAY_CODE(ec, compact_filter_headers);
    RELAY_CODE(ec, fee_filter);
    RELAY_CODE(ec, filter_add);
    RELAY_CODE(ec, filter_clear);
    RELAY_CODE(ec, filter_load);
    RELAY_CODE(ec, get_address);
    RELAY_CODE(ec, get_blocks);
    RELAY_CODE(ec, get_block_transactions);
    RELAY_CODE(ec, get_compact_filter_checkpoint);
    RELAY_CODE(ec, get_compact_filter_headers);
    RELAY_CODE(ec, get_compact_filters);
    RELAY_CODE(ec, get_data);
    RELAY_CODE(ec, get_headers);
    RELAY_CODE(ec, headers);
    RELAY_CODE(ec, inventory);
    RELAY_CODE(ec, memory_pool);
    RELAY_CODE(ec, merkle_block);
    RELAY_CODE(ec, not_found);
    RELAY_CODE(ec, ping);
    RELAY_CODE(ec, pong);
    RELAY_CODE(ec, reject);
    RELAY_CODE(ec, send_compact);
    RELAY_CODE(ec, send_headers);
    RELAY_CODE(ec, transaction);
    RELAY_CODE(ec, verack);
    RELAY_CODE(ec, version);
}

code pump::load(message_type type, uint32_t version,
    system::reader& reader) const
{
    switch (type)
    {
        CASE_RELAY_MESSAGE(reader, version, address);
        CASE_RELAY_MESSAGE(reader, version, alert);
        CASE_HANDLE_MESSAGE(reader, version, block);
        CASE_RELAY_MESSAGE(reader, version, block_transactions);
        CASE_RELAY_MESSAGE(reader, version, compact_block);
        CASE_RELAY_MESSAGE(reader, version, compact_filter);
        CASE_RELAY_MESSAGE(reader, version, compact_filter_checkpoint);
        CASE_RELAY_MESSAGE(reader, version, compact_filter_headers);
        CASE_RELAY_MESSAGE(reader, version, fee_filter);
        CASE_RELAY_MESSAGE(reader, version, filter_add);
        CASE_RELAY_MESSAGE(reader, version, filter_clear);
        CASE_RELAY_MESSAGE(reader, version, filter_load);
        CASE_RELAY_MESSAGE(reader, version, get_address);
        CASE_RELAY_MESSAGE(reader, version, get_blocks);
        CASE_RELAY_MESSAGE(reader, version, get_block_transactions);
        CASE_RELAY_MESSAGE(reader, version, get_compact_filter_checkpoint);
        CASE_RELAY_MESSAGE(reader, version, get_compact_filter_headers);
        CASE_RELAY_MESSAGE(reader, version, get_compact_filters);
        CASE_RELAY_MESSAGE(reader, version, get_data);
        CASE_RELAY_MESSAGE(reader, version, get_headers);
        CASE_RELAY_MESSAGE(reader, version, headers);
        CASE_RELAY_MESSAGE(reader, version, inventory);
        CASE_RELAY_MESSAGE(reader, version, memory_pool);
        CASE_RELAY_MESSAGE(reader, version, merkle_block);
        CASE_RELAY_MESSAGE(reader, version, not_found);
        CASE_HANDLE_MESSAGE(reader, version, ping);
        CASE_HANDLE_MESSAGE(reader, version, pong);
        CASE_RELAY_MESSAGE(reader, version, reject);
        CASE_RELAY_MESSAGE(reader, version, send_compact);
        CASE_RELAY_MESSAGE(reader, version, send_headers);
        CASE_HANDLE_MESSAGE(reader, version, transaction);
        CASE_HANDLE_MESSAGE(reader, version, verack);
        CASE_HANDLE_MESSAGE(reader, version, version);
        case message_type::unknown:
        default:
            return error::not_found;
    }
}

void pump::start()
{
    START_SUBSCRIBER(address);
    START_SUBSCRIBER(alert);
    START_SUBSCRIBER(block);
    START_SUBSCRIBER(block_transactions);
    START_SUBSCRIBER(compact_block);
    START_SUBSCRIBER(compact_filter);
    START_SUBSCRIBER(compact_filter_checkpoint);
    START_SUBSCRIBER(compact_filter_headers);
    START_SUBSCRIBER(fee_filter);
    START_SUBSCRIBER(filter_add);
    START_SUBSCRIBER(filter_clear);
    START_SUBSCRIBER(filter_load);
    START_SUBSCRIBER(get_address);
    START_SUBSCRIBER(get_blocks);
    START_SUBSCRIBER(get_block_transactions);
    START_SUBSCRIBER(get_compact_filter_checkpoint);
    START_SUBSCRIBER(get_compact_filter_headers);
    START_SUBSCRIBER(get_compact_filters);
    START_SUBSCRIBER(get_data);
    START_SUBSCRIBER(get_headers);
    START_SUBSCRIBER(headers);
    START_SUBSCRIBER(inventory);
    START_SUBSCRIBER(memory_pool);
    START_SUBSCRIBER(merkle_block);
    START_SUBSCRIBER(not_found);
    START_SUBSCRIBER(ping);
    START_SUBSCRIBER(pong);
    START_SUBSCRIBER(reject);
    START_SUBSCRIBER(send_compact);
    START_SUBSCRIBER(send_headers);
    START_SUBSCRIBER(transaction);
    START_SUBSCRIBER(verack);
    START_SUBSCRIBER(version);
}

void pump::stop()
{
    STOP_SUBSCRIBER(address);
    STOP_SUBSCRIBER(alert);
    STOP_SUBSCRIBER(block);
    STOP_SUBSCRIBER(block_transactions);
    STOP_SUBSCRIBER(compact_block);
    STOP_SUBSCRIBER(compact_filter);
    STOP_SUBSCRIBER(compact_filter_checkpoint);
    STOP_SUBSCRIBER(compact_filter_headers);
    STOP_SUBSCRIBER(fee_filter);
    STOP_SUBSCRIBER(filter_add);
    STOP_SUBSCRIBER(filter_clear);
    STOP_SUBSCRIBER(filter_load);
    STOP_SUBSCRIBER(get_address);
    STOP_SUBSCRIBER(get_blocks);
    STOP_SUBSCRIBER(get_block_transactions);
    STOP_SUBSCRIBER(get_compact_filter_checkpoint);
    STOP_SUBSCRIBER(get_compact_filter_headers);
    STOP_SUBSCRIBER(get_compact_filters);
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
    STOP_SUBSCRIBER(verack);
    STOP_SUBSCRIBER(version);
}

} // namespace network
} // namespace libbitcoin