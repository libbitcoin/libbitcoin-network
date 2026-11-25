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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_SV2_NEW_EXTENDED_MINING_JOB_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_SV2_NEW_EXTENDED_MINING_JOB_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/sv2/enums/identifiers.hpp>
#include <bitcoin/network/messages/sv2/types.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace sv2 {

/// NewExtendedMiningJob (server to client), a future job if ntime_start is empty.
struct BCT_API new_extended_mining_job
{
    typedef std::shared_ptr<const new_extended_mining_job> cptr;

    static constexpr uint8_t identifier{ identifiers::new_extended_mining_job };
    static constexpr bool channel{ true };

    static cptr deserialize(const std::span<const uint8_t>& data) NOEXCEPT;
    static new_extended_mining_job deserialize(system::reader& source) NOEXCEPT;

    bool serialize(const system::data_slab& data) const NOEXCEPT;
    void serialize(system::writer& sink) const NOEXCEPT;

    size_t size() const NOEXCEPT;

    uint32_t channel_id;
    uint32_t job_id;
    std::optional<uint32_t> ntime_start;
    uint32_t version;
    bool version_rolling_allowed;
    u256s merkle_path;
    system::data_chunk coinbase_tx_prefix;
    system::data_chunk coinbase_tx_suffix;
};

} // namespace sv2
} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
