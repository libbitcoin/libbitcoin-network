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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_JSON_SERIALIZER_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_JSON_SERIALIZER_HPP

#include <sstream>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/json/types.hpp>

namespace libbitcoin {
namespace network {
namespace json {

/// Serialize a request_t or response_t to a JSON string.
/// Injectable output stream type (e.g. std::ostringstream).
template <class Model, class OStream = std::ostringstream>
class serializer
{
public:
    /// Serializes the model to a compact JSON string.
    static string_t write(const Model& model) NOEXCEPT
    {
        OStream out{};
        serializer<Model, OStream> writer(out);
        writer.put(model);
        out.flush();
        return out.str();
    }

protected:
    serializer(OStream& stream) NOEXCEPT 
      : stream_(stream)
    {
    }

    void put(const Model& model) NOEXCEPT
    {
        try
        {
            if constexpr (std::is_same_v<Model, request_t>)
                put_request(model);
            else if constexpr (std::is_same_v<Model, response_t>)
                put_response(model);
        }
        catch (const std::exception&)
        {
            stream_.setstate(std::ios::badbit);
        }
    }

    using keys_t = std::vector<string_t>;
    const keys_t sorted_keys(const object_t& object) NOEXCEPT;
    inline void put_tag(const std::string_view& tag) THROWS;
    inline void put_comma(bool condition) THROWS;
    void put_code(code_t value) THROWS;
    void put_double(number_t value) THROWS;
    void put_version(version value) THROWS;
    void put_string(const std::string_view& text) THROWS;
    void put_key(const std::string_view& key) THROWS;
    void put_id(const identity_t& id) THROWS;
    void put_value(const value_t& value) THROWS;
    void put_error(const result_t& error) THROWS;
    void put_object(const object_t& object) THROWS;
    void put_array(const array_t& array) THROWS;
    void put_request(const request_t& request) THROWS;
    void put_response(const response_t& response) THROWS;

private:
    OStream& stream_;
};

////DECLARE_JSON_VALUE_CONVERTORS(request_t);
////DECLARE_JSON_VALUE_CONVERTORS(response_t);

} // namespace json
} // namespace network
} // namespace libbitcoin

#define TEMPLATE template <class Model, class OStream>
#define CLASS serializer<Model, OStream>

#include <bitcoin/network/impl/messages/json/serializer.ipp>

#undef CLASS
#undef TEMPLATE


#endif
