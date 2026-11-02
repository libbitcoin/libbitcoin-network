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
#ifndef LIBBITCOIN_NETWORK_IO_IO_HPP
#define LIBBITCOIN_NETWORK_IO_IO_HPP

// Asynchronous i/o over iocp (windows), epoll (linux) or kqueue (apple).
// ----------------------------------------------------------------------------
// context:         runs handlers and completes operations on run() threads.
// executor/strand: posts and dispatches to a context, or to a strand of it.
// steady_timer:    waits for an expiry on the steady clock.
// socket:          tcp stream (connect, read, write, shutdown).
// acceptor:        tcp listener.
// resolver:        host name resolution on a context thread.
// operations:      composed reads and writes over any stream.

#include <bitcoin/network/io/buffer.hpp>
#include <bitcoin/network/io/context.hpp>
#include <bitcoin/network/io/operations.hpp>
#include <bitcoin/network/io/socket.hpp>
#include <bitcoin/network/io/steady_timer.hpp>
#include <bitcoin/network/io/strand.hpp>

#endif
