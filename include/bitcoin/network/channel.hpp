/**
 * Copyright (c) 2011-2018 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBBITCOIN_NETWORK_CHANNEL_HPP
#define LIBBITCOIN_NETWORK_CHANNEL_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <string>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/network/const_buffer.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/proxy.hpp>
#include <bitcoin/network/message_subscriber.hpp>
#include <bitcoin/network/settings.hpp>
#include <bitcoin/network/socket.hpp>

namespace libbitcoin {
namespace network {

/// A concrete proxy with timers and state, mostly thread safe.
class BCT_API channel
  : public proxy, track<channel>
{
public:
    typedef std::shared_ptr<channel> ptr;

    /// Construct an instance.
    channel(threadpool& pool, socket::ptr socket, const settings& settings);

    void start(result_handler handler) override;

    virtual bool notify() const;
    virtual void set_notify(bool value);

    virtual uint64_t nonce() const;
    virtual void set_nonce(uint64_t value);

    virtual const message::version& version() const;
    virtual void set_version(const message::version& value);

    virtual hash_digest own_threshold();
    virtual void set_own_threshold(const hash_digest& threshold);

    virtual hash_digest peer_threshold();
    virtual void set_peer_threshold(const hash_digest& threshold);

    virtual void reset_poll();
    virtual void set_poll_handler(result_handler handler);

    virtual bool located(const hash_digest& start,
        const hash_digest& stop) const;
    virtual void set_located(const hash_digest& start,
        const hash_digest& stop);

protected:
    virtual void handle_activity();
    virtual void handle_stopping();

private:
    void do_start(const code& ec, result_handler handler);

    void start_expiration();
    void handle_expiration(const code& ec);

    void start_inactivity();
    void handle_inactivity(const code& ec);

    void start_poll();
    void handle_poll(const code& ec);

    bool notify_;
    uint64_t nonce_;
    hash_digest located_start_;
    hash_digest located_stop_;
    message::version version_;
    deadline::ptr expiration_;
    deadline::ptr inactivity_;
    deadline::ptr poll_;
    bc::atomic<hash_digest> own_threshold_;
    bc::atomic<hash_digest> peer_threshold_;

    // Deperecated.
    bc::atomic<result_handler> poll_handler_;
};

} // namespace network
} // namespace libbitcoin

#endif
