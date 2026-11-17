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

BC_AUTO_TEST_SUITE(tor_certificate_tests)

using namespace bc::system;
using namespace tor;

// A chutney CERTS cell (as in arti tor-cert testvec_certs.rs).
const auto identity_key = base16_array("dcb604db2034b00fd16986d4adb9d16b21cb4e4457a33dec0f538903683e96e9");
const auto signing_key = base16_array("f82294b866a31f01fc5d0da8572850a9b929545c3266558d7d2316e3b74172b0");
const auto link_digest = base16_array("b4fd606b64e4cbd466b8d76cb131069bae6f3aa1878857c9f624e31d77a799b8");
const auto signing_certificate = base16_chunk("01040006cc2a01f82294b866a31f01fc5d0da8572850a9b929545c3266558d7d2316e3b74172b00100200400dcb604db2034b00fd16986d4adb9d16b21cb4e4457a33dec0f538903683e96e9ff1a5203fa27f86ef7528d89a0845d2520166e340754ffea2aae0f612b7ce5da094a0236cdac45034b0b6842c18e7f6b51b93a3cf7e60663b8ad061c30a62602");
const auto link_certificate = base16_chunk("01050006c98a03b4fd606b64e4cbd466b8d76cb131069bae6f3aa1878857c9f624e31d77a799b8007173e5f8068431d0d3f5ee16b4c9ffd59df373e152a87281bae744aa5fcf72171bf4b27c4e8fc1c6a9fc5ca11058bc49647063d7903cfd9f512f89099b27bc0c");
const auto rsa_key = base16_chunk("30818902818100d38b1e6ceb946e0db0751f4cbace3dcb9688b6c25304227b4710c35afb73627e50500f5913e158b621802612d1c75827003703338375237552eb3cd3c12f6ab3604e60c1a2d26bb1fbad206ff023969a90909d6a65a5458a5312c26ebd3a3dad30302d4515cdcd264146ac18e6fc60a04bd3ec327f04294d96ba5aa25b464c3f0203010001");
const auto rsa_crosscert = base16_chunk("dcb604db2034b00fd16986d4adb9d16b21cb4e4457a33dec0f538903683e96e90006da3a805cf6006f9179066534de6b45ad47a5c469063ee462762723396dc9f25452a0a52da3f5087dd239f2a311f6b0d4dfeff4abd089dc3d0237a0abab19eb2045b91cdcaf04be0a72d548a27bf2e77bd876ecfe5e1be622350da6bf31f6e306ed896488dd5b39409b23fc3eb7b2c9f7328eb18da36d54d80575899ea6507ccbfcdf1f");
const auto signature = data_chunk(ed25519::signature_size, 0x00);

// A certificate of the signing key with the extensions.
static data_chunk with_extensions(const data_chunk& extensions, uint8_t count)
{
    const auto head = base16_chunk("01040006cc2a01");
    const auto body = build_chunk({ head, signing_key, to_chunk(count) });
    return build_chunk({ body, extensions, signature });
}

// certificate

BC_AUTO_TEST_CASE(tor_certificate__parse__signing__expected)
{
    certificate out{};
    BC_REQUIRE(parse(out, signing_certificate));
    BC_REQUIRE_EQUAL(out.type, certificate_type::identity_signing);
    BC_REQUIRE_EQUAL(out.expiration, 0x0006cc2a_u32);
    BC_REQUIRE_EQUAL(out.key_type, key_type::ed25519);
    BC_REQUIRE_EQUAL(out.key, signing_key);
    BC_REQUIRE(out.has_signer);
    BC_REQUIRE_EQUAL(out.signer, identity_key);
    BC_REQUIRE_EQUAL(out.signed_data.size(), signing_certificate.size() - ed25519::signature_size);
}

BC_AUTO_TEST_CASE(tor_certificate__is_signed_by__signing_by_identity__true)
{
    certificate out{};
    BC_REQUIRE(parse(out, signing_certificate));
    BC_REQUIRE(is_signed_by(out, identity_key));
    BC_REQUIRE(!is_signed_by(out, signing_key));
}

BC_AUTO_TEST_CASE(tor_certificate__parse__link__expected)
{
    certificate out{};
    BC_REQUIRE(parse(out, link_certificate));
    BC_REQUIRE_EQUAL(out.type, certificate_type::signing_link);
    BC_REQUIRE_EQUAL(out.expiration, 0x0006c98a_u32);
    BC_REQUIRE_EQUAL(out.key_type, key_type::sha256_x509);
    BC_REQUIRE_EQUAL(out.key, link_digest);
    BC_REQUIRE(!out.has_signer);
}

BC_AUTO_TEST_CASE(tor_certificate__is_signed_by__link_by_signing__true)
{
    certificate out{};
    BC_REQUIRE(parse(out, link_certificate));
    BC_REQUIRE(is_signed_by(out, signing_key));
    BC_REQUIRE(!is_signed_by(out, identity_key));
}

BC_AUTO_TEST_CASE(tor_certificate__parse__link_declared_key__digest)
{
    // The key type follows the version, type and expiration.
    auto data = link_certificate;
    data.at(6) = key_type::ed25519;
    certificate out{};
    BC_REQUIRE(parse(out, data));
    BC_REQUIRE_EQUAL(out.key_type, key_type::sha256_x509);
}

BC_AUTO_TEST_CASE(tor_certificate__parse__unrecognized_extension__ignored)
{
    const auto data = with_extensions(base16_chunk("0001" "99" "00" "ff"), 1);
    certificate out{};
    BC_REQUIRE(parse(out, data));
    BC_REQUIRE(!out.has_signer);
}

BC_AUTO_TEST_CASE(tor_certificate__parse__unrecognized_affecting_extension__false)
{
    const auto data = with_extensions(base16_chunk("0001" "99" "01" "ff"), 1);
    certificate out{};
    BC_REQUIRE(!parse(out, data));
}

BC_AUTO_TEST_CASE(tor_certificate__parse__duplicate_signer__false)
{
    const auto signer = splice(base16_chunk("0020" "04" "00"), identity_key);
    const auto data = with_extensions(splice(signer, signer), 2);
    certificate out{};
    BC_REQUIRE(!parse(out, data));
}

BC_AUTO_TEST_CASE(tor_certificate__parse__short_signer__false)
{
    const auto data = with_extensions(base16_chunk("0001" "04" "00" "ff"), 1);
    certificate out{};
    BC_REQUIRE(!parse(out, data));
}

BC_AUTO_TEST_CASE(tor_certificate__parse__version_two__false)
{
    auto data = signing_certificate;
    data.front() = 2;
    certificate out{};
    BC_REQUIRE(!parse(out, data));
}

BC_AUTO_TEST_CASE(tor_certificate__parse__trailing_byte__false)
{
    const auto data = splice(signing_certificate, base16_chunk("00"));
    certificate out{};
    BC_REQUIRE(!parse(out, data));
}

BC_AUTO_TEST_CASE(tor_certificate__parse__truncated__false)
{
    auto data = signing_certificate;
    data.pop_back();
    certificate out{};
    BC_REQUIRE(!parse(out, data));
}

// crosscert

BC_AUTO_TEST_CASE(tor_certificate__parse_crosscert__chutney__expected)
{
    crosscert out{};
    BC_REQUIRE(parse(out, rsa_crosscert));
    BC_REQUIRE_EQUAL(out.key, identity_key);
    BC_REQUIRE_EQUAL(out.expiration, 0x0006da3a_u32);
    BC_REQUIRE_EQUAL(out.signed_data.size(), 36u);
    BC_REQUIRE_EQUAL(out.signature.size(), 128u);
}

BC_AUTO_TEST_CASE(tor_certificate__is_signed_by__crosscert_by_rsa_identity__true)
{
    crosscert out{};
    rsa::public_key key{};
    BC_REQUIRE(parse(out, rsa_crosscert));
    BC_REQUIRE(rsa::decode(key, rsa_key));
    BC_REQUIRE(is_signed_by(out, key));
}

BC_AUTO_TEST_CASE(tor_certificate__is_signed_by__altered_crosscert__false)
{
    auto data = rsa_crosscert;
    data.front() ^= 0x01;
    crosscert out{};
    rsa::public_key key{};
    BC_REQUIRE(parse(out, data));
    BC_REQUIRE(rsa::decode(key, rsa_key));
    BC_REQUIRE(!is_signed_by(out, key));
}

BC_AUTO_TEST_CASE(tor_certificate__parse_crosscert__trailing_byte__false)
{
    crosscert out{};
    BC_REQUIRE(!parse(out, splice(rsa_crosscert, base16_chunk("00"))));
}

// is_expired

BC_AUTO_TEST_CASE(tor_certificate__is_expired__expiration__true)
{
    BC_REQUIRE(is_expired(0x0006c98a, 1601316000));
    BC_REQUIRE(!is_expired(0x0006c98a, 1601315999));
}

BC_AUTO_TEST_SUITE_END()
