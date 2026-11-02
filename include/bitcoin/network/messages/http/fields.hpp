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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_HTTP_FIELDS_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_HTTP_FIELDS_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/http/enums/field.hpp>

namespace libbitcoin {
namespace network {
namespace http {

/// http header fields: OWS is SP and HTAB (less than ascii).
const system::string_list http_whitespace{ " ", "\t" };

/// True if the ascii texts are equal ignoring case.
BCT_API bool iequals(const std::string_view& left,
    const std::string_view& right) NOEXCEPT;

/// True if the comma separated list contains the token, ignoring case.
BCT_API bool has_token(const std::string_view& list,
    const std::string_view& token) NOEXCEPT;

/// Not thread safe, non-virtual.
/// Header fields in serialization order, names compared ignoring case.
class BCT_API fields
{
public:
    struct entry
    {
        /// The field of the name, unknown if not enumerated.
        field id;
        std::string name;
        std::string value;
    };

    using container = std::vector<entry>;
    using const_iterator = container::const_iterator;

    DEFAULT_COPY_MOVE_DESTRUCT(fields);
    fields() NOEXCEPT = default;

    /// Replace all of the name with one value.
    void set(field name, const std::string_view& value) NOEXCEPT;
    void set(const std::string_view& name,
        const std::string_view& value) NOEXCEPT;

    /// Add a value, following any existing values of the name.
    void insert(field name, const std::string_view& value) NOEXCEPT;
    void insert(const std::string_view& name,
        const std::string_view& value) NOEXCEPT;

    /// Remove all of the name, returning the number removed.
    size_t erase(field name) NOEXCEPT;
    size_t erase(const std::string_view& name) NOEXCEPT;

    /// The first of the name, or end().
    const_iterator find(field name) const NOEXCEPT;
    const_iterator find(const std::string_view& name) const NOEXCEPT;

    /// The number of the name.
    size_t count(field name) const NOEXCEPT;
    size_t count(const std::string_view& name) const NOEXCEPT;

    /// The first value of the name, empty if none.
    std::string_view operator[](field name) const NOEXCEPT;
    std::string_view operator[](const std::string_view& name) const NOEXCEPT;

    /// Remove all fields.
    void clear() NOEXCEPT;

    const_iterator begin() const NOEXCEPT;
    const_iterator end() const NOEXCEPT;
    bool empty() const NOEXCEPT;
    size_t size() const NOEXCEPT;

private:
    void add(field id, const std::string_view& name,
        const std::string_view& value) NOEXCEPT;

    container fields_{};
};

/// Does the request have an attachment.
BCT_API bool has_attachment(const fields& header) NOEXCEPT;

/// Does the header include the required websocket upgrade request values.
BCT_API bool is_websocket_upgrade(const fields& header) NOEXCEPT;

/// Generate the required sec_websocket_key response value.
BCT_API std::string to_websocket_accept(const fields& header) NOEXCEPT;

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
