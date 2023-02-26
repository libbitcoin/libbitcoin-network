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
#include <bitcoin/network/log/capture.hpp>

#include <bitcoin/system.hpp>
#include <bitcoin/network/error.hpp>

namespace libbitcoin {
namespace network {

capture::capture() NOEXCEPT
{
}

capture::capture(bool) NOEXCEPT
  : capture()
{
    pool_.stop();
}

capture::~capture() NOEXCEPT
{
    pool_.join();
}

bool capture::stranded() const NOEXCEPT
{
    return strand_.running_in_this_thread();
}

// messages
// ----------------------------------------------------------------------------

// protected
void capture::notify(const code& ec, std::string&& message) const NOEXCEPT
{
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    boost::asio::dispatch(strand_,
        std::bind(&capture::do_notify, this, ec, std::move(message)));
    BC_POP_WARNING()
}

// private
void capture::do_notify(const code& ec,
    const std::string& message) const NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    subscriber_.notify(ec, message);
}

void capture::subscribe(notifier&& handler) NOEXCEPT
{
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    boost::asio::dispatch(strand_,
        std::bind(&capture::do_subscribe,
            this, std::move(handler)));
    BC_POP_WARNING()
}

// private
void capture::do_subscribe(const notifier& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    subscriber_.subscribe(move_copy(handler));
}

// stop
// ----------------------------------------------------------------------------

void capture::stop() NOEXCEPT
{
    stop({});
}

void capture::stop(const std::string& message) NOEXCEPT
{
    stop(error::service_stopped, message);
}

void capture::stop(const code& ec, const std::string& message) NOEXCEPT
{
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    boost::asio::dispatch(strand_,
        std::bind(&capture::do_stop, this, ec, message));
    BC_POP_WARNING()

    pool_.stop();
    BC_DEBUG_ONLY(const auto result =) pool_.join();
    BC_ASSERT_MSG(result, "capture::join");
}

// private
void capture::do_stop(const code& ec, const std::string& message) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Subscriber asserts if stopped with a success code.
    subscriber_.stop(ec, message);
 }

} // namespace network
} // namespace libbitcoin
