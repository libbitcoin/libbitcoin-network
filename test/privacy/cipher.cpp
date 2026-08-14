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

BOOST_AUTO_TEST_SUITE(privacy_cipher_tests)

using namespace network::privacy;
using namespace system;

// The bip324 test vectors are computed for mainnet magic.
constexpr uint32_t mainnet = 0xd9b4bef9;

// bips.dev/324 packet encoding test vector 1 (initiating, packet index 1).
BOOST_AUTO_TEST_CASE(privacy_cipher__encrypt__vector_1__expected)
{
    const ec_secret secret = base16_array("61062ea5071d800bbfd59e2e8b53d47d194b095ae5a4df04936b49772ef0d4d7");
    const cipher::key ours = base16_array("ec0adff257bbfe500c188c80b4fdd640f6b45a482bbc15fc7cef5931deff0aa186f6eb9bba7b85dc4dcc28b28722de1e3d9108b985e2967045668f66098e475b");
    const cipher::key theirs = base16_array("a4a94dfce69b4a2a0a099313d10f9f7e7d649d60501c9e1d274c300e0d89aafaffffffffffffffffffffffffffffffffffffffffffffffffffffffff8faf88d5");
    const cipher::terminator send_terminator = base16_array("faef555dfcdb936425d84aba524758f3");
    const cipher::terminator receive_terminator = base16_array("02cb8ff24307a6e27de3b4e7ea3fa65b");
    const hash_digest session_id = base16_array("ce72dffb015da62b0d0f5474cab8bc72605225b0cee3f62312ec680ec5f41ba5");
    const auto contents = base16_chunk("8e");
    const auto expected = base16_chunk("7530d2a18720162ac09c25329a60d75adf36eda3c3");

    cipher self{ secret, ours };
    BOOST_REQUIRE_EQUAL(self.public_key(), ours);
    BOOST_REQUIRE(self.initialize(theirs, mainnet, true));
    BOOST_REQUIRE_EQUAL(self.session_id(), session_id);
    BOOST_REQUIRE_EQUAL(self.send_terminator(), send_terminator);
    BOOST_REQUIRE_EQUAL(self.receive_terminator(), receive_terminator);

    // Seek to packet index 1 with one decoy encryption.
    data_chunk decoy(cipher::expansion);
    self.encrypt({}, {}, true, decoy);

    data_chunk out(contents.size() + cipher::expansion);
    self.encrypt(contents, {}, false, out);
    BOOST_REQUIRE_EQUAL(out, expected);
}

// bips.dev/324 packet encoding test vector 2 (responding, packet index 999).
BOOST_AUTO_TEST_CASE(privacy_cipher__encrypt__vector_2__expected)
{
    const ec_secret secret = base16_array("6f312890ec83bbb26798abaadd574684a53e74ccef7953b790fcc29409080246");
    const cipher::key ours = base16_array("a8785af31c029efc82fa9fc677d7118031358d7c6a25b5779a9b900e5ccd94aac97eb36a3c5dbcdb2ca5843cc4c2fe0aaa46d10eb3d233a81c3dde476da00eef");
    const cipher::key theirs = base16_array("fffffffffffffffffffffffffffffffffffffffffffffffffffffffefffffc2f0000000000000000000000000000000000000000000000000000000000000000");
    const cipher::terminator send_terminator = base16_array("44737108aec5f8b6c1c277b31bbce9c1");
    const cipher::terminator receive_terminator = base16_array("ca29b3a35237f8212bd13ed187a1da2e");
    const hash_digest session_id = base16_array("b0490e26111cb2d55bbff2ace00f7f644f64006539abb4e7513f05107bb10608");
    const auto contents = base16_chunk("3eb1d4e98035cfd8eeb29bac969ed3824a");
    const auto expected = base16_chunk("d78adbcba0eebfb15cfbd8142c84dc729d233d0dc11b1d851e46a114122b8d5b96b7d59317");

    cipher self{ secret, ours };
    BOOST_REQUIRE(self.initialize(theirs, mainnet, false));
    BOOST_REQUIRE_EQUAL(self.session_id(), session_id);
    BOOST_REQUIRE_EQUAL(self.send_terminator(), send_terminator);
    BOOST_REQUIRE_EQUAL(self.receive_terminator(), receive_terminator);

    // Seek to packet index 999 with decoy encryptions (crosses rekeying).
    data_chunk decoy(cipher::expansion);
    for (size_t packet{}; packet < 999; ++packet)
        self.encrypt({}, {}, true, decoy);

    data_chunk out(contents.size() + cipher::expansion);
    self.encrypt(contents, {}, false, out);
    BOOST_REQUIRE_EQUAL(out, expected);
}

// A full session between two generated keypairs, both directions.
BOOST_AUTO_TEST_CASE(privacy_cipher__round_trip__both_directions__expected)
{
    cipher alpha{};
    cipher beta{};
    BOOST_REQUIRE(alpha.initialize(beta.public_key(), mainnet, true));
    BOOST_REQUIRE(beta.initialize(alpha.public_key(), mainnet, false));

    // Both sides derive the same session and mirrored terminators.
    BOOST_REQUIRE_EQUAL(alpha.session_id(), beta.session_id());
    BOOST_REQUIRE_EQUAL(alpha.send_terminator(), beta.receive_terminator());
    BOOST_REQUIRE_EQUAL(alpha.receive_terminator(), beta.send_terminator());

    const auto aad = base16_chunk("f00dfeed");
    const auto contents = base16_chunk("00112233445566778899aabbccddeeff");
    data_chunk packet(contents.size() + cipher::expansion);
    alpha.encrypt(contents, aad, false, packet);

    // Decrypt on the far side (length, then contents).
    const auto length = beta.decrypt_length(std::span<const uint8_t>{ packet.data(), cipher::length_size });
    BOOST_REQUIRE_EQUAL(length, contents.size());

    bool ignore{ true };
    data_chunk decrypted(length);
    const std::span<const uint8_t> rest{ std::next(packet.data(), cipher::length_size), packet.size() - cipher::length_size };
    BOOST_REQUIRE(beta.decrypt(decrypted, aad, ignore, rest));
    BOOST_REQUIRE(!ignore);
    BOOST_REQUIRE_EQUAL(decrypted, contents);

    // Reply direction.
    data_chunk reply(contents.size() + cipher::expansion);
    beta.encrypt(contents, {}, true, reply);

    const auto reply_length = alpha.decrypt_length(std::span<const uint8_t>{ reply.data(), cipher::length_size });
    BOOST_REQUIRE_EQUAL(reply_length, contents.size());

    data_chunk reply_decrypted(reply_length);
    const std::span<const uint8_t> reply_rest{ std::next(reply.data(), cipher::length_size), reply.size() - cipher::length_size };
    BOOST_REQUIRE(alpha.decrypt(reply_decrypted, {}, ignore, reply_rest));
    BOOST_REQUIRE(ignore);
    BOOST_REQUIRE_EQUAL(reply_decrypted, contents);
}

// Tampered packets do not authenticate.
BOOST_AUTO_TEST_CASE(privacy_cipher__decrypt__tampered__false)
{
    cipher alpha{};
    cipher beta{};
    BOOST_REQUIRE(alpha.initialize(beta.public_key(), mainnet, true));
    BOOST_REQUIRE(beta.initialize(alpha.public_key(), mainnet, false));

    const auto contents = base16_chunk("deadbeef");
    data_chunk packet(contents.size() + cipher::expansion);
    alpha.encrypt(contents, {}, false, packet);
    packet.back() ^= 0x01;

    const auto length = beta.decrypt_length(std::span<const uint8_t>{ packet.data(), cipher::length_size });
    BOOST_REQUIRE_EQUAL(length, contents.size());

    bool ignore{};
    data_chunk decrypted(length);
    const std::span<const uint8_t> rest{ std::next(packet.data(), cipher::length_size), packet.size() - cipher::length_size };
    BOOST_REQUIRE(!beta.decrypt(decrypted, {}, ignore, rest));
}

BOOST_AUTO_TEST_SUITE_END()
