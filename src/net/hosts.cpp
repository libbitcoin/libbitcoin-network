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

#include <bitcoin/system.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/config/utilities.hpp>
#include <bitcoin/network/error.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

using namespace system;
using namespace messages;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

hosts::hosts(const settings& settings) NOEXCEPT
  : file_path_(settings.file()),
    minimum_(settings.address_minimum),
    maximum_(settings.address_maximum),
    capacity_(possible_narrow_cast<size_t>(settings.host_pool_capacity)),
    disabled_(is_zero(capacity_)),
    buffer_(capacity_)
{
}

// O(N).
code hosts::start() NOEXCEPT
{
    if (disabled_)
        return error::success;

    try
    {
        ifstream file{ file_path_, ifstream::in };
        if (!file.good())
            return error::success;

        for (std::string line{}; std::getline(file, line); push(line));

        if (file.bad())
            return error::file_load;
    }
    catch (const std::exception&)
    {
        return error::file_exception;
    }

    if (buffer_.empty())
    {
        code ec;
        std::filesystem::remove(file_path_, ec);
    }

    count_.store(buffer_.size());
    return error::success;
}

// O(N).
code hosts::stop() NOEXCEPT
{
    if (disabled_)
        return error::success;

    // Idempotent stop.
    disabled_ = true;

    if (buffer_.empty())
    {
        code ec;
        std::filesystem::remove(file_path_, ec);
        return ec ? error::file_save : error::success;
    }

    try
    {
        ofstream file{ file_path_, ofstream::out };
        if (!file.good())
            return error::file_save;

        for (const auto& entry: buffer_)
            file << config::address{ entry } << std::endl;

        if (file.bad())
            return error::file_save;
    }
    catch (const std::exception&)
    {
        return error::file_exception;
    }

    buffer_.clear();
    count_.store(zero);
    return error::success;
}

// O(1).
size_t hosts::count() const NOEXCEPT
{
    return count_.load(std::memory_order_relaxed);
}

// O(N) <= could be O(1) with O(1) search.
bool hosts::restore(const address_item& host) NOEXCEPT
{
    if (disabled_)
        return true;

    if (!config::is_valid(host))
        return false;

    // O(N) <= could be resolved with O(1) search.
    const auto it = find(host);

    // O(1).
    if (it != buffer_.end())
        *it = host;
    else
        buffer_.push_back(host);

    count_.store(buffer_.size());
    return true;
}

// O(1).
void hosts::take(const address_item_handler& handler) NOEXCEPT
{
    if (buffer_.empty())
    {
        handler(error::address_not_found, {});
        return;
    }

    count_.store(sub1(buffer_.size()));

    // O(1).
    handler(error::success, pop());
}

// O(N^2) <= could be O(N) with O(1) search.
size_t hosts::save(const address_items& hosts) NOEXCEPT
{
    // If enabled then minimum capacity is one and buffer is at capacity.
    if (disabled_ || hosts.empty())
        return zero;

    // Accept between 1 and all of the filtered addresses, up to capacity.
    const auto usable = std::min(hosts.size(), capacity_);
    const auto random = pseudo_random::next(one, usable);

    // But always accept at least the amount we are short if available.
    const auto gap = capacity_ - buffer_.size();
    const auto accept = std::max(gap, random);

    // Convert minimum desired to nonzero step for iteration.
    const auto step = std::max(usable / accept, one);
    const auto start_size = buffer_.size();

    // O(N^2).
    // Push addresses into the buffer.
    for (auto index = zero; index < usable; index = ceilinged_add(index, step))
    {
        // O(1).
        const auto& host = hosts.at(index);

        // O(N) <= could be resolved with O(1) search.
        if (!exists(host))
        {
            // O(1).
            buffer_.push_back(host);
            count_.store(buffer_.size());
        }
    }

    return buffer_.size() - start_size;
}

// O(N).
void hosts::fetch(const address_handler& handler) const NOEXCEPT
{
    if (buffer_.empty())
    {
        handler(error::address_not_found, {});
        return;
    }

    // Vary the return count (quantity fingerprinting).
    const auto divide = pseudo_random::next<size_t>(minimum_, maximum_);
    const auto size = std::min(messages::max_address, buffer_.size() / divide);

    // Vary the start position (value fingerprinting).
    const auto limit = sub1(buffer_.size());
    auto index = pseudo_random::next(zero, limit);

    // Allocate non-const message (converted to const by return).
    const auto out = to_shared<messages::address>();
    out->addresses.reserve(size);

    // O(N).
    for (size_t count = 0; count < size; ++count)
        out->addresses.push_back(buffer_.at(index++ % limit));

    handler(error::success, out);
}

// private
// ----------------------------------------------------------------------------

// O(1)
inline address_item::cptr hosts::pop() NOEXCEPT
{
    BC_ASSERT_MSG(!buffer_.empty(), "pop from empty buffer");

    const auto host = to_shared<address_item>(std::move(buffer_.front()));
    buffer_.pop_front();
    return host;
}

// O(1).
inline void hosts::push(const std::string& line) NOEXCEPT
{
    try
    {
        buffer_.push_back(config::address{ line }.item());
    }
    catch (std::exception&)
    {
    }
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
