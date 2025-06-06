/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_NETWORK_BOOST_HPP
#define LIBBITCOIN_NETWORK_BOOST_HPP

#include <bitcoin/network/version.hpp>

// Avoid namespace conflict between boost::placeholders and std::placeholders.
// This arises when including <functional>, which declares std::placeholders.
// www.boost.org/doc/libs/1_78_0/boost/bind.hpp
#define BOOST_BIND_NO_PLACEHOLDERS

// Include boost only from here, so placeholders exclusion works.
#include <boost/asio.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/system/error_code.hpp>

#endif
