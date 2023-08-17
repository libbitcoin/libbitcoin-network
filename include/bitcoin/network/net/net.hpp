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
#ifndef LIBBITCOIN_NETWORK_NET_NET_HPP
#define LIBBITCOIN_NETWORK_NET_NET_HPP

#include <bitcoin/network/net/acceptor.hpp>
#include <bitcoin/network/net/broadcaster.hpp>
#include <bitcoin/network/net/channel.hpp>
#include <bitcoin/network/net/connector.hpp>
#include <bitcoin/network/net/deadline.hpp>
#include <bitcoin/network/net/distributor.hpp>
#include <bitcoin/network/net/hosts.hpp>
#include <bitcoin/network/net/proxy.hpp>
#include <bitcoin/network/net/socket.hpp>

// The network classes are entirely lock free.

// Each acceptor, connector, and channel::socket(proxy) operates on an
// independent strand within a shared threadpool owned by the caller.
// Public handlers are invoked by asynchronous methods of these classes within
// the execution context of the respective strand. All public methods excluding
// start are thread safe. All stop methods are idempotent and asynchronous.

// Once stopped and all external pointers released, objects are freed. Stop
// should be called on each acceptor, connector, and channel before calling
// join. Stops are posted, ensuring work remains alive until stop complete.

#endif
