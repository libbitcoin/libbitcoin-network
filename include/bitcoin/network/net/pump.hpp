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
#ifndef LIBBITCOIN_NETWORK_NET_PUMP_HPP
#define LIBBITCOIN_NETWORK_NET_PUMP_HPP

#include <functional>
#include <memory>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

#define SUBSCRIBER(name) name##_subscriber_
#define SUBSCRIBER_TYPE(name) name##_subscriber

#define DEFINE_SUBSCRIBER(name) \
    typedef subscriber<asio::strand, code, system::messages::name::const_ptr> \
        SUBSCRIBER_TYPE(name)

#define SUBSCRIBER_OVERLOAD(name) \
    bool do_subscribe(pump::handler<system::messages::name>&& handler) const \
{ \
    return SUBSCRIBER(name)->subscribe(std::move(handler)); \
}

#define DECLARE_SUBSCRIBER(name) \
    SUBSCRIBER_TYPE(name)::ptr SUBSCRIBER(name)

// TODO: type-constrain templates to a Message to base class, when there is one.

/// Thread safe aggregation of subscribers by message type.
/// Stop is thread safe and idempotent, may be called multiple times.
/// All handlers are posted to the strand provided upon construct.
class BCT_API pump
  : system::noncopyable
{
public:
    /// Helper for external declarations.
    template <class Message>
    using handler = std::function<bool(const code&,
        std::shared_ptr<const Message>)>;

    ////typedef subscriber<asio::strand, code, system::messages::address::const_ptr>
    ////    address_subscriber;
    DEFINE_SUBSCRIBER(address);
    DEFINE_SUBSCRIBER(alert);
    DEFINE_SUBSCRIBER(block);
    DEFINE_SUBSCRIBER(block_transactions);
    DEFINE_SUBSCRIBER(compact_block);
    DEFINE_SUBSCRIBER(compact_filter);
    DEFINE_SUBSCRIBER(compact_filter_checkpoint);
    DEFINE_SUBSCRIBER(compact_filter_headers);
    DEFINE_SUBSCRIBER(fee_filter);
    DEFINE_SUBSCRIBER(filter_add);
    DEFINE_SUBSCRIBER(filter_clear);
    DEFINE_SUBSCRIBER(filter_load);
    DEFINE_SUBSCRIBER(get_address);
    DEFINE_SUBSCRIBER(get_blocks);
    DEFINE_SUBSCRIBER(get_block_transactions);
    DEFINE_SUBSCRIBER(get_compact_filter_checkpoint);
    DEFINE_SUBSCRIBER(get_compact_filter_headers);
    DEFINE_SUBSCRIBER(get_compact_filters);
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
    DEFINE_SUBSCRIBER(verack);
    DEFINE_SUBSCRIBER(version);

    /// Create an instance of this class.
    pump(asio::strand& strand);

    /// Subscribe to receive a notification when a message of type is received.
    /// Subscription handler is retained in the queue until stop.
    template <typename Handler>
    bool subscribe(Handler&& handler)
    {
        return do_subscribe(std::forward<Handler>(handler));
    }

    /// Relay a message instance to each subscriber of the type.
    /// Returns bad_stream if message fails to deserialize, otherwise success.
    virtual code notify(system::messages::identifier id, uint32_t version,
        system::reader& reader) const;

    /// Stop all subscribers, prevents subsequent subscription (idempotent).
    /// The subscriber is stopped regardless of the error code, however by
    /// convention handlers rely on the error code to avoid message processing.
    virtual void stop(const code& ec);

private:
    // Deserialize a stream into a message instance and notify subscribers.
    template <typename Message, typename Subscriber>
    code do_notify(Subscriber& subscriber, uint32_t version,
        system::reader& reader) const
    {
        // TODO: Implement deferred deserialization in messages.
        // TODO: Store buffer and use for hash compute and reserialization.
        // TODO: Give option to purge buffer on deserialization or explicitly.
        // TODO: This accelerates hash compute and allows for fast reject of
        // TODO: messages that we already have (by hash), and to retain the
        // TODO: buffer for relaying to other peers, etc. The hash is a store
        // TODO: key, so we just read it onto the object. The only time we need
        // TODO: to serialize is to send a message over the wire, and only need
        // TODO: to hash for identity when we receive over the wire. See also
        // TODO: comments in proxy::send.

        const auto message = std::make_shared<Message>();
        if (!message->from_data(version, reader))
            return system::error::bad_stream;

        // Subscribers are notified only with stop and success codes.
        subscriber->notify(system::error::success, message);
        return system::error::success;
    }

    ////bool do_subscribe(pump::handler<system::messages::address>&& handler) const
    ////{
    ////    return address_subscriber_->subscribe(std::move(handler));
    ////}
    SUBSCRIBER_OVERLOAD(address);
    SUBSCRIBER_OVERLOAD(alert);
    SUBSCRIBER_OVERLOAD(block);
    SUBSCRIBER_OVERLOAD(block_transactions);
    SUBSCRIBER_OVERLOAD(compact_block);
    SUBSCRIBER_OVERLOAD(compact_filter);
    SUBSCRIBER_OVERLOAD(compact_filter_checkpoint);
    SUBSCRIBER_OVERLOAD(compact_filter_headers);
    SUBSCRIBER_OVERLOAD(fee_filter);
    SUBSCRIBER_OVERLOAD(filter_add);
    SUBSCRIBER_OVERLOAD(filter_clear);
    SUBSCRIBER_OVERLOAD(filter_load);
    SUBSCRIBER_OVERLOAD(get_address);
    SUBSCRIBER_OVERLOAD(get_blocks);
    SUBSCRIBER_OVERLOAD(get_block_transactions);
    SUBSCRIBER_OVERLOAD(get_compact_filter_checkpoint);
    SUBSCRIBER_OVERLOAD(get_compact_filter_headers);
    SUBSCRIBER_OVERLOAD(get_compact_filters);
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
    SUBSCRIBER_OVERLOAD(verack);
    SUBSCRIBER_OVERLOAD(version);

    // These are thread safe.
    /////address_subscriber::ptr address_subscriber_;
    DECLARE_SUBSCRIBER(address);
    DECLARE_SUBSCRIBER(alert);
    DECLARE_SUBSCRIBER(block);
    DECLARE_SUBSCRIBER(block_transactions);
    DECLARE_SUBSCRIBER(compact_block);
    DECLARE_SUBSCRIBER(compact_filter);
    DECLARE_SUBSCRIBER(compact_filter_checkpoint);
    DECLARE_SUBSCRIBER(compact_filter_headers);
    DECLARE_SUBSCRIBER(fee_filter);
    DECLARE_SUBSCRIBER(filter_add);
    DECLARE_SUBSCRIBER(filter_clear);
    DECLARE_SUBSCRIBER(filter_load);
    DECLARE_SUBSCRIBER(get_address);
    DECLARE_SUBSCRIBER(get_blocks);
    DECLARE_SUBSCRIBER(get_block_transactions);
    DECLARE_SUBSCRIBER(get_compact_filter_checkpoint);
    DECLARE_SUBSCRIBER(get_compact_filter_headers);
    DECLARE_SUBSCRIBER(get_compact_filters);
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
    DECLARE_SUBSCRIBER(verack);
    DECLARE_SUBSCRIBER(version);

    // This is thread safe.
    asio::strand& strand_;
};

#undef SUBSCRIBER
#undef SUBSCRIBER_TYPE
#undef DEFINE_SUBSCRIBER
#undef SUBSCRIBER_OVERLOAD
#undef DECLARE_SUBSCRIBER

} // namespace network
} // namespace libbitcoin

#endif
