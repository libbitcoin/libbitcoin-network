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
#include <bitcoin/network/tls/context.hpp>

#include <algorithm>
#include <chrono>
#include <string>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace tls {

using namespace system;

context::context() NOEXCEPT
{
}

bool context::set_chain(const std::string& text) NOEXCEPT
{
    x509::certificates certificates{};
    if (!x509::parse(certificates, text))
        return false;

    const auto& leaf = certificates.front();
    if (leaf.curve != x509::curve::secp256r1)
        return false;

    chain value{};
    for (const auto& certificate: certificates)
        value.push_back(certificate.encoding);

    chain_ = std::move(value);
    public_key_ = leaf.public_key;
    if (!matches())
    {
        chain_.clear();
        public_key_.clear();
        return false;
    }

    return true;
}

bool context::set_key(const std::string& text,
    const std::string& password) NOEXCEPT
{
    x509::secret value{};
    if (!x509::decode_private_key(value, text, password))
        return false;

    const auto previous = key_;
    key_ = value;
    has_key_ = true;
    if (!matches())
    {
        key_ = previous;
        has_key_ = false;
        return false;
    }

    return true;
}

bool context::add_anchors(const std::string& text) NOEXCEPT
{
    x509::certificates certificates{};
    if (!x509::parse(certificates, text))
        return false;

    anchors_.insert(anchors_.end(), certificates.begin(), certificates.end());
    return true;
}

void context::set_verify(bool request, bool require) NOEXCEPT
{
    request_ = request;
    require_ = request && require;
}

void context::set_time(uint64_t seconds) NOEXCEPT
{
    time_ = seconds;
}

bool context::is_ready() const NOEXCEPT
{
    return has_key_ && !chain_.empty();
}

const context::chain& context::certificates() const NOEXCEPT
{
    return chain_;
}

const x509::secret& context::key() const NOEXCEPT
{
    return key_;
}

const x509::certificates& context::anchors() const NOEXCEPT
{
    return anchors_;
}

bool context::request() const NOEXCEPT
{
    return request_;
}

bool context::require() const NOEXCEPT
{
    return require_;
}

uint64_t context::time() const NOEXCEPT
{
    if (!is_zero(time_))
        return time_;

    using namespace std::chrono;
    const auto now = system_clock::now().time_since_epoch();
    return possible_narrow_sign_cast<uint64_t>(
        duration_cast<seconds>(now).count());
}

// A key or chain set alone matches, the pair must share the public key.
bool context::matches() const NOEXCEPT
{
    if (!has_key_ || public_key_.empty())
        return true;

    secp256r1::point_t point{};
    return secp256r1::public_key(point, key_) &&
        std::equal(point.begin(), point.end(), public_key_.begin(),
            public_key_.end());
}

} // namespace tls
} // namespace network
} // namespace libbitcoin
