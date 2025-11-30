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
#include <bitcoin/network/messages/http/enums/media_type.hpp>

#include <algorithm>
#include <unordered_map>
#include <bitcoin/network/define.hpp>

// TODO: rename to media_type.
// TODO: pass in fields vs. string_view.
// TODO: formalize parsing, include priority sort.

namespace libbitcoin {
namespace network {
namespace http {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

using media_bimap = boost::bimap
<
    boost::bimaps::set_of<media_type>, 
    boost::bimaps::set_of<std::string>
>;

static media_bimap construct_media_bimap() NOEXCEPT
{
    media_bimap bimap{};
    bimap.insert({ media_type::application_javascript, "application/javascript" });
    bimap.insert({ media_type::application_json, "application/json" });
    bimap.insert({ media_type::application_octet_stream, "application/octet-stream" });
    bimap.insert({ media_type::application_pdf, "application/pdf" });
    bimap.insert({ media_type::application_xml, "application/xml" });
    bimap.insert({ media_type::application_zip, "application/zip" });
    bimap.insert({ media_type::audio_mpeg, "audio/mpeg" });
    bimap.insert({ media_type::font_woff, "font/woff" });
    bimap.insert({ media_type::font_woff2, "font/woff2" });
    bimap.insert({ media_type::image_gif, "image/gif" });
    bimap.insert({ media_type::image_jpeg, "image/jpeg" });
    bimap.insert({ media_type::image_png, "image/png" });
    bimap.insert({ media_type::image_svg_xml, "image/svg+xml" });
    bimap.insert({ media_type::image_x_icon, "image/x-icon" });
    bimap.insert({ media_type::text_css, "text/css" });
    bimap.insert({ media_type::text_html, "text/html" });
    bimap.insert({ media_type::text_plain, "text/plain" });
    bimap.insert({ media_type::video_mp4, "video/mp4" });
    ////bimap.insert({ media_type::unknown, "unknown" });
    return bimap;
};

const media_bimap& media_map() NOEXCEPT
{
    static const auto types = construct_media_bimap();
    return types;
}

media_type to_media_type(const std::string_view& accept,
    media_type default_) NOEXCEPT
{
    const auto type = media_map().right.find(system::ascii_to_lower(accept));
    return type == media_map().right.end() ? default_ : type->second;
};

std::string from_media_type(media_type type,
    const std::string_view& default_) NOEXCEPT
{
    const auto text = media_map().left.find(type);
    return text == media_map().left.end() ? std::string{ default_ } :
        text->second;
};

media_types to_media_types(const std::string_view& accepts,
    media_type default_) NOEXCEPT
{
    using namespace system;
    const auto tokens = split(accepts, ",");

    media_types out{};
    out.resize(tokens.size());
    std::ranges::transform(tokens, out.begin(),
        [&](const auto& accept) NOEXCEPT
        {
            return to_media_type(split(accept, ";").front(), default_);
        });

    distinct(out);
    return out;
}

std::string from_media_types(const media_types& types,
    const std::string_view& default_) NOEXCEPT
{
    using namespace system;
    string_list out{};
    out.resize(types.size());
    std::ranges::transform(types, out.begin(), [&](auto type) NOEXCEPT
    {
        return from_media_type(type, default_);
    });

    distinct(out);
    return join(out, ",");
}

media_type content_media_type(const std::string_view& content_type,
    media_type default_) NOEXCEPT
{
    if (content_type.empty())
        return default_;

    const auto parts = system::split(content_type, ";");
    if (parts.empty())
        return default_;

    const auto type = system::ascii_to_lower(parts.front());
    const auto found = media_map().right.find(type);
    return found == media_map().right.end() ? default_ : found->second;
}

media_type content_media_type(const fields& fields,
    media_type default_) NOEXCEPT
{
    return content_media_type(fields[field::content_type], default_);
}

media_type extension_media_type(const std::string_view& extension,
    media_type default_) NOEXCEPT
{
    static const std::unordered_map<std::string, media_type> types
    {
        { ".js",    media_type::application_javascript },
        ////{ "",   media_type::application_octet_stream },
        { ".json",  media_type::application_json },
        { ".pdf",   media_type::application_pdf },
        { ".zip",   media_type::application_zip },
        { ".xml",   media_type::application_xml },
        { ".mp3",   media_type::audio_mpeg },
        { ".woff",  media_type::font_woff },
        { ".woff2", media_type::font_woff2 },
        { ".gif",   media_type::image_gif },
        { ".jpg",   media_type::image_jpeg },
        { ".jpeg",  media_type::image_jpeg },
        { ".png",   media_type::image_png },
        { ".svg",   media_type::image_svg_xml },
        { ".ico",   media_type::image_x_icon },
        { ".css",   media_type::text_css },
        { ".html",  media_type::text_html },
        { ".htm",   media_type::text_html },
        { ".txt",   media_type::text_plain },
        { ".mp4",   media_type::video_mp4 }
    };

    const auto type = types.find(system::ascii_to_lower(extension));
    return type == types.end() ? default_ : type->second;
}

media_type file_media_type(const std::filesystem::path& path,
    media_type default_) NOEXCEPT
{
    if (!path.has_extension())
        return default_;

    const auto extension = system::cast_to_string(path.extension().u8string());
    return extension_media_type(extension, default_);
}

media_type target_media_type(const std::string& target,
    media_type default_) NOEXCEPT
{
    // request.target() is a boost::string_view, which requres path conversion.
    return file_media_type(target, default_);
}

BC_POP_WARNING()

} // namespace http
} // namespace network
} // namespace libbitcoin
