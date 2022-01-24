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

#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

/// This class is thread safe.
/// The hosts class manages a thread-safe dynamic store of network addresses.
/// The store can be loaded and saved from/to the specified file path.
/// The file is a line-oriented set of config::authority serializations.
/// Duplicate addresses and those with zero-valued ports are disacarded.
class BCT_API hosts
  : system::noncopyable
{
public:
    typedef std::shared_ptr<hosts> ptr;
    typedef std::function<void(const code&)> result_handler;

    /// Construct an instance.
    hosts(const settings& settings);

    /// Load hosts file if found.
    virtual code start();

    // Save hosts to file.
    virtual code stop();

    virtual size_t count() const;
    virtual code fetch(messages::address_item& out) const;
    virtual code fetch(messages::address_items& out) const;
    virtual code remove(const messages::address_item& host);
    virtual code store(const messages::address_item& host);
    virtual void store(const messages::address_items& hosts,
        result_handler handler);

private:
    typedef boost::circular_buffer<messages::address_item> buffer;
    typedef buffer::iterator iterator;

    iterator find(const messages::address_item& host);

    const size_t capacity_;

    // These are protected by a mutex.
    buffer buffer_;
    std::atomic<bool> stopped_;
    mutable upgrade_mutex mutex_;

    // HACK: we use this because the buffer capacity cannot be set to zero.
    const bool disabled_;
    const boost::filesystem::path file_path_;
};

} // namespace network
} // namespace libbitcoin

#endif

