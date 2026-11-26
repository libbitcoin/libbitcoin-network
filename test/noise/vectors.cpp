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

BC_AUTO_TEST_SUITE(noise_vectors_tests)

using namespace bc::system;

constexpr size_t chunk_size = 65519;

template <size_t Size>
static data_array<Size> part(const data_slice& data, size_t offset)
{
    data_array<Size> out{};
    std::copy_n(std::next(data.begin(), offset), Size, out.begin());
    return out;
}

static hash_digest mix_hash(const hash_digest& hash, const data_slice& data)
{
    return sha256_hash(splice(hash, data));
}

static hash_digest ecdh(const ec_secret& secret, const ec_ellswift& initiator,
    const ec_ellswift& responder, bool responding)
{
    hash_digest out{};
    ellswift::exchange(out, secret, initiator, responder, responding);
    return out;
}

static ec_xonly xonly(const ec_ellswift& key)
{
    ec_compressed point{};
    ellswift::decode(point, key);
    return part<ec_xonly_size>(point, one);
}

static void seal(chacha20_poly1305& cipher, uint64_t& nonce,
    const data_chunk& plain, data_chunk& out)
{
    data_chunk sealed(plain.size() + chacha20_poly1305::expansion);
    cipher.encrypt(plain, {}, 0, nonce++, sealed);
    out.insert(out.end(), sealed.begin(), sealed.end());
}

static hash_digest frame_digest(chacha20_poly1305& cipher, uint64_t& nonce,
    const sv2_noise::frame& frame)
{
    const auto extension = to_little_endian(frame.extension);
    const auto length = to_little_endian(frame.size);
    const data_chunk header
    {
        extension[0], extension[1], frame.type, length[0], length[1], length[2]
    };

    data_chunk payload(frame.size);
    std::iota(payload.begin(), payload.end(), uint8_t{});

    data_chunk out{};
    seal(cipher, nonce, header, out);
    for (size_t start = 0; start < payload.size(); start += chunk_size)
    {
        const auto end = std::min(payload.size(), start + chunk_size);
        seal(cipher, nonce, { std::next(payload.begin(), start),
            std::next(payload.begin(), end) }, out);
    }

    return sha256_hash(out);
}

static std::vector<hash_digest> digests(const hash_digest& key,
    const std::vector<sv2_noise::frame>& frames)
{
    uint64_t nonce{};
    chacha20_poly1305 cipher{ key };
    std::vector<hash_digest> out{};
    for (const auto& frame: frames)
        out.push_back(frame_digest(cipher, nonce, frame));

    return out;
}

static std::vector<hash_digest> expected(
    const std::vector<sv2_noise::frame>& frames)
{
    std::vector<hash_digest> out{};
    for (const auto& frame: frames)
        out.push_back(frame.digest);

    return out;
}

BC_AUTO_TEST_CASE(noise_vectors__sha256__protocol_name__initialized)
{
    const std::string name{ sv2_noise::protocol_name };
    const auto ck = sha256_hash(to_chunk(name));
    const auto h = sha256_hash(ck);
    BC_REQUIRE_EQUAL(ck, sv2_noise::initialized_ck);
    BC_REQUIRE_EQUAL(h, sv2_noise::initialized_h);
}

BC_AUTO_TEST_CASE(noise_vectors__mix_hash__message1__message1_h)
{
    const auto h = mix_hash(mix_hash(sv2_noise::initialized_h, sv2_noise::initiator_ellswift), {});
    BC_REQUIRE_EQUAL(sv2_noise::message1, sv2_noise::initiator_ellswift);
    BC_REQUIRE_EQUAL(h, sv2_noise::message1_h);
    BC_REQUIRE_EQUAL(sv2_noise::message1_ck, sv2_noise::initialized_ck);
}

BC_AUTO_TEST_CASE(noise_vectors__ellswift_decode__keys__public_keys)
{
    ec_compressed initiator{};
    ec_compressed responder{};
    ec_compressed server{};
    const auto initiator_public = secret_to_public(initiator, sv2_noise::initiator_secret);
    const auto responder_public = secret_to_public(responder, sv2_noise::responder_secret);
    const auto server_public = secret_to_public(server, sv2_noise::static_secret);
    BC_REQUIRE(initiator_public);
    BC_REQUIRE(responder_public);
    BC_REQUIRE(server_public);
    BC_REQUIRE_EQUAL(xonly(sv2_noise::initiator_ellswift), part<ec_xonly_size>(initiator, one));
    BC_REQUIRE_EQUAL(xonly(sv2_noise::responder_ellswift), part<ec_xonly_size>(responder, one));
    BC_REQUIRE_EQUAL(xonly(sv2_noise::static_ellswift), part<ec_xonly_size>(server, one));
}

BC_AUTO_TEST_CASE(noise_vectors__exchange__ephemeral__ee_ck)
{
    const auto initiator = ecdh(sv2_noise::initiator_secret, sv2_noise::initiator_ellswift, sv2_noise::responder_ellswift, false);
    const auto responder = ecdh(sv2_noise::responder_secret, sv2_noise::initiator_ellswift, sv2_noise::responder_ellswift, true);
    const auto keys = hkdf<sha256>::key<64>(initiator, sv2_noise::message1_ck, {});
    const auto h = mix_hash(sv2_noise::message1_h, sv2_noise::responder_ellswift);
    BC_REQUIRE_EQUAL(initiator, responder);
    BC_REQUIRE_EQUAL(part<hash_size>(keys, zero), sv2_noise::ee_ck);
    BC_REQUIRE_EQUAL(h, sv2_noise::ee_h);
}

BC_AUTO_TEST_CASE(noise_vectors__exchange__static__es_ck)
{
    const auto initiator = ecdh(sv2_noise::initiator_secret, sv2_noise::initiator_ellswift, sv2_noise::static_ellswift, false);
    const auto responder = ecdh(sv2_noise::static_secret, sv2_noise::initiator_ellswift, sv2_noise::static_ellswift, true);
    const auto keys = hkdf<sha256>::key<64>(initiator, sv2_noise::ee_ck, {});
    const auto h = mix_hash(sv2_noise::ee_h, part<80>(sv2_noise::message2, 64));
    BC_REQUIRE_EQUAL(initiator, responder);
    BC_REQUIRE_EQUAL(part<hash_size>(keys, zero), sv2_noise::es_ck);
    BC_REQUIRE_EQUAL(h, sv2_noise::es_h);
}

BC_AUTO_TEST_CASE(noise_vectors__decrypt__message2__static_key_and_certificate)
{
    const auto ee = ecdh(sv2_noise::initiator_secret, sv2_noise::initiator_ellswift, sv2_noise::responder_ellswift, false);
    const auto es = ecdh(sv2_noise::initiator_secret, sv2_noise::initiator_ellswift, sv2_noise::static_ellswift, false);
    chacha20_poly1305 first{ part<hash_size>(hkdf<sha256>::key<64>(ee, sv2_noise::message1_ck, {}), hash_size) };
    chacha20_poly1305 second{ part<hash_size>(hkdf<sha256>::key<64>(es, sv2_noise::ee_ck, {}), hash_size) };
    ec_ellswift server{};
    data_array<74> certificate{};
    const auto opened_server = first.decrypt(server, sv2_noise::ee_h, 0, 0, part<80>(sv2_noise::message2, 64));
    const auto opened_certificate = second.decrypt(certificate, sv2_noise::es_h, 0, 0, part<90>(sv2_noise::message2, 144));
    const auto h = mix_hash(sv2_noise::es_h, part<90>(sv2_noise::message2, 144));
    BC_REQUIRE_EQUAL(part<64>(sv2_noise::message2, zero), sv2_noise::responder_ellswift);
    BC_REQUIRE(opened_server);
    BC_REQUIRE_EQUAL(server, sv2_noise::static_ellswift);
    BC_REQUIRE(opened_certificate);
    BC_REQUIRE_EQUAL(certificate, sv2_noise::signature_noise_message);
    BC_REQUIRE_EQUAL(h, sv2_noise::message2_h);
}

BC_AUTO_TEST_CASE(noise_vectors__schnorr_sign__certificate__signature_noise_message)
{
    const auto header = splice(to_little_endian(sv2_noise::version), to_little_endian(sv2_noise::valid_from), to_little_endian(sv2_noise::not_valid_after));
    const auto hash = sha256_hash(splice(header, xonly(sv2_noise::static_ellswift)));
    ec_signature signature{};
    const auto created = schnorr::sign(signature, sv2_noise::authority_secret, hash, sv2_noise::certificate_aux);
    const auto verified = schnorr::verify_signature(sv2_noise::authority_xonly, hash, signature);
    BC_REQUIRE_EQUAL(header, part<10>(sv2_noise::signature_noise_message, zero));
    BC_REQUIRE(created);
    BC_REQUIRE(verified);
    BC_REQUIRE_EQUAL(signature, part<64>(sv2_noise::signature_noise_message, 10));
}

BC_AUTO_TEST_CASE(noise_vectors__hkdf__split__transport_keys)
{
    const auto keys = hkdf<sha256>::key<64>({}, sv2_noise::message2_ck, {});
    BC_REQUIRE_EQUAL(sv2_noise::message2_ck, sv2_noise::es_ck);
    BC_REQUIRE_EQUAL(part<hash_size>(keys, zero), sv2_noise::initiator_key);
    BC_REQUIRE_EQUAL(part<hash_size>(keys, hash_size), sv2_noise::responder_key);
}

BC_AUTO_TEST_CASE(noise_vectors__encrypt__initiator_frames__digests)
{
    const auto actual = digests(sv2_noise::initiator_key, sv2_noise::initiator_frames);
    BC_REQUIRE_EQUAL(actual, expected(sv2_noise::initiator_frames));
}

BC_AUTO_TEST_CASE(noise_vectors__encrypt__responder_frames__digests)
{
    const auto actual = digests(sv2_noise::responder_key, sv2_noise::responder_frames);
    BC_REQUIRE_EQUAL(actual, expected(sv2_noise::responder_frames));
}

BC_AUTO_TEST_CASE(noise_vectors__base58_check__authority_xonly__authority_key)
{
    const auto key = encode_base58(insert_checksum<38>({ base16_array("0100"), sv2_noise::authority_xonly }));
    BC_REQUIRE_EQUAL(key, sv2_noise::authority_key);
}

BC_AUTO_TEST_CASE(noise_vectors__base58_check__specification_key__specification_encoding)
{
    const auto key = encode_base58(insert_checksum<38>({ base16_array("0100"), base16_array("76637000979c1c11af0c300bcd8c7fe48610fce9b9c11e3daee35ae0b08a7455") }));
    BC_REQUIRE_EQUAL(key, "9bXiEd8boQVhq7WddEcERUL5tyyJVFYdU8th3HfbNXK3Yw6GRXh");
}

BC_AUTO_TEST_SUITE_END()
