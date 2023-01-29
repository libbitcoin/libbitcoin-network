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
#include <bitcoin/network/async/logger.hpp>

#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/handlers.hpp>
#include <bitcoin/network/boost.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

logger::logger() NOEXCEPT
  : pool_(1, thread_priority::low),
    strand_(pool_.service().get_executor()),
    subscriber_(strand_)
{
}

logger::writer logger::write() const NOEXCEPT
{
    return { *this };
}

// protected
void logger::notify(const code& ec, std::string&& message) const NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&logger::do_notify, this, ec, std::move(message)));
}

// private
void logger::do_notify(const code& ec,
    const std::string& message) const NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");
    subscriber_.notify(ec, message);
}

void logger::subscribe(handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&logger::do_subscribe, this, std::move(handler)));
}

// private
void logger::do_subscribe(const handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");
    subscriber_.subscribe(move_copy(handler));
}

void logger::stop(const std::string& message) NOEXCEPT
{
    // Subscriber asserts if stopped with a success code.
    stop(error::service_stopped, message);
}

void logger::stop(const code& ec, const std::string& message) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&logger::do_stop, this, ec, message));

    pool_.stop();
    BC_DEBUG_ONLY(const auto result =) pool_.join();
    BC_ASSERT_MSG(result, "logger::join");
}

// private
void logger::do_stop(const code& ec, const std::string& message) NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");
    subscriber_.stop(ec, message);
 }

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
