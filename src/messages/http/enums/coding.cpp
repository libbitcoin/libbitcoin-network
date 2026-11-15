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
#include <bitcoin/network/messages/http/enums/coding.hpp>

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/http/fields.hpp>

namespace libbitcoin {
namespace network {
namespace http {

coding to_coding(const std::string_view& name) NOEXCEPT
{
    if (iequals(name, "identity"))
        return coding::identity;

    if (iequals(name, "gzip") || iequals(name, "x-gzip"))
        return coding::gzip;

    if (iequals(name, "deflate"))
        return coding::deflate;

    return coding::unknown;
}

std::string_view to_string(coding value) NOEXCEPT
{
    switch (value)
    {
        case coding::identity:
            return "identity";
        case coding::gzip:
            return "gzip";
        case coding::deflate:
            return "deflate";
        case coding::unknown:
        default:
            return {};
    }
}

} // namespace http
} // namespace network
} // namespace libbitcoin
