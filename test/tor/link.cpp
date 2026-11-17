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

BC_AUTO_TEST_SUITE(tor_link_tests)

using namespace bc::system;
using namespace tor;

// A chutney CERTS cell (as in arti tor-cell testvec_chanmsg.rs), valid at the
// time, for the TLS certificate digest.
constexpr uint64_t now = 1601000000;
const auto link_digest = base16_array("b4fd606b64e4cbd466b8d76cb131069bae6f3aa1878857c9f624e31d77a799b8");
const auto identity_key = base16_array("dcb604db2034b00fd16986d4adb9d16b21cb4e4457a33dec0f538903683e96e9");
const auto rsa_identity = base16_array("2f1fb49bb332a9eec617e41e911c33fb3890aef3");
const auto link_x509 = base16_chunk("3082023a308201a3a003020102020872ab0fe86c771604300d06092a864886f70d01010b0500301f311d301b06035504030c147777772e74636a76356b766a646472322e636f6d301e170d3230303632373030303030305a170d3231303231363233353935395a301c311a301806035504030c117777772e617775676863646e732e6e657430820122300d06092a864886f70d01010105000382010f003082010a0282010100fe255e99e10be986dda5dfb4ad7c39e8efe4e5d79e105ab976942b6829386eb1d7b7afb5ed4fcbec8c1a34c34fd2ed182f8b42a0363453b55ee371f21347235277e2357f28aa10dd82d4248f7d1894ab5d2979654f71c350f73ad9f918d71f34f1505f77ec16391c8340f392f3684ba4bb6a7b022c3954c1158f152f9e3b4e328fab2e4e881903ca0e32204b0fda8ab62b6e215a7bdec3f2ae41dc58eec1252f643a1812a5d71802f48d6133df79955e01f0287bb4ab8cf95bebcd2ec66ef5b38c7b5ec08ded3d6e2a1d57289d773d5cb932803b6d97784d1da36ae71c073adbb7393714001cd7a082512772a7076dc01166417af7c50aef7f15fd0806eb07ff0203010001300d06092a864886f70d01010b050003818100cda4e06bb6bb238b91188a077eeb0a4722518090fd116af6cdeeb0e85cf6d8e242cf861f973a252eb5116709373b5ac817e6f848e2cbd76aef684350d3416aee5f33d56b96982509cdaf11cca7260e5adf6ddd0d017ae9a575c999adae50b3fb7027e810d47c8c5c14618c0aa10d307e304fbe50e868adecb2c9536e2db51be0");
const auto rsa_x509 = base16_chunk("308201b930820122a0030201020208607c28be6c390943300d06092a864886f70d01010b0500301f311d301b06035504030c147777772e74636a76356b766a646472322e636f6d301e170d3230303831303030303030305a170d3231303831303030303030305a301f311d301b06035504030c147777772e74636a76356b766a646472322e636f6d30819f300d06092a864886f70d010101050003818d0030818902818100d38b1e6ceb946e0db0751f4cbace3dcb9688b6c25304227b4710c35afb73627e50500f5913e158b621802612d1c75827003703338375237552eb3cd3c12f6ab3604e60c1a2d26bb1fbad206ff023969a90909d6a65a5458a5312c26ebd3a3dad30302d4515cdcd264146ac18e6fc60a04bd3ec327f04294d96ba5aa25b464c3f0203010001300d06092a864886f70d01010b0500038181003bce561ea7f95cc00b78aab5d69573ff301c282a751d4a651921d042f1becdba24d918a6d8a5e138dc07bba0b335478ae37abd2c93a93932442ae9084329e846170fe0fc4a50aafc804f311cc3ca4f41d845a7ba5901cbbc3e021e9794aac70ce1f37b0a951592db1b64f2b4afb81ae52dbd9b6fede96a5fb8125eb6251ee50a");
const auto signing_certificate = base16_chunk("01040006cc2a01f82294b866a31f01fc5d0da8572850a9b929545c3266558d7d2316e3b74172b00100200400dcb604db2034b00fd16986d4adb9d16b21cb4e4457a33dec0f538903683e96e9ff1a5203fa27f86ef7528d89a0845d2520166e340754ffea2aae0f612b7ce5da094a0236cdac45034b0b6842c18e7f6b51b93a3cf7e60663b8ad061c30a62602");
const auto link_certificate = base16_chunk("01050006c98a03b4fd606b64e4cbd466b8d76cb131069bae6f3aa1878857c9f624e31d77a799b8007173e5f8068431d0d3f5ee16b4c9ffd59df373e152a87281bae744aa5fcf72171bf4b27c4e8fc1c6a9fc5ca11058bc49647063d7903cfd9f512f89099b27bc0c");
const auto rsa_crosscert = base16_chunk("dcb604db2034b00fd16986d4adb9d16b21cb4e4457a33dec0f538903683e96e90006da3a805cf6006f9179066534de6b45ad47a5c469063ee462762723396dc9f25452a0a52da3f5087dd239f2a311f6b0d4dfeff4abd089dc3d0237a0abab19eb2045b91cdcaf04be0a72d548a27bf2e77bd876ecfe5e1be622350da6bf31f6e306ed896488dd5b39409b23fc3eb7b2c9f7328eb18da36d54d80575899ea6507ccbfcdf1f");
const auto relay_netinfo = base16_chunk("5f6f80e104047f0000010104047f000001");
const auto client_netinfo = base16_chunk("0000000004047f00000100");
const auto localhost = ip::address{ ip::ipv4{ { 127, 0, 0, 1 } } };

struct entry
{
    uint8_t type;
    data_chunk body;
};

using entries = std::vector<entry>;

static data_chunk certs_of(const entries& list)
{
    data_chunk out{};
    write::bytes::data sink(out);
    sink.write_byte(possible_narrow_cast<uint8_t>(list.size()));
    for (const auto& value: list)
    {
        const auto size = possible_narrow_cast<uint16_t>(value.body.size());
        sink.write_byte(value.type);
        sink.write_2_bytes_big_endian(size);
        sink.write_bytes(value.body);
    }

    sink.flush();
    return out;
}

static entries chutney()
{
    return
    {
        { certificate_type::link_x509, link_x509 },
        { certificate_type::rsa_identity, rsa_x509 },
        { certificate_type::identity_signing, signing_certificate },
        { certificate_type::signing_link, link_certificate },
        { certificate_type::rsa_crosscert, rsa_crosscert }
    };
}

static data_chunk versions_of(const data_chunk& versions)
{
    return encode({ 0, command::versions, versions }, true);
}

// The responder handshake: VERSIONS, CERTS, AUTH_CHALLENGE and NETINFO.
static data_chunk responder(const entries& list=chutney(),
    const data_chunk& versions=base16_chunk("00030004" "0005"))
{
    const auto methods = base16_chunk("00010001");
    const auto challenge = splice(data_chunk(32, 0x07), methods);
    return build_chunk(
    {
        versions_of(versions),
        encode({ 0, command::certs, certs_of(list) }),
        encode({ 0, command::auth_challenge, challenge }),
        encode({ 0, command::netinfo, relay_netinfo })
    });
}

static entries without(uint8_t type)
{
    auto list = chutney();
    std::erase_if(list, [=](const entry& value) NOEXCEPT
    {
        return value.type == type;
    });

    return list;
}

BC_AUTO_TEST_CASE(tor_link__construct__versions_output)
{
    tor::link handshake{ link_digest, localhost, now };
    BC_REQUIRE_EQUAL(handshake.output(), versions_of(base16_chunk("00040005")));
    BC_REQUIRE(!handshake.is_established());
    BC_REQUIRE(!handshake.is_failed());
}

BC_AUTO_TEST_CASE(tor_link__receive__chutney__established)
{
    tor::link handshake{ link_digest, localhost, now };
    handshake.output().clear();
    const auto received = handshake.receive(responder());
    const auto expected = encode({ 0, command::netinfo, client_netinfo });

    BC_REQUIRE(received);
    BC_REQUIRE(handshake.is_established());
    BC_REQUIRE_EQUAL(handshake.version(), 5u);
    BC_REQUIRE_EQUAL(handshake.rsa_identity(), rsa_identity);
    BC_REQUIRE_EQUAL(handshake.ed25519_identity(), identity_key);
    BC_REQUIRE_EQUAL(handshake.output(), expected);
    BC_REQUIRE(handshake.remainder().empty());
}

BC_AUTO_TEST_CASE(tor_link__receive__split__established)
{
    tor::link handshake{ link_digest, localhost, now };
    const auto data = responder();
    const auto half = data.size() / 2u;
    const auto first = handshake.receive({ data.data(), half });
    const auto established_early = handshake.is_established();
    const auto second = handshake.receive({ std::next(data.data(), half), data.size() - half });

    BC_REQUIRE(first);
    BC_REQUIRE(!established_early);
    BC_REQUIRE(second);
    BC_REQUIRE(handshake.is_established());
}

BC_AUTO_TEST_CASE(tor_link__receive__trailing_cell__remainder)
{
    tor::link handshake{ link_digest, localhost, now };
    const auto trailing = encode({ 0, command::padding, {} });
    BC_REQUIRE(handshake.receive(splice(responder(), trailing)));
    BC_REQUIRE(handshake.is_established());
    BC_REQUIRE_EQUAL(handshake.remainder(), trailing);
}

BC_AUTO_TEST_CASE(tor_link__receive__padding_between__established)
{
    tor::link handshake{ link_digest, localhost, now };
    const auto padding = encode({ 0, command::vpadding, base16_chunk("00") });
    const auto data = build_chunk(
    {
        versions_of(base16_chunk("0004")),
        padding,
        encode({ 0, command::certs, certs_of(chutney()) }),
        padding,
        encode({ 0, command::netinfo, relay_netinfo })
    });

    BC_REQUIRE(handshake.receive(data));
    BC_REQUIRE(handshake.is_established());
    BC_REQUIRE_EQUAL(handshake.version(), 4u);
}

BC_AUTO_TEST_CASE(tor_link__receive__no_common_version__version)
{
    tor::link handshake{ link_digest, localhost, now };
    BC_REQUIRE(!handshake.receive(responder(chutney(), base16_chunk("00030006"))));
    BC_REQUIRE(handshake.reason() == tor::link::failure::version);
}

BC_AUTO_TEST_CASE(tor_link__receive__odd_versions__malformed)
{
    tor::link handshake{ link_digest, localhost, now };
    BC_REQUIRE(!handshake.receive(versions_of(base16_chunk("000400"))));
    BC_REQUIRE(handshake.reason() == tor::link::failure::malformed);
}

BC_AUTO_TEST_CASE(tor_link__receive__netinfo_before_certs__unexpected)
{
    tor::link handshake{ link_digest, localhost, now };
    const auto data = splice(versions_of(base16_chunk("0004")), encode({ 0, command::netinfo, relay_netinfo }));
    BC_REQUIRE(!handshake.receive(data));
    BC_REQUIRE(handshake.reason() == tor::link::failure::unexpected);
}

BC_AUTO_TEST_CASE(tor_link__receive__other_tls_certificate__certificate)
{
    tor::link handshake{ null_hash, localhost, now };
    BC_REQUIRE(!handshake.receive(responder()));
    BC_REQUIRE(handshake.reason() == tor::link::failure::certificate);
}

BC_AUTO_TEST_CASE(tor_link__receive__expired_link_certificate__expired)
{
    tor::link handshake{ link_digest, localhost, 1601316000 };
    BC_REQUIRE(!handshake.receive(responder()));
    BC_REQUIRE(handshake.reason() == tor::link::failure::expired);
}

BC_AUTO_TEST_CASE(tor_link__receive__without_crosscert__certificate)
{
    tor::link handshake{ link_digest, localhost, now };
    BC_REQUIRE(!handshake.receive(responder(without(certificate_type::rsa_crosscert))));
    BC_REQUIRE(handshake.reason() == tor::link::failure::certificate);
}

BC_AUTO_TEST_CASE(tor_link__receive__without_rsa_identity__certificate)
{
    tor::link handshake{ link_digest, localhost, now };
    BC_REQUIRE(!handshake.receive(responder(without(certificate_type::rsa_identity))));
    BC_REQUIRE(handshake.reason() == tor::link::failure::certificate);
}

BC_AUTO_TEST_CASE(tor_link__receive__duplicate_type__certificate)
{
    auto list = chutney();
    list.push_back({ certificate_type::link_x509, link_x509 });
    tor::link handshake{ link_digest, localhost, now };
    BC_REQUIRE(!handshake.receive(responder(list)));
    BC_REQUIRE(handshake.reason() == tor::link::failure::certificate);
}

BC_AUTO_TEST_CASE(tor_link__receive__link_signed_by_identity__certificate)
{
    auto list = without(certificate_type::identity_signing);
    list.push_back({ certificate_type::identity_signing, link_certificate });
    tor::link handshake{ link_digest, localhost, now };
    BC_REQUIRE(!handshake.receive(responder(list)));
    BC_REQUIRE(handshake.reason() == tor::link::failure::certificate);
}

BC_AUTO_TEST_CASE(tor_link__receive__after_failure__false)
{
    tor::link handshake{ null_hash, localhost, now };
    BC_REQUIRE(!handshake.receive(responder()));
    BC_REQUIRE(!handshake.receive(responder()));
}

BC_AUTO_TEST_SUITE_END()
