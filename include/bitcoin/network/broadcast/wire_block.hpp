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
#ifndef LIBBITCOIN_NETWORK_BROADCAST_WIRE_BLOCK_HPP
#define LIBBITCOIN_NETWORK_BROADCAST_WIRE_BLOCK_HPP

#include <bitcoin/network/broadcast/wire_tx.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {

/// Block view, broadcast to service channels, streams with witness.
class wire_block
  : public rpc::streamable
{
public:
    typedef std::shared_ptr<const wire_block> cptr;
    using view = system::chain::view::block;
    using txs = std::vector<wire_tx::cptr>;

    /// The broadcast method (the message itself is never serialized).
    static constexpr auto command = "block";

    DELETE_COPY_MOVE_DESTRUCT(wire_block);

    inline wire_block(const view::cptr& block, bool organized) NOEXCEPT
      : block_(block), organized_(organized), txs_(to_txs(block))
    {
    }

    /// The block view.
    inline const view::cptr& block() const NOEXCEPT
    {
        return block_;
    }

    /// The block was organized (otherwise reorganized).
    inline bool organized() const NOEXCEPT
    {
        return organized_;
    }

    /// The block's transactions, each sharing ownership of the block.
    inline const txs& transactions() const NOEXCEPT
    {
        return txs_;
    }

    inline size_t size() const NOEXCEPT override
    {
        return block_->serialized_size(true);
    }

    inline void write(system::write::bytes::fast& sink) const NOEXCEPT override
    {
        block_->to_data(sink, true);
    }

private:
    static inline txs to_txs(const view::cptr& block) NOEXCEPT
    {
        txs out{};
        out.reserve(block->transactions());
        for (const auto& tx: block->views())
        {
            const wire_tx::view::cptr view{ block, &tx };
            out.push_back(system::emplace_shared<const wire_tx>(view));
        }

        return out;
    }

    // These are thread safe.
    const view::cptr block_;
    const bool organized_;
    const txs txs_;
};

} // namespace network
} // namespace libbitcoin

#endif
