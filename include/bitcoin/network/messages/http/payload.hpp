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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_HTTP_PAYLOAD_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_HTTP_PAYLOAD_HPP

#include <optional>
#include <variant>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/json/body.hpp>

// TODO: rename to value_type.

namespace libbitcoin {
namespace network {
namespace http {

using empty_value = empty_body::value_type;
using json_value = json_body::value_type;
using data_value = data_body::value_type;
using file_value = file_body::value_type;
using string_value = string_body::value_type;
using variant_value = std::variant
<
    empty_value,
    json_value,
    data_value,
    file_value,
    string_value
>;

/// No size(), forces chunked encoding for all types.
/// The pass-thru body(), reader populates in construct.
struct payload
{
    /// Allow default construct (empty optional).
    payload() NOEXCEPT = default;

    /// Forwarding constructors for in-place variant construction.
    FORWARD_VARIANT_CONSTRUCT(payload, inner_)
    FORWARD_VARIANT_ASSIGNMENT(payload, inner_)
    FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(payload, empty_value, inner_)
    FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(payload, json_value, inner_)
    FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(payload, data_value, inner_)
    FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(payload, file_value, inner_)
    FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(payload, string_value, inner_)

    bool has_value() NOEXCEPT
    {
        return inner_.has_value();
    }

    variant_value& value() NOEXCEPT
    {
        return inner_.value();
    }

    const variant_value& value() const NOEXCEPT
    {
        return inner_.value();
    }

private:
    std::optional<variant_value> inner_{};
};

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
