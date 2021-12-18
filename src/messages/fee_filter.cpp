/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/messages/fee_filter.hpp>

#include <cstddef>
#include <cstdint>
#include <bitcoin/system.hpp>
#include <bitcoin/network/messages/enums/identifier.hpp>
#include <bitcoin/network/messages/enums/level.hpp>
#include <bitcoin/network/messages/message.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

using namespace bc::system;
    
const std::string fee_filter::command = "feefilter";
const identifier fee_filter::id = identifier::fee_filter;
const uint32_t fee_filter::version_minimum = level::bip133;
const uint32_t fee_filter::version_maximum = level::maximum_protocol;

// static
size_t fee_filter::size(uint32_t)
{
    return sizeof(uint64_t);
}

// static
fee_filter fee_filter::deserialize(uint32_t version, reader& source)
{
    if (version < version_minimum || version > version_maximum)
        source.invalidate();

    return { source.read_8_bytes_little_endian() };
}

void fee_filter::serialize(uint32_t version, writer& sink) const
{
    sink.write_8_bytes_little_endian(minimum_fee);
}

} // namespace messages
} // namespace network
} // namespace libbitcoin
