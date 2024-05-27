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
#ifndef LIBBITCOIN_NETWORK_LOG_LOGGER_HPP
#define LIBBITCOIN_NETWORK_LOG_LOGGER_HPP

#include <atomic>
#include <chrono>
#include <ostream>
#include <sstream>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/levels.hpp>

namespace libbitcoin {
namespace network {

/// Thread safe logging class.
/// Must be kept in scope until last logger::writer instance is destroyed.
/// Emits streaming writer that commits message upon destruct.
/// Provides subscription to std::string message commitments.
/// Stoppable with optional termination code and message.
class BCT_API logger final
{
public:
    typedef unsubscriber<uint8_t, time_t, const std::string&> message_subscriber;
    typedef message_subscriber::handler message_notifier;

    using time = fine_clock::time_point;
    typedef unsubscriber<uint8_t, uint64_t, const time&> event_subscriber;
    typedef event_subscriber::handler event_notifier;

    /// Use to initialize timer events.
    static time now() NOEXCEPT { return fine_clock::now(); }

    /// Streaming log writer (std::ostringstream), not thread safe.
    class writer final
    {
    public:
        DELETE_COPY_MOVE(writer);

        inline writer(const logger& log, uint8_t level) NOEXCEPT
          : log_(log), level_(level)
        {
        }

        inline ~writer() NOEXCEPT
        {
            // log_.notify() cannot be non-const in destructor.
            BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
            log_.notify(error::success, level_, stream_.str());
            BC_POP_WARNING()
        }

        template <typename Type>
        inline std::ostream& operator<<(const Type& message) NOEXCEPT
        {
            BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
            stream_ << message;
            BC_POP_WARNING()
            return stream_;
        }

    private:
        // This is thread safe.
        // This cannot be declared mutable.
        const logger& log_;
        const uint8_t level_;

        // This is not thread safe.
        std::ostringstream stream_{};
    };

    DELETE_COPY_MOVE(logger);

    /// Construct a started (live) logger.
    logger() NOEXCEPT;

    /// Block on logger threadpool join.
    ~logger() NOEXCEPT;

    /// Obtain streaming writer (must destruct before this).
    /// The writer could capture refcounted logger reference, but this would
    /// require shared logger instances, an unnecessary complication/cost.
    writer write(uint8_t level) const NOEXCEPT;

    /// Fire event with optional value, recorded with current time.
    void fire(uint8_t event_, uint64_t value=zero) const NOEXCEPT;

    /// Fire event with value as duration start to now, in specified unints.
    template <typename Time = milliseconds>
    inline void span(uint8_t event_, const time& start) const NOEXCEPT
    {
        // value parameter is time span in Time units.
        fire(event_, std::chrono::duration_cast<Time>(now() - start).count());
    }

    /// If stopped, handler is invoked with error::subscriber_stopped/defaults
    /// and dropped. Otherwise it is held until stop/drop. False if failed.
    void subscribe_messages(message_notifier&& handler) NOEXCEPT;
    void subscribe_events(event_notifier&& handler) NOEXCEPT;

    /// Stop subscribers/pool with final message/empty posted to subscribers.
    void stop(const code& ec, const std::string& message, uint8_t level) NOEXCEPT;
    void stop(const std::string& message, uint8_t level=levels::quitting) NOEXCEPT;
    void stop(uint8_t level=levels::quitting) NOEXCEPT;
    bool stopped() const NOEXCEPT;

protected:
    bool stranded() const NOEXCEPT;
    void notify(const code& ec, uint8_t level,
        std::string&& message) const NOEXCEPT;

private:
    void do_subscribe_messages(const message_notifier& handler) NOEXCEPT;
    void do_notify_message(const code& ec, uint8_t level, time_t zulu,
        const std::string& message) const NOEXCEPT;

    void do_subscribe_events(const event_notifier& handler) NOEXCEPT;
    void do_notify_event(uint8_t event, uint64_t value,
        const time& point) const NOEXCEPT;

    void do_stop(const code& ec, time_t zulu, const std::string& message,
        uint8_t level) NOEXCEPT;

    // This is protected by strand.
    threadpool pool_{ one, thread_priority::low };

    // These are thread safe.
    std::atomic_bool stopped_{ false };
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    asio::strand strand_{ pool_.service().get_executor() };
    BC_POP_WARNING()

    // These are protected by strand.
    // notify()/do_notify() can be const because of mutable subscriber.
    mutable message_subscriber message_subscriber_{ strand_ };
    mutable event_subscriber event_subscriber_{ strand_ };
};

} // namespace network
} // namespace libbitcoin

#endif
