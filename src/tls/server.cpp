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
#include <bitcoin/network/tls/server.hpp>

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/tls/codec.hpp>
#include <bitcoin/network/tls/constants.hpp>

// rfc8446 (tls 1.3)

namespace libbitcoin {
namespace network {
namespace tls {

using namespace system;

BC_PUSH_WARNING(NO_ARRAY_INDEXING)

constexpr uint8_t compatibility_payload = 0x01;
constexpr std::string_view server_verify{ "TLS 1.3, server CertificateVerify" };
constexpr std::string_view client_verify{ "TLS 1.3, client CertificateVerify" };

// Server preference orders.
constexpr std::array<uint16_t, 3> preferred_suites
{
    aes_128_gcm_sha256,
    aes_256_gcm_sha384,
    chacha20_poly1305_sha256
};

constexpr std::array<uint16_t, 3> preferred_groups
{
    x25519_group,
    secp256r1_group,
    secp384r1_group
};

// Helpers.
// ----------------------------------------------------------------------------

static bool contains(const std::vector<uint16_t>& list, uint16_t value) NOEXCEPT
{
    return std::find(list.cbegin(), list.cend(), value) != list.cend();
}

// A nonempty list that fills the extension body.
static bool to_list(std::vector<uint16_t>& out, const const_byte_span& body,
    bool narrow) NOEXCEPT
{
    reader source{ body };
    out = narrow ? source.read_list_8() : source.read_list_16();
    return source.is_complete() && !out.empty();
}

// Shared secret and own key_share of ephemeral key, false if peer share is
// not valid uncompressed point.
template <typename Curve>
static bool nist_exchange(data_chunk& shared, data_chunk& own,
    const const_byte_span& share) NOEXCEPT
{
    typename Curve::point_t peer{};
    if (share.size() != peer.size())
        return false;

    std::copy(share.begin(), share.end(), peer.begin());
    auto secret = Curve::generate();
    typename Curve::point_t point{};
    typename Curve::shared_t value{};
    const auto valid = Curve::public_key(point, secret) &&
        Curve::agree(value, secret, peer);

    shared = to_chunk(value);
    own = to_chunk(point);
    wipe(secret);
    wipe(value);
    return valid;
}

static server::random make_random() NOEXCEPT
{
    server::random value{};
    maybe_random::fill(value);
    return value;
}

static server::key make_secret() NOEXCEPT
{
    server::key value{};
    maybe_random::fill(value);
    return value;
}

// Construct.
// ----------------------------------------------------------------------------

server::server(const context& context) NOEXCEPT
  : server(context, make_random(), make_secret())
{
}

server::server(const context& context, const random& random,
    const key& ephemeral) NOEXCEPT
  : connection(context, true), random_(random), secret_(ephemeral)
{
}

server::~server() NOEXCEPT
{
    wipe(secret_);
}

// Handshake.
// ----------------------------------------------------------------------------

bool server::is_handshaking() const NOEXCEPT
{
    return (state_ != state::client_hello) && !is_established();
}

bool server::handle_message(uint8_t type, const span& message,
    const span& body) NOEXCEPT
{
    if (is_established())
    {
        if (type == handshake::key_update)
            return handle_key_update(body);

        return fail(alert::unexpected_message);
    }

    switch (state_)
    {
        case state::client_hello:
        case state::retry_hello:
            if (type == handshake::client_hello)
                return handle_client_hello(message, body);
            break;
        case state::client_certificate:
            if (type == handshake::certificate)
                return handle_certificate(message, body);
            break;
        case state::client_certificate_verify:
            if (type == handshake::certificate_verify)
                return handle_certificate_verify(message, body);
            break;
        case state::client_finished:
            if (type == handshake::finished)
                return handle_finished(message, body);
            break;
        default:
            break;
    }

    return fail(alert::unexpected_message);
}

// Client hello.
// ----------------------------------------------------------------------------

bool server::handle_client_hello(const span& message, const span& body) NOEXCEPT
{
    const auto retried = (state_ == state::retry_hello);
    reader hello{ body };
    hello.read_16();
    hello.read_bytes(sizeof(random));
    const auto session = hello.read_vector_8();
    const auto ciphers = hello.read_list_16();
    const auto compression = hello.read_vector_8();

    // Without extensions the client does not offer TLS 1.3.
    if (hello.is_complete())
        return fail(alert::protocol_version);

    const auto extensions = hello.read_vector_16();
    if (!hello.is_complete() || (session.size() > 32u) || ciphers.empty())
        return fail(alert::decode_error);

    if ((compression.size() != one) || is_nonzero(compression.front()))
        return fail(alert::illegal_parameter);

    // Extensions are unique (4.2).
    std::vector<uint16_t> seen{}, versions{}, groups{}, algorithms{};
    span shares{};
    auto has_shares = false, has_early = false;
    reader list{ extensions };
    while (list && !list.is_complete())
    {
        const auto type = list.read_16();
        const auto data = list.read_vector_16();
        if (!list)
            return fail(alert::decode_error);

        if (contains(seen, type))
            return fail(alert::illegal_parameter);

        seen.push_back(type);
        switch (type)
        {
            case extension::supported_versions:
                if (!to_list(versions, data, true))
                    return fail(alert::decode_error);
                break;
            case extension::supported_groups:
                if (!to_list(groups, data, false))
                    return fail(alert::decode_error);
                break;
            case extension::signature_algorithms:
                if (!to_list(algorithms, data, false))
                    return fail(alert::decode_error);
                break;
            case extension::key_share:
                shares = data;
                has_shares = true;
                break;
            case extension::early_data:
                has_early = true;
                break;
            default:
                break;
        }
    }

    if (!contains(versions, version_13))
        return fail(alert::protocol_version);

    const auto suite = std::find_if(preferred_suites.cbegin(),
        preferred_suites.cend(), [&](uint16_t value) NOEXCEPT
        {
            return contains(ciphers, value);
        });
    if (suite == preferred_suites.cend())
        return fail(alert::handshake_failure);

    if (retried && ((*suite != suite_) || has_early))
        return fail(alert::illegal_parameter);

    if (algorithms.empty() || groups.empty() || !has_shares)
        return fail(alert::missing_extension);

    const auto scheme = (context_.curve() == x509::curve::secp256r1) ?
        ecdsa_secp256r1_sha256 : ecdsa_secp384r1_sha384;
    if (!contains(algorithms, scheme))
        return fail(alert::handshake_failure);

    // KeyShareClientHello (4.2.8), the first share of each group.
    std::vector<std::pair<uint16_t, span>> offered{};
    reader share_list{ shares };
    reader entries{ share_list.read_vector_16() };
    if (!share_list.is_complete())
        return fail(alert::decode_error);

    while (entries && !entries.is_complete())
    {
        const auto group = entries.read_16();
        const auto exchange = entries.read_vector_16();
        if (entries)
            offered.emplace_back(group, exchange);
    }

    if (!entries)
        return fail(alert::decode_error);

    const auto share_of = [&](uint16_t group) NOEXCEPT
    {
        const auto entry = std::find_if(offered.cbegin(), offered.cend(),
            [&](const auto& value) NOEXCEPT { return value.first == group; });
        return (entry == offered.cend()) ? span{} : entry->second;
    };

    // The preferred mutual group with a share, else the preferred mutual one.
    const auto mutual = [&](uint16_t group) NOEXCEPT
    {
        return contains(groups, group);
    };
    const auto with_share = [&](uint16_t group) NOEXCEPT
    {
        return mutual(group) && !share_of(group).empty();
    };

    const auto group = std::find_if(preferred_groups.cbegin(),
        preferred_groups.cend(), with_share);
    const auto retry = std::find_if(preferred_groups.cbegin(),
        preferred_groups.cend(), mutual);
    if (retry == preferred_groups.cend())
        return fail(alert::handshake_failure);

    suite_ = *suite;
    if (!retried)
        transcript_.reset(suite_);

    if (group == preferred_groups.cend())
    {
        if (retried)
            return fail(alert::illegal_parameter);

        group_ = *retry;
        send_retry(message, session);
        return true;
    }

    if (retried && (*group != group_))
        return fail(alert::illegal_parameter);

    group_ = *group;
    data_chunk shared{}, own{};
    if (!exchange(shared, own, share_of(group_)))
        return fail(alert::illegal_parameter);

    writer entry{};
    entry.write_16(group_);
    entry.write_vector_16(own);

    add_transcript(message);
    send_hello(random_, session, entry.data());

    const schedule keys{ suite_ };
    handshake_secret_ = keys.handshake_secret(keys.early_secret(), shared);
    wipe(shared.data(), shared.size());

    const auto hash = transcript();
    client_handshake_ = keys.derive_secret(handshake_secret_,
        "c hs traffic", hash);
    server_handshake_ = keys.derive_secret(handshake_secret_,
        "s hs traffic", hash);

    set_send(server_handshake_);
    set_receive(client_handshake_);
    send_flight();

    state_ = context_.request() ? state::client_certificate :
        state::client_finished;
    return true;
}

void server::send_retry(const span& client_hello, const span& session) NOEXCEPT
{
    // The transcript restarts with a hash of the first client hello (4.4.1).
    const auto hash = schedule{ suite_ }.hash(client_hello);
    writer synthetic{};
    synthetic.write_8(handshake::message_hash);
    synthetic.write_vector_24(hash);
    transcript_.reset(suite_);
    add_transcript(synthetic.data());

    writer entry{};
    entry.write_16(group_);
    send_hello(retry_random, session, entry.data());
    state_ = state::retry_hello;
}

// ServerHello, or HelloRetryRequest by its random and key_share (4.1.3).
void server::send_hello(const random& value, const span& session,
    const span& key_share) NOEXCEPT
{
    writer version{};
    version.write_16(version_13);

    writer extensions{};
    extensions.write_16(extension::key_share);
    extensions.write_vector_16(key_share);
    extensions.write_16(extension::supported_versions);
    extensions.write_vector_16(version.data());

    writer body{};
    body.write_16(legacy_version);
    body.write_bytes(value);
    body.write_vector_8(session);
    body.write_16(suite_);
    body.write_8(0x00);
    body.write_vector_16(extensions.data());

    const auto hello = make_message(handshake::server_hello, body.data());
    add_transcript(hello);
    seal(content::handshake, hello);
    send_compatibility(session);
}

// Sent once, after first server handshake message, in compatibility mode
// (nonempty client session id) (D.4).
void server::send_compatibility(const span& session) NOEXCEPT
{
    if (session.empty() || compatibility_sent_)
        return;

    const data_array<one> payload{ compatibility_payload };
    seal(content::change_cipher_spec, payload);
    compatibility_sent_ = true;
}

void server::send_flight() NOEXCEPT
{
    data_chunk messages{};
    const auto append = [&](const data_chunk& message) NOEXCEPT
    {
        add_transcript(message);
        messages.insert(messages.cend(), message.cbegin(), message.cend());
    };

    // EncryptedExtensions (none).
    writer extensions{};
    extensions.write_16(0);
    append(make_message(handshake::encrypted_extensions, extensions.data()));

    // CertificateRequest (4.3.2).
    if (context_.request())
    {
        writer schemes{};
        schemes.write_16(ecdsa_secp256r1_sha256);
        schemes.write_16(ecdsa_secp384r1_sha384);

        writer algorithms{};
        algorithms.write_vector_16(schemes.data());

        writer request_extensions{};
        request_extensions.write_16(extension::signature_algorithms);
        request_extensions.write_vector_16(algorithms.data());

        writer request{};
        request.write_vector_8({});
        request.write_vector_16(request_extensions.data());
        append(make_message(handshake::certificate_request, request.data()));
    }

    // Certificate (4.4.2), CertificateVerify (4.4.3), Finished (4.4.4).
    append(make_certificate(context_.certificates()));
    append(make_verify(server_verify));
    const auto verify_data = schedule{ suite_ }.finished(server_handshake_,
        transcript());
    append(make_message(handshake::finished, verify_data));
    send(content::handshake, messages);

    // Application secrets follow the server finished (7.1).
    derive_traffic();
    set_send(server_traffic_);
}

// Client authentication.
// ----------------------------------------------------------------------------

bool server::handle_certificate(const span& message, const span& body) NOEXCEPT
{
    uint8_t alert{};
    x509::certificates chain{};
    if (!parse_certificate(chain, alert, body))
        return fail(alert);

    add_transcript(message);
    if (chain.empty())
    {
        if (context_.require())
            return fail(alert::certificate_required);

        state_ = state::client_finished;
        return true;
    }

    const auto ec = x509::verify(chain, context_.anchors(), context_.time(),
        x509::purpose::client);
    if (ec)
        return fail(to_alert(ec));

    peer_ = std::move(chain);
    state_ = state::client_certificate_verify;
    return true;
}

bool server::handle_certificate_verify(const span& message,
    const span& body) NOEXCEPT
{
    reader verify{ body };
    const auto scheme = verify.read_16();
    const auto signature = verify.read_vector_16();
    if (!verify.is_complete())
        return fail(alert::decode_error);

    // CertificateRequest schemes by leaf curve.
    const auto& leaf = peer_.front();
    const auto ec = (leaf.key_type == x509::key_type::ec);
    const auto p256 = ec && (scheme == ecdsa_secp256r1_sha256) &&
        (leaf.curve == x509::curve::secp256r1);
    const auto p384 = ec && (scheme == ecdsa_secp384r1_sha384) &&
        (leaf.curve == x509::curve::secp384r1);
    if (!p256 && !p384)
        return fail(alert::illegal_parameter);

    if (!is_verified(leaf, scheme, signature, client_verify))
        return fail(alert::decrypt_error);

    add_transcript(message);
    state_ = state::client_finished;
    return true;
}

// Finished.
// ----------------------------------------------------------------------------

bool server::handle_finished(const span& message, const span& body) NOEXCEPT
{
    const auto expected = schedule{ suite_ }.finished(client_handshake_,
        transcript());
    if ((body.size() != expected.size()) ||
        !constant_time_equal(expected, body))
        return fail(alert::decrypt_error);

    add_transcript(message);
    set_receive(client_traffic_);
    establish();
    return true;
}

// Keys.
// ----------------------------------------------------------------------------

bool server::exchange(data_chunk& shared, data_chunk& own,
    const span& share) NOEXCEPT
{
    if (group_ == secp256r1_group)
        return nist_exchange<secp256r1>(shared, own, share);

    if (group_ == secp384r1_group)
        return nist_exchange<secp384r1>(shared, own, share);

    x25519::key peer{}, value{}, point{};
    if (share.size() != peer.size())
        return false;

    std::copy(share.begin(), share.end(), peer.begin());
    const auto valid = x25519::multiply(value, secret_, peer);
    x25519::multiply(point, secret_);
    shared = to_chunk(value);
    own = to_chunk(point);
    wipe(value);
    return valid;
}

BC_POP_WARNING()

} // namespace tls
} // namespace network
} // namespace libbitcoin
