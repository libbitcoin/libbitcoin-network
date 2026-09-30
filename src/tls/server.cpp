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

constexpr size_t maximum_message = 131072;
constexpr size_t message_header_size = 4;
constexpr size_t verify_padding = 64;
constexpr uint8_t verify_pad = 0x20;
constexpr uint8_t update_requested = 1;
constexpr uint8_t update_not_requested = 0;
constexpr uint8_t compatibility_payload = 0x01;
constexpr uint64_t maximum_records = 16777216;
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

static data_chunk to_bytes(const const_byte_span& bytes) NOEXCEPT
{
    return { bytes.begin(), bytes.end() };
}

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

static data_chunk make_message(uint8_t type, const data_chunk& body) NOEXCEPT
{
    writer out{};
    out.write_8(type);
    out.write_vector_24(body);
    return out.data();
}

static data_chunk verify_content(std::string_view label,
    const schedule::secret& transcript) NOEXCEPT
{
    data_chunk content(verify_padding, verify_pad);
    content.insert(content.cend(), label.cbegin(), label.cend());
    content.push_back(0x00);
    content.insert(content.cend(), transcript.cbegin(), transcript.cend());
    return content;
}

static uint8_t to_alert(const code& ec) NOEXCEPT
{
    if (ec == system::error::certificate_expired ||
        ec == system::error::certificate_not_yet_valid)
        return alert::certificate_expired;

    if (ec == system::error::chain_untrusted)
        return alert::unknown_ca;

    return alert::bad_certificate;
}

template <typename Curve>
static bool verify_signature(const data_chunk& key, const const_byte_span& der,
    const const_byte_span& digest) NOEXCEPT
{
    typename Curve::point_t point{};
    typename Curve::signature_t signature{};
    if ((key.size() != point.size()) || !Curve::decode(signature, der))
        return false;

    std::copy(key.cbegin(), key.cend(), point.begin());
    return Curve::verify(signature, point, digest);
}

// The shared secret and own key_share of an ephemeral key, false if the
// peer share is not a valid uncompressed point.
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

template <typename Curve>
static void write_signature(writer& out, uint16_t scheme,
    const typename Curve::secret_t& key, const const_byte_span& digest) NOEXCEPT
{
    typename Curve::signature_t signature{};
    Curve::sign(signature, key, digest);
    out.write_16(scheme);
    out.write_vector_16(Curve::encode(signature));
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
  : context_(context), random_(random), secret_(ephemeral)
{
}

server::~server() NOEXCEPT
{
    const auto clear = [](data_chunk& secret) NOEXCEPT
    {
        wipe(secret.data(), secret.size());
    };

    wipe(secret_);
    clear(handshake_secret_);
    clear(client_handshake_);
    clear(server_handshake_);
    clear(client_traffic_);
    clear(server_traffic_);
}

// Properties.
// ----------------------------------------------------------------------------

data_chunk& server::output() NOEXCEPT
{
    return output_;
}

size_t server::readable() const NOEXCEPT
{
    return application_.size();
}

bool server::is_established() const NOEXCEPT
{
    return state_ == state::established;
}

bool server::is_closed() const NOEXCEPT
{
    return closed_;
}

bool server::is_close_sent() const NOEXCEPT
{
    return close_sent_;
}

bool server::is_failed() const NOEXCEPT
{
    return state_ == state::failed;
}

uint8_t server::failure() const NOEXCEPT
{
    return failure_;
}

bool server::is_failure_received() const NOEXCEPT
{
    return failure_received_;
}

uint16_t server::suite() const NOEXCEPT
{
    return suite_;
}

uint16_t server::group() const NOEXCEPT
{
    return group_;
}

const x509::certificates& server::peer() const NOEXCEPT
{
    return peer_;
}

// Application data.
// ----------------------------------------------------------------------------

bool server::write(const const_byte_span& data) NOEXCEPT
{
    if (!is_established() || close_sent_)
        return false;

    // The send key is rotated within the AES-GCM record limit (5.5).
    if (send_.sequence() >= maximum_records)
        send_update();

    send(content::application_data, data);
    return true;
}

size_t server::read(const byte_span& out) NOEXCEPT
{
    const auto size = std::min(out.size(), application_.size());
    const auto end = std::next(application_.cbegin(), size);
    std::copy(application_.cbegin(), end, out.begin());
    application_.erase(application_.cbegin(), end);
    return size;
}

void server::close() NOEXCEPT
{
    if (close_sent_ || is_failed())
        return;

    const data_array<two> content{ alert_warning, alert::close_notify };
    send_.seal(output_, content::alert, content);
    close_sent_ = true;
}

// Records.
// ----------------------------------------------------------------------------

bool server::fail(uint8_t description) NOEXCEPT
{
    if (is_failed())
        return false;

    state_ = state::failed;
    failure_ = description;
    const data_array<two> content{ alert_fatal, description };
    send_.seal(output_, content::alert, content);
    return false;
}

bool server::receive(const const_byte_span& data) NOEXCEPT
{
    if (is_failed())
        return false;

    input_.insert(input_.cend(), data.begin(), data.end());

    size_t start{};
    while (!is_failed() && !closed_ &&
        ((input_.size() - start) >= record_header_size))
    {
        const auto available = span{ input_ }.subspan(start);
        reader header{ available.first(record_header_size) };
        const auto type = header.read_8();
        header.read_16();
        const auto size = header.read_16();

        if (size > maximum_ciphertext)
            return fail(alert::record_overflow);

        if (available.size() < (record_header_size + size))
            break;

        // Copies, since handlers may append to buffers.
        const auto to = std::next(available.begin(), record_header_size);
        const data_chunk head{ available.begin(), to };
        const auto fragment = available.subspan(record_header_size, size);
        start += record_header_size + size;

        if (!handle_record(type, head, to_bytes(fragment)))
            return false;
    }

    input_.erase(input_.cbegin(), std::next(input_.cbegin(), start));
    return !is_failed();
}

bool server::handle_record(uint8_t type, const span& header,
    const span& fragment) NOEXCEPT
{
    // A compatibility change_cipher_spec is unprotected and ignored (5).
    if (type == content::change_cipher_spec)
    {
        const auto handshaking =
            (state_ != state::client_hello) &&
            (state_ != state::established);

        if (!handshaking || (!is_one(fragment.size())) ||
            (fragment.front() != compatibility_payload))
            return fail(alert::unexpected_message);

        return true;
    }

    if (!receive_.is_protected())
    {
        if (fragment.size() > maximum_plaintext)
            return fail(alert::record_overflow);

        if (type == content::application_data)
            return fail(alert::unexpected_message);

        return handle_content(type, fragment);
    }

    if (type != content::application_data)
        return fail(alert::unexpected_message);

    uint8_t inner{};
    data_chunk content{};
    if (!receive_.open(inner, content, header, fragment))
        return fail(alert::bad_record_mac);

    if (content.size() > maximum_plaintext)
        return fail(alert::record_overflow);

    return handle_content(inner, content);
}

bool server::handle_content(uint8_t type, const span& content) NOEXCEPT
{
    switch (type)
    {
        case content::alert:
        {
            return handle_alert(content);
        }
        case content::handshake:
        {
            return handle_handshake(content);
        }
        case content::application_data:
        {
            if (!is_established())
                return fail(alert::unexpected_message);

            application_.insert(application_.cend(), content.begin(),
                content.end());
            return true;
        }
        default:
        {
            return fail(alert::unexpected_message);
        }
    }
}

bool server::handle_alert(const span& content) NOEXCEPT
{
    if (content.size() != two)
        return fail(alert::decode_error);

    // Closure alerts are not errors (6.1).
    const auto description = content[one];
    if (description == alert::close_notify)
    {
        closed_ = true;
        return true;
    }

    if (description == alert::user_canceled)
        return true;

    state_ = state::failed;
    failure_ = description;
    failure_received_ = true;
    return false;
}

bool server::handle_handshake(const span& content) NOEXCEPT
{
    if (content.empty())
        return fail(alert::unexpected_message);

    handshake_.insert(handshake_.cend(), content.begin(), content.end());
    while (handshake_.size() >= message_header_size)
    {
        reader header{ span{ handshake_ }.first(message_header_size) };
        const auto type = header.read_8();
        const auto size = header.read_24();
        if (size > maximum_message)
            return fail(alert::decode_error);

        const auto total = message_header_size + size;
        if (handshake_.size() < total)
            break;

        const auto message = to_bytes(span{ handshake_ }.first(total));
        const auto to = std::next(handshake_.cbegin(), total);
        handshake_.erase(handshake_.cbegin(), to);

        const auto epoch = receive_epoch_;
        const auto body = span{ message }.subspan(message_header_size);
        if (!handle_message(type, message, body))
            return false;

        // Handshake messages must not span a change of receive keys (5.1).
        if ((epoch != receive_epoch_) && !handshake_.empty())
            return fail(alert::unexpected_message);
    }

    return true;
}

bool server::handle_message(uint8_t type, const span& message,
    const span& body) NOEXCEPT
{
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
        case state::established:
            if (type == handshake::key_update)
                return handle_key_update(body);
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

    send_.set_secret(suite_, server_handshake_);
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
    send_.seal(output_, content::handshake, hello);
    send_compatibility(session);
}

// Sent once, after the first server handshake message, in compatibility
// mode (a nonempty client session id) (D.4).
void server::send_compatibility(const span& session) NOEXCEPT
{
    if (session.empty() || compatibility_sent_)
        return;

    const data_array<one> payload{ compatibility_payload };
    send_.seal(output_, content::change_cipher_spec, payload);
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

    // Certificate (4.4.2).
    writer entries{};
    for (const auto& certificate: context_.certificates())
    {
        entries.write_vector_24(certificate);
        entries.write_16(0);
    }

    writer certificate{};
    certificate.write_vector_8({});
    certificate.write_vector_24(entries.data());
    append(make_message(handshake::certificate, certificate.data()));

    // CertificateVerify (4.4.3).
    const auto content = verify_content(server_verify, transcript());
    writer verify{};
    if (context_.curve() == x509::curve::secp256r1)
        write_signature<secp256r1>(verify, ecdsa_secp256r1_sha256,
            context_.key(), sha256_hash(content));
    else
        write_signature<secp384r1>(verify, ecdsa_secp384r1_sha384,
            context_.key384(), accumulator<sha512_384>::hash(content));

    append(make_message(handshake::certificate_verify, verify.data()));

    // Finished (4.4.4).
    const schedule keys{ suite_ };
    const auto verify_data = keys.finished(server_handshake_, transcript());
    append(make_message(handshake::finished, verify_data));
    send(content::handshake, messages);

    // Application secrets follow the server finished (7.1).
    auto master = keys.master_secret(handshake_secret_);
    const auto hash = transcript();
    client_traffic_ = keys.derive_secret(master, "c ap traffic", hash);
    server_traffic_ = keys.derive_secret(master, "s ap traffic", hash);
    wipe(master.data(), master.size());
    send_.set_secret(suite_, server_traffic_);
}

void server::send(uint8_t type, const span& data) NOEXCEPT
{
    for (size_t start{}; start < data.size(); start += maximum_plaintext)
    {
        const auto size = std::min(maximum_plaintext, data.size() - start);
        send_.seal(output_, type, data.subspan(start, size));
    }
}

// Client authentication.
// ----------------------------------------------------------------------------

bool server::handle_certificate(const span& message, const span& body) NOEXCEPT
{
    reader certificate{ body };
    const auto request_context = certificate.read_vector_8();
    reader entries{ certificate.read_vector_24() };
    if (!certificate.is_complete())
        return fail(alert::decode_error);

    if (!request_context.empty())
        return fail(alert::illegal_parameter);

    x509::certificates chain{};
    while (entries && !entries.is_complete())
    {
        const auto der = entries.read_vector_24();
        entries.read_vector_16();
        if (!entries)
            return fail(alert::decode_error);

        x509::certificate value{};
        if (!x509::parse(value, der))
            return fail(alert::bad_certificate);

        chain.push_back(std::move(value));
    }

    if (!entries)
        return fail(alert::decode_error);

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

    const auto& leaf = peer_.front();
    const auto content = verify_content(client_verify, transcript());

    auto valid = false;
    if ((scheme == ecdsa_secp256r1_sha256) &&
        (leaf.curve == x509::curve::secp256r1))
    {
        valid = verify_signature<secp256r1>(leaf.public_key, signature,
            sha256_hash(content));
    }
    else if ((scheme == ecdsa_secp384r1_sha384) &&
        (leaf.curve == x509::curve::secp384r1))
    {
        const auto digest = accumulator<sha512_384>::hash(content);
        valid = verify_signature<secp384r1>(leaf.public_key, signature,
            digest);
    }
    else
    {
        return fail(alert::illegal_parameter);
    }

    if (!valid)
        return fail(alert::decrypt_error);

    add_transcript(message);
    state_ = state::client_finished;
    return true;
}

// Finished and key update.
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
    state_ = state::established;
    return true;
}

bool server::handle_key_update(const span& body) NOEXCEPT
{
    if (body.size() != one)
        return fail(alert::decode_error);

    const auto request = body.front();
    if ((request != update_not_requested) && (request != update_requested))
        return fail(alert::illegal_parameter);

    client_traffic_ = schedule{ suite_ }.update(client_traffic_);
    set_receive(client_traffic_);

    if ((request == update_requested) && !close_sent_)
        send_update();

    return true;
}

// KeyUpdate under the current send key, then the next send key (4.6.3).
void server::send_update() NOEXCEPT
{
    writer update{};
    update.write_8(update_not_requested);
    send(content::handshake, make_message(handshake::key_update,
        update.data()));
    server_traffic_ = schedule{ suite_ }.update(server_traffic_);
    send_.set_secret(suite_, server_traffic_);
}

// Keys and transcript.
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

void server::set_receive(const schedule::secret& traffic) NOEXCEPT
{
    receive_.set_secret(suite_, traffic);
    ++receive_epoch_;
}

void server::add_transcript(const span& message) NOEXCEPT
{
    transcript_.write(message);
}

schedule::secret server::transcript() const NOEXCEPT
{
    return transcript_.hash();
}

BC_POP_WARNING()

} // namespace tls
} // namespace network
} // namespace libbitcoin
