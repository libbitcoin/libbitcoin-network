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
#include <bitcoin/network/zmtp/cipher.hpp>

#include <algorithm>
#include <random>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace zmtp {

using namespace system;

BC_PUSH_WARNING(NO_ARRAY_INDEXING)
BC_PUSH_WARNING(NO_DYNAMIC_ARRAY_INDEXING)

// Command names (self-describing, length-prefixed on the wire).
constexpr auto command_hello = "HELLO";
constexpr auto command_welcome = "WELCOME";
constexpr auto command_initiate = "INITIATE";
constexpr auto command_ready = "READY";
constexpr auto command_message = "MESSAGE";

// Box nonce prefixes (16 bytes before a short nonce, 8 before a long one).
constexpr auto prefix_hello = "CurveZMQHELLO---";
constexpr auto prefix_welcome = "WELCOME-";
constexpr auto prefix_cookie = "COOKIE--";
constexpr auto prefix_initiate = "CurveZMQINITIATE";
constexpr auto prefix_vouch = "VOUCH---";
constexpr auto prefix_ready = "CurveZMQREADY---";
constexpr auto prefix_client = "CurveZMQMESSAGEC";
constexpr auto prefix_server = "CurveZMQMESSAGES";

// HELLO carries the version and zero padding before the client key.
constexpr uint8_t version_major = 1;
constexpr uint8_t version_minor = 0;
constexpr size_t hello_padding = 72;
constexpr size_t hello_content = 64;

// This is to be used only for ephemeral network session keys.
// Gather 256 bits of operating system entropy, conditioned by sha256.
static hash_digest entropy() NOEXCEPT
{
    using word = std::random_device::result_type;
    constexpr auto words = hash_size / sizeof(word);

    std::random_device device{};
    data_array<words * sizeof(word)> seed{};
    auto it = seed.begin();

    for (size_t count{}; count < words; ++count)
    {
        const auto value = to_little_endian(device());
        it = std::copy(value.begin(), value.end(), it);
    }

    return sha256_hash(seed);
}

// Keys.
// ----------------------------------------------------------------------------

void cipher::generate(key& secret, key& public_key) NOEXCEPT
{
    // A low order result is astronomically improbable.
    do
    {
        secret = entropy();
    }
    while (!to_public(public_key, secret));
}

bool cipher::to_public(key& public_key, const key& secret) NOEXCEPT
{
    return x25519::multiply(public_key, secret);
}

// NaCl crypto_box_beforenm: the hsalsa20 of the x25519 shared point.
bool cipher::derive(key& out, const key& secret, const key& public_key) NOEXCEPT
{
    key point{};
    if (!x25519::multiply(point, secret, public_key))
        return false;

    salsa20::hsalsa20(out, point, {});
    point = {};
    return true;
}

// Nonces.
// ----------------------------------------------------------------------------

cipher::nonce cipher::make_nonce(const std::string& prefix,
    uint64_t value) NOEXCEPT
{
    BC_ASSERT(prefix.size() == nonce_size - short_nonce_size);
    nonce out{};
    const auto suffix = to_big_endian(value);
    auto it = std::copy(prefix.begin(), prefix.end(), out.begin());
    std::copy(suffix.begin(), suffix.end(), it);
    return out;
}

cipher::nonce cipher::make_nonce(const std::string& prefix,
    const long_nonce& value) NOEXCEPT
{
    BC_ASSERT(prefix.size() == nonce_size - long_nonce_size);
    nonce out{};
    auto it = std::copy(prefix.begin(), prefix.end(), out.begin());
    std::copy(value.begin(), value.end(), it);
    return out;
}

// Read a big-endian short nonce at the offset (as libzmq).
static uint64_t short_nonce_value(const std::span<const uint8_t>& data,
    size_t offset) NOEXCEPT
{
    data_array<sizeof(uint64_t)> bytes{};
    std::copy_n(std::next(data.begin(), offset), bytes.size(), bytes.begin());
    return from_big_endian<uint64_t>(bytes);
}

// Peer short nonces must be strictly increasing.
bool cipher::accept(uint64_t nonce) NOEXCEPT
{
    if (nonce <= peer_nonce_)
        return false;

    peer_nonce_ = nonce;
    return true;
}

// Boxes.
// ----------------------------------------------------------------------------

// Append the box (tag then ciphertext) of plain to out.
void cipher::seal(data_chunk& out, box& sealer, const nonce& value,
    const span& plain) NOEXCEPT
{
    const auto offset = out.size();
    out.resize(offset + plain.size() + tag_size);
    const std::span<uint8_t> cipher{ std::next(out.data(), offset),
        plain.size() + tag_size };
    sealer.encrypt(plain, value, cipher);
}

// Open the box (tag then ciphertext) into out.
bool cipher::open(data_chunk& out, box& opener, const nonce& value,
    const span& cipher) NOEXCEPT
{
    if (cipher.size() < tag_size)
        return false;

    out.resize(cipher.size() - tag_size);
    return opener.decrypt(out, value, cipher);
}

// Commands.
// ----------------------------------------------------------------------------

bool cipher::named(const span& body, const std::string& name) NOEXCEPT
{
    return body.size() > name.size() && body.front() == name.size() &&
        std::equal(name.begin(), name.end(), std::next(body.begin()));
}

void cipher::name(data_chunk& out, const std::string& name) NOEXCEPT
{
    out.push_back(possible_narrow_cast<uint8_t>(name.size()));
    out.insert(out.end(), name.begin(), name.end());
}

// Construct.
// ----------------------------------------------------------------------------

cipher::cipher(const key& secret, const key& public_key) NOEXCEPT
  : server_(true), secret_(secret), public_(public_key), peer_{}
{
}

cipher::cipher(const key& secret, const key& public_key,
    const key& server) NOEXCEPT
  : server_(false), secret_(secret), public_(public_key), peer_(server)
{
}

const cipher::key& cipher::public_key() const NOEXCEPT
{
    return public_;
}

const cipher::key& cipher::peer_key() const NOEXCEPT
{
    return peer_;
}

// Handshake.
// ----------------------------------------------------------------------------

// hello = %d5 "HELLO" version padding client-transient nonce box[64 zeros]
bool cipher::hello(data_chunk& out) NOEXCEPT
{
    if (server_)
        return false;

    // The transient keypair and the (C' -> S) box.
    key shared{};
    generate(transient_secret_, transient_public_);
    if (!derive(shared, transient_secret_, peer_))
        return false;

    hello_box_.emplace(shared);
    shared = {};

    out.clear();
    out.reserve(hello_size);
    name(out, command_hello);
    out.push_back(version_major);
    out.push_back(version_minor);
    out.resize(out.size() + hello_padding);
    out.insert(out.end(), transient_public_.begin(), transient_public_.end());
    const auto nonce = to_big_endian(nonce_);
    out.insert(out.end(), nonce.begin(), nonce.end());

    const data_array<hello_content> zeros{};
    seal(out, *hello_box_, make_nonce(prefix_hello, nonce_), zeros);
    ++nonce_;
    return out.size() == hello_size;
}

// welcome = %d7 "WELCOME" long-nonce box[server-transient cookie]
// cookie = long-nonce box[client-transient server-transient-secret]
bool cipher::welcome(data_chunk& out, const span& hello) NOEXCEPT
{
    if (!server_ || hello.size() != hello_size ||
        !named(hello, command_hello))
        return false;

    // Version (padding is not validated, as libzmq).
    auto offset = add1(std::string{ command_hello }.size());
    if (hello[offset] != version_major || hello[add1(offset)] != version_minor)
        return false;

    // Client transient public key and hello nonce.
    offset += 2 + hello_padding;
    std::copy_n(std::next(hello.begin(), offset), key_size,
        peer_transient_.begin());
    offset += key_size;
    const auto nonce = short_nonce_value(hello, offset);
    offset += short_nonce_size;

    // The (S -> C') box authenticates the client transient key.
    key shared{};
    if (!derive(shared, secret_, peer_transient_))
        return false;

    hello_box_.emplace(shared);
    data_chunk content{};
    if (!accept(nonce) || !open(content, *hello_box_,
        make_nonce(prefix_hello, nonce), hello.subspan(offset)))
        return false;

    // The transient keypair and cookie key are per connection.
    generate(transient_secret_, transient_public_);
    cookie_key_ = entropy();

    // The cookie holds the connection state under the cookie key.
    long_nonce cookie_nonce{};
    std::copy_n(entropy().begin(), long_nonce_size, cookie_nonce.begin());
    data_chunk state{};
    state.insert(state.end(), peer_transient_.begin(), peer_transient_.end());
    state.insert(state.end(), transient_secret_.begin(),
        transient_secret_.end());
    data_chunk cookie(cookie_nonce.begin(), cookie_nonce.end());
    box cookie_box{ cookie_key_ };
    seal(cookie, cookie_box, make_nonce(prefix_cookie, cookie_nonce), state);
    state = {};

    // The welcome box (S -> C') carries the server transient key and cookie.
    long_nonce welcome_nonce{};
    std::copy_n(entropy().begin(), long_nonce_size, welcome_nonce.begin());
    data_chunk plain{};
    plain.insert(plain.end(), transient_public_.begin(),
        transient_public_.end());
    plain.insert(plain.end(), cookie.begin(), cookie.end());

    out.clear();
    out.reserve(welcome_size);
    name(out, command_welcome);
    out.insert(out.end(), welcome_nonce.begin(), welcome_nonce.end());
    seal(out, *hello_box_, make_nonce(prefix_welcome, welcome_nonce), plain);
    shared = {};
    return out.size() == welcome_size;
}

// initiate = %d8 "INITIATE" cookie nonce box[client vouch metadata]
// vouch = long-nonce box[client-transient server]
bool cipher::initiate(data_chunk& out, const span& welcome,
    const span& metadata) NOEXCEPT
{
    if (server_ || !hello_box_ || welcome.size() != welcome_size ||
        !named(welcome, command_welcome))
        return false;

    // The welcome box (S -> C') yields the server transient key and cookie.
    const auto offset = add1(std::string{ command_welcome }.size());
    long_nonce welcome_nonce{};
    std::copy_n(std::next(welcome.begin(), offset), long_nonce_size,
        welcome_nonce.begin());
    data_chunk plain{};
    if (!open(plain, *hello_box_, make_nonce(prefix_welcome, welcome_nonce),
        welcome.subspan(offset + long_nonce_size)))
        return false;

    std::copy_n(plain.begin(), key_size, peer_transient_.begin());
    const span cookie{ std::next(plain.data(), key_size), cookie_size };

    // The session box (C' -> S') and the vouch box (C -> S').
    key shared{};
    if (!derive(shared, transient_secret_, peer_transient_))
        return false;

    session_box_.emplace(shared);
    if (!derive(shared, secret_, peer_transient_))
        return false;

    vouch_box_.emplace(shared);
    shared = {};

    // The vouch binds the client transient key to the client long-term key.
    long_nonce vouch_nonce{};
    std::copy_n(entropy().begin(), long_nonce_size, vouch_nonce.begin());
    data_chunk keys{};
    keys.insert(keys.end(), transient_public_.begin(),
        transient_public_.end());
    keys.insert(keys.end(), peer_.begin(), peer_.end());
    data_chunk vouch(vouch_nonce.begin(), vouch_nonce.end());
    seal(vouch, *vouch_box_, make_nonce(prefix_vouch, vouch_nonce), keys);

    // The initiate box (C' -> S') carries the client key, vouch and metadata.
    data_chunk content{};
    content.insert(content.end(), public_.begin(), public_.end());
    content.insert(content.end(), vouch.begin(), vouch.end());
    content.insert(content.end(), metadata.begin(), metadata.end());

    out.clear();
    name(out, command_initiate);
    out.insert(out.end(), cookie.begin(), cookie.end());
    const auto nonce = to_big_endian(nonce_);
    out.insert(out.end(), nonce.begin(), nonce.end());
    seal(out, *session_box_, make_nonce(prefix_initiate, nonce_), content);
    ++nonce_;
    return true;
}

// ready = %d5 "READY" nonce box[metadata]
bool cipher::ready(data_chunk& out, data_chunk& peer_metadata,
    const span& initiate, const span& metadata) NOEXCEPT
{
    if (!server_ || initiate.size() < initiate_minimum ||
        !named(initiate, command_initiate))
        return false;

    // The cookie restores the connection state.
    auto offset = add1(std::string{ command_initiate }.size());
    long_nonce cookie_nonce{};
    std::copy_n(std::next(initiate.begin(), offset), long_nonce_size,
        cookie_nonce.begin());
    box cookie_box{ cookie_key_ };
    data_chunk state{};
    if (!open(state, cookie_box, make_nonce(prefix_cookie, cookie_nonce),
        initiate.subspan(offset + long_nonce_size,
            cookie_size - long_nonce_size)))
        return false;

    // The cookie must be that of this connection.
    if (!std::equal(peer_transient_.begin(), peer_transient_.end(),
        state.begin()) || !std::equal(transient_secret_.begin(),
            transient_secret_.end(), std::next(state.begin(), key_size)))
        return false;

    state = {};
    offset += cookie_size;
    const auto nonce = short_nonce_value(initiate, offset);
    offset += short_nonce_size;

    // The session box (S' -> C') opens the initiate box.
    key shared{};
    if (!derive(shared, transient_secret_, peer_transient_))
        return false;

    session_box_.emplace(shared);
    data_chunk content{};
    if (!accept(nonce) || !open(content, *session_box_,
        make_nonce(prefix_initiate, nonce), initiate.subspan(offset)))
        return false;

    if (content.size() < key_size + cookie_size)
        return false;

    // The client long-term key opens the vouch (C -> S').
    std::copy_n(content.begin(), key_size, peer_.begin());
    if (!derive(shared, transient_secret_, peer_))
        return false;

    vouch_box_.emplace(shared);
    shared = {};

    long_nonce vouch_nonce{};
    std::copy_n(std::next(content.begin(), key_size), long_nonce_size,
        vouch_nonce.begin());
    data_chunk keys{};
    if (!open(keys, *vouch_box_, make_nonce(prefix_vouch, vouch_nonce),
        span{ content }.subspan(key_size + long_nonce_size,
            cookie_size - long_nonce_size)))
        return false;

    // The vouch must bind the client transient key to this server.
    if (!std::equal(peer_transient_.begin(), peer_transient_.end(),
        keys.begin()) || !std::equal(public_.begin(), public_.end(),
            std::next(keys.begin(), key_size)))
        return false;

    peer_metadata.assign(std::next(content.begin(), key_size + cookie_size),
        content.end());

    // The ready box (S' -> C') carries the server metadata.
    out.clear();
    name(out, command_ready);
    const auto ready_nonce = to_big_endian(nonce_);
    out.insert(out.end(), ready_nonce.begin(), ready_nonce.end());
    seal(out, *session_box_, make_nonce(prefix_ready, nonce_), metadata);
    ++nonce_;

    // The handshake keys are no longer required.
    transient_secret_ = {};
    cookie_key_ = {};
    hello_box_.reset();
    vouch_box_.reset();
    return true;
}

bool cipher::complete(data_chunk& peer_metadata, const span& ready) NOEXCEPT
{
    if (server_ || !session_box_ || ready.size() < ready_minimum ||
        !named(ready, command_ready))
        return false;

    const auto offset = add1(std::string{ command_ready }.size());
    const auto nonce = short_nonce_value(ready, offset);

    if (!accept(nonce) || !open(peer_metadata, *session_box_,
        make_nonce(prefix_ready, nonce), ready.subspan(
            offset + short_nonce_size)))
        return false;

    // The handshake keys are no longer required.
    transient_secret_ = {};
    hello_box_.reset();
    vouch_box_.reset();
    return true;
}

// Messages.
// ----------------------------------------------------------------------------

// message = %d7 "MESSAGE" nonce box[flags body]
bool cipher::encode(data_chunk& out, uint8_t flags, const span& body) NOEXCEPT
{
    if (!session_box_)
        return false;

    data_chunk payload{};
    payload.reserve(add1(body.size()));
    payload.push_back(flags);
    payload.insert(payload.end(), body.begin(), body.end());

    out.clear();
    out.reserve(message_minimum + body.size());
    name(out, command_message);
    const auto nonce = to_big_endian(nonce_);
    out.insert(out.end(), nonce.begin(), nonce.end());
    seal(out, *session_box_, make_nonce(server_ ? prefix_server :
        prefix_client, nonce_), payload);
    ++nonce_;
    return true;
}

bool cipher::decode(uint8_t& flags, data_chunk& body,
    const span& message) NOEXCEPT
{
    if (!session_box_ || message.size() < message_minimum ||
        !named(message, command_message))
        return false;

    const auto offset = add1(std::string{ command_message }.size());
    const auto nonce = short_nonce_value(message, offset);

    if (!accept(nonce) || !open(body, *session_box_, make_nonce(server_ ?
        prefix_client : prefix_server, nonce), message.subspan(
            offset + short_nonce_size)))
        return false;

    // The payload flags byte precedes the frame body.
    flags = body.front();
    body.erase(body.begin());
    return true;
}

BC_POP_WARNING()
BC_POP_WARNING()

} // namespace zmtp
} // namespace network
} // namespace libbitcoin
