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
#include <bitcoin/network/config/credential.hpp>

#include <iostream>
#include <sstream>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace config {

using namespace system;

credential::credential() NOEXCEPT
  : username_{}, password_{}, methods_{}, digest_{}
{
}

credential::credential(const std::string& value) THROWS
  : credential()
{
    std::stringstream(value) >> *this;
}

// Properties.
// ----------------------------------------------------------------------------

const std::string& credential::username() const NOEXCEPT
{
    return username_;
}

const std::string& credential::password() const NOEXCEPT
{
    return password_;
}

const hash_digest& credential::digest() const NOEXCEPT
{
    return digest_;
}

const string_list& credential::methods() const NOEXCEPT
{
    return methods_;
}

// Methods.
// ----------------------------------------------------------------------------

bool credential::permitted(const std::string& method) const NOEXCEPT
{
    return methods_.empty() || contains(methods_, method);
}

std::string credential::to_string() const NOEXCEPT
{
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    const auto methods = join(methods_, ",");
    return username_ + ":" + password_ +
        (methods.empty() ? "" : ":" + methods);
    BC_POP_WARNING()
}

// Operators.
// ----------------------------------------------------------------------------

std::istream& operator>>(std::istream& input, credential& argument) THROWS
{
    std::string value{};
    input >> value;

    // Methods are optional, so a credential has two or three tokens.
    const auto tokens = split(value, ":", true, false);
    if (tokens.size() < two || tokens.size() > add1(two))
        throw istream_exception(value);

    argument.username_ = tokens.front();
    argument.password_ = tokens.at(one);
    // An empty method token is the same as no method token (all methods).
    if (tokens.size() == add1(two) && !tokens.back().empty())
        argument.methods_ = split(tokens.back(), ",");

    const auto header = "Basic " + encode_base64(argument.username_ + ":" +
        argument.password_);

    argument.digest_ = sha256_hash(header);
    return input;
}

std::ostream& operator<<(std::ostream& output,
    const credential& argument) NOEXCEPT
{
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    output << argument.to_string();
    BC_POP_WARNING()
    return output;
}

} // namespace config
} // namespace network
} // namespace libbitcoin
