/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
#include "../../test.hpp"

BC_AUTO_TEST_SUITE(peer_body_tests)

using namespace network::messages::peer;
using system::data_chunk;

constexpr uint32_t magic = 0xd9b4bef9;
constexpr uint64_t nonce = 0x0123456789abcdef;

// All tests frame a ping message (fixed 8 byte payload).
static data_chunk ping_frame()
{
    const ping message{ nonce };
    const auto data = serialize(message, magic, messages::peer::level::bip31);
    BC_REQUIRE(data);
    return *data;
}

static data_chunk frame_head(const data_chunk& framed)
{
    return { framed.begin(), std::next(framed.begin(), heading::size()) };
}

static data_chunk frame_payload(const data_chunk& framed)
{
    return { std::next(framed.begin(), heading::size()), framed.end() };
}

static frame test_frame()
{
    return frame
    {
        .magic = magic,
        .version = messages::peer::level::bip31,
        .witness = true,
        .checksum = true,
        .maximum = heading::maximum_payload(true)
    };
}

static code put_fault(frame& value, const data_chunk& framed)
{
    code ec{};
    body::reader reader{ value };
    reader.init({}, ec);

    const auto head = frame_head(framed);
    reader.put({ head.data(), head.size() }, ec);
    if (ec)
        return value.fault;

    const auto payload = frame_payload(framed);
    reader.put({ payload.data(), payload.size() }, ec);
    BC_REQUIRE(ec);
    return value.fault;
}

BC_AUTO_TEST_CASE(peer_body__need__initial__heading_size)
{
    auto value = test_frame();
    code ec{};
    body::reader reader{ value };
    reader.init({}, ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE_EQUAL(reader.need(), heading::size());
    BC_REQUIRE(!reader.done());
}

BC_AUTO_TEST_CASE(peer_body__put__empty_buffer__defers)
{
    auto value = test_frame();
    code ec{};
    body::reader reader{ value };
    reader.init({}, ec);

    const auto consumed = reader.put({ nullptr, 0 }, ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE_EQUAL(consumed, 0u);
    BC_REQUIRE(!reader.done());

    reader.finish(ec);
    BC_REQUIRE(ec);
}

BC_AUTO_TEST_CASE(peer_body__put__framed_sequence__message)
{
    const auto data = ping_frame();
    const auto head = frame_head(data);
    const auto payload = frame_payload(data);

    auto value = test_frame();
    code ec{};
    body::reader reader{ value };
    reader.init({}, ec);

    BC_REQUIRE_EQUAL(reader.put({ head.data(), head.size() }, ec), heading::size());
    BC_REQUIRE(!ec);
    BC_REQUIRE(!reader.done());
    BC_REQUIRE_EQUAL(reader.need(), sizeof(uint64_t));
    BC_REQUIRE_EQUAL(value.head.command, "ping");
    BC_REQUIRE_EQUAL(value.head.payload_size, sizeof(uint64_t));

    BC_REQUIRE_EQUAL(reader.put({ payload.data(), payload.size() }, ec), payload.size());
    BC_REQUIRE(!ec);
    BC_REQUIRE(reader.done());
    BC_REQUIRE_EQUAL(reader.need(), 0u);

    reader.finish(ec);
    BC_REQUIRE(!ec);

    const auto message = value.payload.get<const ping>();
    BC_REQUIRE(message);
    BC_REQUIRE_EQUAL(message->nonce, nonce);
}

BC_AUTO_TEST_CASE(peer_body__put__short_reads__defer)
{
    const auto data = ping_frame();
    const auto head = frame_head(data);
    const auto payload = frame_payload(data);

    auto value = test_frame();
    code ec{};
    body::reader reader{ value };
    reader.init({}, ec);

    // Partial heading defers.
    BC_REQUIRE_EQUAL(reader.put({ head.data(), 10 }, ec), 0u);
    BC_REQUIRE(!ec);
    BC_REQUIRE_EQUAL(reader.need(), heading::size());

    BC_REQUIRE_EQUAL(reader.put({ head.data(), head.size() }, ec), heading::size());
    BC_REQUIRE(!ec);

    // Partial payload defers.
    BC_REQUIRE_EQUAL(reader.put({ payload.data(), payload.size() - 1 }, ec), 0u);
    BC_REQUIRE(!ec);
    BC_REQUIRE(!reader.done());

    BC_REQUIRE_EQUAL(reader.put({ payload.data(), payload.size() }, ec), payload.size());
    BC_REQUIRE(!ec);
    BC_REQUIRE(reader.done());
}

BC_AUTO_TEST_CASE(peer_body__put__empty_payload__completes_on_heading)
{
    const data_chunk payload{};
    data_chunk data(heading::size());
    const auto head = heading::factory(magic, "verack", payload);
    BC_REQUIRE(head.serialize({ data.data(), std::next(data.data(), heading::size()) }));

    auto value = test_frame();
    code ec{};
    body::reader reader{ value };
    reader.init({}, ec);

    BC_REQUIRE_EQUAL(reader.put({ data.data(), data.size() }, ec), heading::size());
    BC_REQUIRE(!ec);
    BC_REQUIRE(reader.done());
    BC_REQUIRE_EQUAL(reader.need(), 0u);

    reader.finish(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(value.payload.get<const version_acknowledge>());
}

BC_AUTO_TEST_CASE(peer_body__put__invalid_magic__fault)
{
    auto value = test_frame();
    value.magic = 0x0b110907;
    BC_REQUIRE_EQUAL(put_fault(value, ping_frame()), error::invalid_magic);
}

BC_AUTO_TEST_CASE(peer_body__put__oversized_payload__fault)
{
    auto value = test_frame();
    value.maximum = 4;
    BC_REQUIRE_EQUAL(put_fault(value, ping_frame()), error::oversized_payload);
}

BC_AUTO_TEST_CASE(peer_body__put__invalid_checksum__fault)
{
    auto data = ping_frame();
    data.back() ^= 0x01;
    auto value = test_frame();
    BC_REQUIRE_EQUAL(put_fault(value, data), error::invalid_checksum);
}

BC_AUTO_TEST_CASE(peer_body__put__corrupt_payload_without_checksum__invalid_message_fault)
{
    // A short payload deserializes as invalid (checksum disabled to reach it).
    const data_chunk payload(3, 0x42);
    data_chunk data(heading::size() + payload.size());
    const auto head = heading::factory(magic, "ping", payload);
    BC_REQUIRE(head.serialize({ data.data(), std::next(data.data(), heading::size()) }));
    std::copy(payload.begin(), payload.end(), std::next(data.begin(), heading::size()));

    auto value = test_frame();
    value.checksum = false;
    BC_REQUIRE_EQUAL(put_fault(value, data), error::invalid_message);
}

BC_AUTO_TEST_CASE(peer_body__put__invalid_block_payload__invalid_message_fault)
{
    // Serialization validity only (not consensus), checksum disabled.
    const data_chunk payload(10, 0x42);
    data_chunk data(heading::size() + payload.size());
    const auto head = heading::factory(magic, "block", payload);
    BC_REQUIRE(head.serialize({ data.data(), std::next(data.data(), heading::size()) }));
    std::copy(payload.begin(), payload.end(), std::next(data.begin(), heading::size()));

    auto value = test_frame();
    value.checksum = false;
    BC_REQUIRE_EQUAL(put_fault(value, data), error::invalid_message);
}

BC_AUTO_TEST_CASE(peer_body__put__unknown_command__invalid_message_fault)
{
    const data_chunk payload{};
    data_chunk data(heading::size());
    const auto head = heading::factory(magic, "bogus", payload);
    BC_REQUIRE(head.serialize({ data.data(), std::next(data.data(), heading::size()) }));

    auto value = test_frame();
    code ec{};
    body::reader reader{ value };
    reader.init({}, ec);

    reader.put({ data.data(), data.size() }, ec);
    BC_REQUIRE(ec);
    BC_REQUIRE_EQUAL(value.fault, error::invalid_message);
}

BC_AUTO_TEST_CASE(peer_body__put__checksum_disabled__message)
{
    auto data = ping_frame();

    // Corrupt the checksum field only (bytes 20-23), validation disabled.
    data.at(20) ^= 0x01;
    const auto head = frame_head(data);
    const auto payload = frame_payload(data);

    auto value = test_frame();
    value.checksum = false;

    code ec{};
    body::reader reader{ value };
    reader.init({}, ec);
    BC_REQUIRE_EQUAL(reader.put({ head.data(), head.size() }, ec), heading::size());
    BC_REQUIRE(!ec);
    BC_REQUIRE_EQUAL(reader.put({ payload.data(), payload.size() }, ec), payload.size());
    BC_REQUIRE(!ec);
    BC_REQUIRE(reader.done());
}

// The framed reader parses the caller buffer supplied at construct, which is
// how socket::peer_read drives it (the buffer is not consumed by the parse).

static data_chunk block_frame()
{
    const system::settings settings{ system::chain::selection::mainnet };
    const auto payload = settings.genesis_block.to_data(true);
    const auto head = heading::factory(magic, "block", payload);
    data_chunk framed(heading::size() + payload.size());
    const auto start = framed.data();
    BC_REQUIRE(head.serialize({ start, std::next(start, heading::size()) }));
    std::copy(payload.begin(), payload.end(), std::next(framed.begin(), heading::size()));
    return framed;
}

BC_AUTO_TEST_CASE(peer_body__put__framed_buffer_sequence__message)
{
    const auto data = ping_frame();
    const auto head = frame_head(data);
    auto buffer = frame_payload(data);

    auto value = test_frame();
    code ec{};
    body::reader reader{ value, buffer };
    reader.init({}, ec);

    BC_REQUIRE_EQUAL(reader.put({ head.data(), head.size() }, ec), heading::size());
    BC_REQUIRE(!ec);
    BC_REQUIRE_EQUAL(reader.need(), sizeof(uint64_t));

    BC_REQUIRE_EQUAL(reader.put({ buffer.data(), buffer.size() }, ec), buffer.size());
    BC_REQUIRE(!ec);
    BC_REQUIRE(reader.done());

    reader.finish(ec);
    BC_REQUIRE(!ec);

    const auto message = value.payload.get<const ping>();
    BC_REQUIRE(message);
    BC_REQUIRE_EQUAL(message->nonce, nonce);
}

BC_AUTO_TEST_CASE(peer_body__put__framed_buffer__buffer_retained)
{
    const auto data = ping_frame();
    const auto head = frame_head(data);
    const auto payload = frame_payload(data);
    auto buffer = payload;

    auto value = test_frame();
    code ec{};
    body::reader reader{ value, buffer };
    reader.init({}, ec);
    reader.put({ head.data(), head.size() }, ec);
    reader.put({ buffer.data(), buffer.size() }, ec);
    BC_REQUIRE(reader.done());

    // The message parses the buffer in place, leaving it to the caller.
    BC_REQUIRE_EQUAL(buffer, payload);
}

BC_AUTO_TEST_CASE(peer_body__put__framed_buffer_empty_payload__completes_on_heading)
{
    const data_chunk payload{};
    data_chunk data(heading::size());
    const auto head = heading::factory(magic, "verack", payload);
    BC_REQUIRE(head.serialize({ data.data(), std::next(data.data(), heading::size()) }));

    // Stale content from a prior message must not reach the parse.
    data_chunk buffer(42, 0x42);

    auto value = test_frame();
    code ec{};
    body::reader reader{ value, buffer };
    reader.init({}, ec);

    BC_REQUIRE_EQUAL(reader.put({ data.data(), data.size() }, ec), heading::size());
    BC_REQUIRE(!ec);
    BC_REQUIRE(reader.done());

    reader.finish(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(value.payload.get<const version_acknowledge>());
}

BC_AUTO_TEST_CASE(peer_body__put__framed_buffer_invalid_checksum__fault)
{
    auto data = ping_frame();
    data.back() ^= 0x01;
    const auto head = frame_head(data);
    auto buffer = frame_payload(data);

    auto value = test_frame();
    code ec{};
    body::reader reader{ value, buffer };
    reader.init({}, ec);

    reader.put({ head.data(), head.size() }, ec);
    BC_REQUIRE(!ec);

    reader.put({ buffer.data(), buffer.size() }, ec);
    BC_REQUIRE(ec);
    BC_REQUIRE_EQUAL(value.fault, error::invalid_checksum);
}

BC_AUTO_TEST_CASE(peer_body__put__framed_buffer_block__message)
{
    const auto data = block_frame();
    const auto head = frame_head(data);
    auto buffer = frame_payload(data);

    auto value = test_frame();
    code ec{};
    body::reader reader{ value, buffer };
    reader.init({}, ec);

    BC_REQUIRE_EQUAL(reader.put({ head.data(), head.size() }, ec), heading::size());
    BC_REQUIRE(!ec);
    BC_REQUIRE_EQUAL(reader.need(), buffer.size());

    BC_REQUIRE_EQUAL(reader.put({ buffer.data(), buffer.size() }, ec), buffer.size());
    BC_REQUIRE(!ec);
    BC_REQUIRE(reader.done());

    const auto message = value.payload.get<const block>();
    BC_REQUIRE(message);
    BC_REQUIRE(message->block.is_valid());
}

BC_AUTO_TEST_CASE(peer_body__writer__frame__emitted)
{
    auto value = test_frame();
    value.message = rpc::any_t{ system::to_shared(ping{ nonce }) };
    value.index = messages::peer::registry::index_of<ping>();

    code ec{};
    body::writer writer{ value };
    writer.init(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(!writer.done());
    BC_REQUIRE_EQUAL(*value.data, ping_frame());

    const auto out = writer.get(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(out.has_value());
    BC_REQUIRE(!out->second);
    BC_REQUIRE(writer.done());
    BC_REQUIRE_EQUAL(out->first.data(), value.data->data());
    BC_REQUIRE_EQUAL(out->first.size(), value.data->size());
}

BC_AUTO_TEST_CASE(peer_body__writer__get_twice__empty)
{
    auto value = test_frame();
    value.message = rpc::any_t{ system::to_shared(ping{ nonce }) };
    value.index = messages::peer::registry::index_of<ping>();

    code ec{};
    body::writer writer{ value };
    writer.init(ec);

    const auto out = writer.get(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(out.has_value());
    BC_REQUIRE(writer.done());

    const auto empty = writer.get(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(!empty.has_value());
}

BC_AUTO_TEST_CASE(peer_body__writer__no_message__error)
{
    auto value = test_frame();
    code ec{};
    body::writer writer{ value };
    writer.init(ec);
    BC_REQUIRE(ec);
}

// v2 put (identified payload)

static const auto ping_payload = system::base16_chunk("efcdab8967452301");

BC_AUTO_TEST_CASE(peer_body__put_v2__identifier__message)
{
    auto value = test_frame();
    code ec{};
    body::reader reader{ value };
    reader.init({}, ec);

    reader.put(identifiers::ping, {}, ping_payload, ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(reader.done());
    BC_REQUIRE_EQUAL(reader.need(), 0u);
    BC_REQUIRE_EQUAL(value.head.magic, magic);
    BC_REQUIRE_EQUAL(value.head.command, ping::command);
    BC_REQUIRE_EQUAL(value.head.payload_size, ping_payload.size());
    BC_REQUIRE_EQUAL(value.head.checksum, 0u);

    reader.finish(ec);
    BC_REQUIRE(!ec);

    const auto message = value.payload.get<const ping>();
    BC_REQUIRE(message);
    BC_REQUIRE_EQUAL(message->nonce, nonce);
}

BC_AUTO_TEST_CASE(peer_body__put_v2__command__message)
{
    auto value = test_frame();
    code ec{};
    body::reader reader{ value };
    reader.init({}, ec);

    reader.put(identifiers::unassigned, pong::command, ping_payload, ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(reader.done());
    BC_REQUIRE_EQUAL(value.head.command, pong::command);

    const auto message = value.payload.get<const pong>();
    BC_REQUIRE(message);
    BC_REQUIRE_EQUAL(message->nonce, nonce);
}

BC_AUTO_TEST_CASE(peer_body__put_v2__unknown_command__unknown_message_fault)
{
    auto value = test_frame();
    code ec{};
    body::reader reader{ value };
    reader.init({}, ec);

    reader.put(identifiers::unassigned, "bogus", ping_payload, ec);
    BC_REQUIRE(ec);
    BC_REQUIRE(!reader.done());
    BC_REQUIRE_EQUAL(value.fault, error::unknown_message);
}

BC_AUTO_TEST_CASE(peer_body__put_v2__unknown_identifier__unknown_message_fault)
{
    auto value = test_frame();
    code ec{};
    body::reader reader{ value };
    reader.init({}, ec);

    reader.put(uint8_t{ 255 }, ping::command, ping_payload, ec);
    BC_REQUIRE(ec);
    BC_REQUIRE_EQUAL(value.fault, error::unknown_message);
}

BC_AUTO_TEST_CASE(peer_body__put_v2__oversized_payload__fault)
{
    auto value = test_frame();
    value.maximum = sub1(ping_payload.size());
    code ec{};
    body::reader reader{ value };
    reader.init({}, ec);

    reader.put(identifiers::ping, {}, ping_payload, ec);
    BC_REQUIRE(ec);
    BC_REQUIRE_EQUAL(value.fault, error::oversized_payload);
}

BC_AUTO_TEST_CASE(peer_body__put_v2__short_payload__invalid_message_fault)
{
    auto value = test_frame();
    code ec{};
    body::reader reader{ value };
    reader.init({}, ec);

    reader.put(identifiers::ping, {}, system::base16_chunk("efcd"), ec);
    BC_REQUIRE(ec);
    BC_REQUIRE(!reader.done());
    BC_REQUIRE_EQUAL(value.fault, error::invalid_message);
}

BC_AUTO_TEST_SUITE_END()
