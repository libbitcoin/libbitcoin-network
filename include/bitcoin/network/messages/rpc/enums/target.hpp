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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_RPC_TARGET_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_RPC_TARGET_HPP

#include <filesystem>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/async/async.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace rpc {

/// Enumeration of valid http 1.1 target types.
enum class target
{
    origin,
    absolute,
    authority,
    asterisk,
    unknown
};

/// "/index.html?field=value" (no authority).
BCT_API bool is_origin_form(const std::string& target) NOEXCEPT;

/// "scheme://www.boost.org/index.html?field=value" (no fragment).
BCT_API bool is_absolute_form(const std::string& target) NOEXCEPT;

/// Used for CONNECT method.
/// Requires leading "//", which is not allowed by parse_authority.
BCT_API bool is_authority_form(const std::string& target) NOEXCEPT;

/// Asterisk only.
/// Used for OPTIONS method.
BCT_API bool is_asterisk_form(const std::string& target) NOEXCEPT;

/// Validate method against target and return enumerated type of target.
BCT_API target to_target(const std::string& value, http::verb method) NOEXCEPT;

/// Sanitize base/target to ensure it remains strictly within base.
BCT_API std::filesystem::path sanitize_origin(
    const std::filesystem::path& base, const std::string& target) NOEXCEPT;

/// Returned file is closed if failed.
/// Sanitize base/target to ensure it remains strictly within base.
BCT_API http_file get_file_body(const std::filesystem::path&) NOEXCEPT;

/// Defaults to "application/octet-stream".
/// Return mime type (e.g. "image/jpeg") for given file system path.
BCT_API const std::string& get_mime_type(
    const std::filesystem::path& path) NOEXCEPT;

} // namespace rpc
} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
