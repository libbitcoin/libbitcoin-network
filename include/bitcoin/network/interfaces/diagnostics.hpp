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
#ifndef LIBBITCOIN_NETWORK_INTERFACES_DIAGNOSTICS_HPP
#define LIBBITCOIN_NETWORK_INTERFACES_DIAGNOSTICS_HPP

#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {

/// Channel diagnostics, broadcast to peer channels (never serialized).
/// The round completes when the last reference to the race is released, so
/// each subscriber retains the message until it has added its own row.
class BCT_API diagnostics
{
public:
    typedef std::shared_ptr<const diagnostics> cptr;
    typedef race_all<const code&> race;

    /// The broadcast method, this message has no wire representation.
    static constexpr auto command = "diagnostics";

    /// The channels to be captured, each determines its own membership.
    enum class target
    {
        all,
        inbound,
        outbound,
        manual,
        channel
    };

    /// The diagnostic state of one channel.
    struct row
    {
        uint64_t identifier;
        config::address address;
        target group;
        uint32_t version;
        uint64_t services;
        uint64_t sent;
        size_t start_height;
        bool encrypted;
        std::string agent;
    };

    typedef std::vector<row> rows;

    /// The captured rows, shared by the message and the race completer.
    class BCT_API sink
    {
    public:
        typedef std::shared_ptr<sink> ptr;

        DELETE_COPY_MOVE_DESTRUCT(sink);

        sink() NOEXCEPT;

        /// Add the row of a member channel (thread safe).
        void add(row&& value) NOEXCEPT;

        /// The captured rows, read only upon race completion.
        const rows& captured() const NOEXCEPT;

    private:
        // This is protected by mutex.
        mutable std::mutex mutex_{};
        rows rows_{};
    };

    DELETE_COPY_MOVE_DESTRUCT(diagnostics);

    /// Capture the given group, or the channel of the given identifier.
    diagnostics(const race::ptr& complete, const sink::ptr& captured,
        target group, uint64_t channel={}) NOEXCEPT;

    /// The identified channel is a member (target is channel).
    bool member(uint64_t identifier) const NOEXCEPT;

    /// The grouped channel is a member (target is not channel).
    bool member(target group) const NOEXCEPT;

    /// Add the row of a member channel (thread safe).
    void add(row&& value) const NOEXCEPT;

private:
    // These are thread safe.
    const race::ptr race_;
    const sink::ptr sink_;
    const target group_;
    const uint64_t channel_;
};

} // namespace network
} // namespace libbitcoin

#endif
