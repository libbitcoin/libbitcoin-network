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
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <boost/circular_buffer.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

/// Virtual, not thread safe.
/// The store can be loaded and saved from/to the specified file path.
/// The file is a line-oriented set of config::authority serializations.
/// Duplicate addresses and those with zero-valued ports are disacarded.
class BCT_API hosts
  : system::noncopyable
{
public:
    DEFAULT4(hosts);

    ////typedef std::shared_ptr<hosts> ptr;
    typedef std::function<void(const code&, const messages::address_item&)>
        address_item_handler;
    typedef std::function<void(const code&, const messages::address_items&)>
        address_items_handler;

    /// Construct an instance.
    hosts(const settings& settings) NOEXCEPT;
    virtual ~hosts() NOEXCEPT;

    /// Load hosts file.
    virtual code start() NOEXCEPT;

    // Save hosts to file.
    virtual code stop() NOEXCEPT;

    // Thread safe, inexact (ok).
    virtual size_t count() const NOEXCEPT;

    virtual void store(const messages::address_item& host) NOEXCEPT;
    virtual void store(const messages::address_items& hosts) NOEXCEPT;
    virtual void remove(const messages::address_item& host) NOEXCEPT;
    virtual void fetch(const address_item_handler& handler) const NOEXCEPT;
    virtual void fetch(const address_items_handler& handler) const NOEXCEPT;

private:
    typedef boost::circular_buffer<messages::address_item> buffer;

    buffer::iterator find(const messages::address_item& host) NOEXCEPT;

    // This is thread safe.
    std::atomic<size_t> count_;

    // These are not thread safe.
    const bool disabled_;
    const size_t capacity_;
    const std::filesystem::path file_path_;
    buffer buffer_;
    bool stopped_;
};

} // namespace network
} // namespace libbitcoin

#endif

