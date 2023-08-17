/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

using namespace system;
using namespace messages;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

hosts::hosts(const settings& settings, const logger& log) NOEXCEPT
  : settings_(settings),
    buffer_(settings.host_pool_capacity),
    reporter(log)
{
}

// Start/stop.
// ----------------------------------------------------------------------------

// O(N).
code hosts::start() NOEXCEPT
{
    // Not idempotent start.
    if (is_zero(buffer_.capacity()))
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

    LOGN("Loaded (" << buffer_.size() << ") addresses.");
    hosts_count_.store(buffer_.size());
    return error::success;
}

// O(N).
code hosts::stop() NOEXCEPT
{
    // Idempotent stop
    if (is_zero(buffer_.capacity()) || stopped_)
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

    LOGN("Saved (" << buffer_.size() << ") addresses.");
    buffer_.clear();
    hosts_count_.store(zero);
    return error::success;
}

// Properties.
// ----------------------------------------------------------------------------

size_t hosts::count() const NOEXCEPT
{
    return hosts_count_.load();
}

size_t hosts::reserved() const NOEXCEPT
{
    return authorities_count_.load();
}

// Usage.
// ----------------------------------------------------------------------------

// O(1).
void hosts::take(address_item_handler&& handler) NOEXCEPT
{
    if (stopped_)
    {
        handler(error::service_stopped, {});
        return;
    }

    // O(1) average, O(N) worst case.
    while (!buffer_.empty())
    {
        const auto host = pop();
        if (!is_reserved(*host))
        {
            hosts_count_.store(buffer_.size());
            handler(error::success, host);
            return;
        }
    }

    hosts_count_.store(zero);
    handler(error::address_not_found, {});
}

// O(N) <= could be O(1) with O(1) search.
void hosts::restore(const address_item_cptr& host,
    result_handler&& handler) NOEXCEPT
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
    hosts_count_.store(buffer_.size());
    handler(error::success);
}

// Negotiation.
// ----------------------------------------------------------------------------

// O(N).
void hosts::fetch(address_handler&& handler) const NOEXCEPT
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

    // Vary the return count (quantity fingerprinting).
    const auto divide = pseudo_random::next<size_t>(
        settings_.address_lower, settings_.address_upper);
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

    const auto start_size = buffer_.size();

    // O(N^2).
    // Push addresses into the buffer.
    for (const auto& host: message->addresses)
    {
        // O(N) <= could be resolved with O(1) search.
        if (!is_reserved(host) && !is_pooled(host))
        {
            // O(1).
            buffer_.push_back(host);
            hosts_count_.store(buffer_.size());
        }
    }

    handler(error::success, buffer_.size() - start_size);
}

// private
// ----------------------------------------------------------------------------

// O(1).
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

        if (!messages::is_specified(item))
        {
            LOGF("Address unspecified upon load [" << line << "].");
        }
        else if (settings_.disabled(item))
        {
            LOGF("Address disabled upon load [" << line << "].");
        }
        else if (settings_.insufficient(item))
        {
            LOGF("Address insufficient upon load [" << line << "].");
        }
        else if (settings_.unsupported(item))
        {
            LOGF("Address unsupported upon load [" << line << "].");
        }
        else if (settings_.peered(item))
        {
            LOGF("Address peered upon load [" << line << "].");
        }
        else if (settings_.blacklisted(item))
        {
            LOGF("Address blacklisted upon load [" << line << "].");
        }
        else if (!settings_.whitelisted(item))
        {
            LOGF("Address not whitelisted upon load [" << line << "].");
        }
        else
        {
            buffer_.push_back(item);
            ////LOGF("Address excluded upon load [" << line << "].");
        }
    }
    catch (std::exception&)
    {
        LOGF("Address failed to deserialize [" << line << "].");
    }
}

// Reservation.
// ----------------------------------------------------------------------------
// atomic unordered set: contains, insert, erase.

// O(1).
// private
inline bool hosts::is_reserved(const config::authority& host) const NOEXCEPT
{
    return authorities_.contains(host);
}

// O(1).
// Channel is connected (infrequent).
bool hosts::reserve(const config::authority& host) NOEXCEPT
{
    const auto result = authorities_.insert(host).second;
    if (result) ++authorities_count_;
    return result;
}

// O(1).
// Channel is unconnected (infrequent).
bool hosts::unreserve(const config::authority& host) NOEXCEPT
{
    const auto result = to_bool(authorities_.erase(host));
    if (result) --authorities_count_;
    return result;
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
