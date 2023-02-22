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

#include <functional>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/error.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

using namespace system;
using namespace messages;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

hosts::hosts(threadpool& pool, const settings& settings) NOEXCEPT
  : settings_(settings),
    minimum_(settings.address_minimum),
    maximum_(settings.address_maximum),
    capacity_(possible_narrow_cast<size_t>(settings.host_pool_capacity)),
    disabled_(is_zero(capacity_)),
    strand_(pool.service().get_executor()),
    buffer_(capacity_)
{
}

// Start/stop.
// ----------------------------------------------------------------------------

// O(N).
code hosts::start() NOEXCEPT
{
    // Not idempotent start.
    if (disabled_)
        return error::success;

    if (!stopped_)
        return error::operation_failed;

    // Restartable.
    stopped_ = false;

    try
    {
        ifstream file{ settings_.file(), ifstream::in };
        if (!file.good())
            return error::success;

        for (std::string line{}; std::getline(file, line);)
            push(line);

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
        std::filesystem::remove(settings_.file(), ec);
    }

    count_.store(buffer_.size());
    return error::success;
}

// O(N).
code hosts::stop() NOEXCEPT
{
    // Idempotent stop
    if (disabled_ || stopped_)
        return error::success;

    stopped_ = true;

    if (buffer_.empty())
    {
        code ec;
        std::filesystem::remove(settings_.file(), ec);
        return ec ? error::file_save : error::success;
    }

    try
    {
        ofstream file{ settings_.file(), ofstream::out };
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

// Properties.
// ----------------------------------------------------------------------------

// O(1).
size_t hosts::count() const NOEXCEPT
{
    return count_.load();
}

// thread safe
size_t hosts::reserved() const NOEXCEPT
{
    return authorities_.size();
}

// Usage.
// ----------------------------------------------------------------------------

// O(1).
void hosts::take(address_item_handler&& handler) NOEXCEPT
{
    boost::asio::post(strand_,
        std::bind(&hosts::do_take, this, std::move(handler)));
}

void hosts::do_take(const address_item_handler& handler) NOEXCEPT
{
    if (buffer_.empty())
    {
        handler(error::address_not_found, {});
        return;
    }

    count_.store(sub1(buffer_.size()));

    // O(1).
    // TODO: pop until empty pool or found valid (settings/authorities).
    handler(error::success, pop());
}

// O(N) <= could be O(1) with O(1) search.
void hosts::restore(const address_item_cptr& host,
    result_handler&& handler) NOEXCEPT
{
    boost::asio::post(strand_,
        std::bind(&hosts::do_restore, this, host, std::move(handler)));
}

void hosts::do_restore(const address_item_cptr& host,
    const result_handler& handler) NOEXCEPT
{
    if (stopped_)
    {
        handler(error::service_stopped);
        return;
    }

    // O(N) <= could be resolved with O(1) search.
    const auto it = find(*host);

    // O(1).
    if (it != buffer_.end())
    {
        *it = *host;
        handler(error::success);
        return;
    }

    // O(1).
    buffer_.push_back(*host);
    count_.store(buffer_.size());
    handler(error::success);
}

// Negotiation.
// ----------------------------------------------------------------------------

// O(N).
void hosts::fetch(address_handler&& handler) const NOEXCEPT
{
    boost::asio::post(strand_,
        std::bind(&hosts::do_fetch, this, std::move(handler)));
}

void hosts::do_fetch(const address_handler& handler) const NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

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
    for (auto count = zero; count < size; ++count)
        out->addresses.push_back(buffer_.at(index++ % limit));

    handler(error::success, out);
}

// O(N^2) <= could be O(N) with O(1) search.
void hosts::save(const address_cptr& message, count_handler&& handler) NOEXCEPT
{
    boost::asio::post(strand_,
        std::bind(&hosts::do_save, this, message, std::move(handler)));
}

void hosts::do_save(const address_cptr& message,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    if (stopped_)
    {
        handler(error::service_stopped, zero);
        return;
    }

    if (message->addresses.empty())
    {
        handler(error::address_not_found, zero);
        return;
    }

    // Accept between 1 and all of the addresses, up to capacity.
    // If started/enabled then minimum capacity is one (usable > 0).
    const auto usable = std::min(message->addresses.size(), capacity_);
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
        const auto& host = message->addresses.at(index);

        // O(N) <= could be resolved with O(1) search.

        // TODO: ***is_reserved is not thread safe.***
        if (!is_reserved(host) && !is_pooled(host))
        {
            // O(1).
            buffer_.push_back(host);
            count_.store(buffer_.size());
        }
    }

    handler(error::success, buffer_.size() - start_size);
}

// Reservation.
// ----------------------------------------------------------------------------
// requires network strand.

bool hosts::is_reserved(const config::authority& host) const NOEXCEPT
{
    return authorities_.contains(host);
}

bool hosts::reserve(const config::authority& host) NOEXCEPT
{
    if (authorities_.size() < authorities_.max_size() &&
        authorities_.insert(host).second)
    {
        authorities_count_.store(authorities_.size());
        return true;
    }

    return false;
}

bool hosts::unreserve(const config::authority& host) NOEXCEPT
{
    if (to_bool(authorities_.erase(host)))
    {
        authorities_count_.store(authorities_.size());
        return true;
    }

    return false;
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
        const config::address item{ line };

        if (!messages::is_specified(item)
            || settings_.disabled(item)
            || settings_.insufficient(item)
            || settings_.unsupported(item)
            || settings_.blacklisted(item)
            || !settings_.whitelisted(item))
            return;

        buffer_.push_back(item);
    }
    catch (std::exception&)
    {
    }
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
