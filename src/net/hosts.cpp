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
#include <bitcoin/network/net/hosts.hpp>

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>
#include <bitcoin/system.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/error.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

using namespace bc::system;
using namespace config;
using namespace messages;

inline bool is_invalid(const address_item& host) NOEXCEPT
{
    return is_zero(host.port) || host.ip == null_ip_address;
}

// TODO: add min/max denominators to settings.
// TODO: use std::map to avoid insertion searches.
// TODO: create full space-delimited network_address serialization.
// TODO: Use to/from string format as opposed to wire serialization.
// TODO: manage timestamps (active channels are connected < 3 hours ago).
// TODO: change to network_address bimap hash table with services and age.
// TODO: create full space-delimited network_address serialization.
// TODO: Use to/from string format as opposed to wire serialization.
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
hosts::hosts(const logger& log, const settings& settings) NOEXCEPT
  : file_path_(settings.path),
    count_(zero),
    minimum_(5),
    maximum_(10),
    capacity_(possible_narrow_cast<size_t>(settings.host_pool_capacity)),
    disabled_(is_zero(capacity_)),
    buffer_(capacity_),
    stopped_(true),
    reporter(log)
{
}
BC_POP_WARNING()

hosts::~hosts() NOEXCEPT
{
    BC_ASSERT(stopped_);
}

size_t hosts::count() const NOEXCEPT
{
    return count_.load(std::memory_order_relaxed);
}

code hosts::start() NOEXCEPT
{
    if (disabled_)
        return error::success;

    if (!stopped_)
        return error::operation_failed;

    stopped_ = false;

    try
    {
        BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
        ifstream file(file_path_.string(), ifstream::in);
        if (!file.good())
        {
            LOG("Hosts file not found.");
            return error::success;
        }

        std::string line;
        while (std::getline(file, line))
        {
            const config::authority entry(line);
            const auto host = entry.to_address_item();

            if (!is_invalid(host))
                buffer_.push_back(host);
        }

        if (file.bad())
            return error::file_load;

        BC_POP_WARNING()
    }
    catch (const std::exception&)
    {
        return error::file_load;
    }

    count_.store(buffer_.size(), std::memory_order_relaxed);
    return error::success;
}

code hosts::stop() NOEXCEPT
{
    if (disabled_)
        return error::success;

    if (stopped_)
        return error::success;

    stopped_ = true;

    try
    {
        BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
        ofstream file(file_path_.string(), ofstream::out);
        if (!file.good())
            return error::file_save;

        for (const auto& entry: buffer_)
            file << config::authority(entry) << std::endl;

        if (file.bad())
            return error::file_save;

        BC_POP_WARNING()
    }
    catch (const std::exception&)
    {
        return error::file_save;
    }

    buffer_.clear();
    count_.store(zero, std::memory_order_relaxed);
    return error::success;
}

void hosts::store(const address_item& host) NOEXCEPT
{
    if (disabled_ || stopped_)
        return;

    // Do not treat invalid address as an error, just log it.
    if (is_invalid(host))
    {
        LOG("Invalid host address from peer.");
        return;
    }

    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    if (find(host) == buffer_.end())
    {
        buffer_.push_back(host);
        count_.store(buffer_.size(), std::memory_order_relaxed);
    }
    BC_POP_WARNING()
}

void hosts::store(const address_items& hosts) NOEXCEPT
{
    // If enabled then minimum capacity is one and buffer is at capacity.
    if (disabled_ || stopped_ || hosts.empty())
        return;

    // Accept between 1 and all of this peer's addresses up to capacity.
    const auto usable = std::min(hosts.size(), capacity_);
    const auto random = pseudo_random::next(one, usable);

    // But always accept at least the amount we are short if available.
    const auto gap = capacity_ - buffer_.size();
    const auto accept = std::max(gap, random);

    // Convert minimum desired to nonzero step for iteration.
    const auto step = std::max(usable / accept, one);
    auto accepted = zero;

    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    for (size_t index = 0; index < usable; index = ceilinged_add(index, step))
    {
        const auto& host = hosts.at(index);

        if (is_invalid(host))
        {
            LOG("Invalid host address in peer set.");
            continue;
        }

        if (find(host) != buffer_.end())
            continue;

        ++accepted;
        buffer_.push_back(host);
        count_.store(buffer_.size(), std::memory_order_relaxed);

    }
    BC_POP_WARNING()

    LOG("Accepted (" << accepted << " of " << hosts.size() <<
        ") host addresses from peer.");
}

void hosts::remove(const address_item& host) NOEXCEPT
{
    if (stopped_ || buffer_.empty())
        return;

    const auto it = find(host);

    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    if (it != buffer_.end())
    {
        buffer_.erase(it);
        count_.store(buffer_.size(), std::memory_order_relaxed);
        return;
    }
    BC_POP_WARNING()

    LOG("Address to remove not found.");
}

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
void hosts::fetch(const address_item_handler& handler) const NOEXCEPT
{
    if (stopped_)
    {
        handler(error::service_stopped, {});
        return;
    }

    if (buffer_.empty())
    {
        handler(error::address_not_found, {});
        return;
    }

    // Randomly select an address from the buffer.
    const auto limit = sub1(buffer_.size());
    const auto index = pseudo_random::next(zero, limit);
    handler(error::success, buffer_.at(index));
}
BC_POP_WARNING()

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
void hosts::fetch(const address_items_handler& handler) const NOEXCEPT
{
    if (stopped_)
    {
        handler(error::service_stopped, {});
        return;
    }

    if (buffer_.empty())
    {
        handler(error::address_not_found, {});
        return;
    }

    const auto out_count = std::min(messages::max_address,
        buffer_.size() / pseudo_random::next<size_t>(minimum_, maximum_));

    const auto limit = sub1(buffer_.size());
    auto index = pseudo_random::next(zero, limit);

    address_items out{};
    out.reserve(out_count);
    for (size_t count = 0; count < out_count; ++count)
        out.push_back(buffer_.at(index++ % limit));

    pseudo_random::shuffle(out);
    handler(error::success, out);
}
BC_POP_WARNING()

// private
hosts::buffer::iterator hosts::find(const address_item& host) NOEXCEPT
{
    // TODO: add equality operator to address_item.
    const auto found = [&host](const address_item& entry) NOEXCEPT
    {
        return entry.port == host.port && entry.ip == host.ip;
    };

    return std::find_if(buffer_.begin(), buffer_.end(), found);
}

} // namespace network
} // namespace libbitcoin
