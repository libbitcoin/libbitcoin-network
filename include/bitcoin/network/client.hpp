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
#ifndef LIBBITCOIN_NETWORK_CLIENT_HPP
#define LIBBITCOIN_NETWORK_CLIENT_HPP

#include <filesystem>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

/// HTTP/S admin web server settings.
struct BCT_API admin
{
    /// Properties.
    uint16_t connections{};
    config::endpoints binds{};
    std::filesystem::path path{};
    config::authority host{};

    /// Helpers.
    virtual bool enabled() const NOEXCEPT;
};

/// HTTP/S block explorer settings.
struct BCT_API explore
{
    /// Properties.
    uint16_t connections{};
    config::endpoints binds{};
    std::filesystem::path path{};
    config::authority host{};

    /// Helpers.
    virtual bool enabled() const NOEXCEPT;
};

/// Native RESTful query interface settings.
struct BCT_API rest
{
    /// Properties.
    uint16_t connections{};
    config::endpoints binds{};
    config::authority host{};

    /// Helpers.
    virtual bool enabled() const NOEXCEPT;
};

// Native WebSocket query interface settings.
struct BCT_API websocket
{
    /// Properties.
    uint16_t connections{};
    config::endpoints binds{};
    config::authority host{};

    /// Helpers.
    virtual bool enabled() const NOEXCEPT;
};

// bitcoind compatibility interface settings.
struct BCT_API bitcoind
{
    /// Properties.
    uint16_t connections{};
    config::endpoints binds{};
    config::authority host{};

    /// Helpers.
    virtual bool enabled() const NOEXCEPT;
};

// Electrum compatibility interface settings.
struct BCT_API electrum
{
    /// Properties.
    uint16_t connections{};
    config::endpoints binds{};

    /// Helpers.
    virtual bool enabled() const NOEXCEPT;
};

// Stratum v1 compatibility interface settings.
struct BCT_API stratum_v1
{
    /// Properties.
    uint16_t connections{};
    config::endpoints binds{};

    /// Helpers.
    virtual bool enabled() const NOEXCEPT;
};

// Stratum v2 compatibility interface settings.
struct BCT_API stratum_v2
{
    /// Properties.
    uint16_t connections{};
    config::endpoints binds{};

    /// Helpers.
    virtual bool enabled() const NOEXCEPT;
};

} // namespace network
} // namespace libbitcoin

#endif
