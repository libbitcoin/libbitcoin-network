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
#include <bitcoin/network/tls/connection.hpp>

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

static data_chunk to_bytes(const const_byte_span& bytes) NOEXCEPT
{
    return { bytes.begin(), bytes.end() };
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

template <typename Curve>
static void write_signature(writer& out, uint16_t scheme,
    const typename Curve::secret_t& key, const const_byte_span& digest) NOEXCEPT
{
    typename Curve::signature_t signature{};
    Curve::sign(signature, key, digest);
    out.write_16(scheme);
    out.write_vector_16(Curve::encode(signature));
}

// Content signed by CertificateVerify (4.4.3).
static data_chunk verify_content(std::string_view label,
    const schedule::secret& transcript) NOEXCEPT
{
    data_chunk content(verify_padding, verify_pad);
    content.insert(content.cend(), label.cbegin(), label.cend());
    content.push_back(0x00);
    content.insert(content.cend(), transcript.cbegin(), transcript.cend());
    return content;
}

// Construct.
// ----------------------------------------------------------------------------

connection::connection(const context& context, bool server) NOEXCEPT
  : context_(context), server_(server)
{
}

connection::~connection() NOEXCEPT
{
    const auto clear = [](data_chunk& secret) NOEXCEPT
    {
        wipe(secret.data(), secret.size());
    };

    clear(handshake_secret_);
    clear(client_handshake_);
    clear(server_handshake_);
    clear(client_traffic_);
    clear(server_traffic_);
}

// Properties.
// ----------------------------------------------------------------------------

data_chunk& connection::output() NOEXCEPT
{
    return output_;
}

size_t connection::readable() const NOEXCEPT
{
    return application_.size();
}

bool connection::is_established() const NOEXCEPT
{
    return established_;
}

bool connection::is_closed() const NOEXCEPT
{
    return closed_;
}

bool connection::is_close_sent() const NOEXCEPT
{
    return close_sent_;
}

bool connection::is_failed() const NOEXCEPT
{
    return failed_;
}

uint8_t connection::failure() const NOEXCEPT
{
    return failure_;
}

bool connection::is_failure_received() const NOEXCEPT
{
    return failure_received_;
}

uint16_t connection::suite() const NOEXCEPT
{
    return suite_;
}

uint16_t connection::group() const NOEXCEPT
{
    return group_;
}

const x509::certificates& connection::peer() const NOEXCEPT
{
    return peer_;
}

// Application data.
// ----------------------------------------------------------------------------

bool connection::write(const const_byte_span& data) NOEXCEPT
{
    if (!is_established() || close_sent_)
        return false;

    // Send key rotated within AES-GCM record limit (5.5).
    if (send_.sequence() >= maximum_records)
        send_update(false);

    send(content::application_data, data);
    return true;
}

size_t connection::read(const byte_span& out) NOEXCEPT
{
    const auto size = std::min(out.size(), application_.size());
    const auto end = std::next(application_.cbegin(), size);
    std::copy(application_.cbegin(), end, out.begin());
    application_.erase(application_.cbegin(), end);
    return size;
}

bool connection::update(bool request) NOEXCEPT
{
    if (!is_established() || close_sent_)
        return false;

    send_update(request);
    return true;
}

void connection::close() NOEXCEPT
{
    if (close_sent_ || is_failed())
        return;

    const data_array<two> content{ alert_warning, alert::close_notify };
    seal(content::alert, content);
    close_sent_ = true;
}

// Records.
// ----------------------------------------------------------------------------

bool connection::fail(uint8_t description) NOEXCEPT
{
    if (is_failed())
        return false;

    failed_ = true;
    failure_ = description;
    const data_array<two> content{ alert_fatal, description };
    seal(content::alert, content);
    return false;
}

void connection::establish() NOEXCEPT
{
    established_ = true;
}

bool connection::receive(const const_byte_span& data) NOEXCEPT
{
    if (is_failed() || closed_)
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

bool connection::handle_record(uint8_t type, const span& header,
    const span& fragment) NOEXCEPT
{
    // Compatibility change_cipher_spec unprotected and ignored (5).
    if (type == content::change_cipher_spec)
    {
        if (!is_handshaking() || (!is_one(fragment.size())) ||
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

bool connection::handle_content(uint8_t type, const span& content) NOEXCEPT
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

bool connection::handle_alert(const span& content) NOEXCEPT
{
    if (content.size() != two)
        return fail(alert::decode_error);

    // Closure alerts not errors (6.1).
    const auto description = content[one];
    if (description == alert::close_notify)
    {
        closed_ = true;
        return true;
    }

    if (description == alert::user_canceled)
        return true;

    failed_ = true;
    failure_ = description;
    failure_received_ = true;
    return false;
}

bool connection::handle_handshake(const span& content) NOEXCEPT
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

        // Handshake messages must not span receive key change (5.1).
        if ((epoch != receive_epoch_) && !handshake_.empty())
            return fail(alert::unexpected_message);
    }

    return true;
}

void connection::seal(uint8_t type, const span& data) NOEXCEPT
{
    send_.seal(output_, type, data);
}

void connection::send(uint8_t type, const span& data) NOEXCEPT
{
    for (size_t start{}; start < data.size(); start += maximum_plaintext)
    {
        const auto size = std::min(maximum_plaintext, data.size() - start);
        seal(type, data.subspan(start, size));
    }
}

// Keys and transcript.
// ----------------------------------------------------------------------------

bool connection::handle_key_update(const span& body) NOEXCEPT
{
    if (body.size() != one)
        return fail(alert::decode_error);

    const auto request = body.front();
    if ((request != update_not_requested) && (request != update_requested))
        return fail(alert::illegal_parameter);

    auto& traffic = receive_traffic();
    traffic = schedule{ suite_ }.update(traffic);
    set_receive(traffic);

    if ((request == update_requested) && !close_sent_)
        send_update(false);

    return true;
}

// KeyUpdate under current send key, then next send key (4.6.3).
void connection::send_update(bool request) NOEXCEPT
{
    writer update{};
    update.write_8(request ? update_requested : update_not_requested);
    send(content::handshake, make_message(handshake::key_update,
        update.data()));

    auto& traffic = send_traffic();
    traffic = schedule{ suite_ }.update(traffic);
    set_send(traffic);
}

schedule::secret& connection::send_traffic() NOEXCEPT
{
    return server_ ? server_traffic_ : client_traffic_;
}

schedule::secret& connection::receive_traffic() NOEXCEPT
{
    return server_ ? client_traffic_ : server_traffic_;
}

void connection::set_send(const schedule::secret& traffic) NOEXCEPT
{
    send_.set_secret(suite_, traffic);
}

void connection::set_receive(const schedule::secret& traffic) NOEXCEPT
{
    receive_.set_secret(suite_, traffic);
    ++receive_epoch_;
}

void connection::derive_traffic() NOEXCEPT
{
    const schedule keys{ suite_ };
    auto master = keys.master_secret(handshake_secret_);
    const auto hash = transcript();
    client_traffic_ = keys.derive_secret(master, "c ap traffic", hash);
    server_traffic_ = keys.derive_secret(master, "s ap traffic", hash);
    wipe(master.data(), master.size());
}

void connection::add_transcript(const span& message) NOEXCEPT
{
    transcript_.write(message);
}

schedule::secret connection::transcript() const NOEXCEPT
{
    return transcript_.hash();
}

// Handshake messages.
// ----------------------------------------------------------------------------

data_chunk connection::make_message(uint8_t type,
    const data_chunk& body) NOEXCEPT
{
    writer out{};
    out.write_8(type);
    out.write_vector_24(body);
    return out.data();
}

// Certificate (4.4.2), without extensions.
data_chunk connection::make_certificate(const context::chain& chain) NOEXCEPT
{
    writer entries{};
    for (const auto& certificate: chain)
    {
        entries.write_vector_24(certificate);
        entries.write_16(0);
    }

    writer certificate{};
    certificate.write_vector_8({});
    certificate.write_vector_24(entries.data());
    return make_message(handshake::certificate, certificate.data());
}

// Certificate (4.4.2), empty request context, chain may be empty.
bool connection::parse_certificate(x509::certificates& out, uint8_t& alert,
    const span& body) NOEXCEPT
{
    reader certificate{ body };
    const auto request_context = certificate.read_vector_8();
    reader entries{ certificate.read_vector_24() };
    alert = alert::decode_error;
    if (!certificate.is_complete())
        return false;

    alert = alert::illegal_parameter;
    if (!request_context.empty())
        return false;

    x509::certificates chain{};
    while (entries && !entries.is_complete())
    {
        const auto der = entries.read_vector_24();
        entries.read_vector_16();
        alert = alert::decode_error;
        if (!entries)
            return false;

        x509::certificate value{};
        alert = alert::bad_certificate;
        if (!x509::parse(value, der))
            return false;

        chain.push_back(std::move(value));
    }

    alert = alert::decode_error;
    if (!entries)
        return false;

    out = std::move(chain);
    return true;
}

uint8_t connection::to_alert(const code& ec) NOEXCEPT
{
    if (ec == system::error::certificate_expired ||
        ec == system::error::certificate_not_yet_valid)
        return alert::certificate_expired;

    if (ec == system::error::chain_untrusted)
        return alert::unknown_ca;

    return alert::bad_certificate;
}

data_chunk connection::make_verify(std::string_view label) const NOEXCEPT
{
    const auto content = verify_content(label, transcript());
    writer verify{};
    if (context_.curve() == x509::curve::secp256r1)
        write_signature<secp256r1>(verify, ecdsa_secp256r1_sha256,
            context_.key(), sha256_hash(content));
    else
        write_signature<secp384r1>(verify, ecdsa_secp384r1_sha384,
            context_.key384(), accumulator<sha512_384>::hash(content));

    return make_message(handshake::certificate_verify, verify.data());
}

bool connection::is_verified(const x509::certificate& leaf, uint16_t scheme,
    const span& signature, std::string_view label) const NOEXCEPT
{
    const auto content = verify_content(label, transcript());
    const auto ec = (leaf.key_type == x509::key_type::ec);
    const auto rsa = (leaf.key_type == x509::key_type::rsa);
    rsa::public_key key{};

    switch (scheme)
    {
        case ecdsa_secp256r1_sha256:
        {
            return ec && (leaf.curve == x509::curve::secp256r1) &&
                verify_signature<secp256r1>(leaf.public_key, signature,
                    sha256_hash(content));
        }
        case ecdsa_secp384r1_sha384:
        {
            const auto digest = accumulator<sha512_384>::hash(content);
            return ec && (leaf.curve == x509::curve::secp384r1) &&
                verify_signature<secp384r1>(leaf.public_key, signature,
                    digest);
        }
        case rsa_pss_rsae_sha256:
        {
            return rsa && rsa::decode(key, leaf.public_key) &&
                rsa::verify_pss(key, signature, sha256_hash(content));
        }
        case rsa_pss_rsae_sha384:
        {
            const auto digest = accumulator<sha512_384>::hash(content);
            return rsa && rsa::decode(key, leaf.public_key) &&
                rsa::verify_pss(key, signature, digest);
        }
        case rsa_pss_rsae_sha512:
        {
            return rsa && rsa::decode(key, leaf.public_key) &&
                rsa::verify_pss(key, signature, sha512_hash(content));
        }
        default:
        {
            return false;
        }
    }
}

BC_POP_WARNING()

} // namespace tls
} // namespace network
} // namespace libbitcoin
