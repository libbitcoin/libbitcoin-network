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
#ifndef LIBBITCOIN_NETWORK_NET_DISTRIBUTOR_CLIENT_HPP
#define LIBBITCOIN_NETWORK_NET_DISTRIBUTOR_CLIENT_HPP

#include <utility>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/client/messages.hpp>

namespace libbitcoin {
namespace network {

#define SUBSCRIBER(name) name##_subscriber_
#define SUBSCRIBER_TYPE(name) name##_subscriber
#define DECLARE_SUBSCRIBER(name) SUBSCRIBER_TYPE(name) SUBSCRIBER(name)
#define HANDLER(name) distributor_client::handler<http::method::name>
#define DEFINE_SUBSCRIBER(name) \
    using SUBSCRIBER_TYPE(name) = subscriber<const http::method::name&>
#define SUBSCRIBER_OVERLOAD(name) \
    code do_subscribe(HANDLER(name)&& handler) NOEXCEPT \
    { return SUBSCRIBER(name).subscribe(std::forward<HANDLER(name)>(handler)); }

/// Not thread safe.
class BCT_API distributor_client
{
public:
    /// Helper for external declarations.
    template <class Method>
    using handler = std::function<void(const code&, const Method&)>;

    DELETE_COPY_MOVE_DESTRUCT(distributor_client);

    DEFINE_SUBSCRIBER(get);
    DEFINE_SUBSCRIBER(head);
    DEFINE_SUBSCRIBER(post);
    DEFINE_SUBSCRIBER(put);
    DEFINE_SUBSCRIBER(delete_);
    DEFINE_SUBSCRIBER(trace);
    DEFINE_SUBSCRIBER(options);
    DEFINE_SUBSCRIBER(connect);
    DEFINE_SUBSCRIBER(unknown);

    /// Create an instance of this class.
    distributor_client(asio::strand& strand) NOEXCEPT;

    /// If stopped, handler is invoked with error::subscriber_stopped.
    /// If key exists, handler is invoked with error::subscriber_exists.
    /// Otherwise handler retained. Subscription code is also returned here.
    template <typename Handler>
    code subscribe(Handler&& handler) NOEXCEPT
    {
        return do_subscribe(std::forward<Handler>(handler));
    }

    /// Relay a message instance to each subscriber of the request method.
    virtual void notify(
        const http::string_request_cptr& request) const NOEXCEPT;

    /// Stop all subscribers, prevents subsequent subscription (idempotent).
    /// The subscriber is stopped regardless of the error code, however by
    /// convention handlers rely on the error code to avoid message processing.
    virtual void stop(const code& ec) NOEXCEPT;

private:
    // Deserialize a stream into a message instance and notify subscribers.
    template <typename Subscriber, class Method>
    void do_notify(const code& ec, Subscriber& subscriber,
        const Method& method) const NOEXCEPT
    {
        BC_ASSERT_MSG(ec || method, "success with null request");
        subscriber.notify(ec, method);
    }

    SUBSCRIBER_OVERLOAD(get);
    SUBSCRIBER_OVERLOAD(head);
    SUBSCRIBER_OVERLOAD(post);
    SUBSCRIBER_OVERLOAD(put);
    SUBSCRIBER_OVERLOAD(delete_);
    SUBSCRIBER_OVERLOAD(trace);
    SUBSCRIBER_OVERLOAD(options);
    SUBSCRIBER_OVERLOAD(connect);
    SUBSCRIBER_OVERLOAD(unknown);

    // These are thread safe.
    DECLARE_SUBSCRIBER(get);
    DECLARE_SUBSCRIBER(head);
    DECLARE_SUBSCRIBER(post);
    DECLARE_SUBSCRIBER(put);
    DECLARE_SUBSCRIBER(delete_);
    DECLARE_SUBSCRIBER(trace);
    DECLARE_SUBSCRIBER(options);
    DECLARE_SUBSCRIBER(connect);
    DECLARE_SUBSCRIBER(unknown);
};

#undef SUBSCRIBER
#undef SUBSCRIBER_TYPE
#undef DECLARE_SUBSCRIBER
#undef HANDLER
#undef DEFINE_SUBSCRIBER
#undef SUBSCRIBER_OVERLOAD

} // namespace network
} // namespace libbitcoin

#endif
