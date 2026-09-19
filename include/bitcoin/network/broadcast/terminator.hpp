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
#ifndef LIBBITCOIN_NETWORK_BROADCAST_TERMINATOR_HPP
#define LIBBITCOIN_NETWORK_BROADCAST_TERMINATOR_HPP

#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

/// Channel stop, broadcast to peer channels (never serialized).
/// The round completes upon the first match, otherwise upon the last
/// message reference release.
class BCT_API terminator
{
public:
    typedef std::shared_ptr<const terminator> cptr;
    using address_item = messages::peer::address_item;
    using race = race_any<const code&>;

    /// The broadcast method (there is no wire representation).
    static constexpr auto command = "terminator";

    DELETE_COPY_MOVE_DESTRUCT(terminator);

    /// Stop the channel of the given identifier or address with code.
    terminator(const race::ptr& complete, const code& reason,
        uint64_t channel, const config::address& address={}) NOEXCEPT;

    /// The message targets the identified channel or address.
    bool targets(uint64_t identifier,
        const config::address& address) const NOEXCEPT;

    /// The code with which the targeted channel stops.
    const code& reason() const NOEXCEPT;

    /// Complete the round as the stopped target (thread safe).
    void stopped() const NOEXCEPT;

private:
    // These are thread safe.
    const race::ptr race_;
    const code reason_;
    const uint64_t channel_;
    const config::address address_;
};

} // namespace network
} // namespace libbitcoin

#endif
