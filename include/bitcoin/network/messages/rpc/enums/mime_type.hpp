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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_RPC_ENUMS_MIME_TYPE_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_RPC_ENUMS_MIME_TYPE_HPP

#include <filesystem>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace rpc {

/// Enumeration of utilized MIME types.
enum class mime_type
{
    application_javascript,
    application_json,
    application_octet,
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

using mime_types = std::vector<mime_type>;

BCT_API mime_type to_mime_type(const std::string& accept,
    mime_type default_=mime_type::unknown) NOEXCEPT;
BCT_API std::string from_mime_type(mime_type type,
    const std::string& default_="unknown") NOEXCEPT;

BCT_API mime_types to_mime_types(const std::string& accepts,
    mime_type default_=mime_type::unknown) NOEXCEPT;
BCT_API std::string from_mime_types(const mime_types& types,
    const std::string& default_="unknown") NOEXCEPT;

BCT_API mime_type file_mime_type(const std::filesystem::path& path,
    mime_type default_=mime_type::application_octet) NOEXCEPT;

} // namespace rpc
} // namespace messages

using http_mime_type = messages::rpc::mime_type;

} // namespace network
} // namespace libbitcoin

#endif
