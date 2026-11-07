/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
    io::post(strand_,
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
    BC_ASSERT(stranded());

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
    io::post(strand_,
        std::bind(&logger::do_notify_message, this, ec, level, zulu_time(),
            std::move(message)));
}

// private
void logger::do_notify_message(const code& ec, uint8_t level, time_t zulu,
    const std::string& message) const NOEXCEPT
{
    BC_ASSERT(stranded());
    message_subscriber_.notify(ec, level, zulu, message);
}

void logger::subscribe_messages(const interest::ptr& value,
    message_notifier&& handler) const NOEXCEPT
{
    if (stopped())
    {
        value->set(zero);
        handler(error::service_stopped, {}, {}, {});
        return;
    }

    const std::weak_ptr<interest> weak{ value };
    io::post(strand_,
        std::bind(&logger::do_subscribe_messages,
            this, weak, std::move(handler)));
}

// private
void logger::update_interest(uint64_t from, uint64_t to) const NOEXCEPT
{
    for (size_t level{}; level < interest_.size(); ++level)
    {
        const auto was = system::get_right(from, level);
        const auto is = system::get_right(to, level);

        if (is && !was)
            ++interest_.at(level);
        else if (was && !is)
            --interest_.at(level);
    }
}

// private
void logger::do_subscribe_messages(const std::weak_ptr<interest>& weak,
    const message_notifier& handler) const NOEXCEPT
{
    BC_ASSERT(stranded());
    message_subscriber_.subscribe(
        [weak, notify = handler](const code& ec, uint8_t level,
            time_t zulu, const std::string& message) NOEXCEPT
        {
            const auto value = weak.lock();
            if (!value)
                return false;

            if (!ec && !value->contains(level))
                return true;

            if (notify(ec, level, zulu, message) && !ec)
                return true;

            value->set(zero);
            return false;
        });
}

// interest
// ----------------------------------------------------------------------------

logger::interest::interest(const logger& log, uint64_t mask) NOEXCEPT
  : log_(log), mask_(mask)
{
    log_.update_interest(zero, mask);
}

logger::interest::~interest() NOEXCEPT
{
    set(zero);
}

uint64_t logger::interest::set(uint64_t mask) NOEXCEPT
{
    const auto previous = mask_.exchange(mask);
    log_.update_interest(previous, mask);
    return previous;
}

void logger::interest::enable(uint64_t mask) NOEXCEPT
{
    const auto previous = mask_.fetch_or(mask);
    log_.update_interest(previous, system::bit_or(previous, mask));
}

void logger::interest::disable(uint64_t mask) NOEXCEPT
{
    const auto keep = system::bit_not(mask);
    const auto previous = mask_.fetch_and(keep);
    log_.update_interest(previous, system::bit_and(previous, keep));
}

uint64_t logger::interest::toggle(uint64_t mask) NOEXCEPT
{
    const auto previous = mask_.fetch_xor(mask);
    log_.update_interest(previous, system::bit_xor(previous, mask));
    return previous;
}

bool logger::interest::contains(uint8_t level) const NOEXCEPT
{
    return level < bits<uint64_t> && system::get_right(mask_.load(), level);
}

// events
// ----------------------------------------------------------------------------

void logger::fire(uint8_t event_, uint64_t value) const NOEXCEPT
{
    io::post(strand_,
        std::bind(&logger::do_notify_event,
            this, event_, value, fine_clock::now()));
}

// private
void logger::do_notify_event(uint8_t event_, uint64_t value,
    const time& point) const NOEXCEPT
{
    BC_ASSERT(stranded());
    event_subscriber_.notify(error::success, event_, value, point);
}

void logger::subscribe_events(event_notifier&& handler) const NOEXCEPT
{
    if (stopped())
    {
        handler(error::service_stopped, {}, {}, {});
        return;
    }

    io::post(strand_,
        std::bind(&logger::do_subscribe_events,
            this, std::move(handler)));
}

// private
void logger::do_subscribe_events(const event_notifier& handler) const NOEXCEPT
{
    BC_ASSERT(stranded());
    event_subscriber_.subscribe(move_copy(handler));
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
