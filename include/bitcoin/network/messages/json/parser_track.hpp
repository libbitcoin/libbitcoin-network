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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_JSON_PARSER_TRACK_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_JSON_PARSER_TRACK_HPP

namespace libbitcoin {
namespace network {
namespace json {

class BCT_API parser_track
{
public:
    inline void delimiter() NOEXCEPT
    {
        comma_ = true;
    }

    inline void add() NOEXCEPT
    {
        empty_ = false;
        comma_ = false;
    }

    inline bool allow_add() const NOEXCEPT
    {
        return empty_ || comma_;
    }

    inline bool allow_delimiter() const NOEXCEPT
    {
        return !allow_add();
    }

    inline bool allow_close() const NOEXCEPT
    {
        return !comma_;
    }

    inline void reset() NOEXCEPT
    {
        empty_ = true;
        comma_ = false;
    }

private:
    bool empty_{ true };
    bool comma_{ false };
};
 
} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
