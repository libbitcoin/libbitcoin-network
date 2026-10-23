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
#ifndef LIBBITCOIN_NETWORK_BROADCAST_WIRE_TX_HPP
#define LIBBITCOIN_NETWORK_BROADCAST_WIRE_TX_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {

/// Transaction view, broadcast to service channels, streams with witness.
class wire_tx
  : public rpc::streamable
{
public:
    typedef std::shared_ptr<const wire_tx> cptr;
    using view = system::chain::view::transaction;

    /// The broadcast method (the message itself is never serialized).
    static constexpr auto command = "tx";

    DELETE_COPY_MOVE_DESTRUCT(wire_tx);

    inline wire_tx(const view::cptr& tx) NOEXCEPT
      : tx_(tx)
    {
    }

    /// The transaction view.
    inline const view::cptr& tx() const NOEXCEPT
    {
        return tx_;
    }

    inline size_t size() const NOEXCEPT override
    {
        return tx_->serialized_size(true);
    }

    inline void write(system::write::bytes::fast& sink) const NOEXCEPT override
    {
        tx_->to_data(sink, true);
    }

private:
    // This is thread safe.
    const view::cptr tx_;
};

} // namespace network
} // namespace libbitcoin

#endif
