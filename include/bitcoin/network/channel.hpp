/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_NETWORK_CHANNEL_HPP
#define LIBBITCOIN_NETWORK_CHANNEL_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <string>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/message_subscriber.hpp>
#include <bitcoin/network/proxy.hpp>
#include <bitcoin/network/settings.hpp>

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

    // Properties.

    virtual bool notify() const;
    virtual void set_notify(bool value);

    virtual uint64_t nonce() const;
    virtual void set_nonce(uint64_t value);

    virtual version_const_ptr peer_version() const;
    virtual void set_peer_version(version_const_ptr value);

protected:
    virtual void signal_activity() override;
    virtual void handle_stopping() override;
    virtual bool stopped(const code& ec) const;

    // Expose polymorphic stopped method from base.
    using proxy::stopped;

private:
    void do_start(const code& ec, result_handler handler);

    void start_expiration();
    void handle_expiration(const code& ec);

    void start_inactivity();
    void handle_inactivity(const code& ec);

    std::atomic<bool> notify_;
    std::atomic<uint64_t> nonce_;
    bc::atomic<version_const_ptr> peer_version_;
    deadline::ptr expiration_;
    deadline::ptr inactivity_;
};

} // namespace network
} // namespace libbitcoin

#endif
