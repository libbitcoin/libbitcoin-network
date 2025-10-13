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
#ifndef LIBBITCOIN_NETWORK_PROTOCOL_CLIENT_HTML_HPP
#define LIBBITCOIN_NETWORK_PROTOCOL_CLIENT_HTML_HPP

#include <filesystem>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/client.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/http/messages.hpp>
#include <bitcoin/network/protocols/protocol_client_http.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

class BCT_API protocol_client_html
  : public protocol_client_http, protected tracker<protocol_client_html>
{
public:
    typedef std::shared_ptr<protocol_client_html> ptr;

    protocol_client_html(const session::ptr& session,
        const channel::ptr& channel, const html_server& options) NOEXCEPT;

protected:
    /// Message handlers by http method.
    void handle_receive_get(const code& ec,
        const http::method::get& request) NOEXCEPT override;

    /// Senders.
    void send_file(const http::string_request& request,
        http::file&& file, http::mime_type type) NOEXCEPT;

    /// Utilities.
    std::filesystem::path to_local_path(
        const std::string& target) const NOEXCEPT;

private:
    // These are thread safe.
    const std::filesystem::path& root_;
    const std::string& default_;
};

} // namespace network
} // namespace libbitcoin

#endif
