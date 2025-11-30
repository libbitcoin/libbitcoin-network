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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_HTTP_ENUMS_MEDIA_TYPE_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_HTTP_ENUMS_MEDIA_TYPE_HPP

#include <filesystem>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace http {

/// Enumeration of utilized MEDIA types.
enum class media_type
{
    application_javascript,
    application_json,
    application_octet_stream,
    application_pdf,
    application_xml,
    application_zip,
    audio_mpeg,
    font_woff,
    font_woff2,
    image_gif,
    image_jpeg,
    image_png,
    image_svg_xml,
    image_x_icon,
    text_css,
    text_html,
    text_plain,
    video_mp4,
    unknown
};

using media_types = std::vector<media_type>;

BCT_API media_type to_media_type(const std::string_view& accept,
    media_type default_=media_type::unknown) NOEXCEPT;
BCT_API std::string from_media_type(media_type type,
    const std::string_view& default_="unknown") NOEXCEPT;

BCT_API media_types to_media_types(const std::string_view& accepts,
    media_type default_=media_type::unknown) NOEXCEPT;
BCT_API std::string from_media_types(const media_types& types,
    const std::string_view& default_="unknown") NOEXCEPT;

BCT_API media_type content_media_type(const std::string_view& content_type,
    media_type default_=media_type::unknown) NOEXCEPT;
BCT_API media_type content_media_type(const http::fields& fields,
    media_type default_ = media_type::unknown) NOEXCEPT;

BCT_API media_type extension_media_type(const std::string_view& extension,
    media_type default_=media_type::unknown) NOEXCEPT;
BCT_API media_type file_media_type(const std::filesystem::path& path,
    media_type default_=media_type::unknown) NOEXCEPT;
BCT_API media_type target_media_type(const std::string& target,
    media_type default_=media_type::unknown) NOEXCEPT;

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
