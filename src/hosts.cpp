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
#include <bitcoin/network/hosts.hpp>

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

using namespace bc::config;

#define NAME "hosts"

// TODO: change to network_address bimap hash table with services and age.
hosts::hosts(const settings& settings)
  : capacity_(static_cast<size_t>(settings.host_pool_capacity)),
    buffer_(std::max(capacity_, static_cast<size_t>(1u))),
    stopped_(true),
    file_path_(settings.hosts_file),
    disabled_(capacity_ == 0)
{
}

// private
hosts::iterator hosts::find(const address& host)
{
    const auto found = [&host](const address& entry)
    {
        return entry.port() == host.port() && entry.ip() == host.ip();
    };

    return std::find_if(buffer_.begin(), buffer_.end(), found);
}

size_t hosts::count() const
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    shared_lock lock(mutex_);

    return buffer_.size();
    ///////////////////////////////////////////////////////////////////////////
}

code hosts::fetch(address& out) const
{
    if (disabled_)
        return error::not_found;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    shared_lock lock(mutex_);

    if (stopped_)
        return error::service_stopped;

    if (buffer_.empty())
        return error::not_found;

    // Randomly select an address from the buffer.
    const auto random = pseudo_random::next(0, buffer_.size() - 1);
    const auto index = static_cast<size_t>(random);
    out = buffer_[index];
    return error::success;
    ///////////////////////////////////////////////////////////////////////////
}

code hosts::fetch(address::list& out) const
{
    if (disabled_)
        return error::not_found;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    {
        shared_lock lock(mutex_);

        if (stopped_)
            return error::service_stopped;

        if (buffer_.empty())
            return error::not_found;

        const auto out_count = std::min(max_address, std::min(buffer_.size(),
            capacity_) / static_cast<size_t>(pseudo_random::next(5, 10)));

        if (out_count == 0)
            return error::success;

        const auto limit = buffer_.size() - 1;
        auto index = static_cast<size_t>(pseudo_random::next(0, limit));

        out.reserve(out_count);
        for (size_t count = 0; count < out_count; ++count)
            out.push_back(buffer_[index++ % limit]);
    }
    ///////////////////////////////////////////////////////////////////////////

    pseudo_random::shuffle(out);
    return error::success;
}

// load
code hosts::start()
{
    if (disabled_)
        return error::success;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    mutex_.lock_upgrade();

    if (!stopped_)
    {
        mutex_.unlock_upgrade();
        //---------------------------------------------------------------------
        return error::operation_failed;
    }

    mutex_.unlock_upgrade_and_lock();
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    stopped_ = false;
    bc::ifstream file(file_path_.string());
    const auto file_error = file.bad();

    if (!file_error)
    {
        std::string line;

        while (std::getline(file, line))
        {
            // TODO: create full space-delimited network_address serialization.
            // Use to/from string format as opposed to wire serialization.
            config::authority host(line);

            if (host.port() != 0)
                buffer_.push_back(host.to_network_address());
        }
    }

    mutex_.unlock();
    ///////////////////////////////////////////////////////////////////////////

    if (file_error)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failed to save hosts file.";
        return error::file_system;
    }

    return error::success;
}

// load
code hosts::stop()
{
    if (disabled_)
        return error::success;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    mutex_.lock_upgrade();

    if (stopped_)
    {
        mutex_.unlock_upgrade();
        //---------------------------------------------------------------------
        return error::success;
    }

    mutex_.unlock_upgrade_and_lock();
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    stopped_ = true;
    bc::ofstream file(file_path_.string());
    const auto file_error = file.bad();

    if (!file_error)
    {
        for (const auto& entry: buffer_)
        {
            // TODO: create full space-delimited network_address serialization.
            // Use to/from string format as opposed to wire serialization.
            file << config::authority(entry) << std::endl;
        }

        buffer_.clear();
    }

    mutex_.unlock();
    ///////////////////////////////////////////////////////////////////////////

    if (file_error)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failed to load hosts file.";
        return error::file_system;
    }

    return error::success;
}

code hosts::remove(const address& host)
{
    if (disabled_)
        return error::not_found;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    mutex_.lock_upgrade();

    if (stopped_)
    {
        mutex_.unlock_upgrade();
        //---------------------------------------------------------------------
        return error::service_stopped;
    }

    auto it = find(host);

    if (it != buffer_.end())
    {
        mutex_.unlock_upgrade_and_lock();
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        buffer_.erase(it);

        mutex_.unlock();
        //---------------------------------------------------------------------
        return error::success;
    }

    mutex_.unlock_upgrade();
    ///////////////////////////////////////////////////////////////////////////

    return error::not_found;
}

code hosts::store(const address& host)
{
    if (disabled_)
        return error::success;

    if (!host.is_valid())
    {
        // Do not treat invalid address as an error, just log it.
        LOG_DEBUG(LOG_NETWORK)
            << "Invalid host address from peer.";

        return error::success;
    }

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    mutex_.lock_upgrade();

    if (stopped_)
    {
        mutex_.unlock_upgrade();
        //---------------------------------------------------------------------
        return error::service_stopped;
    }

    if (find(host) == buffer_.end())
    {
        mutex_.unlock_upgrade_and_lock();
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        buffer_.push_back(host);

        mutex_.unlock();
        //---------------------------------------------------------------------
        return error::success;
    }

    mutex_.unlock_upgrade();
    ///////////////////////////////////////////////////////////////////////////

    ////// We don't treat redundant address as an error, just log it.
    ////LOG_DEBUG(LOG_NETWORK)
    ////    << "Redundant host address [" << authority(host) << "] from peer.";

    return error::success;
}

void hosts::store(const address::list& hosts, result_handler handler)
{
    if (disabled_ || hosts.empty())
    {
        handler(error::success);
        return;
    }

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    mutex_.lock_upgrade();

    if (stopped_)
    {
        mutex_.unlock_upgrade();
        //---------------------------------------------------------------------
        handler(error::service_stopped);
        return;
    }

    // Accept between 1 and all of this peer's addresses up to capacity.
    const auto capacity = buffer_.capacity();
    const auto usable = std::min(hosts.size(), capacity);
    const auto random = static_cast<size_t>(pseudo_random::next(1, usable));

    // But always accept at least the amount we are short if available.
    const auto gap = capacity - buffer_.size();
    const auto accept = std::max(gap, random);

    // Convert minimum desired to step for iteration, no less than 1.
    const auto step = std::max(usable / accept, size_t(1));
    size_t accepted = 0;

    mutex_.unlock_upgrade_and_lock();
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    for (size_t index = 0; index < usable; index = ceiling_add(index, step))
    {
        const auto& host = hosts[index];

        // Do not treat invalid address as an error, just log it.
        if (!host.is_valid())
        {
            LOG_DEBUG(LOG_NETWORK)
                << "Invalid host address from peer.";
            continue;
        }

        // Do not allow duplicates in the host cache.
        if (find(host) == buffer_.end())
        {
            ++accepted;
            buffer_.push_back(host);
        }
    }

    mutex_.unlock();
    ///////////////////////////////////////////////////////////////////////////

    LOG_VERBOSE(LOG_NETWORK)
        << "Accepted (" << accepted << " of " << hosts.size()
        << ") host addresses from peer.";

    handler(error::success);
}

} // namespace network
} // namespace libbitcoin
