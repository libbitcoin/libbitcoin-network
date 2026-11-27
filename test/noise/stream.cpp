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
#include <optional>
#include "../test.hpp"
#include "vectors.hpp"

BC_AUTO_TEST_SUITE(noise_stream_tests)

using namespace network::noise;
using system::data_chunk;

constexpr uint32_t validity = 3600;

// A core frame of message type one with the given payload.
static data_chunk frame(const data_chunk& payload)
{
    const auto size = payload.size();
    data_chunk out
    {
        0x00, 0x00, 0x01,
        system::narrow_cast<uint8_t>(size),
        system::narrow_cast<uint8_t>(size >> 8),
        system::narrow_cast<uint8_t>(size >> 16)
    };

    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

struct noise_stream_fixture
{
    DELETE_COPY_MOVE(noise_stream_fixture);

    noise_stream_fixture() NOEXCEPT
    {
        io::socket server{ service };
        io::socket client{ service };
        io::acceptor acceptor{ service, test::loopback(0) };
        acceptor.async_accept(server, [](const code& ec) NOEXCEPT
        {
            BC_REQUIRE(!ec);
        });
        client.async_connect(acceptor.local_endpoint(),
            [](const code& ec) NOEXCEPT
            {
                BC_REQUIRE(!ec);
            });

        run();
        responder.emplace(std::move(server), instance);
        initiator.emplace(std::move(client));
    }

    ~noise_stream_fixture() NOEXCEPT = default;

    void run() NOEXCEPT
    {
        service.run();
        service.restart();
    }

    void shake(code& responded, code& initiated) NOEXCEPT
    {
        responder->async_handshake([&](const code& ec) NOEXCEPT
        {
            responded = ec;
        });
        initiator->async_handshake([&](const code& ec) NOEXCEPT
        {
            initiated = ec;
        });

        run();
    }

    void shake() NOEXCEPT
    {
        code responded{ error::would_block };
        code initiated{ error::would_block };
        shake(responded, initiated);
        BC_REQUIRE(!responded);
        BC_REQUIRE(!initiated);
    }

    // Each chunk is written by its own write.
    code write(stream& to, const std::vector<data_chunk>& chunks) NOEXCEPT
    {
        code result{};
        for (const auto& chunk: chunks)
        {
            io::async_write(to, io::buffer(chunk),
                [&](const code& ec, size_t) NOEXCEPT
                {
                    if (ec) result = ec;
                });

            run();
        }

        return result;
    }

    data_chunk read(stream& from, size_t size, code& result) NOEXCEPT
    {
        data_chunk out(size);
        result = error::would_block;
        io::async_read(from, io::buffer(out),
            [&](const code& ec, size_t) NOEXCEPT
            {
                result = ec;
            });

        run();
        return out;
    }

    io::context service{};
    const context instance
    {
        sv2_noise::static_secret, sv2_noise::authority_secret, validity
    };

    std::optional<stream> responder{};
    std::optional<stream> initiator{};
};

BC_FIXTURE_TEST_CASE(noise_stream__handshake__both_sides__success, noise_stream_fixture)
{
    code responded{ error::would_block };
    code initiated{ error::would_block };
    shake(responded, initiated);
    BC_REQUIRE(!responded);
    BC_REQUIRE(!initiated);
}

BC_FIXTURE_TEST_CASE(noise_stream__handshake__initiator__authorized_static_key, noise_stream_fixture)
{
    shake();
    system::ec_xonly server{};
    const auto decoded = certificate::to_xonly(server, initiator->static_key());
    const auto verified = certificate::verify(initiator->certificate(), server, sv2_noise::authority_xonly, unix_time());
    BC_REQUIRE(decoded);
    BC_REQUIRE(verified);
}

BC_FIXTURE_TEST_CASE(noise_stream__write__initiator_frame__responder_reads_frame, noise_stream_fixture)
{
    shake();
    const auto expected = frame(system::base16_chunk("00112233445566778899"));
    const auto written = write(*initiator, { expected });
    code read_ec{};
    const auto actual = read(*responder, expected.size(), read_ec);
    BC_REQUIRE(!written);
    BC_REQUIRE(!read_ec);
    BC_REQUIRE_EQUAL(actual, expected);
}

BC_FIXTURE_TEST_CASE(noise_stream__write__responder_frame__initiator_reads_frame, noise_stream_fixture)
{
    shake();
    const auto expected = frame(system::base16_chunk("ffeeddccbbaa"));
    const auto written = write(*responder, { expected });
    code read_ec{};
    const auto actual = read(*initiator, expected.size(), read_ec);
    BC_REQUIRE(!written);
    BC_REQUIRE(!read_ec);
    BC_REQUIRE_EQUAL(actual, expected);
}

BC_FIXTURE_TEST_CASE(noise_stream__write__empty_payload__responder_reads_heading, noise_stream_fixture)
{
    shake();
    const auto expected = frame({});
    const auto written = write(*initiator, { expected });
    code read_ec{};
    const auto actual = read(*responder, expected.size(), read_ec);
    BC_REQUIRE(!written);
    BC_REQUIRE(!read_ec);
    BC_REQUIRE_EQUAL(actual, expected);
}

BC_FIXTURE_TEST_CASE(noise_stream__write__frame_split_across_writes__responder_reads_frame, noise_stream_fixture)
{
    shake();
    const auto expected = frame(system::base16_chunk("0102030405060708"));
    const data_chunk first(expected.begin(), std::next(expected.begin(), 4));
    const data_chunk second(std::next(expected.begin(), 4), expected.end());
    const auto written = write(*initiator, { first, second });
    code read_ec{};
    const auto actual = read(*responder, expected.size(), read_ec);
    BC_REQUIRE(!written);
    BC_REQUIRE(!read_ec);
    BC_REQUIRE_EQUAL(actual, expected);
}

BC_FIXTURE_TEST_CASE(noise_stream__write__two_frames_one_write__responder_reads_both, noise_stream_fixture)
{
    shake();
    const auto one = frame(system::base16_chunk("aa"));
    const auto two = frame(system::base16_chunk("bbcc"));
    const auto expected = system::build_chunk({ one, two });
    const auto written = write(*initiator, { expected });
    code read_ec{};
    const auto actual = read(*responder, expected.size(), read_ec);
    BC_REQUIRE(!written);
    BC_REQUIRE(!read_ec);
    BC_REQUIRE_EQUAL(actual, expected);
}

BC_FIXTURE_TEST_CASE(noise_stream__write__payload_above_chunk__responder_reads_frame, noise_stream_fixture)
{
    shake();
    const auto expected = frame(data_chunk(cipher::chunk_size + 42u, 0x5a));
    const auto written = write(*initiator, { expected });
    code read_ec{};
    const auto actual = read(*responder, expected.size(), read_ec);
    BC_REQUIRE(!written);
    BC_REQUIRE(!read_ec);
    BC_REQUIRE_EQUAL(actual, expected);
}

BC_FIXTURE_TEST_CASE(noise_stream__read__tampered_heading__protocol_violation, noise_stream_fixture)
{
    shake();
    const data_chunk garbage(cipher::sealed_heading_size, 0x00);
    test::write(initiator->next_layer(), io::buffer(garbage));
    code read_ec{};
    read(*responder, cipher::heading_size, read_ec);
    BC_REQUIRE_EQUAL(read_ec, error::protocol_violation);
}

BC_FIXTURE_TEST_CASE(noise_stream__handshake__detected_prefix__initiator_completes, noise_stream_fixture)
{
    cipher client{};
    cipher::act1 act1{};
    cipher::act2 act2{};
    cipher::key key{};
    cipher::certificate proof{};
    data_chunk prefix(tls::stream::detection_size);
    code ignore{};
    code responded{ error::would_block };
    client.initiate(act1);
    test::write(initiator->next_layer(), io::buffer(act1));
    io::read(responder->next_layer(), io::buffer(prefix), ignore);
    responder->async_handshake(prefix, [&](const code& ec) NOEXCEPT { responded = ec; });
    run();
    io::read(initiator->next_layer(), io::buffer(act2), ignore);
    const auto completed = client.complete(key, proof, act2);
    BC_REQUIRE(!responded);
    BC_REQUIRE(completed);
    BC_REQUIRE_EQUAL(key, instance.static_key());
}

BC_AUTO_TEST_SUITE_END()
