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
#ifndef LIBBITCOIN_NETWORK_SESSION_HPP
#define LIBBITCOIN_NETWORK_SESSION_HPP

#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <vector>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/error.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

#define SESSION_ARGS(handler, args) \
    std::forward<Handler>(handler), \
    shared_from_base<Session>(), \
    std::forward<Args>(args)...
#define BOUND_SESSION(handler, args) \
    std::bind(SESSION_ARGS(handler, args))

#define SESSION_ARGS_TYPE(handler, args) \
    std::forward<Handler>(handler), \
    std::shared_ptr<Session>(), \
    std::forward<Args>(args)...
#define BOUND_SESSION_TYPE(handler, args) \
    std::bind(SESSION_ARGS_TYPE(handler, args))

class p2p;

/// Virtual base class for maintaining the lifetime of a channel set, thread safe.
class BCT_API session
  : public enable_shared_from_base<session>, system::noncopyable
{
public:
    typedef std::function<void(const code&)> result_handler;
    typedef std::function<void(const code&, channel::ptr)> channel_handler;

    virtual void start(result_handler handler);
    virtual void stop();

    const network::settings& settings() const;

protected:
    typedef subscriber<code> stop_subscriber;

    session(p2p& network);
    virtual ~session();

    /// Bind a method in the derived class.
    template <class Session, typename Handler, typename... Args>
    auto bind(Handler&& handler, Args&&... args) ->
        decltype(BOUND_SESSION_TYPE(handler, args)) const
    {
        return BOUND_SESSION(handler, args);
    }

    // Channel sequence.

    // Call from base to initialize a new channel.
    virtual void start_channel(channel::ptr channel,
        result_handler handle_started, result_handler handle_stopped);

    // Override to change default protocol attachments.
    virtual void attach_handshake(const channel::ptr& channel,
        result_handler handler) const;
    virtual void attach_protocols(const channel::ptr& channel) const;

    // Factories.
    acceptor::ptr create_acceptor();
    connector::ptr create_connector();
    connectors_ptr create_connectors(size_t count);

    // Properties.
    bool stopped() const noexcept;
    bool stranded() const;
    size_t address_count() const;
    size_t channel_count() const;
    size_t inbound_channel_count() const;
    bool blacklisted(const config::authority& authority) const;
    virtual bool inbound() const noexcept;
    virtual bool notify() const noexcept;

    // Utilities.
    friend class protocol;
    void fetch(hosts::address_item_handler handler) const;
    void fetches(hosts::address_items_handler handler) const;
    void save(const messages::address_item& address,
        result_handler complete) const;
    void saves(const messages::address_items& addresses,
        result_handler complete) const;

    // These are not thread safe.
    deadline::ptr timer_;
    stop_subscriber::ptr stop_subscriber_;

private:
    void handle_channel_start(const code& ec, channel::ptr channel,
        result_handler started, result_handler stopped);

    void handle_handshake(const code& ec, channel::ptr channel,
        result_handler handler);
    void handle_channel_started(const code& ec, channel::ptr channel,
        result_handler handler);
    void handle_channel_stopped(const code& ec, channel::ptr channel,
        result_handler handler);

    void do_attach_handshake(const channel::ptr& channel,
        result_handler handshake) const;
    void do_handle_handshake(const code& ec, channel::ptr channel,
        result_handler start);
    void do_attach_protocols(const channel::ptr& channel) const;
    void do_handle_channel_started(const code& ec, channel::ptr channel,
        result_handler started);
    void do_handle_channel_stopped(const code& ec, channel::ptr channel,
        result_handler stopped);

    // These are thread safe.
    std::atomic<bool> stopped_;
    p2p& network_;

    // This is not thread safe.
    std::vector<connector::ptr> connectors_;
};

#undef SESSION_ARGS
#undef BOUND_SESSION
#undef SESSION_ARGS_TYPE
#undef BOUND_SESSION_TYPE

} // namespace network
} // namespace libbitcoin

#endif
