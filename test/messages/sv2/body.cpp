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

BC_AUTO_TEST_SUITE(sv2_body_tests)

using namespace bc::system;
using namespace network::messages::sv2;

// All message tests frame setup_connection_success{ 2, 0 } (6 byte payload).
static data_chunk success_frame() NOEXCEPT
{
    return base16_chunk("000001060000020000000000");
}

static data_chunk frame_head(const data_chunk& framed) NOEXCEPT
{
    return { framed.begin(), std::next(framed.begin(), heading::size()) };
}

static data_chunk frame_payload(const data_chunk& framed) NOEXCEPT
{
    return { std::next(framed.begin(), heading::size()), framed.end() };
}

static frame test_frame() NOEXCEPT
{
    return frame{ .maximum = heading::maximum_payload };
}

static frame out_frame() NOEXCEPT
{
    frame value{};
    value.message = rpc::any_t{ to_shared(setup_connection_success{ 2, 0 }) };
    value.index = registry::index_of<setup_connection_success>();
    value.size = 6;
    return value;
}

// The fault of the read of a heading and then its payload.
static code put_fault(frame& value, const data_chunk& framed) NOEXCEPT
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
    return value.fault;
}

// The read of a heading and then its payload completes.
static bool put_done(frame& value, const data_chunk& framed) NOEXCEPT
{
    code ec{};
    body::reader reader{ value };
    reader.init({}, ec);

    const auto head = frame_head(framed);
    reader.put({ head.data(), head.size() }, ec);
    if (ec)
        return false;

    const auto payload = frame_payload(framed);
    if (!reader.done())
        reader.put({ payload.data(), payload.size() }, ec);

    return !ec && reader.done();
}

BC_AUTO_TEST_CASE(sv2_body__need__initial__heading_size)
{
    auto value = test_frame();
    code ec{};
    body::reader reader{ value };
    reader.init({}, ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE_EQUAL(reader.need(), heading::size());
    BC_REQUIRE(!reader.done());
}

BC_AUTO_TEST_CASE(sv2_body__put__empty_buffer__defers)
{
    auto value = test_frame();
    code ec{};
    body::reader reader{ value };
    reader.init({}, ec);

    BC_REQUIRE_EQUAL(reader.put({ nullptr, 0 }, ec), 0u);
    BC_REQUIRE(!ec);
    BC_REQUIRE(!reader.done());

    reader.finish(ec);
    BC_REQUIRE(ec);
}

BC_AUTO_TEST_CASE(sv2_body__put__framed_sequence__message)
{
    const auto data = success_frame();
    const auto head = frame_head(data);
    const auto payload = frame_payload(data);

    auto value = test_frame();
    code ec{};
    body::reader reader{ value };
    reader.init({}, ec);

    BC_REQUIRE_EQUAL(reader.put({ head.data(), head.size() }, ec), heading::size());
    BC_REQUIRE(!ec);
    BC_REQUIRE(!reader.done());
    BC_REQUIRE_EQUAL(reader.need(), 6u);
    BC_REQUIRE_EQUAL(value.head.message_type, 0x01u);
    BC_REQUIRE_EQUAL(value.head.message_length, 6u);

    BC_REQUIRE_EQUAL(reader.put({ payload.data(), payload.size() }, ec), payload.size());
    BC_REQUIRE(!ec);
    BC_REQUIRE(reader.done());
    BC_REQUIRE_EQUAL(reader.need(), 0u);

    reader.finish(ec);
    BC_REQUIRE(!ec);

    const auto message = value.payload.get<const setup_connection_success>();
    BC_REQUIRE(message);
    BC_REQUIRE_EQUAL(message->used_version, 2u);
    BC_REQUIRE_EQUAL(message->flags, 0u);
}

BC_AUTO_TEST_CASE(sv2_body__put__short_reads__defer)
{
    const auto data = success_frame();
    const auto head = frame_head(data);
    const auto payload = frame_payload(data);

    auto value = test_frame();
    code ec{};
    body::reader reader{ value };
    reader.init({}, ec);

    // Partial heading defers.
    BC_REQUIRE_EQUAL(reader.put({ head.data(), 5 }, ec), 0u);
    BC_REQUIRE(!ec);
    BC_REQUIRE_EQUAL(reader.need(), heading::size());

    BC_REQUIRE_EQUAL(reader.put({ head.data(), head.size() }, ec), heading::size());
    BC_REQUIRE(!ec);

    // Partial payload defers.
    BC_REQUIRE_EQUAL(reader.put({ payload.data(), 5 }, ec), 0u);
    BC_REQUIRE(!ec);
    BC_REQUIRE(!reader.done());

    BC_REQUIRE_EQUAL(reader.put({ payload.data(), payload.size() }, ec), payload.size());
    BC_REQUIRE(!ec);
    BC_REQUIRE(reader.done());
}

BC_AUTO_TEST_CASE(sv2_body__put__caller_buffer__message)
{
    const auto data = success_frame();
    const auto head = frame_head(data);
    auto buffer = frame_payload(data);

    auto value = test_frame();
    code ec{};
    body::reader reader{ value, buffer };
    reader.init({}, ec);

    BC_REQUIRE_EQUAL(reader.put({ head.data(), head.size() }, ec), heading::size());
    BC_REQUIRE(!ec);
    BC_REQUIRE_EQUAL(reader.put({ buffer.data(), buffer.size() }, ec), buffer.size());
    BC_REQUIRE(!ec);
    BC_REQUIRE(reader.done());
    BC_REQUIRE(value.payload.get<const setup_connection_success>());
}

BC_AUTO_TEST_CASE(sv2_body__put__unregistered_empty__completes_on_heading_without_payload)
{
    const auto data = base16_chunk("00007f000000");
    auto value = test_frame();
    code ec{};
    body::reader reader{ value };
    reader.init({}, ec);

    BC_REQUIRE_EQUAL(reader.put({ data.data(), data.size() }, ec), heading::size());
    BC_REQUIRE(!ec);
    BC_REQUIRE(reader.done());
    BC_REQUIRE(!value.fault);
    BC_REQUIRE(!value.payload);
    BC_REQUIRE_EQUAL(value.head.index(), registry::unknown);
}

BC_AUTO_TEST_CASE(sv2_body__put__unregistered_type__completes_without_payload)
{
    auto value = test_frame();
    BC_REQUIRE(put_done(value, base16_chunk("00007f020000aabb")));
    BC_REQUIRE(!value.fault);
    BC_REQUIRE(!value.payload);
    BC_REQUIRE_EQUAL(value.head.message_type, 0x7fu);
}

BC_AUTO_TEST_CASE(sv2_body__put__extension__completes_without_payload)
{
    auto value = test_frame();
    BC_REQUIRE(put_done(value, base16_chunk("010001060000020000000000")));
    BC_REQUIRE(!value.fault);
    BC_REQUIRE(!value.payload);
    BC_REQUIRE_EQUAL(value.head.extension(), 1u);
}

BC_AUTO_TEST_CASE(sv2_body__put__channel_bit_of_unchanneled__invalid_heading_fault)
{
    auto value = test_frame();
    BC_REQUIRE_EQUAL(put_fault(value, base16_chunk("008001060000020000000000")), network::error::invalid_heading);
}

BC_AUTO_TEST_CASE(sv2_body__put__no_channel_bit_of_channeled__invalid_heading_fault)
{
    auto value = test_frame();
    BC_REQUIRE_EQUAL(put_fault(value, base16_chunk("00002124000001030202202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f")), network::error::invalid_heading);
}

BC_AUTO_TEST_CASE(sv2_body__put__oversized_payload__fault)
{
    auto value = test_frame();
    value.maximum = 5;
    BC_REQUIRE_EQUAL(put_fault(value, success_frame()), network::error::oversized_payload);
}

BC_AUTO_TEST_CASE(sv2_body__put__short_payload__invalid_message_fault)
{
    auto value = test_frame();
    BC_REQUIRE_EQUAL(put_fault(value, base16_chunk("0000010500000200000000")), network::error::invalid_message);
}

BC_AUTO_TEST_CASE(sv2_body__put__trailing_byte__invalid_message_fault)
{
    auto value = test_frame();
    BC_REQUIRE_EQUAL(put_fault(value, base16_chunk("00000107000002000000000000")), network::error::invalid_message);
}

BC_AUTO_TEST_CASE(sv2_body__init__after_message__resets)
{
    auto value = test_frame();
    BC_REQUIRE(put_done(value, success_frame()));
    BC_REQUIRE(value.payload);

    code ec{};
    body::reader reader{ value };
    reader.init({}, ec);
    BC_REQUIRE(!value.payload);
    BC_REQUIRE(!value.fault);
}

BC_AUTO_TEST_CASE(sv2_body__writer__no_buffer__frame_in_data)
{
    auto value = out_frame();
    code ec{};
    body::writer writer{ value };
    writer.init(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(value.data);
    BC_REQUIRE_EQUAL(*value.data, success_frame());

    const auto out = writer.get(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(out);
    BC_REQUIRE(!out->second);
    BC_REQUIRE_EQUAL(out->first.data(), value.data->data());
    BC_REQUIRE_EQUAL(out->first.size(), value.data->size());
}

BC_AUTO_TEST_CASE(sv2_body__writer__buffer__frame_in_buffer)
{
    http::flat_buffer buffer{ 64 * 1024 };
    auto value = out_frame();
    value.buffer = &buffer;

    code ec{};
    body::writer writer{ value };
    writer.init(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(!value.data);

    const auto out = writer.get(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(out);
    BC_REQUIRE(!out->second);
    const auto bytes = pointer_cast<const uint8_t>(out->first.data());
    BC_REQUIRE_EQUAL(data_chunk(bytes, std::next(bytes, out->first.size())), success_frame());
}

BC_AUTO_TEST_CASE(sv2_body__writer__buffer_below_frame__frame_in_data)
{
    http::flat_buffer buffer{ heading::size() };
    auto value = out_frame();
    value.buffer = &buffer;

    code ec{};
    body::writer writer{ value };
    writer.init(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(value.data);
    BC_REQUIRE_EQUAL(*value.data, success_frame());
}

BC_AUTO_TEST_CASE(sv2_body__writer__get_twice__done_and_empty)
{
    auto value = out_frame();
    code ec{};
    body::writer writer{ value };
    writer.init(ec);
    BC_REQUIRE(!writer.done());
    BC_REQUIRE(writer.get(ec));
    BC_REQUIRE(writer.done());
    BC_REQUIRE(!writer.get(ec));
    BC_REQUIRE(!ec);
}

BC_AUTO_TEST_CASE(sv2_body__writer__other_message_index__error)
{
    auto value = out_frame();
    value.index = registry::index_of<set_target>();

    code ec{};
    body::writer writer{ value };
    writer.init(ec);
    BC_REQUIRE(ec);
}

BC_AUTO_TEST_CASE(sv2_body__writer__unknown_index__error)
{
    auto value = out_frame();
    value.index = registry::unknown;

    code ec{};
    body::writer writer{ value };
    writer.init(ec);
    BC_REQUIRE(ec);
}

BC_AUTO_TEST_SUITE_END()
