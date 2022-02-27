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
#ifndef LIBBITCOIN_NETWORK_NET_CHANNEL_HPP
#define LIBBITCOIN_NETWORK_NET_CHANNEL_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/net/proxy.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

class session;

/// This class is thread safe (see comments on versions in cpp).
/// A channel is a proxy with logged timers and state.
/// Stop is thread safe and idempotent, may be called multiple times.
class BCT_API channel
  : public proxy, track<channel>
{
public:
    typedef std::shared_ptr<channel> ptr;

    /// Attach a protocol to the channel, caller must start returned protocol.
    template <class Protocol, typename... Args>
    typename Protocol::ptr do_attach(const session& session, Args&&... args)
    {
        BC_ASSERT_MSG(stranded(), "subscribe_stop");

        // HACK: public but must be called from channel strand.
        // HACK: this avoids need for post/callback during attach.
        if (!stranded())
            return nullptr;

        // Protocols are attached after channel start.
        const auto protocol = std::make_shared<Protocol>(session,
            shared_from_base<channel>(), std::forward<Args>(args)...);

        // Protocol lifetime is ensured by the channel stop subscriber.
        do_subscribe_stop([=](const code&) { protocol->nop(); });
        return protocol;
    }

    channel(socket::ptr socket, const settings& settings);

    void start() override;
    void stop(const code& ec) override;

    uint64_t nonce() const noexcept;
    uint32_t negotiated_version() const noexcept;
    void set_negotiated_version(uint32_t value) noexcept;
    messages::version::ptr peer_version() const noexcept;
    void set_peer_version(messages::version::ptr value) noexcept;

protected:
    void do_stop(const code& ec);

    size_t maximum_payload() const noexcept override;
    uint32_t protocol_magic() const noexcept override;
    bool validate_checksum() const noexcept override;
    bool verbose() const noexcept override;
    uint32_t version() const noexcept override;
    void signal_activity() override;

private:
    void start_expiration();
    void handle_expiration(const code& ec);

    void start_inactivity();
    void handle_inactivity(const code& ec);

    // These are thread safe.
    const size_t maximum_payload_;
    const uint32_t protocol_magic_;
    const uint64_t channel_nonce_;
    const bool validate_checksum_;
    const bool verbose_logging_;

    // These are not thread safe.
    uint32_t negotiated_version_;
    messages::version::ptr peer_version_;
    deadline::ptr expiration_;
    deadline::ptr inactivity_;
};

} // namespace network
} // namespace libbitcoin

#endif
