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
#ifndef LIBBITCOIN_NETWORK_TEST_NOISE_VECTORS_HPP
#define LIBBITCOIN_NETWORK_TEST_NOISE_VECTORS_HPP

#include "../test.hpp"

// sv2 Noise NX (sv2-spec 4), from a reference over the bip324 and bip340
// reference code, reproduced byte for byte by SRI noise_sv2 (main 67e7bd7).

namespace sv2_noise {

constexpr auto protocol_name = "Noise_NX_Secp256k1+EllSwift_ChaChaPoly_SHA256";
constexpr auto authority_key = "9br3pDEyhsQoXmojoXVRsNGBa8xNw1F35npYzXM5yWDfZaxjaEo";

constexpr auto authority_secret = system::base16_array("8db036afbf4962033bf40ab365acbff06ff97557748ec0bfcc73bdaa9f93c10f");
constexpr auto authority_xonly = system::base16_array("a006b0296b471e3234b428d9382de575654ae0c5f6b41056bd2332b07327b048");
constexpr auto static_secret = system::base16_array("b6695a1ef24a25ac673a41b04290a363338c059d173d6ed198d21428261127e6");
constexpr auto static_ellswift = system::base16_array("b064e778df21f39c7c3f1ce01cfa2e7bba72d3f9af40e8bab6a753c7c3e719d2caaf80c8b2c2be72fef69a0282db0e77c62384b3eaf70d46953c6769c5f92044");
constexpr auto initiator_secret = system::base16_array("f33e3ec708f381ba59b55185e0e5c5efebebf5250b9ea94118d8ae2f6543fb08");
constexpr auto initiator_ellswift = system::base16_array("5b19ef8a73dbd8cdefe0b3698b1ecde96697b2b2de6fc50bd5c3265e414bc615837f247b9ecd4d4c5530964706aa43bb73b2ddfc022c5729a522f5d312ee8436");
constexpr auto responder_secret = system::base16_array("ab3a3da4bff749c2d32a8f90ad816c92774a9a9010cd809cc9d155747049b45e");
constexpr auto responder_ellswift = system::base16_array("56a621c640bbb918afe365c689a974f5dbbd63268279f02b7cc8c6e7518e282d6ee596534a476cc385978c4a9f5a0a19eff5fe45cc4a0f24e67f691b5e0d07e2");
constexpr auto certificate_aux = system::base16_array("55ecfd017c7b5ddc8cb19c6af2891e9f56206be5e704e49e05acc9bd0407279f");
constexpr uint16_t version = 0;
constexpr uint32_t valid_from = 1700000000;
constexpr uint32_t not_valid_after = 1900000000;

// Handshake hash (h) and chaining key (ck) after each step.
constexpr auto initialized_h = system::base16_array("922fa32e4f487c0d59caa3bed7899ce3d98db7e13dbd3b7cf2d23dd433dc6104");
constexpr auto initialized_ck = system::base16_array("2eb47881208e9eee1f669f67c66ee70ea9ea88090d503fe830dc4bc83e29bf10");
constexpr auto message1_h = system::base16_array("0ea0bb64b88d73ef60a48565f31874a325ad2c8b6690e6c1cf7cf5f2e9fe4b58");
constexpr auto message1_ck = system::base16_array("2eb47881208e9eee1f669f67c66ee70ea9ea88090d503fe830dc4bc83e29bf10");
constexpr auto ee_h = system::base16_array("08a52fca186fdd8a2d731e525c5fe167ff957f918cb1fe5d71be5da4fc282ba5");
constexpr auto ee_ck = system::base16_array("da941453451c92a7b10de9f41f14f595d0736c429443776e7f5e1000678f9cdc");
constexpr auto es_h = system::base16_array("918fb389415e90a92a119a02c55c1cfd92cf8a7a61d577d12a5dbbec2a0742c5");
constexpr auto es_ck = system::base16_array("2f31a1c5fd1c1847fff458a91e48c3a6533486803527e6ff4fd3c5fa3619e9af");
constexpr auto message2_h = system::base16_array("d530ad16caf7f362591e521c1706e7bf6cb5106b6b8eaac20af78a667cebc0e0");
constexpr auto message2_ck = system::base16_array("2f31a1c5fd1c1847fff458a91e48c3a6533486803527e6ff4fd3c5fa3619e9af");

constexpr auto signature_noise_message = system::base16_array("000000f1536500b33f71f5c52e0c1de2080074aa5bba625860617bdd23bb549936165f70f8ae6011417e81750af780796072ceff32a40d3c20a641408de3de9963c8d9f9c9206a108817");
constexpr auto message1 = system::base16_array("5b19ef8a73dbd8cdefe0b3698b1ecde96697b2b2de6fc50bd5c3265e414bc615837f247b9ecd4d4c5530964706aa43bb73b2ddfc022c5729a522f5d312ee8436");
constexpr auto message2 = system::base16_array("56a621c640bbb918afe365c689a974f5dbbd63268279f02b7cc8c6e7518e282d6ee596534a476cc385978c4a9f5a0a19eff5fe45cc4a0f24e67f691b5e0d07e2bb880058852e92ad144116e557202a3f00049f71d64ed75ad5671387eaec057d71102a540a6b3067db06879349e3425c47e4abd35a787296ba02a456756750ae562a04e58e8a632088f563fdb33bcaf34f4ab700e421b5a33caa5e1d8ada6182abfa48db676fea706988927b2d0d402da7228831c204deeccead0faf8ae549464183691fe233b5642b92b3dc30b69189d280a28a3c52d78e8942a11680972c00dc4ca642d8fe9de21853");

// Transport keys, initiator to responder and responder to initiator.
constexpr auto initiator_key = system::base16_array("be02c290834bd33ed36d8db82dc3c76d62de47d2248724584c7ab285bfe9a922");
constexpr auto responder_key = system::base16_array("956d735e56fee69cb7ea5030c5b9073da0cd6f2b683c7275e3ff1482168bac15");

// Byte i of a payload is i mod 256. Each direction is a sequence of frames
// with nonces counting from zero, the digest the sha256 of the frame.
struct frame
{
    uint16_t extension;
    uint8_t type;
    uint32_t size;
    system::hash_digest digest;
};

const std::vector<frame> initiator_frames
{
    { 0x0000, 0x1f, 0, system::base16_array("19c5fcc3a15cae3991af0fd0bb571dd5184c49e476fbba68772f1809ddb01bee") },
    { 0x8000, 0x20, 1, system::base16_array("f914330f6d90ce48bf3f067739ccd52ad11d2b2383981cf05539de025bb879c0") },
    { 0x0000, 0x21, 6, system::base16_array("9394b4e16f83e85b37857a3a472165f6ab52c26b2f451999ba8a978d98a43d96") },
    { 0x8000, 0x22, 65518, system::base16_array("0d3fa09370bd9788b67c54f65a1f539b583fbf443c80aaa65b3d8a715c2b1f1b") },
    { 0x0000, 0x23, 65519, system::base16_array("9763ed79afaa015328646811ec8d90d947866022523bea6026f1f0246c19c7b7") },
    { 0x8000, 0x24, 65520, system::base16_array("2a98a0095338779aa26b77300dd2c99a43c617fdac21817c11e7127fd5cdada4") },
    { 0x0000, 0x25, 131038, system::base16_array("09839606c59e6550574c4a4ce90d776d2f6f7776bbfd86b39cbe4a52292d60a5") },
    { 0x8000, 0x26, 131039, system::base16_array("ae4155e87974a5d12e23558721b4603498f8ade5298d1ab31085cfa5447d3428") },
    { 0x0000, 0x27, 200000, system::base16_array("c8309a14b1d86dfcf732aa7760c65fa5fab916c491afead20b5d6ee8061259d5") },
};

const std::vector<frame> responder_frames
{
    { 0x0000, 0x1f, 0, system::base16_array("e04768ca15b045bce6f5fd6e35fe3e3c8a87cbf634a658ca77463bb90a19f216") },
    { 0x8000, 0x20, 1, system::base16_array("0a548afc0545932e94437fa8b2b6eb65cfc33d8eaa51b533b42fe81a26fe8ed2") },
    { 0x0000, 0x21, 6, system::base16_array("0d5e7a7483ad5c3708c8990ee1a48c23348f6f2f3250f631f57f700d5bf1c52e") },
    { 0x8000, 0x22, 65518, system::base16_array("eb36eeac27d3d4c80bd8a53fee181b1ce8432f1d755d690c0b86a047c608273d") },
    { 0x0000, 0x23, 65519, system::base16_array("4e32c2d37a1f62c881bcea7bace906756fca166c05794dbd5a289380fe312f47") },
    { 0x8000, 0x24, 65520, system::base16_array("f094d87c1636910b569c9f95db8a41f2d8afa4301b8fb0f860ba0b23cbc97d6f") },
    { 0x0000, 0x25, 131038, system::base16_array("d7329d81b0bc66c4c25d244ad92672183d32df928582418ad575379250c52e08") },
    { 0x8000, 0x26, 131039, system::base16_array("694a8b27b0e88ecb9cdc95f20f6b1d349964b7cadce9a4c13d6965833f4f66f8") },
    { 0x0000, 0x27, 200000, system::base16_array("5faf43101d69d392eba2c6f764bbaaa9522190d58e694ac53d4bb3d2e55fc891") },
};

} // namespace sv2_noise

#endif
