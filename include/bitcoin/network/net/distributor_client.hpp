/////**
//// * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
//// *
//// * This file is part of libbitcoin.
//// *
//// * This program is free software: you can redistribute it and/or modify
//// * it under the terms of the GNU Affero General Public License as published by
//// * the Free Software Foundation, either version 3 of the License, or
//// * (at your option) any later version.
//// *
//// * This program is distributed in the hope that it will be useful,
//// * but WITHOUT ANY WARRANTY; without even the implied warranty of
//// * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//// * GNU Affero General Public License for more details.
//// *
//// * You should have received a copy of the GNU Affero General Public License
//// * along with this program.  If not, see <http://www.gnu.org/licenses/>.
//// */
////#ifndef LIBBITCOIN_NETWORK_NET_DISTRIBUTOR_CLIENT_HPP
////#define LIBBITCOIN_NETWORK_NET_DISTRIBUTOR_CLIENT_HPP
////
////#include <utility>
////////#include <bitcoin/network/async/async.hpp>
////#include <bitcoin/network/define.hpp>
////#include <bitcoin/network/messages/rpc/messages.hpp>
////
////namespace libbitcoin {
////namespace network {
////
////#define SUBSCRIBER(name) name##_subscriber_
////#define SUBSCRIBER_TYPE(name) name##_subscriber
////#define DECLARE_SUBSCRIBER(name) SUBSCRIBER_TYPE(name) SUBSCRIBER(name)
////#define DEFINE_SUBSCRIBER(name) using SUBSCRIBER_TYPE(name) = \
////    unsubscriber<const messages::rpc::name::cptr&>
////#define SUBSCRIBER_OVERLOAD(name) code do_subscribe( \
////    distributor_client::handler<messages::rpc::name>&& handler) NOEXCEPT \
////    { return SUBSCRIBER(name).subscribe( \
////      std::forward<distributor_client::handler<messages::rpc::name>>(handler)); }
////
/////// Not thread safe.
////class BCT_API distributor_client
////{
////public:
////    /// Helper for external declarations.
////    template <class Message>
////    using handler = std::function<bool(const code&,
////        const typename Message::cptr&)>;
////
////    DELETE_COPY_MOVE_DESTRUCT(distributor_client);
////
////    DEFINE_SUBSCRIBER(ping);
////
////    /// Create an instance of this class.
////    distributor_client(asio::strand& strand) NOEXCEPT;
////
////    /// If stopped, handler is invoked with error::subscriber_stopped.
////    /// If key exists, handler is invoked with error::subscriber_exists.
////    /// Otherwise handler retained. Subscription code is also returned here.
////    template <typename Handler>
////    code subscribe(Handler&& handler) NOEXCEPT
////    {
////        return do_subscribe(std::forward<Handler>(handler));
////    }
////
////    /// Relay a message instance to each subscriber of the type.
////    /// Returns error code if fails to deserialize, otherwise success.
////    virtual code notify(messages::rpc::identifier id,
////        const system::data_chunk& data) NOEXCEPT;
////
////    /// Stop all subscribers, prevents subsequent subscription (idempotent).
////    /// The subscriber is stopped regardless of the error code, however by
////    /// convention handlers rely on the error code to avoid message processing.
////    virtual void stop(const code& ec) NOEXCEPT;
////
////private:
////    // Deserialize a stream into a message instance and notify subscribers.
////    template <typename Message, typename Subscriber>
////    code do_notify(Subscriber& subscriber,
////        const system::data_chunk& data) NOEXCEPT
////    {
////        // Avoid deserialization if there are no subscribers for the type.
////        if (!subscriber.empty())
////        {
////            const auto ptr = messages::rpc::deserialize<Message>(data);
////            if (!ptr)
////                return error::invalid_message;
////
////            // Subscribers are notified only with stop code or error::success.
////            subscriber.notify(error::success, ptr);
////        }
////
////        return error::success;
////    }
////
////    SUBSCRIBER_OVERLOAD(ping);
////
////    // These are thread safe.
////    DECLARE_SUBSCRIBER(ping);
////};
////
////#undef SUBSCRIBER
////#undef SUBSCRIBER_TYPE
////#undef DEFINE_SUBSCRIBER
////#undef SUBSCRIBER_OVERLOAD
////#undef DECLARE_SUBSCRIBER
////
////} // namespace network
////} // namespace libbitcoin
////
////#endif
