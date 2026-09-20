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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_RPC_SIZE_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_RPC_SIZE_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/rpc/model.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

/// Estimated serialized size of a message, which sizes the serialization
/// buffer of its writer. Text is measured, all other values are charged the
/// element, as are the quotation and delimiters of each element.
constexpr size_t element_size = 16;
constexpr size_t envelope_size = 64;

BCT_API size_t to_size(const value_t& value) NOEXCEPT;
BCT_API size_t to_size(const array_t& values) NOEXCEPT;
BCT_API size_t to_size(const object_t& values) NOEXCEPT;
BCT_API size_t to_size(const params_option& params) NOEXCEPT;
BCT_API size_t to_size(const response_t& message) NOEXCEPT;
BCT_API size_t to_size(const request_t& message) NOEXCEPT;

} // namespace rpc
} // namespace network
} // namespace libbitcoin

#endif
