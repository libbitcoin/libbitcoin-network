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
#ifndef LIBBITCOIN_NETWORK_NET_BROADCASTER_HPP
#define LIBBITCOIN_NETWORK_NET_BROADCASTER_HPP

#include <functional>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {

#define SUBSCRIBER(name) name##_subscriber_
#define SUBSCRIBER_TYPE(name) name##_subscriber
#define DECLARE_SUBSCRIBER(name) SUBSCRIBER_TYPE(name) SUBSCRIBER(name)
#define DEFINE_SUBSCRIBER(name) using SUBSCRIBER_TYPE(name) = \
    desubscriber<channel_id, const messages::name::cptr&, channel_id>
#define SUBSCRIBER_OVERLOAD(name) code do_subscribe( \
    broadcaster::handler<messages::name>&& handler, channel_id id) NOEXCEPT \
    { return SUBSCRIBER(name).subscribe(std::move(handler), id); }
#define CASE_NOTIFY(name) case messages::identifier::name: \
    return SUBSCRIBER(name).notify(error::success, message, sender)

/// Not thread safe.
class BCT_API broadcaster
{
public:
    using channel_id = uint64_t;

    /// Helper for external declarations.
    template <class Message>
    using handler = std::function<bool(const code&,
        const typename Message::cptr&, channel_id)>;

    DELETE_COPY_MOVE_DESTRUCT(broadcaster);

    ////using address_subscriber = desubscriber<channel_id, const messages::address::cptr&, channel_id>;
    DEFINE_SUBSCRIBER(address);
    DEFINE_SUBSCRIBER(alert);
    DEFINE_SUBSCRIBER(block);
    DEFINE_SUBSCRIBER(bloom_filter_add);
    DEFINE_SUBSCRIBER(bloom_filter_clear);
    DEFINE_SUBSCRIBER(bloom_filter_load);
    DEFINE_SUBSCRIBER(client_filter);
    DEFINE_SUBSCRIBER(client_filter_checkpoint);
    DEFINE_SUBSCRIBER(client_filter_headers);
    DEFINE_SUBSCRIBER(compact_block);
    DEFINE_SUBSCRIBER(compact_transactions);
    DEFINE_SUBSCRIBER(fee_filter);
    DEFINE_SUBSCRIBER(get_address);
    DEFINE_SUBSCRIBER(get_blocks);
    DEFINE_SUBSCRIBER(get_client_filter_checkpoint);
    DEFINE_SUBSCRIBER(get_client_filter_headers);
    DEFINE_SUBSCRIBER(get_client_filters);
    DEFINE_SUBSCRIBER(get_compact_transactions);
    DEFINE_SUBSCRIBER(get_data);
    DEFINE_SUBSCRIBER(get_headers);
    DEFINE_SUBSCRIBER(headers);
    DEFINE_SUBSCRIBER(inventory);
    DEFINE_SUBSCRIBER(memory_pool);
    DEFINE_SUBSCRIBER(merkle_block);
    DEFINE_SUBSCRIBER(not_found);
    DEFINE_SUBSCRIBER(ping);
    DEFINE_SUBSCRIBER(pong);
    DEFINE_SUBSCRIBER(reject);
    DEFINE_SUBSCRIBER(send_compact);
    DEFINE_SUBSCRIBER(send_headers);
    DEFINE_SUBSCRIBER(transaction);
    DEFINE_SUBSCRIBER(version);
    DEFINE_SUBSCRIBER(version_acknowledge);

    /// Create an instance of this class.
    broadcaster(asio::strand& strand) NOEXCEPT;

    /// If stopped, handler is invoked with error::subscriber_stopped.
    /// If key exists, handler is invoked with error::subscriber_exists.
    /// Otherwise handler retained. Subscription code is also returned here.
    template <typename Handler>
    code subscribe(Handler&& handler, channel_id subscriber) NOEXCEPT
    {
        return do_subscribe(std::forward<Handler>(handler), subscriber);
    }

    /// Relay a message instance to each subscriber of the type.
    template <typename Message>
    code notify(const typename Message::cptr& message,
        channel_id sender) NOEXCEPT
    {
        switch (Message::id)
        {
            ////case messages::identifier::address:
            ////    return address_subscriber_.notify(error::success, message, sender);
            CASE_NOTIFY(address);
            CASE_NOTIFY(alert);
            CASE_NOTIFY(block);
            CASE_NOTIFY(bloom_filter_add);
            CASE_NOTIFY(bloom_filter_clear);
            CASE_NOTIFY(bloom_filter_load);
            CASE_NOTIFY(client_filter);
            CASE_NOTIFY(client_filter_checkpoint);
            CASE_NOTIFY(client_filter_headers);
            CASE_NOTIFY(compact_block);
            CASE_NOTIFY(compact_transactions);
            CASE_NOTIFY(fee_filter);
            CASE_NOTIFY(get_address);
            CASE_NOTIFY(get_blocks);
            CASE_NOTIFY(get_client_filter_checkpoint);
            CASE_NOTIFY(get_client_filter_headers);
            CASE_NOTIFY(get_client_filters);
            CASE_NOTIFY(get_compact_transactions);
            CASE_NOTIFY(get_data);
            CASE_NOTIFY(get_headers);
            CASE_NOTIFY(headers);
            CASE_NOTIFY(inventory);
            CASE_NOTIFY(memory_pool);
            CASE_NOTIFY(merkle_block);
            CASE_NOTIFY(not_found);
            CASE_NOTIFY(ping);
            CASE_NOTIFY(pong);
            CASE_NOTIFY(reject);
            CASE_NOTIFY(send_compact);
            CASE_NOTIFY(send_headers);
            CASE_NOTIFY(transaction);
            CASE_NOTIFY(version);
            CASE_NOTIFY(version_acknowledge);
            case messages::identifier::unknown:
            default:
                return error::unknown_message;
        }
    }

    /// Stop all subscribers, prevents subsequent subscription (idempotent).
    /// The subscriber is stopped regardless of the error code, however by
    /// convention handlers rely on the error code to avoid message processing.
    virtual void stop(const code& ec) NOEXCEPT;

private:
    ////code do_subscribe(broadcaster::handler<messages::address>&& handler, channel_id id) NOEXCEPT
    ////{
    ////    return address_subscriber_.subscribe(std::move(handler), id);
    ////}
    SUBSCRIBER_OVERLOAD(address);
    SUBSCRIBER_OVERLOAD(alert);
    SUBSCRIBER_OVERLOAD(block);
    SUBSCRIBER_OVERLOAD(bloom_filter_add);
    SUBSCRIBER_OVERLOAD(bloom_filter_clear);
    SUBSCRIBER_OVERLOAD(bloom_filter_load);
    SUBSCRIBER_OVERLOAD(client_filter);
    SUBSCRIBER_OVERLOAD(client_filter_checkpoint);
    SUBSCRIBER_OVERLOAD(client_filter_headers);
    SUBSCRIBER_OVERLOAD(compact_block);
    SUBSCRIBER_OVERLOAD(compact_transactions);
    SUBSCRIBER_OVERLOAD(fee_filter);
    SUBSCRIBER_OVERLOAD(get_address);
    SUBSCRIBER_OVERLOAD(get_blocks);
    SUBSCRIBER_OVERLOAD(get_client_filter_checkpoint);
    SUBSCRIBER_OVERLOAD(get_client_filter_headers);
    SUBSCRIBER_OVERLOAD(get_client_filters);
    SUBSCRIBER_OVERLOAD(get_compact_transactions);
    SUBSCRIBER_OVERLOAD(get_data);
    SUBSCRIBER_OVERLOAD(get_headers);
    SUBSCRIBER_OVERLOAD(headers);
    SUBSCRIBER_OVERLOAD(inventory);
    SUBSCRIBER_OVERLOAD(memory_pool);
    SUBSCRIBER_OVERLOAD(merkle_block);
    SUBSCRIBER_OVERLOAD(not_found);
    SUBSCRIBER_OVERLOAD(ping);
    SUBSCRIBER_OVERLOAD(pong);
    SUBSCRIBER_OVERLOAD(reject);
    SUBSCRIBER_OVERLOAD(send_compact);
    SUBSCRIBER_OVERLOAD(send_headers);
    SUBSCRIBER_OVERLOAD(transaction);
    SUBSCRIBER_OVERLOAD(version);
    SUBSCRIBER_OVERLOAD(version_acknowledge);

    // These are thread safe.
    ////address_subscriber address_subscriber_;
    DECLARE_SUBSCRIBER(address);
    DECLARE_SUBSCRIBER(alert);
    DECLARE_SUBSCRIBER(block);
    DECLARE_SUBSCRIBER(bloom_filter_add);
    DECLARE_SUBSCRIBER(bloom_filter_clear);
    DECLARE_SUBSCRIBER(bloom_filter_load);
    DECLARE_SUBSCRIBER(client_filter);
    DECLARE_SUBSCRIBER(client_filter_checkpoint);
    DECLARE_SUBSCRIBER(client_filter_headers);
    DECLARE_SUBSCRIBER(compact_block);
    DECLARE_SUBSCRIBER(compact_transactions);
    DECLARE_SUBSCRIBER(fee_filter);
    DECLARE_SUBSCRIBER(get_address);
    DECLARE_SUBSCRIBER(get_blocks);
    DECLARE_SUBSCRIBER(get_client_filter_checkpoint);
    DECLARE_SUBSCRIBER(get_client_filter_headers);
    DECLARE_SUBSCRIBER(get_client_filters);
    DECLARE_SUBSCRIBER(get_compact_transactions);
    DECLARE_SUBSCRIBER(get_data);
    DECLARE_SUBSCRIBER(get_headers);
    DECLARE_SUBSCRIBER(headers);
    DECLARE_SUBSCRIBER(inventory);
    DECLARE_SUBSCRIBER(memory_pool);
    DECLARE_SUBSCRIBER(merkle_block);
    DECLARE_SUBSCRIBER(not_found);
    DECLARE_SUBSCRIBER(ping);
    DECLARE_SUBSCRIBER(pong);
    DECLARE_SUBSCRIBER(reject);
    DECLARE_SUBSCRIBER(send_compact);
    DECLARE_SUBSCRIBER(send_headers);
    DECLARE_SUBSCRIBER(transaction);
    DECLARE_SUBSCRIBER(version);
    DECLARE_SUBSCRIBER(version_acknowledge);
};

#undef SUBSCRIBER
#undef SUBSCRIBER_TYPE
#undef DECLARE_SUBSCRIBER
#undef DEFINE_SUBSCRIBER
#undef SUBSCRIBER_OVERLOAD
#undef CASE_NOTIFY

} // namespace network
} // namespace libbitcoin

#endif
