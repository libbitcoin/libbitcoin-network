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

#include <cstdint>
#include <functional>
#include <memory>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {

#define SUBSCRIBER(name) name##_subscriber_
#define SUBSCRIBER_TYPE(name) name##_subscriber

#define DEFINE_SUBSCRIBER(name) \
    typedef subscriber<const code&, const messages::name::ptr&> \
        SUBSCRIBER_TYPE(name)

#define SUBSCRIBER_OVERLOAD(name) \
void do_subscribe(pump::handler<messages::name>&& handler) const noexcept \
{ \
    SUBSCRIBER(name)->subscribe(std::move(handler)); \
}

#define DECLARE_SUBSCRIBER(name) \
    SUBSCRIBER_TYPE(name)::ptr SUBSCRIBER(name)

/// Not thread safe.
/// All handlers are posted to the strand.
class BCT_API pump
  : system::noncopyable
{
public:
    /// Helper for external declarations.
    template <class Message>
    using handler = std::function<void(const code&,
        const std::shared_ptr<const Message>&)>;

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
    pump(asio::strand& strand) noexcept;

    /// Subscription handlers are retained in the queue until stop.
    /// No invocation occurs if the subscriber is stopped at time of subscribe.
    template <typename Handler>
    void subscribe(Handler&& handler) noexcept
    {
        do_subscribe(std::forward<Handler>(handler));
    }

    /// Relay a message instance to each subscriber of the type.
    /// Returns error code if fails to deserialize, otherwise success.
    virtual code notify(messages::identifier id, uint32_t version,
        system::reader& source) const noexcept;

    /// Stop all subscribers, prevents subsequent subscription (idempotent).
    /// The subscriber is stopped regardless of the error code, however by
    /// convention handlers rely on the error code to avoid message processing.
    virtual void stop(const code& ec) noexcept;

private:
    // TODO: change integer version() to active() structure.
    // Deserialize a stream into a message instance and notify subscribers.
    template <typename Message, typename Subscriber>
    code do_notify(Subscriber& subscriber, uint32_t version,
        system::reader& source) const noexcept
    {
        // TODO: account for witness parameter in active() structure.
        const auto message = messages::deserialize<Message>(source, version);

        if (!source)
            return error::invalid_message;

        // Subscribers are notified only with stop code or error::success.
        subscriber->notify(error::success, message);
        return error::success;
    }

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
#undef DEFINE_SUBSCRIBER
#undef SUBSCRIBER_OVERLOAD
#undef DECLARE_SUBSCRIBER

} // namespace network
} // namespace libbitcoin

#endif
