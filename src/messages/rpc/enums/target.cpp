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

using namespace boost::urls;
using namespace system::wallet;

bool is_origin_form(const std::string& target) NOEXCEPT
{
    try
    {
        // "/index.html?field=value" (no authority)
        return !parse_origin_form(target).has_error();
    }
    catch (const std::exception&)
    {
        // s.size() > url_view::max_size
        return false;
    }
}

bool is_absolute_form(const std::string& target) NOEXCEPT
{
    try
    {
        // "foo://www.boost.org/index.html?field=value" (no fragment)
        const auto uri = parse_absolute_uri(target);
        if (uri.has_error())
            return false;

        // Limit to http/s.
        const auto scheme = uri->scheme_id();
        return scheme == scheme::http || scheme == scheme::https;
    }
    catch (const std::exception&)
    {
        // s.size() > url_view::max_size
        return false;
    }
}

// Used for CONNECT method.
bool is_authority_form(const std::string& target) NOEXCEPT
{
    // Requires leading "//", which is not allowed by parse_authority.
    if (!target.starts_with("//"))
        return false;

    // "[ userinfo "@" ] host [ ":" port ]"
    const auto at = std::next(target.begin(), two);
    return !parse_authority(std::string_view{ at, target.end() }).has_error();
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
