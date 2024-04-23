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
#ifndef LIBBITCOIN_NETWORK_PROTOCOL_VERSION_70001_HPP
#define LIBBITCOIN_NETWORK_PROTOCOL_VERSION_70001_HPP

#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol_version_31402.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

class BCT_API protocol_version_70001
  : public protocol_version_31402, protected tracker<protocol_version_70001>
{
public:
    typedef std::shared_ptr<protocol_version_70001> ptr;

    /// Construct a version protocol instance using configured values.
    protocol_version_70001(const session::ptr& session,
        const channel::ptr& channel) NOEXCEPT;

    /// Construct a version protocol instance using parameterized services.
    protocol_version_70001(const session::ptr& session,
        const channel::ptr& channel, uint64_t minimum_services,
        uint64_t maximum_services, bool relay) NOEXCEPT;

protected:
    messages::version version_factory(bool relay=false) const NOEXCEPT override;

private:
    // This is thread safe (const).
    const bool relay_;
};

} // namespace network
} // namespace libbitcoin

#endif
