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
#include <regex>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/rpc/enums/magic_numbers.hpp>

namespace libbitcoin {
namespace network {
namespace http {

// This is sub1(size_t)! so we use an internal define (e.g. 1kb).
////constexpr auto max_url = boost::url_view::max_size();

bool is_origin_form(const std::string& target) NOEXCEPT
{
    if (target.size() > max_url)
        return false;

    // Excludes leading "//", which is allowed by parse_origin_form.
    if (target.starts_with("//"))
        return false;

    try
    {
        // "/index.html?field=value" (no authority).
        return !boost::urls::parse_origin_form(target).has_error();
    }
    catch (...)
    {
        // target.size() > url_view::max_size.
        return false;
    }
}

bool is_absolute_form(const std::string& target) NOEXCEPT
{
    if (target.size() > max_url)
        return false;

    try
    {
        // "scheme://www.boost.org/index.html?field=value" (no fragment).
        const auto uri = boost::urls::parse_absolute_uri(target);
        if (uri.has_error())
            return false;

        // Limit to http/s.
        const auto scheme = uri->scheme_id();
        return scheme == boost::urls::scheme::http
            || scheme == boost::urls::scheme::https;
    }
    catch (...)
    {
        // target.size() > url_view::max_size.
        return false;
    }
}

// Used for CONNECT method.
bool is_authority_form(const std::string& target) NOEXCEPT
{
    if (target.size() > max_url)
        return false;

    // Requires leading "//", which is not allowed by parse_authority.
    if (!target.starts_with("//"))
        return false;

    // "[ userinfo "@" ] host [ ":" port ]"
    const auto at = std::next(target.begin(), two);
    const auto authority = std::string_view{ at, target.end() };
    return !boost::urls::parse_authority(authority).has_error();
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

bool is_safe_target(const std::string& target) NOEXCEPT
{
    try
    {
        static const std::regex allowed
        {
            R"(^/([a-zA-Z0-9_\-\.]+/)*[a-zA-Z0-9_\-\.]+$)",
            std::regex::optimize
        };

        return std::regex_match(target, allowed) &&
            (target.find("..") == std::string::npos);
    }
    catch (...)
    {
        return false;
    }
}

static bool contains(const std::filesystem::path& canonicalized_root,
    const std::filesystem::path& canonicalized_path) THROWS
{
    // Ensure that canonicalized path is within root.
    const auto full = canonicalized_path.generic_u8string();
    const auto base = canonicalized_root.generic_u8string();
    return full.starts_with(base) && (full.size() > base.size());
}

// weakly_canonical allows for files/directories that don't exist.
std::filesystem::path to_canonical(const std::filesystem::path& root,
    const std::string& target) NOEXCEPT
{
    try
    {
        using namespace system;
        using namespace std::filesystem;

        // Ensure that root is an absolute path.
        if (!root.is_absolute())
            return {};

        // Ensure that canonicalized root is a directory.
        const auto dir = weakly_canonical(root);
        if (exists(dir) && !is_directory(dir))
            return {};

        // Win32 fs::path assumes std::string is ANSI, so cast to u8string.
        const auto norm = cast_to_u8string(target);

        // The / path concat operator fails with a leading slash on target.
        // But target always has a leading slash, so just string concatenate.
        const auto full = root.generic_u8string() + norm;

        // Ensure that if canonicalized path exists it is a regular file.
        const auto path = weakly_canonical(full);
        if (exists(path) && !is_regular_file(path))
            return {};

        // Ensure that canonicalized path is within base.
        if (contains(dir, path))
            return path;
    }
    catch (...)
    {
    }

    return {};
}

// sanitize out to ensure it remains strictly within base, empty if failed. 
std::filesystem::path sanitize_origin(const std::filesystem::path& base,
    const std::string& target) NOEXCEPT
{
    if (is_origin_form(target) && is_safe_target(target))
        return to_canonical(base, target);

    return {};
}

http::file get_file_body(const std::filesystem::path& path) NOEXCEPT
{
    using namespace system;
    using namespace boost::beast;

    // http::file::open accepts a "utf-8 encoded path to the file" on win32.
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

} // namespace http
} // namespace network
} // namespace libbitcoin
