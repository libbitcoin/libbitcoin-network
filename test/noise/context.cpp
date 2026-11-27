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

BC_AUTO_TEST_SUITE(noise_context_tests)

using namespace network::noise;
using namespace system;

constexpr uint32_t validity = 3600;
constexpr uint32_t now = 1800000000;

static ec_xonly to_xonly(const cipher::key& key)
{
    ec_xonly out{};
    certificate::to_xonly(out, key);
    return out;
}

static ec_xonly to_xonly(const ec_secret& secret)
{
    ec_xonly out{};
    certificate::to_xonly(out, secret);
    return out;
}

BC_AUTO_TEST_CASE(noise_context__construct__invalid_static_secret__invalid)
{
    const context instance{ ec_secret{}, sv2_noise::signature_noise_message };
    BC_REQUIRE(!instance.valid());
}

BC_AUTO_TEST_CASE(noise_context__construct__invalid_authority_secret__invalid)
{
    const context instance{ sv2_noise::static_secret, ec_secret{}, validity };
    BC_REQUIRE(!instance.valid());
}

BC_AUTO_TEST_CASE(noise_context__construct__pre_issued__static_keypair)
{
    const context instance{ sv2_noise::static_secret, sv2_noise::signature_noise_message };
    BC_REQUIRE(instance.valid());
    BC_REQUIRE(!instance.authority().has_value());
    BC_REQUIRE_EQUAL(instance.static_secret(), sv2_noise::static_secret);
    BC_REQUIRE_EQUAL(to_xonly(instance.static_key()), to_xonly(sv2_noise::static_secret));
}

BC_AUTO_TEST_CASE(noise_context__issue__pre_issued__pre_issued_certificate)
{
    const context instance{ sv2_noise::static_secret, sv2_noise::signature_noise_message };
    certificate::data out{};
    const auto result = instance.issue(out, now);
    BC_REQUIRE(result);
    BC_REQUIRE_EQUAL(out, sv2_noise::signature_noise_message);
}

BC_AUTO_TEST_CASE(noise_context__construct__issuing__authority_key)
{
    const context instance{ sv2_noise::static_secret, sv2_noise::authority_secret, validity };
    BC_REQUIRE(instance.valid());
    BC_REQUIRE(instance.authority().has_value());
    BC_REQUIRE_EQUAL(instance.authority().value(), sv2_noise::authority_xonly);
}

BC_AUTO_TEST_CASE(noise_context__issue__issuing__verifiable_window_around_now)
{
    const context instance{ sv2_noise::static_secret, sv2_noise::authority_secret, validity };
    certificate::data out{};
    const auto result = instance.issue(out, now);
    const auto verified = certificate::verify(out, to_xonly(instance.static_key()), sv2_noise::authority_xonly, now);
    BC_REQUIRE(result);
    BC_REQUIRE(verified);
    BC_REQUIRE_EQUAL(certificate::valid_from(out), now - validity);
    BC_REQUIRE_EQUAL(certificate::not_valid_after(out), now + validity);
}

BC_AUTO_TEST_CASE(noise_context__issue__issuing_near_zero__floored_window)
{
    const context instance{ sv2_noise::static_secret, sv2_noise::authority_secret, validity };
    certificate::data out{};
    const auto result = instance.issue(out, 42);
    BC_REQUIRE(result);
    BC_REQUIRE_EQUAL(certificate::valid_from(out), 0u);
    BC_REQUIRE_EQUAL(certificate::not_valid_after(out), 42u + validity);
}

BC_AUTO_TEST_SUITE_END()
