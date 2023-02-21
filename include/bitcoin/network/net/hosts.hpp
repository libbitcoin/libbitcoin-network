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
#include <boost/circular_buffer.hpp>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
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
    hosts(const settings& settings) NOEXCEPT;

    /// Load hosts file.
    virtual code start() NOEXCEPT;

    /// Save hosts to file.
    virtual code stop() NOEXCEPT;

    /// Thread safe, inexact (ok).
    virtual size_t count() const NOEXCEPT;

    /// Store the host in the table (e.g. after use), false if invalid.
    virtual bool restore(const messages::address_item& host) NOEXCEPT;

    /// Take one random host from the table (non-const).
    virtual void take(const address_item_handler& handler) NOEXCEPT;

    /// Save random subset of hosts (e.g obtained from peer), count of accept.
    virtual size_t save(const messages::address_items& hosts) NOEXCEPT;

    /// Obtain a random set of hosts (e.g for relay to peer).
    virtual void fetch(const address_handler& handler) const NOEXCEPT;

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
    inline bool exists(const messages::address_item& host) NOEXCEPT
    {
        BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
        return find(host) != buffer_.end();
        BC_POP_WARNING()
    }

    // Push a buffer entry if the line is valid.
    void push_valid(const std::string& line) NOEXCEPT;

    // These are thread safe.
    const std::filesystem::path file_path_;
    std::atomic<size_t> count_{};
    const size_t minimum_;
    const size_t maximum_;
    const size_t capacity_;

    // These are not thread safe.
    bool disabled_;
    buffer buffer_;
};

} // namespace network
} // namespace libbitcoin

#endif
