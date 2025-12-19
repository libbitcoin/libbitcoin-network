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
#include <bitcoin/network/rpc/rpc.hpp>

#include <variant>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/rpc/enums/version.hpp>

// boost::json parse/seralize is not exception safe.

namespace libbitcoin {
namespace network {
namespace rpc {

// version
// ----------------------------------------------------------------------------

DEFINE_JSON_FROM_TAG(version)
{
    switch (instance)
    {
        case version::v1: value = "1.0"; break;
        case version::v2: value = "2.0"; break;
        default: value = {}; break;
    }
}

DEFINE_JSON_TO_TAG(version)
{
    if (value.is_string())
    {
        const auto& text = value.as_string();
        if (text == "1.0") return version::v1;
        if (text == "2.0") return version::v2;
    }

    return version::invalid;
}

// value_t
// ----------------------------------------------------------------------------

DEFINE_JSON_FROM_TAG(value_t)
{
    // In the general model, all numbers serialize to double.
    std::visit(overload
    {
        [&](null_t) NOEXCEPT
        {
            value = nullptr;
        },
        [&](boolean_t visit) NOEXCEPT
        {
            value = visit;
        },
        [&](number_t visit) NOEXCEPT
        {
            value = visit;
        },
        [&](int8_t visit) THROWS
        {
            value = value_from(visit).as_double();
        },
        [&](int16_t visit) THROWS
        {
            value = value_from(visit).as_double();
        },
        [&](int32_t visit) THROWS
        {
            value = value_from(visit).as_double();
        },
        [&](int64_t visit) THROWS
        {
            value = value_from(visit).as_double();
        },
        [&](uint8_t visit) THROWS
        {
            value = value_from(visit).as_double();
        },
        [&](uint16_t visit) THROWS
        {
            value = value_from(visit).as_double();
        },
        [&](uint32_t visit) THROWS
        {
            value = value_from(visit).as_double();
        },
        [&](uint64_t visit) THROWS
        {
            value = value_from(visit).as_double();
        },

        [&](const string_t& visit) THROWS
        {
            value = visit;
        },
        [&](const array_t& visit) THROWS
        {
            value = value_from(visit);
        },
        [&](const object_t& visit) THROWS
        {
            value = value_from(visit);
        }
    }, instance.value());
}

DEFINE_JSON_TO_TAG(value_t)
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
        // In the general model, all numbers deserialize from double.
        return { std::in_place_type<number_t>, value.to_number<number_t>() };
    }
    else if (value.is_string())
    {
        return { std::in_place_type<string_t>, value.as_string() };
    }
    else if (value.is_array())
    {
        return { std::in_place_type<array_t>, value_to<array_t>(value) };
    }
    else if (value.is_object())
    {
        return { std::in_place_type<object_t>, value_to<object_t>(value) };
    }

    throw ostream_exception{ "value_t" };
}

// identity_t
// ----------------------------------------------------------------------------

DEFINE_JSON_FROM_TAG(identity_t)
{
    std::visit(overload
    {
        [&](null_t) NOEXCEPT
        {
            value = boost::json::value{};
        },
        [&](code_t visit) NOEXCEPT
        {
            value = visit;
        },
        [&](const string_t& visit) THROWS
        {
            value = visit;
        }
    }, instance);
}

DEFINE_JSON_TO_TAG(identity_t)
{
    if (value.is_null())
    {
        return { null_t{} };
    }
    else if (value.is_number())
    {
        return { code_t{ value.to_number<code_t>() } };
    }
    else if (value.is_string())
    {
        return { string_t{ value.as_string() } };
    }

    throw ostream_exception{ "identity_t" };
}

// request_t
// ----------------------------------------------------------------------------

DEFINE_JSON_FROM_TAG(request_t)
{
    boost::json::object object{};

    if (instance.jsonrpc != version::undefined)
    {
        object["jsonrpc"] = value_from(instance.jsonrpc);
    }

    if (instance.id.has_value())
    {
        object["id"] = value_from(instance.id.value());
    }

    if (!instance.method.empty())
    {
        object["method"] = value_from(instance.method);
    }

    if (instance.params.has_value())
    {
        if (const auto& params = instance.params.value();
            std::holds_alternative<array_t>(params))
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

// Bogus warnings on `it` not checked.
BC_PUSH_WARNING(NO_UNGUARDED_POINTERS)

DEFINE_JSON_TO_TAG(request_t)
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
        request.method = it->value().as_string();
    }

    // id
    if (const auto it = object.find("id"); it != object.end())
    {
        request.id = value_to<identity_t>(it->value());
    }

    // params
    if (const auto it = object.find("params"); it != object.end())
    {
        if (const auto& params = it->value(); params.is_array())
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

DEFINE_JSON_FROM_TAG(response_t)
{
    boost::json::object object{};

    if (instance.jsonrpc != version::undefined)
    {
        object["jsonrpc"] = value_from(instance.jsonrpc);
    }

    if (instance.id.has_value())
    {
        object["id"] = value_from(instance.id.value());
    }

    if (instance.error.has_value())
    {
        const auto& result = instance.error.value();

        object["error"] =
        {
            { "code", result.code },
            { "message", result.message }
        };

        if (result.data.has_value())
        {
            object["data"] = value_from(result.data.value());
        }
    }
    else if (instance.jsonrpc == version::v1 || 
        instance.jsonrpc == version::undefined)
    {
        object["error"] = boost::json::value{};
    }

    if (instance.result.has_value())
    {
        object["result"] = value_from(instance.result.value());
    }
    else if (instance.jsonrpc == version::v1 ||
        instance.jsonrpc == version::undefined)
    {
        object["result"] = boost::json::value{};
    }

    value = object;
}

DEFINE_JSON_TO_TAG(response_t)
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
        if (const auto& id = it->value(); id.is_null())
        {
            response.id = { null_t{} };
        }
        else if (id.is_number())
        {
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

BC_POP_WARNING()

} // namespace rpc
} // namespace network
} // namespace libbitcoin
