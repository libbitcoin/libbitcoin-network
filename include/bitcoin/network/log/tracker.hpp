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
#ifndef LIBBITCOIN_NETWORK_LOG_TRACKER_HPP
#define LIBBITCOIN_NETWORK_LOG_TRACKER_HPP

#include <atomic>
#include <typeinfo>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/levels.hpp>
#include <bitcoin/network/log/logger.hpp>

namespace libbitcoin {
namespace network {

template <class Class>
class tracker
{
protected:
    DEFAULT_COPY_MOVE(tracker);

#if defined(HAVE_LOGO)

    tracker(const logger& log) NOEXCEPT
      : log_(log)
    {
        LOGO(typeid(Class).name() << "(" << ++instances_ << ")");
    }

    ~tracker() NOEXCEPT
    {
        LOGO(typeid(Class).name() << "(" << --instances_ << ")~");
    }

private:
    // These are thread safe.
    static inline std::atomic<size_t> instances_{};
    const logger& log_;

#else // HAVE_LOGO

    tracker(const logger&) NOEXCEPT
    {
    }

    ~tracker() NOEXCEPT
    {
    }

#endif // HAVE_LOGO
};

} // namespace network
} // namespace libbitcoin

#endif
