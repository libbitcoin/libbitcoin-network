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
#include <bitcoin/network/log/logger.hpp>

#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/log/levels.hpp>
#include <bitcoin/network/log/timer.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
    
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

logger::logger() NOEXCEPT
{
}

logger::~logger() NOEXCEPT
{
    stop();

    if (!pool_.join())
    {
        BC_ASSERT_MSG(false, "failed to join logger threadpool");
        std::abort();
    }
}

logger::writer logger::write(uint8_t level) const NOEXCEPT
{
    return { *this, level };
}

bool logger::stranded() const NOEXCEPT
{
    return strand_.running_in_this_thread();
}

// stop
// ----------------------------------------------------------------------------
// Threadpool is started on construct, can only be stopped.

void logger::stop(uint8_t level) NOEXCEPT
{
    stop({}, level);
}

void logger::stop(const std::string& message, uint8_t level) NOEXCEPT
{
    stop(error::service_stopped, message, level);
}

void logger::stop(const code& ec, const std::string& message,
    uint8_t level) NOEXCEPT
{
    // Guard subscribers.
    stopped_.store(true);

    // Protect pool and subscribers (idempotent but not thread safe).
    boost::asio::post(strand_,
        std::bind(&logger::do_stop,
            this, ec, zulu_time(), message, level));
}

bool logger::stopped() const NOEXCEPT
{
    return stopped_.load();
}

// private
void logger::do_stop(const code& ec, time_t zulu, const std::string& message,
    uint8_t level) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Subscriber asserts if stopped with a success code.
    message_subscriber_.stop(ec, level, zulu, message);
    event_subscriber_.stop(ec, {}, {}, {});

    // Stop threadpool keep-alive, all work must self-terminate to affect join.
    pool_.stop();
 }

// messages
// ----------------------------------------------------------------------------

// protected
void logger::notify(const code& ec, uint8_t level,
    std::string&& message) const NOEXCEPT
{
    boost::asio::post(strand_,
        std::bind(&logger::do_notify_message, this, ec, level, zulu_time(),
            std::move(message)));
}

// private
void logger::do_notify_message(const code& ec, uint8_t level, time_t zulu,
    const std::string& message) const NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    message_subscriber_.notify(ec, level, zulu, message);
}

void logger::subscribe_messages(message_notifier&& handler) NOEXCEPT
{
    if (stopped())
    {
        handler(error::service_stopped, {}, {}, {});
        return;
    }

    boost::asio::post(strand_,
        std::bind(&logger::do_subscribe_messages,
            this, std::move(handler)));
}

// private
void logger::do_subscribe_messages(const message_notifier& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    message_subscriber_.subscribe(move_copy(handler));
}

// events
// ----------------------------------------------------------------------------

void logger::fire(uint8_t event_, uint64_t value) const NOEXCEPT
{
    boost::asio::post(strand_,
        std::bind(&logger::do_notify_event,
            this, event_, value, fine_clock::now()));
}

// private
void logger::do_notify_event(uint8_t event_, uint64_t value,
    const time& point) const NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    event_subscriber_.notify(error::success, event_, value, point);
}

void logger::subscribe_events(event_notifier&& handler) NOEXCEPT
{
    if (stopped())
    {
        handler(error::service_stopped, {}, {}, {});
        return;
    }

    boost::asio::post(strand_,
        std::bind(&logger::do_subscribe_events,
            this, std::move(handler)));
}

// private
void logger::do_subscribe_events(const event_notifier& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    event_subscriber_.subscribe(move_copy(handler));
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
