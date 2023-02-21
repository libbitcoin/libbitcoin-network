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
    disabled_(is_zero(capacity_))
{
    buffer_.reserve(capacity_);
}

size_t hosts::count() const NOEXCEPT
{
    return count_.load();
}

// private
inline void hosts::push_valid(const std::string& line) NOEXCEPT
{
    try
    {
        // O(1) average case, position arbitary.
        buffer_.insert(config::address{ line }.item());
    }
    catch (std::exception&)
    {
    }
}

code hosts::start() NOEXCEPT
{
    if (disabled_)
        return error::success;

    try
    {
        ifstream file{ file_path_, ifstream::in };
        if (!file.good())
            return error::success;

        for (std::string line{}; std::getline(file, line);)
            push_valid(line);

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

// push
bool hosts::restore(const address_item& host) NOEXCEPT
{
    if (disabled_)
        return true;

    if (!config::is_valid(host))
        return false;

    // Equality ignores timestamp and services.

    // O(1) average case.
    // Erase existing address (just in case somehow reintroduced).
    buffer_.erase(host);

    // O(1) average case, position arbitary.
    buffer_.insert(host);
    count_.store(buffer_.size());
    return true;
}

// pop
void hosts::take(const address_item_handler& handler) NOEXCEPT
{
    if (buffer_.empty())
    {
        handler(error::address_not_found, {});
        return;
    }

    // Select address from random buffer position.
    const auto limit = sub1(buffer_.size());
    const auto index = pseudo_random::next(zero, limit);

    // O(N) walking pointers.
    // Search is fast but not random indexation.
    const auto it = std::next(buffer_.begin(), index);
    const auto host = std::make_shared<address_item>(*it);

    // O(1) average case.
    buffer_.erase(it);
    count_.store(buffer_.size());
    handler(error::success, host);
}

// push(N)
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

    // Push selected addresses into the buffer, keep public count current.
    for (size_t index = 0; index < usable; index = ceilinged_add(index, step))
    {
        // Rejected if already present, position arbitary.
        buffer_.insert(hosts.at(index));
        count_.store(buffer_.size());
    }

    // Report number accepted.
    return buffer_.size() - start_size;
}

// pop(N)
void hosts::fetch(const address_handler& handler) const NOEXCEPT
{
    if (buffer_.empty())
    {
        handler(error::address_not_found, {});
        return;
    }

    // Vary the return count (quantity fingerprinting).
    const auto divide = pseudo_random::next<size_t>(minimum_, maximum_);
    const auto size = std::min(messages::max_address, buffer_.size() /
        std::max(divide, one));

    // Vary the start position (value fingerprinting).
    const auto limit = sub1(buffer_.size());
    auto index = pseudo_random::next(zero, limit);

    // Copy addresses into non-const message (converted to const by return).
    const auto out = to_shared<messages::address>();
    out->addresses.reserve(size);

    // O(N^2) walking pointers [bounded by 1000 x buffer.size] :O.
    for (size_t count = 0; count < size; ++count)
        out->addresses.push_back(*std::next(buffer_.begin(), index++ % limit));

    ////// Shuffle the message (order fingerprinting).
    ////pseudo_random::shuffle(out->addresses);
    handler(error::success, out);
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
