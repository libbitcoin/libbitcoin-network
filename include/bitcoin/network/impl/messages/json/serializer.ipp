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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_JSON_SERIALIZER_IPP
#define LIBBITCOIN_NETWORK_MESSAGES_JSON_SERIALIZER_IPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/json/enums/version.hpp>
#include <bitcoin/network/messages/json/types.hpp>

// TODO: provide << overloads for system::ostream string/char.
// TODO: convert int64_t and double to string before streaming.
// TODO: boost::json uses std::to_chars() for number formatting.

namespace libbitcoin {
namespace network {
namespace json {

// Project keys into sorted vector for predictable output.
TEMPLATE
const typename CLASS::keys_t CLASS::sorted_keys(const object_t& object) NOEXCEPT
{
    keys_t keys{};
    keys.reserve(object.size());
    for (const auto& pair: object)
        keys.push_back(pair.first);

    std::sort(keys.begin(), keys.end());
    return keys;
}

TEMPLATE
inline void CLASS::put_tag(const std::string_view& tag) THROWS
{
    // tags are literal, so can bypass escaping.
    stream_ << '\"' << tag << "\":";
}

TEMPLATE
inline void CLASS::put_comma(bool condition) THROWS
{
    if (condition) stream_ << ",";
}

TEMPLATE
void CLASS::put_code(code_t value) THROWS
{
    // << override (int64_t) that's not pre-formatted as string/char.
    stream_ << value;
}

TEMPLATE
void CLASS::put_double(number_t value) THROWS
{
    // << override (double) that's not pre-formatted as string/char.
    stream_ << value;
}

TEMPLATE
void CLASS::put_version(version value) THROWS
{
    switch (value)
    {
        case version::v1: stream_ << R"("1.0")"; break;
        case version::v2: stream_ << R"("2.0")"; break;
        default: stream_ << R"("")";
    }
}

TEMPLATE
void CLASS::put_string(const std::string_view& text) THROWS
{
    stream_ << '"';

    for (const char character: text)
    {
        switch (character)
        {
            case '"' : stream_ << R"(\")"; break;
            case '\\': stream_ << R"(\\)"; break;
            case '\b': stream_ << R"(\b)"; break;
            case '\f': stream_ << R"(\f)"; break;
            case '\n': stream_ << R"(\n)"; break;
            case '\r': stream_ << R"(\r)"; break;
            case '\t': stream_ << R"(\t)"; break;
            default: stream_ << character;
        }
    }

    stream_ << '"';
}

TEMPLATE
void CLASS::put_key(const std::string_view& key) THROWS
{
    // keys are dynamic, so require escaping.
    put_string(key);
    stream_ << ":";
}

TEMPLATE
void CLASS::put_id(const identity_t& id) THROWS
{
    std::visit(overload
    {
        [&](null_t)
        {
            stream_ << "null";
        },
        [&](code_t visit)
        {
            put_code(visit);
        },
        [&](const string_t& visit)
        {
            put_string(visit);
        }
    }, id);
}

// value_t<object_t> and value_t<array_t> are stored as string blobs.
TEMPLATE
void CLASS::put_value(const value_t& value) THROWS
{
    std::visit(overload
    {
        [&](null_t)
        {
            stream_ << "null";
        },
        [&](boolean_t visit)
        {
            stream_ << (visit ? "true" : "false");
        },
        [&](number_t visit)
        {
            put_double(visit);
        },
        [&](const string_t& visit)
        {
            put_string(visit);
        },
        [&](const array_t& visit)
        {
            if (visit.empty())
                throw ostream_exception{ "empty-array" };

            const auto& first = visit.front().inner;
            if (!std::holds_alternative<string_t>(first))
                throw ostream_exception{ "non-string-array-value" };

            stream_ << std::get<string_t>(first);
        },
        [&](const object_t& visit)
        {
            if (visit.empty())
                throw ostream_exception{ "empty-object" };

            const auto& first = visit.begin()->second.inner;
            if (!std::holds_alternative<string_t>(first))
                throw ostream_exception{ "non-string-object-value" };

            stream_ << std::get<string_t>(first);
        }
    }, value.inner);
}

TEMPLATE
void CLASS::put_error(const result_t& error) THROWS
{
    stream_ << '{';
    put_tag("code");
    put_code(error.code);
    put_comma(true);
    put_tag("message");
    put_string(error.message);

    if (error.data.has_value())
    {
        put_comma(true);
        put_tag("data");
        put_value(error.data.value());
    }

    stream_ << '}';
}

TEMPLATE
void CLASS::put_object(const object_t& object) THROWS
{
    stream_ << '{';

    auto first = true;
    for (const auto& key: sorted_keys(object))
    {
        put_comma(!first);
        put_key(key);
        put_value(object.at(key));
        first = false;
    }

    stream_ << '}';
}

TEMPLATE
void CLASS::put_array(const array_t& array) THROWS
{
    stream_ << '[';

    for (size_t index{}; index < array.size(); ++index)
    {
        put_comma(!is_zero(index));
        put_value(array.at(index));
    }

    stream_ << ']';
}

////boost::json::serialize(boost::json::value_from(request));
TEMPLATE
void CLASS::put_request(const request_t& request) THROWS
{
    stream_ << '{';

    const auto has_jsonrpc = (request.jsonrpc != version::undefined);
    if (has_jsonrpc)
    {
        put_tag("jsonrpc");
        put_version(request.jsonrpc);
    }

    if (request.id.has_value())
    {
        put_comma(has_jsonrpc);
        put_tag("id");
        put_id(request.id.value());
    }

    if (!request.method.empty())
    {
        put_comma(has_jsonrpc || request.id.has_value());
        put_tag("method");
        put_string(request.method);
    }

    if (request.params.has_value())
    {
        put_comma(has_jsonrpc || request.id.has_value() ||
            !request.method.empty());
        put_tag("params");

        const auto& params = request.params.value();
        if (std::holds_alternative<array_t>(params))
            put_array(std::get<array_t>(params));
        else
            put_object(std::get<object_t>(params));
    }

    stream_ << '}';
}

////boost::json::serialize(boost::json::value_from(response));
TEMPLATE
void CLASS::put_response(const response_t& response) THROWS
{
    stream_ << '{';

    const auto has_jsonrpc = (response.jsonrpc != version::undefined);
    if (has_jsonrpc)
    {
        put_tag("jsonrpc");
        put_version(response.jsonrpc);
    }

    if (response.id.has_value())
    {
        put_comma(has_jsonrpc);
        put_tag("id");
        put_id(response.id.value());
    }

    if (response.error.has_value())
    {
        put_comma(has_jsonrpc || response.id.has_value());
        put_tag("error");
        put_error(response.error.value());
    }

    if (response.result.has_value())
    {
        put_comma(has_jsonrpc || response.id.has_value() ||
            response.error.has_value());
        put_tag("result");
        put_value(response.result.value());
    }

    stream_ << '}';
}

} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
