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
#include <bitcoin/network/async/subscriber.hpp>
#include <bitcoin/network/async/threadpool.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/error.hpp>

namespace libbitcoin {
namespace network {

class BCT_API logger
{
public:
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
        const logger& log_;
        std::ostringstream stream_{};
    };

    typedef std::function<void(const code&, const std::string&)> handler;

    logger() NOEXCEPT;

    writer write() const NOEXCEPT;
    void subscribe(handler&& handler) NOEXCEPT;

    /// Stop the subscriber/pool with a final message posted to subscribers.
    void stop(const std::string& message) NOEXCEPT;
    void stop(const code& ec, const std::string& message) NOEXCEPT;

protected:
    void notify(const code& ec, std::string&& message) const NOEXCEPT;

private:
    threadpool pool_;
    asio::strand strand_;
    subscriber<const code&, const std::string&> subscriber_;

    void do_subscribe(const handler& handler) NOEXCEPT;
    void do_notify(const code& ec, const std::string& message) const NOEXCEPT;
    void do_stop(const code& ec, const std::string& message) NOEXCEPT;
};

} // namespace network
} // namespace libbitcoin

#endif
