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
#include <bitcoin/network/messages/rpc/enums/target.hpp>

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/rpc/enums/method.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace rpc {

using namespace system::wallet;

bool is_origin_form(const std::string& target) NOEXCEPT
{
    // path-absolute ; begins with "/" but not "//"
    // datatracker.ietf.org/doc/html/rfc3986#section-3.3
    if (target.empty() || !target.starts_with("/") || target.starts_with("//"))
        return false;

    // TODO: boost::urls::parse_origin_form (exact).
    uri uri{};
    if (!uri.decode(target))
        return false;

    // Path-absolute and optional query.
    return !uri.has_scheme()
        && !uri.has_authority()
        && !uri.has_fragment();
}

bool is_absolute_form(const std::string& target) NOEXCEPT
{
    if (target.empty())
        return false;

    // TODO: boost::urls::parse_absolute_uri (exact).
    uri uri{};
    if (!uri.decode(target))
        return false;

    // Specific schemes required, uri normalizes to lower case.
    // At least http/s scheme and authority, optional path and query.
    const auto scheme = uri.scheme();
    return (scheme == "http" || scheme == "https")
        && uri.has_authority()
        && !uri.has_fragment();
}

// Used for CONNECT method.
bool is_authority_form(const std::string& target) NOEXCEPT
{
    if (target.empty())
        return false;

    // TODO: boost::urls::parse_authority (exact).
    uri uri{};
    if (!uri.decode(target))
        return false;

    // Authority only (current uri requires //, which is not allowed by http).
    return !uri.has_scheme()
        && uri.has_authority()
        && uri.path().empty()
        && !uri.has_query()
        && !uri.has_fragment();
}

// Used for OPTIONS method.
bool is_asterisk_form(const std::string& target) NOEXCEPT
{
    // Asterisk only.
    return target == "*";
}

target to_target(const std::string& value, method method) NOEXCEPT
{
    switch (method)
    {
        case method::get:
        case method::head:
        case method::post:
        case method::put:
        case method::delete_:
        case method::trace:
        {
            return is_origin_form(value) ?
                target::origin : (is_absolute_form(value) ?
                    target::absolute : target::undefined);
        }
        case method::options:
        {
            return is_asterisk_form(value) ?
                target::asterisk : (is_origin_form(value) ?
                    target::origin : (is_absolute_form(value) ?
                        target::absolute : target::undefined));
        }
        case method::connect:
        {
            return is_authority_form(value) ?
                target::authority : target::undefined;
        }
        default:
        case method::undefined:
            return target::undefined;
    }
}

} // namespace rpc
} // namespace messages
} // namespace network
} // namespace libbitcoin
