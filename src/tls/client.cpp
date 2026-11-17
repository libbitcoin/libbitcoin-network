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
#include <bitcoin/network/tls/client.hpp>

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/tls/codec.hpp>
#include <bitcoin/network/tls/constants.hpp>

// rfc8446 (tls 1.3), rfc6066 (server_name)

namespace libbitcoin {
namespace network {
namespace tls {

using namespace system;

BC_PUSH_WARNING(NO_ARRAY_INDEXING)

constexpr uint8_t compatibility_payload = 0x01;
constexpr uint8_t host_name = 0x00;
constexpr std::string_view server_verify{ "TLS 1.3, server CertificateVerify" };
constexpr std::string_view client_verify{ "TLS 1.3, client CertificateVerify" };

// Helpers.
// ----------------------------------------------------------------------------

static bool contains(const std::vector<uint16_t>& list, uint16_t value) NOEXCEPT
{
    return std::find(list.cbegin(), list.cend(), value) != list.cend();
}

static void write_list(writer& out, const std::vector<uint16_t>& list,
    bool narrow) NOEXCEPT
{
    writer values{};
    for (const auto value: list)
        values.write_16(value);

    if (narrow)
        out.write_vector_8(values.data());
    else
        out.write_vector_16(values.data());
}

static void write_extension(writer& out, uint16_t type,
    const data_chunk& data) NOEXCEPT
{
    out.write_16(type);
    out.write_vector_16(data);
}

template <typename Curve>
static bool nist_agree(data_chunk& shared,
    const typename Curve::secret_t& secret,
    const const_byte_span& peer) NOEXCEPT
{
    typename Curve::point_t point{};
    typename Curve::shared_t value{};
    if (peer.size() != point.size())
        return false;

    std::copy(peer.begin(), peer.end(), point.begin());
    const auto valid = Curve::agree(value, secret, point);
    shared = to_chunk(value);
    wipe(value);
    return valid;
}

static client::random make_random() NOEXCEPT
{
    client::random value{};
    maybe_random::fill(value);
    return value;
}

static client::key make_secret() NOEXCEPT
{
    client::key value{};
    maybe_random::fill(value);
    return value;
}

// Construct.
// ----------------------------------------------------------------------------

client::client(const context& context) NOEXCEPT
  : client(context, make_random(), make_secret())
{
}

client::client(const context& context, const random& random,
    const key& ephemeral) NOEXCEPT
  : connection(context, false), random_(random), secret_(ephemeral),
    session_(context.offered().session), shared_(context.offered().shares)
{
    maybe_random::fill(session_);
    send_hello();
}

client::~client() NOEXCEPT
{
    wipe(secret_);
    wipe(secret256_);
    wipe(secret384_);
}

// Properties.
// ----------------------------------------------------------------------------

bool client::is_retried() const NOEXCEPT
{
    return retried_;
}

bool client::is_requested() const NOEXCEPT
{
    return requested_;
}

// Handshake.
// ----------------------------------------------------------------------------

bool client::is_handshaking() const NOEXCEPT
{
    return !is_established();
}

bool client::handle_message(uint8_t type, const span& message,
    const span& body) NOEXCEPT
{
    if (is_established())
    {
        if (type == handshake::key_update)
            return handle_key_update(body);

        // Tickets ignored, no resumption (4.6.1).
        if (type == handshake::new_session_ticket)
            return true;

        return fail(alert::unexpected_message);
    }

    switch (state_)
    {
        case state::server_hello:
            if (type == handshake::server_hello)
                return handle_server_hello(message, body);
            break;
        case state::encrypted_extensions:
            if (type == handshake::encrypted_extensions)
                return handle_encrypted_extensions(message, body);
            break;
        case state::certificate:
            if (!requested_ && (type == handshake::certificate_request))
                return handle_certificate_request(message, body);
            if (type == handshake::certificate)
                return handle_certificate(message, body);
            break;
        case state::certificate_verify:
            if (type == handshake::certificate_verify)
                return handle_certificate_verify(message, body);
            break;
        case state::finished:
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

// ClientHello (4.1.2), after HelloRetryRequest shares its group, echoes cookie.
void client::send_hello() NOEXCEPT
{
    const auto& offer = context_.offered();

    writer versions{};
    write_list(versions, { version_13 }, true);

    writer groups{};
    write_list(groups, offer.groups, false);

    writer algorithms{};
    write_list(algorithms, offer.algorithms, false);

    writer entries{};
    for (const auto group: shared_)
    {
        entries.write_16(group);
        entries.write_vector_16(share(group));
    }

    writer shares{};
    shares.write_vector_16(entries.data());

    writer extensions{};
    const auto& name = context_.name();
    if (!name.empty())
    {
        writer server{};
        server.write_8(host_name);
        server.write_vector_16(data_chunk{ name.begin(), name.end() });

        writer names{};
        names.write_vector_16(server.data());
        write_extension(extensions, extension::server_name, names.data());
    }

    write_extension(extensions, extension::supported_versions, versions.data());
    write_extension(extensions, extension::supported_groups, groups.data());
    write_extension(extensions, extension::signature_algorithms,
        algorithms.data());
    write_extension(extensions, extension::key_share, shares.data());

    if (!cookie_.empty())
    {
        writer cookie{};
        cookie.write_vector_16(cookie_);
        write_extension(extensions, extension::cookie, cookie.data());
    }

    writer suites{};
    write_list(suites, offer.suites, false);

    writer body{};
    body.write_16(legacy_version);
    body.write_bytes(random_);
    body.write_vector_8(session_);
    body.write_bytes(suites.data());
    body.write_vector_8(data_array<one>{ 0x00 });
    body.write_vector_16(extensions.data());

    const auto hello = make_message(handshake::client_hello, body.data());
    if (retried_)
        add_transcript(hello);
    else
        hello_ = hello;

    seal(content::handshake, hello);
}

// Compatibility mode (session id) sends once, unprotected, before second
// ClientHello or first protected record (D.4).
void client::send_compatibility() NOEXCEPT
{
    if (session_.empty() || compatibility_sent_)
        return;

    const data_array<one> payload{ compatibility_payload };
    seal(content::change_cipher_spec, payload);
    compatibility_sent_ = true;
}

// Server hello.
// ----------------------------------------------------------------------------

bool client::handle_server_hello(const span& message, const span& body) NOEXCEPT
{
    const auto& offer = context_.offered();
    reader hello{ body };
    const auto version = hello.read_16();
    const auto value = hello.read_bytes(sizeof(random));
    const auto session = hello.read_vector_8();
    const auto suite = hello.read_16();
    const auto compression = hello.read_8();
    const auto extensions = hello.read_vector_16();
    if (!hello.is_complete())
        return fail(alert::decode_error);

    if (version != legacy_version)
        return fail(alert::protocol_version);

    const auto retry = std::equal(value.begin(), value.end(),
        retry_random.begin(), retry_random.end());
    if (!std::equal(session.begin(), session.end(), session_.begin(),
        session_.end()) || is_nonzero(compression) ||
        !contains(offer.suites, suite) || (retried_ && (suite != suite_)))
        return fail(alert::illegal_parameter);

    // Extensions unique and requested (4.2), retry may add cookie.
    std::vector<uint16_t> seen{};
    uint16_t selected{}, group{};
    span cookie{}, exchange{};
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
        reader extension{ data };
        switch (type)
        {
            case extension::supported_versions:
            {
                selected = extension.read_16();
                break;
            }
            case extension::key_share:
            {
                group = extension.read_16();
                if (!retry)
                    exchange = extension.read_vector_16();
                break;
            }
            case extension::cookie:
            {
                if (!retry)
                    return fail(alert::unsupported_extension);

                cookie = extension.read_vector_16();
                break;
            }
            default:
            {
                return fail(alert::unsupported_extension);
            }
        }

        if (!extension.is_complete())
            return fail(alert::decode_error);
    }

    if (!list)
        return fail(alert::decode_error);

    if (selected != version_13)
        return fail(alert::protocol_version);

    const auto keyed = contains(seen, extension::key_share);
    suite_ = suite;
    if (retry)
        return handle_retry(message, keyed, group, cookie);

    if (!keyed)
        return fail(alert::missing_extension);

    data_chunk shared{};
    if (!contains(shared_, group) || !agree(shared, group, exchange))
        return fail(alert::illegal_parameter);

    if (!retried_)
    {
        transcript_.reset(suite_);
        add_transcript(hello_);
    }

    group_ = group;
    add_transcript(message);

    const schedule keys{ suite_ };
    handshake_secret_ = keys.handshake_secret(keys.early_secret(), shared);
    wipe(shared.data(), shared.size());

    const auto hash = transcript();
    client_handshake_ = keys.derive_secret(handshake_secret_,
        "c hs traffic", hash);
    server_handshake_ = keys.derive_secret(handshake_secret_,
        "s hs traffic", hash);

    // Later client records, including alerts, are protected (5.1).
    send_compatibility();
    set_send(client_handshake_);
    set_receive(server_handshake_);
    state_ = state::encrypted_extensions;
    return true;
}

// HelloRetryRequest (4.1.4), once, unshared group or cookie, changing hello.
bool client::handle_retry(const span& message, bool keyed, uint16_t group,
    const span& cookie) NOEXCEPT
{
    if (retried_)
        return fail(alert::unexpected_message);

    if (keyed && (!contains(context_.offered().groups, group) ||
        contains(shared_, group)))
        return fail(alert::illegal_parameter);

    if (!keyed && cookie.empty())
        return fail(alert::illegal_parameter);

    // Transcript restarts with hash of first client hello (4.4.1).
    transcript_.reset(suite_);
    const auto hash = schedule{ suite_ }.hash(hello_);
    writer synthetic{};
    synthetic.write_8(handshake::message_hash);
    synthetic.write_vector_24(hash);
    add_transcript(synthetic.data());
    add_transcript(message);

    retried_ = true;
    if (keyed)
        shared_ = { group };

    cookie_.assign(cookie.begin(), cookie.end());
    send_compatibility();
    send_hello();
    return true;
}

// Encrypted extensions.
// ----------------------------------------------------------------------------

// Only server name acknowledgment or server groups (4.2).
bool client::handle_encrypted_extensions(const span& message,
    const span& body) NOEXCEPT
{
    reader extensions{ body };
    reader list{ extensions.read_vector_16() };
    if (!extensions.is_complete())
        return fail(alert::decode_error);

    std::vector<uint16_t> seen{};
    while (list && !list.is_complete())
    {
        const auto type = list.read_16();
        const auto data = list.read_vector_16();
        if (!list)
            return fail(alert::decode_error);

        if (contains(seen, type))
            return fail(alert::illegal_parameter);

        seen.push_back(type);
        const auto acknowledged = (type == extension::server_name) &&
            !context_.name().empty() && data.empty();
        if (!acknowledged && (type != extension::supported_groups))
            return fail(alert::unsupported_extension);
    }

    if (!list)
        return fail(alert::decode_error);

    add_transcript(message);
    state_ = state::certificate;
    return true;
}

// Server authentication.
// ----------------------------------------------------------------------------

// CertificateRequest (4.3.2), signature_algorithms required.
bool client::handle_certificate_request(const span& message,
    const span& body) NOEXCEPT
{
    reader request{ body };
    const auto request_context = request.read_vector_8();
    reader list{ request.read_vector_16() };
    if (!request.is_complete())
        return fail(alert::decode_error);

    if (!request_context.empty())
        return fail(alert::illegal_parameter);

    std::vector<uint16_t> seen{}, algorithms{};
    while (list && !list.is_complete())
    {
        const auto type = list.read_16();
        const auto data = list.read_vector_16();
        if (!list)
            return fail(alert::decode_error);

        if (contains(seen, type))
            return fail(alert::illegal_parameter);

        seen.push_back(type);
        if (type == extension::signature_algorithms)
        {
            reader source{ data };
            algorithms = source.read_list_16();
            if (!source.is_complete() || algorithms.empty())
                return fail(alert::decode_error);
        }
    }

    if (!list)
        return fail(alert::decode_error);

    if (algorithms.empty())
        return fail(alert::missing_extension);

    // Without acceptable key client sends empty Certificate.
    const auto scheme = (context_.curve() == x509::curve::secp256r1) ?
        ecdsa_secp256r1_sha256 : ecdsa_secp384r1_sha384;
    authenticate_ = context_.is_ready() && contains(algorithms, scheme);
    requested_ = true;
    add_transcript(message);
    return true;
}

bool client::handle_certificate(const span& message, const span& body) NOEXCEPT
{
    uint8_t alert{};
    x509::certificates chain{};
    if (!parse_certificate(chain, alert, body))
        return fail(alert);

    if (chain.empty())
        return fail(alert::decode_error);

    // Unanchored, caller binds leaf to identity.
    if (!context_.unanchored())
    {
        const auto ec = x509::verify(chain, context_.anchors(),
            context_.time(), x509::purpose::server);
        if (ec)
            return fail(to_alert(ec));
    }

    peer_ = std::move(chain);
    add_transcript(message);
    state_ = state::certificate_verify;
    return true;
}

bool client::handle_certificate_verify(const span& message,
    const span& body) NOEXCEPT
{
    reader verify{ body };
    const auto scheme = verify.read_16();
    const auto signature = verify.read_vector_16();
    if (!verify.is_complete())
        return fail(alert::decode_error);

    if (!contains(context_.offered().algorithms, scheme))
        return fail(alert::illegal_parameter);

    if (!is_verified(peer_.front(), scheme, signature, server_verify))
        return fail(alert::decrypt_error);

    add_transcript(message);
    state_ = state::finished;
    return true;
}

// Finished.
// ----------------------------------------------------------------------------

bool client::handle_finished(const span& message, const span& body) NOEXCEPT
{
    const auto expected = schedule{ suite_ }.finished(server_handshake_,
        transcript());
    if ((body.size() != expected.size()) ||
        !constant_time_equal(expected, body))
        return fail(alert::decrypt_error);

    // Application secrets follow server finished (7.1).
    add_transcript(message);
    derive_traffic();
    send_flight();
    return true;
}

// Certificate and CertificateVerify if requested, then Finished (4.4).
void client::send_flight() NOEXCEPT
{
    data_chunk messages{};
    const auto append = [&](const data_chunk& message) NOEXCEPT
    {
        add_transcript(message);
        messages.insert(messages.cend(), message.cbegin(), message.cend());
    };

    if (requested_)
    {
        append(make_certificate(authenticate_ ? context_.certificates() :
            context::chain{}));

        if (authenticate_)
            append(make_verify(client_verify));
    }

    const auto verify_data = schedule{ suite_ }.finished(client_handshake_,
        transcript());
    append(make_message(handshake::finished, verify_data));
    send(content::handshake, messages);

    set_send(client_traffic_);
    set_receive(server_traffic_);
    establish();
}

// Keys.
// ----------------------------------------------------------------------------

// Group key share, ephemeral key generated on first use.
data_chunk client::share(uint16_t group) NOEXCEPT
{
    if (group == secp256r1_group)
    {
        secp256r1::point_t point{};
        secret256_ = secp256r1::generate();
        secp256r1::public_key(point, secret256_);
        return to_chunk(point);
    }

    if (group == secp384r1_group)
    {
        secp384r1::point_t point{};
        secret384_ = secp384r1::generate();
        secp384r1::public_key(point, secret384_);
        return to_chunk(point);
    }

    x25519::key point{};
    x25519::multiply(point, secret_);
    return to_chunk(point);
}

bool client::agree(data_chunk& shared, uint16_t group,
    const span& peer) const NOEXCEPT
{
    if (group == secp256r1_group)
        return nist_agree<secp256r1>(shared, secret256_, peer);

    if (group == secp384r1_group)
        return nist_agree<secp384r1>(shared, secret384_, peer);

    x25519::key point{}, value{};
    if (peer.size() != point.size())
        return false;

    std::copy(peer.begin(), peer.end(), point.begin());
    const auto valid = x25519::multiply(value, secret_, point);
    shared = to_chunk(value);
    wipe(value);
    return valid;
}

BC_POP_WARNING()

} // namespace tls
} // namespace network
} // namespace libbitcoin
