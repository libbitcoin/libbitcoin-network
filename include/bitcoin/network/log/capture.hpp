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
#ifndef LIBBITCOIN_NETWORK_LOG_CAPTURE_HPP
#define LIBBITCOIN_NETWORK_LOG_CAPTURE_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/error.hpp>

namespace libbitcoin {
namespace network {

/// Thread safe console input class.
class BCT_API capture final
{
public:
    typedef unsubscriber<const std::string&> subscriber;
    typedef subscriber::handler notifier;

    DELETE_COPY_MOVE(capture);

    capture() NOEXCEPT;
    capture(bool) NOEXCEPT;
    ~capture() NOEXCEPT;

    void start() const NOEXCEPT;
    void subscribe(notifier&& handler) NOEXCEPT;
    void stop(const code& ec, const std::string& message) NOEXCEPT;
    void stop(const std::string& message) NOEXCEPT;
    void stop() NOEXCEPT;

protected:
    void notify(const code& ec, std::string&& message) const NOEXCEPT;

private:
    bool stranded() const NOEXCEPT;
    void do_subscribe(const notifier& handler) NOEXCEPT;
    void do_notify(const code& ec, const std::string& message) const NOEXCEPT;
    void do_stop(const code& ec, const std::string& message) NOEXCEPT;

    // This is protected by strand.
    threadpool pool_{ one, thread_priority::low };

    // This is thread safe.
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    asio::strand strand_{ pool_.service().get_executor() };
    BC_POP_WARNING()

    // This is protected by strand.
    mutable subscriber subscriber_{ strand_ };
};

} // namespace network
} // namespace libbitcoin

#endif
