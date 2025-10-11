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
#include <bitcoin/network/messages/rpc/enums/mime_type.hpp>

#include <algorithm>
#include <unordered_map>
#include <bitcoin/network/boost.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace rpc {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

using mime_bimap = boost::bimap
<
    boost::bimaps::set_of<mime_type>, 
    boost::bimaps::set_of<std::string>
>;

static mime_bimap construct_mime_bimap() NOEXCEPT
{
    mime_bimap bimap{};
    bimap.insert({ mime_type::application_javascript, "application/javascript" });
    bimap.insert({ mime_type::application_json, "application/json" });
    bimap.insert({ mime_type::application_octet, "application/octet-stream" });
    bimap.insert({ mime_type::application_pdf, "application/pdf" });
    bimap.insert({ mime_type::application_xml, "application/xml" });
    bimap.insert({ mime_type::application_zip, "application/zip" });
    bimap.insert({ mime_type::audio_mpeg, "audio/mpeg" });
    bimap.insert({ mime_type::font_woff, "font/woff" });
    bimap.insert({ mime_type::font_woff2, "font/woff2" });
    bimap.insert({ mime_type::image_gif, "image/gif" });
    bimap.insert({ mime_type::image_jpeg, "image/jpeg" });
    bimap.insert({ mime_type::image_png, "image/png" });
    bimap.insert({ mime_type::image_svg_xml, "image/svg+xml" });
    bimap.insert({ mime_type::image_x_icon, "image/x-icon" });
    bimap.insert({ mime_type::text_css, "text/css" });
    bimap.insert({ mime_type::text_html, "text/html" });
    bimap.insert({ mime_type::text_plain, "text/plain" });
    bimap.insert({ mime_type::video_mp4, "video/mp4" });
    ////bimap.insert({ mime_type::unknown, "unknown" });
    return bimap;
};

const mime_bimap& mime_map() NOEXCEPT
{
    static const auto types = construct_mime_bimap();
    return types;
}

mime_type to_mime_type(const std::string& accept, mime_type default_) NOEXCEPT
{
    const auto type = mime_map().right.find(system::ascii_to_lower(accept));
    return type == mime_map().right.end() ? default_ : type->second;
};

std::string from_mime_type(mime_type type, const std::string& default_) NOEXCEPT
{
    const auto text = mime_map().left.find(type);
    return text == mime_map().left.end() ? default_ : text->second;
};

mime_types to_mime_types(const std::string& accepts,
    mime_type default_) NOEXCEPT
{
    using namespace system;
    const auto tokens = split(accepts, ",");

    mime_types out{};
    out.resize(tokens.size());
    std::ranges::transform(tokens, out.begin(), [&](const auto& accept) NOEXCEPT
    {
        return to_mime_type(split(accept, ";").front(), default_);
    });

    distinct(out);
    return out;
}

std::string from_mime_types(const mime_types& types,
    const std::string& default_) NOEXCEPT
{
    using namespace system;
    string_list out{};
    out.resize(types.size());
    std::ranges::transform(types, out.begin(), [&](auto type) NOEXCEPT
    {
        return from_mime_type(type, default_);
    });

    distinct(out);
    return join(out, ",");
}

mime_type file_mime_type(const std::filesystem::path& path,
    mime_type default_) NOEXCEPT
{
    static const std::unordered_map<std::string, mime_type> types
    {
        { ".js",    mime_type::application_javascript },
        ////{ "",   mime_type::application_octet },
        { ".json",  mime_type::application_json },
        { ".pdf",   mime_type::application_pdf },
        { ".zip",   mime_type::application_zip },
        { ".xml",   mime_type::application_xml },
        { ".mp3",   mime_type::audio_mpeg },
        { ".woff",  mime_type::font_woff },
        { ".woff2", mime_type::font_woff2 },
        { ".gif",   mime_type::image_gif },
        { ".jpg",   mime_type::image_jpeg },
        { ".jpeg",  mime_type::image_jpeg },
        { ".png",   mime_type::image_png },
        { ".svg",   mime_type::image_svg_xml },
        { ".ico",   mime_type::image_x_icon },
        { ".css",   mime_type::text_css },
        { ".html",  mime_type::text_html },
        { ".htm",   mime_type::text_html },
        { ".txt",   mime_type::text_plain },
        { ".mp4",   mime_type::video_mp4 }
    };

    if (!path.has_extension())
        return default_;

    const auto extension = system::cast_to_string(path.extension().u8string());
    const auto type = types.find(system::ascii_to_lower(extension));
    return type == types.end() ? default_ : type->second;
}

BC_POP_WARNING()

} // namespace rpc
} // namespace messages
} // namespace network
} // namespace libbitcoin
