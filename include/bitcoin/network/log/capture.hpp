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

#include <atomic>
#include <optional>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

/// Thread safe (except for start) console input class.
class BCT_API capture final
{
public:
    typedef unsubscriber<const std::string&> subscriber;
    typedef subscriber::handler notifier;

    DELETE_COPY_MOVE(capture);

    /// Input stream to capture.
    capture(std::istream& input) NOEXCEPT;

    /// Input stream to capture and halt text (trimmed).
    capture(std::istream& input, const std::string& halt) NOEXCEPT;

    /// Stops and joins on destruct.
    ~capture() NOEXCEPT;

    /// Start only once, neither thread safe nor idempotent.
    void start() NOEXCEPT;

    /// Avoid initial loss from istream by completing subscribe before start.
    void subscribe(notifier&& handler, result_handler&& complete) NOEXCEPT;

    /// Signal stop any time before or after calling start.
    void stop() NOEXCEPT;

protected:
    bool stranded() const NOEXCEPT;
    void notify(const code& ec, const std::string& line) const NOEXCEPT;

private:
    void do_stop() NOEXCEPT;
    void do_start() NOEXCEPT;
    void do_notify(const code& ec, const std::string& line) const NOEXCEPT;
    void do_subscribe(const notifier& handler,
        const result_handler& complete) NOEXCEPT;

    // These are protected by strand.
    std::istream& input_;
    threadpool pool_{ two, thread_priority::high };

    // These are thread safe.
    const std::optional<std::string> halt_;
    std::atomic_bool stopped_{ false };
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    asio::strand strand_{ pool_.service().get_executor() };
    BC_POP_WARNING()

    // This is protected by strand.
    mutable subscriber subscriber_{ strand_ };
};

} // namespace network
} // namespace libbitcoin

#endif
