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
#ifndef LIBBITCOIN_NETWORK_BROADCAST_DIAGNOSTICS_HPP
#define LIBBITCOIN_NETWORK_BROADCAST_DIAGNOSTICS_HPP

#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {

/// Channel diagnostics, broadcast to peer channels (never serialized).
/// The round completes when the last message reference is released.
class BCT_API diagnostics
{
public:
    typedef std::shared_ptr<const diagnostics> cptr;
    using race = race_all<const code&>;

    /// The broadcast method (there is no wire representation).
    static constexpr auto command = "diagnostics";

    /// The channels to be captured.
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
        /// identity.
        target group;
        uint64_t identifier;
        config::address address;
        config::address local;
        config::endpoint binding;

        /// Negotiation.
        bool encrypted;
        bool peer_relay;
        size_t peer_start_height;
        uint32_t peer_version;
        uint64_t peer_services;
        uint64_t peer_minimum_fee;
        std::string peer_user_agent;

        /// Rate.
        uint32_t created;
        uint32_t last_read;
        uint32_t last_write;
        int64_t time_offset;
        uint64_t bytes_sent;
        uint64_t bytes_received;

        /// Ping.
        steady_clock::duration ping_time;
        steady_clock::duration minimum_ping_time;
        steady_clock::duration pending_ping_time;
    };

    using rows = std::vector<row>;

    /// The captured rows.
    class BCT_API sink
    {
    public:
        typedef std::shared_ptr<sink> ptr;

        DELETE_COPY_MOVE_DESTRUCT(sink);

        sink() NOEXCEPT;
        void add(row&& value) NOEXCEPT;
        const rows& captured() const NOEXCEPT;

    private:
        // These are protected by mutex.
        mutable std::mutex mutex_{};
        rows rows_{};
    };

    DELETE_COPY_MOVE_DESTRUCT(diagnostics);

    diagnostics(const race::ptr& complete, const sink::ptr& captured,
        target group, uint64_t channel={}) NOEXCEPT;

    bool targets(uint64_t identifier) const NOEXCEPT;
    bool targets(target group) const NOEXCEPT;
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
