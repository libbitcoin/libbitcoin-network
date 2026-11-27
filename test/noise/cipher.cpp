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
#include "../test.hpp"
#include "vectors.hpp"

BC_AUTO_TEST_SUITE(noise_cipher_tests)

using namespace network::noise;
using namespace system;

typedef std::vector<sv2_noise::frame> frames;
typedef data_array<cipher::heading_size> heading;

static bool handshake(cipher& initiator, cipher& responder)
{
    cipher::act1 request{};
    cipher::act2 response{};
    cipher::key static_key{};
    cipher::certificate proof{};
    initiator.initiate(request);
    return responder.respond(response, request) &&
        initiator.complete(static_key, proof, response);
}

static bool complete_tampered(size_t index)
{
    cipher initiator{ sv2_noise::initiator_secret,
        sv2_noise::initiator_ellswift };

    cipher::act1 request{};
    cipher::key static_key{};
    cipher::certificate proof{};
    auto response = sv2_noise::message2;
    response.at(index) ^= 1u;
    initiator.initiate(request);
    return initiator.complete(static_key, proof, response);
}

// Byte i of a payload is i mod 256.
static data_chunk plain_frame(const sv2_noise::frame& frame)
{
    const auto extension = to_little_endian(frame.extension);
    const auto length = to_little_endian(frame.size);
    data_chunk out
    {
        extension[0], extension[1], frame.type, length[0], length[1], length[2]
    };

    data_chunk payload(frame.size);
    std::iota(payload.begin(), payload.end(), uint8_t{});
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

static data_chunk seal(cipher& sender, const data_chunk& frame)
{
    const auto payload = frame.size() - cipher::heading_size;
    data_chunk out(cipher::sealed_heading_size + cipher::sealed_size(payload));
    return sender.encrypt(out, frame) ? out : data_chunk{};
}

static data_chunk open(cipher& receiver, const data_chunk& sealed)
{
    const cipher::span in{ sealed };
    heading head{};
    if (!receiver.decrypt_heading(head, in.first(cipher::sealed_heading_size)))
        return {};

    const auto size = from_little_array<size_t>(slice<3, 6>(head));
    data_chunk out(cipher::heading_size + size);
    std::copy(head.begin(), head.end(), out.begin());
    const std::span<uint8_t> payload{ out };
    return receiver.decrypt_payload(payload.subspan(cipher::heading_size),
        in.subspan(cipher::sealed_heading_size)) ? out : data_chunk{};
}

static std::vector<hash_digest> digests(cipher& sender, const frames& values)
{
    std::vector<hash_digest> out{};
    for (const auto& frame: values)
        out.push_back(sha256_hash(seal(sender, plain_frame(frame))));

    return out;
}

static std::vector<hash_digest> expected(const frames& values)
{
    std::vector<hash_digest> out{};
    for (const auto& frame: values)
        out.push_back(frame.digest);

    return out;
}

static std::vector<hash_digest> opened(cipher& sender, cipher& receiver,
    const frames& values)
{
    std::vector<hash_digest> out{};
    for (const auto& frame: values)
        out.push_back(sha256_hash(open(receiver,
            seal(sender, plain_frame(frame)))));

    return out;
}

static std::vector<hash_digest> plains(const frames& values)
{
    std::vector<hash_digest> out{};
    for (const auto& frame: values)
        out.push_back(sha256_hash(plain_frame(frame)));

    return out;
}

BC_AUTO_TEST_CASE(noise_cipher__sealed_size__chunk_boundaries__expected)
{
    BC_REQUIRE_EQUAL(cipher::sealed_size(0), 0u);
    BC_REQUIRE_EQUAL(cipher::sealed_size(1), 17u);
    BC_REQUIRE_EQUAL(cipher::sealed_size(65519), 65535u);
    BC_REQUIRE_EQUAL(cipher::sealed_size(65520), 65552u);
    BC_REQUIRE_EQUAL(cipher::sealed_size(131038), 131070u);
    BC_REQUIRE_EQUAL(cipher::sealed_size(131039), 131087u);
}

BC_AUTO_TEST_CASE(noise_cipher__initiate__vector_keys__message1)
{
    cipher initiator{ sv2_noise::initiator_secret, sv2_noise::initiator_ellswift };
    cipher::act1 out{};
    initiator.initiate(out);
    BC_REQUIRE_EQUAL(out, sv2_noise::message1);
    BC_REQUIRE_EQUAL(initiator.public_key(), sv2_noise::initiator_ellswift);
    BC_REQUIRE_EQUAL(initiator.handshake_hash(), sv2_noise::message1_h);
}

BC_AUTO_TEST_CASE(noise_cipher__respond__message1__message2)
{
    cipher responder{ sv2_noise::responder_secret, sv2_noise::responder_ellswift, sv2_noise::static_secret, sv2_noise::static_ellswift, sv2_noise::signature_noise_message };
    cipher::act2 out{};
    const auto result = responder.respond(out, sv2_noise::message1);
    BC_REQUIRE(result);
    BC_REQUIRE_EQUAL(out, sv2_noise::message2);
    BC_REQUIRE_EQUAL(responder.handshake_hash(), sv2_noise::message2_h);
}

BC_AUTO_TEST_CASE(noise_cipher__complete__message2__static_key_and_certificate)
{
    cipher initiator{ sv2_noise::initiator_secret, sv2_noise::initiator_ellswift };
    cipher::act1 request{};
    cipher::key static_key{};
    cipher::certificate proof{};
    initiator.initiate(request);
    const auto result = initiator.complete(static_key, proof, sv2_noise::message2);
    BC_REQUIRE(result);
    BC_REQUIRE_EQUAL(static_key, sv2_noise::static_ellswift);
    BC_REQUIRE_EQUAL(proof, sv2_noise::signature_noise_message);
    BC_REQUIRE_EQUAL(initiator.handshake_hash(), sv2_noise::message2_h);
}

BC_AUTO_TEST_CASE(noise_cipher__complete__tampered_ephemeral__false)
{
    BC_REQUIRE(!complete_tampered(0));
}

BC_AUTO_TEST_CASE(noise_cipher__complete__tampered_static__false)
{
    BC_REQUIRE(!complete_tampered(64));
}

BC_AUTO_TEST_CASE(noise_cipher__complete__tampered_certificate__false)
{
    BC_REQUIRE(!complete_tampered(144));
}

BC_AUTO_TEST_CASE(noise_cipher__complete__tampered_tag__false)
{
    BC_REQUIRE(!complete_tampered(233));
}

BC_AUTO_TEST_CASE(noise_cipher__encrypt__initiator_frames__digests)
{
    cipher initiator{ sv2_noise::initiator_secret, sv2_noise::initiator_ellswift };
    cipher responder{ sv2_noise::responder_secret, sv2_noise::responder_ellswift, sv2_noise::static_secret, sv2_noise::static_ellswift, sv2_noise::signature_noise_message };
    const auto result = handshake(initiator, responder);
    const auto actual = digests(initiator, sv2_noise::initiator_frames);
    BC_REQUIRE(result);
    BC_REQUIRE_EQUAL(actual, expected(sv2_noise::initiator_frames));
}

BC_AUTO_TEST_CASE(noise_cipher__encrypt__responder_frames__digests)
{
    cipher initiator{ sv2_noise::initiator_secret, sv2_noise::initiator_ellswift };
    cipher responder{ sv2_noise::responder_secret, sv2_noise::responder_ellswift, sv2_noise::static_secret, sv2_noise::static_ellswift, sv2_noise::signature_noise_message };
    const auto result = handshake(initiator, responder);
    const auto actual = digests(responder, sv2_noise::responder_frames);
    BC_REQUIRE(result);
    BC_REQUIRE_EQUAL(actual, expected(sv2_noise::responder_frames));
}

BC_AUTO_TEST_CASE(noise_cipher__decrypt__initiator_frames__plain_frames)
{
    cipher initiator{ sv2_noise::initiator_secret, sv2_noise::initiator_ellswift };
    cipher responder{ sv2_noise::responder_secret, sv2_noise::responder_ellswift, sv2_noise::static_secret, sv2_noise::static_ellswift, sv2_noise::signature_noise_message };
    const auto result = handshake(initiator, responder);
    const auto actual = opened(initiator, responder, sv2_noise::initiator_frames);
    BC_REQUIRE(result);
    BC_REQUIRE_EQUAL(actual, plains(sv2_noise::initiator_frames));
}

BC_AUTO_TEST_CASE(noise_cipher__decrypt__responder_frames__plain_frames)
{
    cipher initiator{ sv2_noise::initiator_secret, sv2_noise::initiator_ellswift };
    cipher responder{ sv2_noise::responder_secret, sv2_noise::responder_ellswift, sv2_noise::static_secret, sv2_noise::static_ellswift, sv2_noise::signature_noise_message };
    const auto result = handshake(initiator, responder);
    const auto actual = opened(responder, initiator, sv2_noise::responder_frames);
    BC_REQUIRE(result);
    BC_REQUIRE_EQUAL(actual, plains(sv2_noise::responder_frames));
}

BC_AUTO_TEST_CASE(noise_cipher__decrypt_heading__tampered_then_intact__false_then_true)
{
    cipher initiator{ sv2_noise::initiator_secret, sv2_noise::initiator_ellswift };
    cipher responder{ sv2_noise::responder_secret, sv2_noise::responder_ellswift, sv2_noise::static_secret, sv2_noise::static_ellswift, sv2_noise::signature_noise_message };
    const auto result = handshake(initiator, responder);
    const auto plain = plain_frame(sv2_noise::initiator_frames.front());
    const auto sealed = seal(initiator, plain);
    auto tampered = sealed;
    tampered.front() ^= 1u;
    heading head{};
    const auto rejected = responder.decrypt_heading(head, tampered);
    const auto accepted = responder.decrypt_heading(head, sealed);
    BC_REQUIRE(result);
    BC_REQUIRE(!rejected);
    BC_REQUIRE(accepted);
    BC_REQUIRE_EQUAL(to_chunk(head), plain);
}

BC_AUTO_TEST_CASE(noise_cipher__complete__generated_keys__static_key_and_matching_hash)
{
    cipher initiator{};
    cipher responder{ sv2_noise::static_secret, sv2_noise::static_ellswift, sv2_noise::signature_noise_message };
    cipher::act1 request{};
    cipher::act2 response{};
    cipher::key static_key{};
    cipher::certificate proof{};
    initiator.initiate(request);
    const auto responded = responder.respond(response, request);
    const auto completed = initiator.complete(static_key, proof, response);
    BC_REQUIRE(responded);
    BC_REQUIRE(completed);
    BC_REQUIRE_EQUAL(static_key, sv2_noise::static_ellswift);
    BC_REQUIRE_EQUAL(proof, sv2_noise::signature_noise_message);
    BC_REQUIRE_EQUAL(initiator.handshake_hash(), responder.handshake_hash());
}

BC_AUTO_TEST_CASE(noise_cipher__decrypt__generated_keys__plain_frames)
{
    cipher initiator{};
    cipher responder{ sv2_noise::static_secret, sv2_noise::static_ellswift, sv2_noise::signature_noise_message };
    const auto result = handshake(initiator, responder);
    const auto actual = opened(initiator, responder, sv2_noise::initiator_frames);
    BC_REQUIRE(result);
    BC_REQUIRE_EQUAL(actual, plains(sv2_noise::initiator_frames));
}

BC_AUTO_TEST_SUITE_END()
