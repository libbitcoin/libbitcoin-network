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
#include <bitcoin/network/messages/json/serializer.hpp>

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/json/enums/version.hpp>
#include <bitcoin/network/messages/json/types.hpp>

namespace libbitcoin {
namespace network {
namespace json {

#if defined (UNDEFINED)

using namespace boost::json;

// Uses std::to_chars() for number formatting.
// Also supports streaming and pretty print formatting.

// version
// ----------------------------------------------------------------------------

void tag_invoke(value_from_tag, value& value, version instance) THROWS
{
    switch (instance)
    {
        case version::v1: value = "1.0"; break;
        case version::v2: value = "2.0"; break;
        default: value = {}; break;
    }
}

version tag_invoke(value_to_tag<version>, const value& value) NOEXCEPT
{
    if (value.is_string())
    {
        const auto& str = value.get_string();
        if (str == "1.0") return version::v1;
        if (str == "2.0") return version::v2;
    }

    return version::invalid;
}

// value_t
// ----------------------------------------------------------------------------

// Project keys into sorted vector for predictable output.
const std::vector<string_t> sorted_keys(const object_t& object) NOEXCEPT
{
    std::vector<string_t> keys{};
    keys.reserve(object.size());
    for (const auto& pair : object)
        keys.push_back(pair.first);

    std::sort(keys.begin(), keys.end());
    return keys;
}

void tag_invoke(value_from_tag, value& value, const value_t& instance) THROWS
{
    std::visit(overload
    {
        [&] (null_t) NOEXCEPT
        {
            value = nullptr;
        },
        [&](boolean_t visit) NOEXCEPT
        {
            value = visit;
        },
        [&](number_t visit) NOEXCEPT
        {
            // This may serialize to scientific notation.
            // Number conversion is not controlled (boost default behavior).
            value = visit;
        },
        [&](const string_t& visit) THROWS
        {
            value = visit;
        },
        [&](const array_t& visit) THROWS
        {
            array array{};
            array.reserve(visit.size());
            for (const auto& element : visit)
                array.emplace_back(value_from(element));

            value = std::move(array);
        },
        [&](const object_t& visit) THROWS
        {
            object object{};
            object.reserve(visit.size());
            for (const auto& key: sorted_keys(visit))
                object[key] = value_from(key);

            value = std::move(object);
        }
    }, instance.inner);
}

value_t tag_invoke(value_to_tag<value_t>, const value& value) THROWS
{
    if (value.is_null())
    {
        return { std::in_place_type<null_t> };
    }
    else if (value.is_bool())
    {
        return { std::in_place_type<boolean_t>, value.as_bool() };
    }
    else if (value.is_number())
    {
        // Number conversion is not controlled (boost default behavior).
        return { std::in_place_type<number_t>, value.to_number<number_t>() };
    }
    else if (value.is_string())
    {
        return { std::in_place_type<string_t>, value.as_string() };
    }
    else if (value.is_array())
    {
        array_t array{};
        const auto& container = value.as_array();
        array.reserve(container.size());
        for (const auto& element : container)
            array.emplace_back(value_to<value_t>(element));

        return { std::in_place_type<array_t>, std::move(array) };
    }
    else if (value.is_object())
    {
        object_t object{};
        const auto& container = value.as_object();
        object.reserve(container.size());
        for (const auto& element : container)
            object.emplace(element.key(), value_to<value_t>(element.value()));

        return { std::in_place_type<object_t>, std::move(object) };
    }

    throw ostream_exception{ "value_to_tag<value_t>" };
}

// identity_t
// ----------------------------------------------------------------------------

void tag_invoke(value_from_tag, value& value, const identity_t& instance) THROWS
{
    std::visit(overload
    {
        [&] (null_t) NOEXCEPT
        {
            value = boost::json::value{};
        },
        [&](code_t visit) NOEXCEPT
        {
            // Number conversion is not controlled (boost default behavior).
            value = visit;
        },
        [&](const string_t& visit) THROWS
        {
            value = visit;
        }
    }, instance);
}

identity_t tag_invoke(value_to_tag<identity_t>, const value& value) THROWS
{
    if (value.is_null())
    {
        return { null_t{} };
    }
    else if (value.is_number())
    {
        // Number conversion is not controlled (boost default behavior).
        return { code_t{ value.to_number<code_t>() } };
    }
    else if (value.is_string())
    {
        return { string_t{ value.as_string() } };
    }

    throw ostream_exception{ "value_to_tag<identity_t>" };
}

// request_t
// ----------------------------------------------------------------------------

void tag_invoke(value_from_tag, value& value, const request_t& request) NOEXCEPT
{
    boost::json::object object{};

    if (request.jsonrpc != version::undefined)
    {
        object["jsonrpc"] = value_from(request.jsonrpc);
    }

    if (request.id.has_value())
    {
        object["id"] = value_from(request.id.value());
    }

    if (!request.method.empty())
    {
        object["method"] = value_from(request.method);
    }

    if (request.params.has_value())
    {
        const auto& params = request.params.value();

        if (std::holds_alternative<array_t>(params))
        {
            object["params"] = value_from(std::get<array_t>(params));
        }
        else
        {
            object["params"] = value_from(std::get<object_t>(params));
        }
    }

    value = object;
}

request_t tag_invoke(value_to_tag<request_t>, const value& value) NOEXCEPT
{
    request_t request{};
    const auto& object = value.as_object();

    // jsonrpc
    if (const auto it = object.find("jsonrpc"); it != object.end())
    {
        request.jsonrpc = value_to<version>(it->value());
    }

    // method
    if (const auto it = object.find("method"); it != object.end())
    {
        request.method = it->value().get_string();
    }

    // id
    if (const auto it = object.find("id"); it != object.end())
    {
        request.id = value_to<identity_t>(it->value());
    }

    // params
    if (const auto it = object.find("params"); it != object.end())
    {
        const auto& params = it->value();
        if (params.is_array())
        {
            request.params = params_t
            {
                std::in_place_type<array_t>, value_to<array_t>(params)
            };
        }
        else if (params.is_object())
        {
            request.params = params_t
            {
                std::in_place_type<object_t>, value_to<object_t>(params)
            };
        }
    }

    return request;
}

// response_t
// ----------------------------------------------------------------------------

void tag_invoke(value_from_tag, value& value, const response_t& response) NOEXCEPT
{
    boost::json::object object{};

    if (response.jsonrpc != version::undefined)
    {
        object["jsonrpc"] = value_from(response.jsonrpc);
    }

    if (response.id.has_value())
    {
        object["id"] = value_from(response.id.value());
    }

    if (response.error.has_value())
    {
        const auto& result = response.error.value();
        boost::json::object error{};
        error["code"] = result.code;
        error["message"] = result.message;

        if (result.data.has_value())
        {
            error["data"] = value_from(result.data.value());
        }

        object["error"] = error;
    }

    if (response.result.has_value())
    {
        object["result"] = value_from(response.result.value());
    }

    value = object;
}

response_t tag_invoke(value_to_tag<response_t>, const value& value) NOEXCEPT
{
    response_t response{};
    const auto& object = value.as_object();

    // jsonrpc
    if (const auto it = object.find("jsonrpc"); it != object.end())
    {
        response.jsonrpc = value_to<version>(it->value());
    }

    // id
    if (const auto it = object.find("id"); it != object.end())
    {
        const auto& id = it->value();
        if (id.is_null())
        {
            response.id = { null_t{} };
        }
        else if (id.is_number())
        {
            // int64_t conversion is not controlled (boost default behavior).
            response.id = { id.to_number<code_t>() };
        }
        else if (id.is_string())
        {
            response.id = { string_t{ id.as_string() } };
        }
    }

    // error
    if (const auto it = object.find("error"); it != object.end())
    {
        result_t result{};
        const auto& error = it->value().as_object();

        if (const auto code_it = error.find("code");
            code_it != error.end())
        {
            // int64_t conversion is not controlled (boost default behavior).
            result.code = code_it->value().to_number<code_t>();
        }

        if (const auto message_it = error.find("message");
            message_it != error.end())
        {
            result.message = message_it->value().as_string();
        }

        if (const auto data_it = error.find("data");
            data_it != error.end())
        {
            result.data = value_to<value_option>(data_it->value());
        }

        response.error = result;
    }

    // result
    if (const auto it = object.find("result"); it != object.end())
    {
        response.result = value_to<value_option>(it->value());
    }

    return response;
}

#endif // UNDEFINED

} // namespace json
} // namespace network
} // namespace libbitcoin
