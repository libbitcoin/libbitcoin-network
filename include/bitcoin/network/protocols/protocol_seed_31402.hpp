/////**
//// * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
//// *
//// * This file is part of libbitcoin.
//// *
//// * This program is free software: you can redistribute it and/or modify
//// * it under the terms of the GNU Affero General Public License as published by
//// * the Free Software Foundation, either version 3 of the License, or
//// * (at your option) any later version.
//// *
//// * This program is distributed in the hope that it will be useful,
//// * but WITHOUT ANY WARRANTY; without even the implied warranty of
//// * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//// * GNU Affero General Public License for more details.
//// *
//// * You should have received a copy of the GNU Affero General Public License
//// * along with this program.  If not, see <http://www.gnu.org/licenses/>.
//// */
////#ifndef LIBBITCOIN_NETWORK_PROTOCOL_SEED_31402_HPP
////#define LIBBITCOIN_NETWORK_PROTOCOL_SEED_31402_HPP
////
////#include <memory>
////#include <string>
////#include <bitcoin/system.hpp>
////#include <bitcoin/network/define.hpp>
////#include <bitcoin/network/messages/messages.hpp>
////#include <bitcoin/network/net/net.hpp>
////#include <bitcoin/network/protocols/protocol.hpp>
////
////namespace libbitcoin {
////namespace network {
////
////class session;
////
/////// Seeding protocol.
/////// Attach this to a channel immediately following seed handshake completion.
////class BCT_API protocol_seed_31402
////  : public protocol_timer, track<protocol_seed_31402>
////{
////public:
////    typedef std::shared_ptr<protocol_seed_31402> ptr;
////
////    protocol_seed_31402(const session& session, channel::ptr channel);
////
////    /// Start protocol (strand required).
////    void start() override;
////
////protected:
////    // Expose polymorphic start method from base.
////    using protocol_timer::start;
////
////    virtual void handle_complete(const code& ec);
////    virtual void handle_send_address(const code& ec);
////    virtual void handle_send_get_address(const code& ec);
////    virtual void handle_load_addresses(const code& ec);
////
////    virtual void handle_receive_address(const code& ec,
////        messages::address::ptr address);
////    virtual void handle_receive_get_address(const code& ec,
////        messages::get_address::ptr message);
////
////    const std::string& name() const override;
////
////    size_t events_;
////};
////
////} // namespace network
////} // namespace libbitcoin
////
////#endif
