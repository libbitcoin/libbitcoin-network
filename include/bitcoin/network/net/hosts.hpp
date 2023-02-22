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
#ifndef LIBBITCOIN_NETWORK_NET_HOSTS_HPP
#define LIBBITCOIN_NETWORK_NET_HOSTS_HPP

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <functional>
#include <memory>
#include <unordered_set>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/boost.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

/// Config wrappers are used for serialization/deserialization.
/// Hosts passes message and message item pointers, not config items.
typedef std::function<void(const code&, const address_cptr&)> address_handler;
typedef std::function<void(const code&, const address_item_cptr&)>
    address_item_handler;

/// Virtual, not thread safe.
/// Duplicate and invalid addresses are disacarded.
/// The file is loaded and saved from/to the settings-specified path.
/// The file is a line-oriented textual serialization (config::authority+).
class BCT_API hosts
{
public:
    DELETE_COPY_MOVE_DESTRUCT(hosts);

    /// Construct an instance.
    hosts(threadpool& pool, const settings& settings) NOEXCEPT;

    /// Start/stop.
    /// -----------------------------------------------------------------------
    /// Not thread safe.

    /// Load addresses from file.
    virtual code start() NOEXCEPT;

    /// Save addresses to file.
    virtual code stop() NOEXCEPT;

    /// Properties.
    /// -----------------------------------------------------------------------
    /// Thread safe.

    /// Count of pooled addresses, thread safe.
    virtual size_t count() const NOEXCEPT;

    /// The count of reserved addresses (currently connected), thread safe.
    virtual size_t reserved() const NOEXCEPT;

    /// Usage.
    /// -----------------------------------------------------------------------
    /// Thread safe.

    /// Take one random address from the table (non-const).
    virtual void take(address_item_handler&& handler) NOEXCEPT;

    /// Store the address in the table (after use).
    virtual void restore(const address_item_cptr& host,
        result_handler&& handler) NOEXCEPT;

    /// Negotiation.
    /// -----------------------------------------------------------------------
    /// Thread safe.

    /// Obtain a random set of addresses (for relay to peer).
    virtual void fetch(address_handler&& handler) const NOEXCEPT;

    /// Save random subset of addresses (from peer), count of accept.
    virtual void save(const address_cptr& message,
        count_handler&& handler) NOEXCEPT;

    /// Reservation.
    /// -----------------------------------------------------------------------
    /// Not thread safe.

    /// True if the address is reserved (currently connected).
    virtual bool is_reserved(const config::authority& host) const NOEXCEPT;

    /// Reserve the address (currently connected), false if was reserved.
    virtual bool reserve(const config::authority& host) NOEXCEPT;

    /// Unreserve the address (no longer connected), false if was not reserved.
    virtual bool unreserve(const config::authority& host) NOEXCEPT;

private:
    typedef boost::circular_buffer<messages::address_item> buffer;

    // Equality ignores timestamp and services.
    inline buffer::iterator find(const messages::address_item& host) NOEXCEPT
    {
        BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
        return std::find(buffer_.begin(), buffer_.end(), host);
        BC_POP_WARNING()
    }

    // Equality ignores timestamp and services.
    inline bool is_pooled(const messages::address_item& host) NOEXCEPT
    {
        BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
        return find(host) != buffer_.end();
        BC_POP_WARNING()
    }

    inline messages::address_item::cptr pop() NOEXCEPT;
    inline void push(const std::string& line) NOEXCEPT;

    void do_take(const address_item_handler& handler) NOEXCEPT;
    void do_restore(const address_item_cptr& host,
        const result_handler& handler) NOEXCEPT;
    void do_fetch(const address_handler& handler) const NOEXCEPT;
    void do_save(const address_cptr& message,
        const count_handler& handler) NOEXCEPT;

    // These are thread safe.
    std::atomic<size_t> count_{};
    const settings& settings_;
    const size_t minimum_;
    const size_t maximum_;
    const size_t capacity_;
    asio::strand strand_;

    // These are not thread safe.
    bool disabled_;
    buffer buffer_;
    bool stopped_{ true };
    std::atomic<size_t> authorities_count_{};
    std::unordered_set<config::authority> authorities_{};
};

} // namespace network
} // namespace libbitcoin

#endif
