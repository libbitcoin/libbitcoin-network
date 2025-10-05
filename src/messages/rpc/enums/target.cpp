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
#include <bitcoin/network/messages/rpc/enums/target.hpp>

#include <filesystem>
#include <unordered_map>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace rpc {

using namespace boost::urls;
using namespace boost::beast::http;

bool is_origin_form(const std::string& target) NOEXCEPT
{
    // Excludes leading "//", which is allowed by parse_origin_form.
    if (target.starts_with("//"))
        return false;

    try
    {
        // "/index.html?field=value" (no authority).
        return !parse_origin_form(target).has_error();
    }
    catch (const std::exception&)
    {
        // s.size() > url_view::max_size
        return false;
    }
}

bool is_absolute_form(const std::string& target) NOEXCEPT
{
    try
    {
        // "scheme://www.boost.org/index.html?field=value" (no fragment).
        const auto uri = parse_absolute_uri(target);
        if (uri.has_error())
            return false;

        // Limit to http/s.
        const auto scheme = uri->scheme_id();
        return scheme == scheme::http || scheme == scheme::https;
    }
    catch (const std::exception&)
    {
        // s.size() > url_view::max_size
        return false;
    }
}

// Used for CONNECT method.
bool is_authority_form(const std::string& target) NOEXCEPT
{
    // Requires leading "//", which is not allowed by parse_authority.
    if (!target.starts_with("//"))
        return false;

    // "[ userinfo "@" ] host [ ":" port ]"
    const auto at = std::next(target.begin(), two);
    return !parse_authority(std::string_view{ at, target.end() }).has_error();
}

// Used for OPTIONS method.
bool is_asterisk_form(const std::string& target) NOEXCEPT
{
    // Asterisk only.
    return target == "*";
}

// common http verbs or unknown
target to_target(const std::string& value, verb method) NOEXCEPT
{
    switch (method)
    {
        case verb::get:
        case verb::head:
        case verb::post:
        case verb::put:
        case verb::delete_:
        case verb::trace:
        {
            return is_origin_form(value) ?
                target::origin : (is_absolute_form(value) ?
                    target::absolute : target::unknown);
        }
        case verb::options:
        {
            return is_asterisk_form(value) ?
                target::asterisk : (is_origin_form(value) ?
                    target::origin : (is_absolute_form(value) ?
                        target::absolute : target::unknown));
        }
        case verb::connect:
        {
            return is_authority_form(value) ?
                target::authority : target::unknown;
        }
        default:
        case verb::unknown:
            return target::unknown;
    }
}

///////////////////////////////////////////////////////////////////////////////
// TODO: sanitize out to ensure it remains strictly within base. 
///////////////////////////////////////////////////////////////////////////////
std::filesystem::path sanitize_origin(const std::filesystem::path& base,
    const std::string& target) NOEXCEPT
{
    // Valid origin requires leading slash.
    if (is_origin_form(target))
    {
        try
        {
            std::filesystem::path path{ base };
            path += system::to_path(target);
            return path;
        }
        catch (...)
        {
        }
    }

    return {};
}

http_file get_file_body(const std::filesystem::path& path) NOEXCEPT
{
    using namespace system;
    using namespace boost::beast;

    // http_file::open accepts a "utf-8 encoded path to the file" on win32.
    const auto utf8_path = from_path(path);
    http::file_body::value_type file{};

    try
    {
        error_code code{};
        file.open(utf8_path.c_str(), file_mode::read, code);
        if (code) file.close();
    }
    catch (...)
    {
    }

    return file;
}

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

const std::string& get_mime_type(const std::filesystem::path& path) NOEXCEPT
{
    static const std::string default_type{ "application/octet-stream" };
    static const std::unordered_map<std::string, std::string> types
    {
        { ".html",  "text/html" },
        { ".htm",   "text/html" },
        { ".css",   "text/css" },
        { ".js",    "application/javascript" },
        { ".json",  "application/json" },
        { ".xml",   "application/xml" },
        { ".txt",   "text/plain" },
        { ".png",   "image/png" },
        { ".jpg",   "image/jpeg" },
        { ".jpeg",  "image/jpeg" },
        { ".gif",   "image/gif" },
        { ".svg",   "image/svg+xml" },
        { ".ico",   "image/x-icon" },
        { ".pdf",   "application/pdf" },
        { ".zip",   "application/zip" },
        { ".mp4",   "video/mp4" },
        { ".mp3",   "audio/mpeg" },
        { ".woff",  "font/woff" },
        { ".woff2", "font/woff2" }
    };

    if (!path.has_extension())
        return default_type;

    using namespace system;
    const auto type = types.find(ascii_to_lower(path.extension().string()));
    return type != types.end() ? type->second : default_type;
}

BC_POP_WARNING()

} // namespace rpc
} // namespace messages
} // namespace network
} // namespace libbitcoin
