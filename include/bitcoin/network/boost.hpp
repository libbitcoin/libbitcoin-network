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
#ifndef LIBBITCOIN_NETWORK_BOOST_HPP
#define LIBBITCOIN_NETWORK_BOOST_HPP

#include <bitcoin/network/version.hpp>

// "By default, enable_current_exception and enable_error_info are integrated
// directly in the throw_exception function. Defining BOOST_EXCEPTION_DISABLE
// disables this integration."
// www.boost.org/doc/libs/1_78_0/libs/exception/doc/configuration_macros.html
// This does not prevent interfaces that are documented to throw from doing so.
// It only prevents boost from internally wrapping the exception object with
// another class (in boost/throw_exception.hpp). Nearly all instances of the
// internal boost exceptions affecting this library occur in streambuf and are
// caught and presumed discarded in std::istream (standards allow propagation).
// See more comments in streamers.hpp on streams that may throw exceptions.
// Must be set on the command line to ensure it is captured by all includes.
////#define BOOST_EXCEPTION_DISABLE

// Avoid namespace conflict between boost::placeholders and std::placeholders.
// This arises when including <functional>, which declares std::placeholders.
// www.boost.org/doc/libs/1_78_0/boost/bind.hpp
#define BOOST_BIND_NO_PLACEHOLDERS

// Include boost in cpp files only from here, so placeholders exclusion works.
// Avoid use in header includes due to warning repetition (boost/format.hpp).
#include <boost/asio.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/regex.hpp>
#include <boost/system/error_code.hpp>

#endif
