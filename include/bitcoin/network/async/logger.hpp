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
#ifndef LIBBITCOIN_NETWORK_ASYNC_LOGGER_HPP
#define LIBBITCOIN_NETWORK_ASYNC_LOGGER_HPP

#include <ostream>
#include <sstream>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/asio.hpp>
#include <bitcoin/network/async/unsubscriber.hpp>
#include <bitcoin/network/async/threadpool.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/error.hpp>

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
    typedef unsubscriber<const std::string&> subscriber;
    typedef subscriber::handler notifier;

    /// Streaming log writer (std::ostringstream), not thread safe.
    class writer final
    {
    public:
        DELETE_COPY_MOVE(writer);

        inline writer(const logger& log) NOEXCEPT
          : log_(log)
        {
        }

        inline ~writer() NOEXCEPT
        {
            // log_.notify() cannot be non-const in destructor.
            BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
            log_.notify(error::success, stream_.str());
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

        // This is not thread safe.
        std::ostringstream stream_{};
    };

    DELETE_COPY_MOVE(logger);

    /// Construct a started (live) logger.
    logger() NOEXCEPT;

    /// Construct a stopped (dead) logger.
    logger(bool) NOEXCEPT;

    /// Block on logger threadpool join.
    ~logger() NOEXCEPT;

    /// Obtain streaming writer (must destruct before this).
    /// The writer could capture refcounted logger reference, but this would
    /// require shared logger instances, an unnecessary complication/cost.
    writer write() const NOEXCEPT;

    /// If stopped, handler is invoked with error::subscriber_stopped/defaults
    /// and dropped. Otherwise it is held until stop/drop. False if failed.
    void subscribe(notifier&& handler) NOEXCEPT;

    /// Stop subscriber/pool with final message/empty posted to subscribers.
    void stop(const code& ec, const std::string& message) NOEXCEPT;
    void stop(const std::string& message) NOEXCEPT;
    void stop() NOEXCEPT;

protected:
    /// Only writer can access notify, must destruct before logger.
    void notify(const code& ec, std::string&& message) const NOEXCEPT;

private:
    void do_subscribe(const notifier& handler) NOEXCEPT;
    void do_notify(const code& ec, const std::string& message) const NOEXCEPT;
    void do_stop(const code& ec, const std::string& message) NOEXCEPT;

    // This is protected by strand.
    threadpool pool_;

    // This is thread safe.
    asio::strand strand_;

    // These are protected by strand.
    // notify()/do_notify() can be const because of mutable subscriber.
    mutable subscriber subscriber_;
};

} // namespace network
} // namespace libbitcoin

#endif
