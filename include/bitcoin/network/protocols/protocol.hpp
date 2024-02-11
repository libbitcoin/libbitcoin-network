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
#ifndef LIBBITCOIN_NETWORK_PROTOCOL_HPP
#define LIBBITCOIN_NETWORK_PROTOCOL_HPP

#include <functional>
#include <memory>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/sessions/sessions.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

#define BOUND_PROTOCOL(method, args) \
    std::bind(std::forward<Method>(method), shared_from_base<Protocol>(), \
        std::forward<Args>(args)...)

/// This class is thread safe, except for:
/// * start/started must be called on strand.
/// * setters should only be invoked during handshake.
/// Abstract base class for protocols.
/// handle_ methods are always invoked on the strand.
class BCT_API protocol
  : public enable_shared_from_base<protocol>, public reporter
{
public:
    DELETE_COPY_MOVE(protocol);

    /// The channel is stopping (called on strand by stop subscription).
    /// This must be called only from the channel strand (not thread safe).
    virtual void stopping(const code& ec) NOEXCEPT;

private:
    template <class Message, typename Handler>
    bool handle_broadcast(const code& ec, const typename Message::cptr& message,
        uint64_t sender, const Handler& handler) NOEXCEPT
    {
        if (stopped(ec))
            return false;

        // Invoke subscriber on channel strand with given parameters.
        boost::asio::post(channel_->strand(),
            std::bind(handler, ec, message, sender));

        return true;
    }

protected:
    /// Messaging.
    /// -----------------------------------------------------------------------

    /// Bind a method in base or derived class (use BIND#).
    template <class Protocol, typename Method, typename... Args>
    auto bind(Method&& method, Args&&... args) NOEXCEPT
    {
        return BOUND_PROTOCOL(method, args);
    }

    /// Post a method in base or derived class to channel strand (use POST#).
    template <class Protocol, typename Method, typename... Args>
    auto post(Method&& method, Args&&... args) NOEXCEPT
    {
        return boost::asio::post(channel_->strand(),
            BOUND_PROTOCOL(method, args));
    }

    /// Send a message instance to peer (use SEND#).
    template <class Protocol, class Message, typename Method, typename... Args>
    void send(const Message& message, Method&& method, Args&&... args) NOEXCEPT
    {
        BC_ASSERT_MSG(stranded(), "strand");
        channel_->send<Message>(message, BOUND_PROTOCOL(method, args));
    }

    /// Subscribe to channel messages by type (use SUBSCRIBE_CHANNEL#).
    /// Method is invoked with error::subscriber_stopped if already stopped.
    template <class Protocol, class Message, typename Method, typename... Args>
    void subscribe_channel(Method&& method, Args&&... args) NOEXCEPT
    {
        BC_ASSERT_MSG(stranded(), "strand");
        channel_->subscribe<Message>(BOUND_PROTOCOL(method, args));
    }

    /// Subscribe to messages broadcasts by type (use SUBSCRIBE_BROADCAST#).
    /// Method is invoked with error::subscriber_stopped if already stopped.
    template <class Protocol, class Message, typename Method, typename... Args>
    void subscribe_broadcast(Method&& method, Args&&... args) NOEXCEPT
    {
        BC_ASSERT_MSG(stranded(), "strand");

        // handler is a bool function, causes problem with std::bind.
        const auto bouncer =
        [self = shared_from_this(), handler = BOUND_PROTOCOL(method, args)]
        (const auto& ec, const typename Message::cptr& message, auto id)
        {
            return self->handle_broadcast<Message>(ec, message, id, handler);
        };

        session_.subscribe<Message>(bouncer, channel_->identifier());
    }

    /// Broadcast a message instance to peers (use BROADCAST).
    template <class Message>
    void broadcast(const typename Message::cptr& message) NOEXCEPT
    {
        BC_ASSERT_MSG(stranded(), "strand");
        session_.broadcast<Message>(message, channel_->identifier());
    }

    /// Start/Stop.
    /// -----------------------------------------------------------------------

    /// Construct an instance.
    protocol(session& session, const channel::ptr& channel) NOEXCEPT;

    /// Asserts that protocol is stopped.
    virtual ~protocol() NOEXCEPT;

    /// Set protocol started state (strand required).
    virtual void start() NOEXCEPT;

    /// Get protocol started state (strand required).
    virtual bool started() const NOEXCEPT;

    /// Channel is stopped or code set.
    virtual bool stopped(const code& ec=error::success) const NOEXCEPT;

    /// Stop the channel.
    virtual void stop(const code& ec) NOEXCEPT;

    /// Pause the channel (strand required).
    virtual void pause() NOEXCEPT;

    /// Properties.
    /// -----------------------------------------------------------------------

    /// The current thread is on the channel strand.
    virtual bool stranded() const NOEXCEPT;

    /// The authority of the peer.
    virtual config::authority authority() const NOEXCEPT;

    /// The outbound address of the peer.
    virtual const config::address& outbound() const NOEXCEPT;

    /// The nonce of the channel.
    virtual uint64_t nonce() const NOEXCEPT;

    /// The start height (for version message).
    virtual size_t start_height() const NOEXCEPT;

    /// The protocol version of the peer.
    virtual messages::version::cptr peer_version() const NOEXCEPT;

    /// Set protocol version of the peer (set only during handshake).
    virtual void set_peer_version(const messages::version::cptr& value) NOEXCEPT;

    /// The negotiated protocol version.
    virtual uint32_t negotiated_version() const NOEXCEPT;

    /// Set negotiated protocol version (set only during handshake).
    virtual void set_negotiated_version(uint32_t value) NOEXCEPT;

    /// Network settings.
    virtual const network::settings& settings() const NOEXCEPT;

    /// Advertised addresses with own services and current timestamp.
    virtual messages::address selfs() const NOEXCEPT;

    /// Channel identifier (for broadcast identification).
    virtual uint64_t identifier() const NOEXCEPT;

    /// Addresses.
    /// -----------------------------------------------------------------------

    /// Number of entries in the address pool.
    virtual size_t address_count() const NOEXCEPT;

    /// Fetch a set of peer addresses from the address pool.
    virtual void fetch(address_handler&& handler) NOEXCEPT;

    /// Save a set of peer addresses to the address pool.
    virtual void save(const address_cptr& message,
        count_handler&& handler) NOEXCEPT;

    /// Capture send results, use for no-op send handling (logged).
    virtual void handle_send(const code& ec) NOEXCEPT;

private:
    void handle_fetch(const code& ec, const address_cptr& message,
        const address_handler& handler) NOEXCEPT;
    void handle_save(const code& ec, size_t accepted,
        const count_handler& handler) NOEXCEPT;

    // This is mostly thread safe, and used in a thread safe manner.
    // pause/resume/paused/attach not invoked, setters limited to handshake.
    channel::ptr channel_;

    // This is thread safe.
    session& session_;

    // This is protected by strand.
    bool started_{};
};

#undef BOUND_PROTOCOL

// See define.hpp for BIND# macros.

#define POST1(method, p1) \
    post<CLASS>(&CLASS::method, p1)
#define POST2(method, p1, p2) \
    post<CLASS>(&CLASS::method, p1, p2)
#define POST3(method, p1, p2, p3) \
    post<CLASS>(&CLASS::method, p1, p2, p3)

#define SEND1(message, method, p1) \
    send<CLASS>(message, &CLASS::method, p1)
#define SEND2(message, method, p1, p2) \
    send<CLASS>(message, &CLASS::method, p1, p2)
#define SEND3(message, method, p1, p2, p3) \
    send<CLASS>(message, &CLASS::method, p1, p2, p3)

#define SUBSCRIBE_CHANNEL1(message, method, p1) \
    subscribe_channel<CLASS, message>(&CLASS::method, p1)
#define SUBSCRIBE_CHANNEL2(message, method, p1, p2) \
    subscribe_channel<CLASS, message>(&CLASS::method, p1, p2)
#define SUBSCRIBE_CHANNEL3(message, method, p1, p2, p3) \
    subscribe_channel<CLASS, message>(&CLASS::method, p1, p2, p3)
#define SUBSCRIBE_CHANNEL4(message, method, p1, p2, p3, p4) \
    subscribe_channel<CLASS, message>(&CLASS::method, p1, p2, p3, p4)

////#define SUBSCRIBE_BROADCAST1(message, method, p1) \
////    subscribe_broadcast<CLASS, message>(&CLASS::method, p1)
////#define SUBSCRIBE_BROADCAST2(message, method, p1, p2) \
////    subscribe_broadcast<CLASS, message>(&CLASS::method, p1, p2)
#define SUBSCRIBE_BROADCAST3(message, method, p1, p2, p3) \
    subscribe_broadcast<CLASS, message>(&CLASS::method, p1, p2, p3)
#define SUBSCRIBE_BROADCAST4(message, method, p1, p2, p3, p4) \
    subscribe_broadcast<CLASS, message>(&CLASS::method, p1, p2, p3, p4)

#define BROADCAST(message, ptr) broadcast<message>(ptr)

} // namespace network
} // namespace libbitcoin

#endif
