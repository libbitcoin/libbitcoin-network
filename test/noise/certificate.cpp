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

BC_AUTO_TEST_SUITE(noise_certificate_tests)

using namespace network::noise;
using namespace system;

static ec_xonly server_xonly()
{
    ec_xonly out{};
    certificate::to_xonly(out, sv2_noise::static_ellswift);
    return out;
}

static bool verify_at(uint32_t now)
{
    return certificate::verify(sv2_noise::signature_noise_message,
        server_xonly(), sv2_noise::authority_xonly, now);
}

static bool verify_tampered(size_t index)
{
    auto tampered = sv2_noise::signature_noise_message;
    tampered.at(index) ^= 1u;
    return certificate::verify(tampered, server_xonly(),
        sv2_noise::authority_xonly, sv2_noise::valid_from);
}

static std::string encode_prefixed(uint8_t first, uint8_t second)
{
    const data_array<2> prefix{ first, second };
    return encode_base58(insert_checksum<38>(
    {
        prefix,
        sv2_noise::authority_xonly
    }));
}

BC_AUTO_TEST_CASE(noise_certificate__to_xonly__authority_secret__authority_xonly)
{
    ec_xonly out{};
    const auto result = certificate::to_xonly(out, sv2_noise::authority_secret);
    BC_REQUIRE(result);
    BC_REQUIRE_EQUAL(out, sv2_noise::authority_xonly);
}

BC_AUTO_TEST_CASE(noise_certificate__to_xonly__invalid_secret__false)
{
    ec_xonly out{};
    BC_REQUIRE(!certificate::to_xonly(out, ec_secret{}));
}

BC_AUTO_TEST_CASE(noise_certificate__to_xonly__static_ellswift__static_secret_xonly)
{
    ec_xonly expected{};
    ec_xonly out{};
    const auto from_secret = certificate::to_xonly(expected, sv2_noise::static_secret);
    const auto from_key = certificate::to_xonly(out, sv2_noise::static_ellswift);
    BC_REQUIRE(from_secret);
    BC_REQUIRE(from_key);
    BC_REQUIRE_EQUAL(out, expected);
}

BC_AUTO_TEST_CASE(noise_certificate__sign__vector_inputs__signature_noise_message)
{
    certificate::data out{};
    const auto result = certificate::sign(out, sv2_noise::authority_secret, server_xonly(), sv2_noise::valid_from, sv2_noise::not_valid_after, sv2_noise::certificate_aux);
    BC_REQUIRE(result);
    BC_REQUIRE_EQUAL(out, sv2_noise::signature_noise_message);
}

BC_AUTO_TEST_CASE(noise_certificate__sign__random_auxiliary__verifies)
{
    certificate::data out{};
    const auto issued = certificate::sign(out, sv2_noise::authority_secret, server_xonly(), sv2_noise::valid_from, sv2_noise::not_valid_after);
    const auto verified = certificate::verify(out, server_xonly(), sv2_noise::authority_xonly, sv2_noise::valid_from);
    BC_REQUIRE(issued);
    BC_REQUIRE(verified);
}

BC_AUTO_TEST_CASE(noise_certificate__sign__invalid_authority__false)
{
    certificate::data out{};
    BC_REQUIRE(!certificate::sign(out, ec_secret{}, server_xonly(), sv2_noise::valid_from, sv2_noise::not_valid_after));
}

BC_AUTO_TEST_CASE(noise_certificate__window__vector__vector_window)
{
    BC_REQUIRE_EQUAL(certificate::valid_from(sv2_noise::signature_noise_message), sv2_noise::valid_from);
    BC_REQUIRE_EQUAL(certificate::not_valid_after(sv2_noise::signature_noise_message), sv2_noise::not_valid_after);
}

BC_AUTO_TEST_CASE(noise_certificate__verify__window_bounds__true)
{
    BC_REQUIRE(verify_at(sv2_noise::valid_from));
    BC_REQUIRE(verify_at(sv2_noise::not_valid_after));
}

BC_AUTO_TEST_CASE(noise_certificate__verify__outside_window__false)
{
    BC_REQUIRE(!verify_at(sv2_noise::valid_from - 1u));
    BC_REQUIRE(!verify_at(sv2_noise::not_valid_after + 1u));
}

BC_AUTO_TEST_CASE(noise_certificate__verify__other_authority__false)
{
    BC_REQUIRE(!certificate::verify(sv2_noise::signature_noise_message, server_xonly(), server_xonly(), sv2_noise::valid_from));
}

BC_AUTO_TEST_CASE(noise_certificate__verify__other_server__false)
{
    BC_REQUIRE(!certificate::verify(sv2_noise::signature_noise_message, sv2_noise::authority_xonly, sv2_noise::authority_xonly, sv2_noise::valid_from));
}

BC_AUTO_TEST_CASE(noise_certificate__verify__tampered_version__false)
{
    BC_REQUIRE(!verify_tampered(0));
}

BC_AUTO_TEST_CASE(noise_certificate__verify__tampered_signature__false)
{
    BC_REQUIRE(!verify_tampered(10));
}

BC_AUTO_TEST_CASE(noise_certificate__encode_authority__vector__vector_key)
{
    BC_REQUIRE_EQUAL(certificate::encode_authority(sv2_noise::authority_xonly), sv2_noise::authority_key);
}

BC_AUTO_TEST_CASE(noise_certificate__encode_authority__specification_key__specification_encoding)
{
    BC_REQUIRE_EQUAL(certificate::encode_authority(base16_array("76637000979c1c11af0c300bcd8c7fe48610fce9b9c11e3daee35ae0b08a7455")), "9bXiEd8boQVhq7WddEcERUL5tyyJVFYdU8th3HfbNXK3Yw6GRXh");
}

BC_AUTO_TEST_CASE(noise_certificate__decode_authority__vector_key__authority_xonly)
{
    ec_xonly out{};
    const auto result = certificate::decode_authority(out, sv2_noise::authority_key);
    BC_REQUIRE(result);
    BC_REQUIRE_EQUAL(out, sv2_noise::authority_xonly);
}

BC_AUTO_TEST_CASE(noise_certificate__decode_authority__other_prefix__false)
{
    ec_xonly out{};
    const auto encoded = encode_prefixed(0x02, 0x00);
    BC_REQUIRE(!certificate::decode_authority(out, encoded));
}

BC_AUTO_TEST_CASE(noise_certificate__decode_authority__bad_checksum__false)
{
    ec_xonly out{};
    BC_REQUIRE(!certificate::decode_authority(out, "9br3pDEyhsQoXmojoXVRsNGBa8xNw1F35npYzXM5yWDfZaxjaEp"));
}

BC_AUTO_TEST_CASE(noise_certificate__decode_authority__malformed__false)
{
    ec_xonly out{};
    BC_REQUIRE(!certificate::decode_authority(out, "0OIl"));
}

BC_AUTO_TEST_SUITE_END()
