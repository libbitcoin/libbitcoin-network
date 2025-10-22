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

#include <sstream>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/json/enums/version.hpp>
#include <bitcoin/network/messages/json/types.hpp>

namespace libbitcoin {
namespace network {
namespace json {

string_t to_string(version value) NOEXCEPT
{
    switch (value)
    {
        case version::v1:
            return "1.0";
        case version::v2:
            return "2.0";
        default:
            return {};
    }
}

// Since this is a utf-8 string, only escape required characters.
string_t quote_and_escape_string(const string_t& text) THROWS
{
    std::ostringstream out{};
    out << '"';

    for (const char character: text)
    {
        switch (character)
        {
            case '"' : out << R"(\")";
                break;
            case '\\': out << R"(\\)";
                break;
            case '\b': out << R"(\b)";
                break;
            case '\f': out << R"(\f)";
                break;
            case '\n': out << R"(\n)";
                break;
            case '\r': out << R"(\r)";
                break;
            case '\t': out << R"(\t)";
                break;
            default:
                out << character;
                break;
        }
    }

    out << '"';
    return out.str();
}

// Serializes value_t to JSON string, handling blobs as nested structures.
string_t serialize_value(const value_t& value) THROWS
{
    std::ostringstream out{};
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
            out << quote_and_escape_string(visit);
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

    return out.str();
}

string_t serialize_request(const request_t& request) THROWS
{
    std::ostringstream out{};
    out << '{';

    const auto jsonrpc = to_string(request.jsonrpc);
    const auto& method = request.method;

    if (!jsonrpc.empty())
        out << "\"jsonrpc\":\"" << jsonrpc << '"';

    if (!jsonrpc.empty() && !method.empty())
        out << ",";

    if (!method.empty())
        out << "\"method\":\"" << method << '"';

    if (request.id.has_value())
    {
        if (!jsonrpc.empty() || !method.empty())
            out << ",";

        out << "\"id\":";

        const auto& id = request.id.value();
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
                out << quote_and_escape_string(visit);
            }
        }, id);
    }

    if (request.params.has_value())
    {
        if (!jsonrpc.empty() || !method.empty() || request.id.has_value())
            out << ",";

        out << "\"params\":";

        const auto& params = request.params.value();
        if (std::holds_alternative<array_t>(params))
        {
            out << '[';

            const auto& parameters = std::get<array_t>(params);
            for (auto index = zero; index < parameters.size(); ++index)
            {
                if (!is_zero(index)) out << ',';
                out << serialize_value(parameters.at(index));
            }
            out << ']';
        }
        else
        {
            out << '{';

            auto first = true;
            const auto& parameters = std::get<object_t>(params);

            // Sort keys for predictable output.
            std::vector<string_t> keys{};
            keys.reserve(parameters.size());
            for (const auto& pair: parameters)
                keys.push_back(pair.first);

            std::sort(keys.begin(), keys.end());

            for (const auto& key: keys)
            {
                if (!first) out << ',';
                out << quote_and_escape_string(key);
                out << ":" << serialize_value(parameters.at(key));
                first = false;
            }

            out << '}';
        }
    }

    out << '}';
    return out.str();
}

string_t serialize_response(const response_t& response) THROWS
{
    // TODO: Implement response serialization.
    return to_string(response.jsonrpc);
}

// ----------------------------------------------------------------------------

string_t serialize(const request_t& request) NOEXCEPT
{
    try
    {
        return serialize_request(request);
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
    try
    {
        return serialize_response(response);
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
