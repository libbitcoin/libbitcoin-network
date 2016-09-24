/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBBITCOIN_NETWORK_LOGGING_HPP
#define LIBBITCOIN_NETWORK_LOGGING_HPP

#include <fstream>
#include <iostream>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

/// Set up global logging.
BCT_API void initialize_logging(std::ofstream& debug, std::ofstream& error,
    std::ostream& output_stream, std::ostream& error_stream);

} // namespace network
} // namespace libbitcoin

#endif
