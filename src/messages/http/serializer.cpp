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
#include <bitcoin/network/messages/http/serializer.hpp>

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/http/header.hpp>

namespace libbitcoin {
namespace network {
namespace http {

using namespace system;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

static std::string version_text(uint32_t version) NOEXCEPT
{
    return "HTTP/" + serialize(version / 10u) + "." + serialize(version % 10u);
}

static void append_fields(std::string& out, const fields& header) NOEXCEPT
{
    for (const auto& entry: header)
    {
        out += entry.name;
        out += ": ";
        out += entry.value;
        out += "\r\n";
    }

    out += "\r\n";
}

std::string serialize_header(const request_header& header) NOEXCEPT
{
    std::string out{ header.method_string() };
    out += " ";
    out += header.target();
    out += " ";
    out += version_text(header.version());
    out += "\r\n";
    append_fields(out, header);
    return out;
}

std::string serialize_header(const response_header& header) NOEXCEPT
{
    auto out = version_text(header.version());
    out += " ";
    out += serialize(header.result_int());
    out += " ";
    out += header.reason();
    out += "\r\n";
    append_fields(out, header);
    return out;
}

std::string chunk_header(size_t size) NOEXCEPT
{
    constexpr std::string_view digits{ "0123456789abcdef" };
    std::string out{};
    do
    {
        out.insert(out.begin(), digits[size % 16u]);
        size /= 16u;
    } while (!is_zero(size));

    return out + "\r\n";
}

BC_POP_WARNING()

} // namespace http
} // namespace network
} // namespace libbitcoin
