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
#include <bitcoin/network/messages/json/serialize.hpp>

#include <sstream>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/json/enums/version.hpp>
#include <bitcoin/network/messages/json/types.hpp>

namespace libbitcoin {
namespace network {
namespace json {
    
using stream = std::ostringstream;

// local
// ----------------------------------------------------------------------------

// Project keys into sorted vector for predictable output.
const std::vector<string_t> sorted_keys(const object_t& object) NOEXCEPT
{
    std::vector<string_t> keys{};
    keys.reserve(object.size());
    for (const auto& pair: object)
        keys.push_back(pair.first);

    std::sort(keys.begin(), keys.end());
    return keys;
}

void put_version(stream& out, version value) THROWS
{
    switch (value)
    {
        case version::v1: out << R"("1.0")"; break;
        case version::v2: out << R"("2.0")"; break;
        default: out << R"("")";
    }
}

stream& put_string(stream& out, const std::string_view& text) THROWS
{
    out << '"';

    for (const char character: text)
    {
        switch (character)
        {
            case '"' : out << R"(\")"; break;
            case '\\': out << R"(\\)"; break;
            case '\b': out << R"(\b)"; break;
            case '\f': out << R"(\f)"; break;
            case '\n': out << R"(\n)"; break;
            case '\r': out << R"(\r)"; break;
            case '\t': out << R"(\t)"; break;
            default: out << character;
        }
    }

    out << '"';
    return out;
}

void put_key(stream& out, const std::string_view& key) THROWS
{
    // keys are dynamic, so require escaping.
    put_string(out, key) << ":";
}

inline stream& put_tag(stream& out, const std::string_view& tag) THROWS
{
    // tags are literal, so can bypass escaping.
    out << '\"' << tag << "\":";
    return out;
}

inline void put_comma(stream& out, bool condition) THROWS
{
    if (condition) out << ",";
}

void put_id(stream& out, const id_t& id) THROWS
{
    std::visit(overload
    {
        [&](null_t)
        {
            out << "null";
        },
        [&](code_t visit)
        {
            out << visit;
        },
        [&](const string_t& visit)
        {
            put_string(out, visit);
        }
    }, id);
}

// value_t<object_t> and value_t<array_t> are stored as string blobs.
void put_value(stream& out, const value_t& value) THROWS
{
    std::visit(overload
    {
        [&](null_t)
        {
            out << "null";
        },
        [&](boolean_t visit)
        {
            out << (visit ? "true" : "false");
        },
        [&](number_t visit)
        {
            out << visit;
        },
        [&](const string_t& visit)
        {
            put_string(out, visit);
        },
        [&](const array_t& visit)
        {
            if (visit.empty())
            {
                throw ostream_exception{ "empty-array" };
                return;
            }

            const auto& first = visit.front().inner;
            if (!std::holds_alternative<string_t>(first))
            {
                throw ostream_exception{ "non-string-array-value" };
                return;
            }

            out << std::get<string_t>(first);
        },
        [&](const object_t& visit)
        {
            if (visit.empty())
            {
                throw ostream_exception{ "empty-object" };
                return;
            }

            const auto& first = visit.begin()->second.inner;
            if (!std::holds_alternative<string_t>(first))
            {
                throw ostream_exception{ "non-string-object-value" };
                return;
            }

            out << std::get<string_t>(first);
        }
    }, value.inner);
}

void put_error(stream& out, const result_t& error) THROWS
{
    out << '{';
    put_tag(out, "code") << error.code;
    put_comma(out, true);
    put_tag(out, "message");
    put_string(out, error.message);

    if (error.data.has_value())
    {
        put_comma(out, true);
        put_tag(out, "data");
        put_value(out, error.data.value());
    }

    out << '}';
}

void put_object(stream& out, const object_t& object) THROWS
{
    out << '{';

    auto first = true;
    for (const auto& key: sorted_keys(object))
    {
        put_comma(out, !first);
        put_key(out, key);
        put_value(out, object.at(key));
        first = false;
    }

    out << '}';
}

void put_array(stream& out, const array_t& array) THROWS
{
    out << '[';

    for (size_t index{}; index < array.size(); ++index)
    {
        put_comma(out, !is_zero(index));
        put_value(out, array.at(index));
    }

    out << ']';
}

void put_request(stream& out, const request_t& request) THROWS
{
    out << '{';

    const auto has_jsonrpc = (request.jsonrpc != version::undefined);
    if (has_jsonrpc)
    {
        put_tag(out, "jsonrpc");
        put_version(out, request.jsonrpc);
    }

    if (request.id.has_value())
    {
        put_comma(out, has_jsonrpc);
        put_tag(out, "id");
        put_id(out, request.id.value());
    }

    if (!request.method.empty())
    {
        put_comma(out, has_jsonrpc || request.id.has_value());
        put_tag(out, "method");
        put_string(out, request.method);
    }

    if (request.params.has_value())
    {
        put_comma(out, !has_jsonrpc || request.id.has_value() ||
            !request.method.empty());
        put_tag(out, "params");

        const auto& params = request.params.value();
        if (std::holds_alternative<array_t>(params))
            put_array(out, std::get<array_t>(params));
        else
            put_object(out, std::get<object_t>(params));
    }

    out << '}';
}

void put_response(stream& out, const response_t& response) THROWS
{
    out << '{';

    const auto has_jsonrpc = (response.jsonrpc != version::undefined);
    if (has_jsonrpc)
    {
        put_tag(out, "jsonrpc");
        put_version(out, response.jsonrpc);
    }

    if (response.id.has_value())
    {
        put_comma(out, has_jsonrpc);
        put_tag(out, "id");
        put_id(out, response.id.value());
    }

    if (response.error.has_value())
    {
        put_comma(out, has_jsonrpc || response.id.has_value());
        put_tag(out, "error");
        put_error(out, response.error.value());
    }

    if (response.result.has_value())
    {
        put_comma(out, has_jsonrpc || response.id.has_value() ||
            response.error.has_value());
        put_tag(out, "result");
        put_value(out, response.result.value());
    }

    out << '}';
}

// ----------------------------------------------------------------------------

string_t serialize(const request_t& request) NOEXCEPT
{
    stream out{};
    try
    {
        put_request(out, request);
        return out.str();
    }
    catch (const std::exception& e)
    {
        try
        {
            return e.what();
        }
        catch(...)
        {
            return "json request";
        }
    }
}

string_t serialize(const response_t& response) NOEXCEPT
{
    stream out{};
    try
    {
        put_response(out, response);
        return out.str();
    }
    catch (const std::exception& e)
    {
        try
        {
            return e.what();
        }
        catch (...)
        {
            return "json response";
        }
    }
}

} // namespace json
} // namespace network
} // namespace libbitcoin
