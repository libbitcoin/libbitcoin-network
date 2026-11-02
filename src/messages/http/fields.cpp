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
#include <bitcoin/network/messages/http/fields.hpp>

#include <algorithm>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/http/enums/field.hpp>

namespace libbitcoin {
namespace network {
namespace http {

using namespace system;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

constexpr char lower(char character) NOEXCEPT
{
    return (character >= 'A' && character <= 'Z') ?
        static_cast<char>(character + ('a' - 'A')) : character;
}

bool iequals(const std::string_view& left,
    const std::string_view& right) NOEXCEPT
{
    return left.size() == right.size() && std::equal(left.begin(),
        left.end(), right.begin(), [](char first, char second) NOEXCEPT
        {
            return lower(first) == lower(second);
        });
}

bool has_token(const std::string_view& list,
    const std::string_view& token) NOEXCEPT
{
    return std::ranges::any_of(split(list, { "," }, http_whitespace),
        [&token](const auto& item) NOEXCEPT
        {
            return iequals(item, token);
        });
}

// fields
// ----------------------------------------------------------------------------

void fields::set(field name, const std::string_view& value) NOEXCEPT
{
    erase(name);
    add(name, to_string(name), value);
}

void fields::set(const std::string_view& name,
    const std::string_view& value) NOEXCEPT
{
    erase(name);
    add(to_field(name), name, value);
}

void fields::insert(field name, const std::string_view& value) NOEXCEPT
{
    add(name, to_string(name), value);
}

void fields::insert(const std::string_view& name,
    const std::string_view& value) NOEXCEPT
{
    add(to_field(name), name, value);
}

size_t fields::erase(field name) NOEXCEPT
{
    return erase(to_string(name));
}

size_t fields::erase(const std::string_view& name) NOEXCEPT
{
    return std::erase_if(fields_, [&name](const auto& item) NOEXCEPT
    {
        return iequals(item.name, name);
    });
}

fields::const_iterator fields::find(field name) const NOEXCEPT
{
    return find(to_string(name));
}

fields::const_iterator fields::find(
    const std::string_view& name) const NOEXCEPT
{
    return std::ranges::find_if(fields_, [&name](const auto& item) NOEXCEPT
    {
        return iequals(item.name, name);
    });
}

size_t fields::count(field name) const NOEXCEPT
{
    return count(to_string(name));
}

size_t fields::count(const std::string_view& name) const NOEXCEPT
{
    return possible_narrow_sign_cast<size_t>(std::ranges::count_if(fields_,
        [&name](const auto& item) NOEXCEPT
        {
            return iequals(item.name, name);
        }));
}

std::string_view fields::operator[](field name) const NOEXCEPT
{
    return (*this)[to_string(name)];
}

std::string_view fields::operator[](
    const std::string_view& name) const NOEXCEPT
{
    const auto it = find(name);
    return it == end() ? std::string_view{} : std::string_view{ it->value };
}

void fields::clear() NOEXCEPT
{
    fields_.clear();
}

fields::const_iterator fields::begin() const NOEXCEPT
{
    return fields_.begin();
}

fields::const_iterator fields::end() const NOEXCEPT
{
    return fields_.end();
}

bool fields::empty() const NOEXCEPT
{
    return fields_.empty();
}

size_t fields::size() const NOEXCEPT
{
    return fields_.size();
}

// Values of a name are kept adjacent, in order of addition.
void fields::add(field id, const std::string_view& name,
    const std::string_view& value) NOEXCEPT
{
    const auto last = std::ranges::find_if(fields_.rbegin(), fields_.rend(),
        [&name](const auto& item) NOEXCEPT
        {
            return iequals(item.name, name);
        });

    const auto at = last == fields_.rend() ? fields_.end() : last.base();
    fields_.insert(at, entry{ id, std::string{ name }, std::string{ value } });
}

// utilities
// ----------------------------------------------------------------------------

// Simple test for leading "filename" assumes not other token starts with
// "filename" unless it is also an attachment (such as "filename*"). Otherwise
// the request is not valid anyway, so we can assume it has an attachment.
bool has_attachment(const fields& header) NOEXCEPT
{
    const auto disposition = header[field::content_disposition];
    const auto lower = ascii_to_lower(disposition);
    return std::ranges::any_of(split(lower, { ";" }, http_whitespace),
        [](auto& token) NOEXCEPT
        {
            return token.starts_with("filename");
        });
}

bool is_websocket_upgrade(const fields& header) NOEXCEPT
{
    if (header[field::sec_websocket_key].empty() ||
        header[field::upgrade] != "websocket")
        return false;

    return has_token(header[field::connection], "upgrade");
}

std::string to_websocket_accept(const fields& header) NOEXCEPT
{
    constexpr auto magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    const std::string key{ header[field::sec_websocket_key] };
    return key.empty() ? key : encode_base64(sha1_hash(key + magic));
}

BC_POP_WARNING()

} // namespace http
} // namespace network
} // namespace libbitcoin
