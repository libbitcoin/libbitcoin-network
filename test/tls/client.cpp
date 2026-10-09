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
#include "client.hpp"

#include <algorithm>
#include <iterator>

namespace test {

using namespace bc::system;
using namespace network::tls;
using reader = network::tls::reader;
using writer = network::tls::writer;

constexpr size_t verify_padding = 64;

static data_chunk message_of(uint8_t type, const data_chunk& body)
{
    writer out{};
    out.write_8(type);
    out.write_vector_24(body);
    return out.data();
}

static data_chunk verify_content(const std::string& label,
    const schedule::secret& hash)
{
    const data_chunk padding(verify_padding, 0x20);
    const data_array<one> separator{ 0x00 };
    return build_chunk({ padding, label, separator, hash });
}

tls_client::tls_client(const options& value) NOEXCEPT
  : options_(value)
{
    maybe_random::fill(secret_);
    secret256_ = secp256r1::generate();
    secret384_ = secp384r1::generate();
    session_.resize(value.session);
    maybe_random::fill(session_);
}

data_chunk& tls_client::output() NOEXCEPT { return output_; }
bool tls_client::is_established() const NOEXCEPT { return state_ == state::done; }
bool tls_client::is_closed() const NOEXCEPT { return closed_; }
bool tls_client::is_failed() const NOEXCEPT { return failed_; }
uint8_t tls_client::failure() const NOEXCEPT { return failure_; }
bool tls_client::is_retried() const NOEXCEPT { return retried_; }
bool tls_client::is_requested() const NOEXCEPT { return requested_; }
uint16_t tls_client::suite() const NOEXCEPT { return suite_; }

void tls_client::start() NOEXCEPT
{
    hello(false);
}

void tls_client::hello(bool retry) NOEXCEPT
{
    writer suites{};
    for (const auto suite: options_.suites)
        suites.write_16(suite);

    writer versions{};
    versions.write_16(version_13);

    writer groups{};
    for (const auto group: options_.groups)
        groups.write_16(group);

    writer algorithms{};
    for (const auto algorithm: options_.algorithms)
        algorithms.write_16(algorithm);

    writer shares{};
    const auto shared = retry ? std::vector<uint16_t>{ retry_group_ } : options_.shares;
    for (const auto group: shared)
    {
        shares.write_16(group);
        shares.write_vector_16(share(group));
    }

    writer vector_versions{};
    vector_versions.write_vector_8(versions.data());
    writer vector_groups{};
    vector_groups.write_vector_16(groups.data());
    writer vector_algorithms{};
    vector_algorithms.write_vector_16(algorithms.data());
    writer vector_shares{};
    vector_shares.write_vector_16(shares.data());

    writer extensions{};
    extensions.write_16(extension::supported_versions);
    extensions.write_vector_16(vector_versions.data());
    extensions.write_16(extension::supported_groups);
    extensions.write_vector_16(vector_groups.data());
    extensions.write_16(extension::signature_algorithms);
    extensions.write_vector_16(vector_algorithms.data());
    extensions.write_16(extension::key_share);
    extensions.write_vector_16(vector_shares.data());

    data_array<32> random{};
    maybe_random::fill(random);

    writer body{};
    body.write_16(legacy_version);
    body.write_bytes(random);
    body.write_vector_8(session_);
    body.write_vector_16(suites.data());
    body.write_vector_8(data_array<1>{ 0x00 });
    body.write_vector_16(extensions.data());

    const auto message = message_of(handshake::client_hello, body.data());
    add(message);
    send_.seal(output_, content::handshake, message);
}

bool tls_client::fail(uint8_t alert) NOEXCEPT
{
    if (failed_)
        return false;

    failed_ = true;
    failure_ = alert;
    const data_array<2> content{ alert_fatal, alert };
    send_.seal(output_, content::alert, content);
    return false;
}

bool tls_client::receive(const const_byte_span& data) NOEXCEPT
{
    if (failed_)
        return false;

    input_.insert(input_.end(), data.begin(), data.end());
    while (!failed_ && !closed_ && input_.size() >= record_header_size)
    {
        reader header{ const_byte_span{ input_ }.first(record_header_size) };
        const auto type = header.read_8();
        header.read_16();
        const auto size = header.read_16();
        if (input_.size() < record_header_size + size)
            break;

        const auto head = data_chunk(input_.begin(), std::next(input_.begin(), record_header_size));
        const auto fragment = data_chunk(std::next(input_.begin(), record_header_size), std::next(input_.begin(), record_header_size + size));
        input_.erase(input_.begin(), std::next(input_.begin(), record_header_size + size));

        if (type == content::change_cipher_spec)
            continue;

        if (!receive_.is_protected())
        {
            if (!handle(type, fragment))
                return false;

            continue;
        }

        uint8_t inner{};
        data_chunk content{};
        if (!receive_.open(inner, content, head, fragment))
            return fail(alert::bad_record_mac);

        if (!handle(inner, content))
            return false;
    }

    return !failed_;
}

bool tls_client::handle(uint8_t type, const const_byte_span& content) NOEXCEPT
{
    if (type == content::alert)
    {
        if ((content.size() == 2u) && (content[1] == alert::close_notify))
        {
            closed_ = true;
            return true;
        }

        failed_ = true;
        failure_ = content.size() == 2u ? content[1] : alert::decode_error;
        return false;
    }

    if (type == content::application_data)
    {
        application_.insert(application_.end(), content.begin(), content.end());
        return true;
    }

    if (type != content::handshake)
        return fail(alert::unexpected_message);

    handshake_.insert(handshake_.end(), content.begin(), content.end());
    while (handshake_.size() >= 4u)
    {
        reader header{ const_byte_span{ handshake_ }.first(4) };
        const auto kind = header.read_8();
        const auto size = header.read_24();
        if (handshake_.size() < 4u + size)
            break;

        const auto message = data_chunk(handshake_.begin(), std::next(handshake_.begin(), 4u + size));
        handshake_.erase(handshake_.begin(), std::next(handshake_.begin(), 4u + size));
        const auto body = const_byte_span{ message }.subspan(4);
        if (!handle_message(kind, message, body))
            return false;
    }

    return true;
}

bool tls_client::handle_message(uint8_t type, const const_byte_span& message,
    const const_byte_span& body) NOEXCEPT
{
    switch (state_)
    {
        case state::hello:
            if (type == handshake::server_hello)
                return handle_hello(message, body);
            break;
        case state::encrypted:
            if (type == handshake::encrypted_extensions)
            {
                add(message);
                state_ = state::certificate;
                return true;
            }
            break;
        case state::certificate:
            if (type == handshake::certificate_request)
            {
                add(message);
                requested_ = true;
                return true;
            }
            if (type == handshake::certificate)
                return handle_certificate(message, body);
            break;
        case state::verify:
            if (type == handshake::certificate_verify)
                return handle_verify(message, body);
            break;
        case state::finished:
            if (type == handshake::finished)
                return handle_finished(message, body);
            break;
        case state::done:
            if (type == handshake::key_update)
            {
                server_traffic_ = schedule{ suite_ }.update(server_traffic_);
                receive_.set_secret(suite_, server_traffic_);
                if (!body.empty() && (body.front() == 1u))
                    update(false);

                return true;
            }
            break;
    }

    return fail(alert::unexpected_message);
}

bool tls_client::handle_hello(const const_byte_span& message,
    const const_byte_span& body) NOEXCEPT
{
    reader server_hello{ body };
    server_hello.read_16();
    const auto random = server_hello.read_bytes(32);
    server_hello.read_vector_8();
    suite_ = server_hello.read_16();
    server_hello.read_8();
    reader extensions{ server_hello.read_vector_16() };
    if (!server_hello.is_complete())
        return fail(alert::decode_error);

    if (std::equal(random.begin(), random.end(), retry_random.begin()))
    {
        if (retried_)
            return fail(alert::unexpected_message);

        while (extensions && !extensions.is_complete())
        {
            const auto type = extensions.read_16();
            reader data{ extensions.read_vector_16() };
            if (type == extension::key_share)
                retry_group_ = data.read_16();
        }

        retried_ = true;
        const auto first = hash();
        writer synthetic{};
        synthetic.write_8(handshake::message_hash);
        synthetic.write_vector_24(first);
        transcript_.clear();
        add(synthetic.data());
        add(message);
        hello(true);
        return true;
    }

    uint16_t group{};
    const_byte_span peer{};
    while (extensions && !extensions.is_complete())
    {
        const auto type = extensions.read_16();
        reader data{ extensions.read_vector_16() };
        if (type == extension::key_share)
        {
            group = data.read_16();
            peer = data.read_vector_16();
        }
    }

    data_chunk shared{};
    if (!agree(shared, group, peer))
        return fail(alert::illegal_parameter);

    add(message);
    const schedule keys{ suite_ };
    handshake_secret_ = keys.handshake_secret(keys.early_secret(), shared);
    const auto transcript = hash();
    client_handshake_ = keys.derive_secret(handshake_secret_, "c hs traffic", transcript);
    server_handshake_ = keys.derive_secret(handshake_secret_, "s hs traffic", transcript);
    receive_.set_secret(suite_, server_handshake_);
    state_ = state::encrypted;
    return true;
}

bool tls_client::handle_certificate(const const_byte_span& message,
    const const_byte_span& body) NOEXCEPT
{
    reader certificate{ body };
    certificate.read_vector_8();
    reader entries{ certificate.read_vector_24() };
    x509::certificates chain{};
    while (entries && !entries.is_complete())
    {
        const auto der = entries.read_vector_24();
        entries.read_vector_16();
        x509::certificate value{};
        if (!x509::parse(value, der))
            return fail(alert::bad_certificate);

        chain.push_back(std::move(value));
    }

    if (chain.empty())
        return fail(alert::decode_error);

    if (!options_.anchors.empty())
    {
        const auto time = is_zero(options_.time) ? chain.front().not_before : options_.time;
        if (x509::verify(chain, options_.anchors, time, x509::purpose::server))
            return fail(alert::bad_certificate);
    }

    server_key_ = chain.front().public_key;
    add(message);
    state_ = state::verify;
    return true;
}

bool tls_client::handle_verify(const const_byte_span& message,
    const const_byte_span& body) NOEXCEPT
{
    reader verify{ body };
    const auto scheme = verify.read_16();
    const auto der = verify.read_vector_16();
    const auto content = verify_content("TLS 1.3, server CertificateVerify", hash());

    auto valid = false;
    if (scheme == ecdsa_secp256r1_sha256)
    {
        secp256r1::point_t point{};
        secp256r1::signature_t signature{};
        if ((server_key_.size() != point.size()) || !secp256r1::decode(signature, der))
            return fail(alert::illegal_parameter);

        std::copy(server_key_.cbegin(), server_key_.cend(), point.begin());
        valid = secp256r1::verify(signature, point, sha256_hash(content));
    }
    else if (scheme == ecdsa_secp384r1_sha384)
    {
        secp384r1::point_t point{};
        secp384r1::signature_t signature{};
        if ((server_key_.size() != point.size()) || !secp384r1::decode(signature, der))
            return fail(alert::illegal_parameter);

        std::copy(server_key_.cbegin(), server_key_.cend(), point.begin());
        valid = secp384r1::verify(signature, point, accumulator<sha512_384>::hash(content));
    }
    else
    {
        return fail(alert::illegal_parameter);
    }

    if (!valid)
        return fail(alert::decrypt_error);

    add(message);
    state_ = state::finished;
    return true;
}

bool tls_client::handle_finished(const const_byte_span& message,
    const const_byte_span& body) NOEXCEPT
{
    const schedule keys{ suite_ };
    const auto expected = keys.finished(server_handshake_, hash());
    if (!std::equal(body.begin(), body.end(), expected.begin(), expected.end()))
        return fail(alert::decrypt_error);

    add(message);
    const auto master = keys.master_secret(handshake_secret_);
    const auto transcript = hash();
    client_traffic_ = keys.derive_secret(master, "c ap traffic", transcript);
    server_traffic_ = keys.derive_secret(master, "s ap traffic", transcript);

    send_.set_secret(suite_, client_handshake_);
    data_chunk flight{};
    if (requested_)
    {
        writer entries{};
        for (const auto& certificate: options_.chain)
        {
            entries.write_vector_24(certificate);
            entries.write_16(0);
        }

        writer certificate{};
        certificate.write_vector_8({});
        certificate.write_vector_24(entries.data());
        const auto certificate_message = message_of(handshake::certificate, certificate.data());
        add(certificate_message);
        flight.insert(flight.end(), certificate_message.begin(), certificate_message.end());

        if (!options_.chain.empty())
        {
            const auto content = verify_content("TLS 1.3, client CertificateVerify", hash());
            writer verify{};
            if (options_.key384)
            {
                secp384r1::signature_t signature{};
                secp384r1::sign(signature, *options_.key384, accumulator<sha512_384>::hash(content));
                verify.write_16(ecdsa_secp384r1_sha384);
                verify.write_vector_16(secp384r1::encode(signature));
            }
            else
            {
                secp256r1::signature_t signature{};
                secp256r1::sign(signature, options_.key, sha256_hash(content));
                verify.write_16(ecdsa_secp256r1_sha256);
                verify.write_vector_16(secp256r1::encode(signature));
            }

            const auto verify_message = message_of(handshake::certificate_verify, verify.data());
            add(verify_message);
            flight.insert(flight.end(), verify_message.begin(), verify_message.end());
        }
    }

    const auto finished = message_of(handshake::finished, keys.finished(client_handshake_, hash()));
    add(finished);
    flight.insert(flight.end(), finished.begin(), finished.end());
    send_.seal(output_, content::handshake, flight);

    send_.set_secret(suite_, client_traffic_);
    receive_.set_secret(suite_, server_traffic_);
    state_ = state::done;
    return true;
}

bool tls_client::write(const const_byte_span& data) NOEXCEPT
{
    if (!is_established())
        return false;

    send_.seal(output_, content::application_data, data);
    return true;
}

data_chunk tls_client::read() NOEXCEPT
{
    auto out = std::move(application_);
    application_.clear();
    return out;
}

void tls_client::update(bool request) NOEXCEPT
{
    const data_array<1> value{ request ? uint8_t{ 1 } : uint8_t{ 0 } };
    send_.seal(output_, content::handshake, message_of(handshake::key_update, to_chunk(value)));
    client_traffic_ = schedule{ suite_ }.update(client_traffic_);
    send_.set_secret(suite_, client_traffic_);
}

void tls_client::close() NOEXCEPT
{
    const data_array<2> content{ alert_warning, alert::close_notify };
    send_.seal(output_, content::alert, content);
}

void tls_client::add(const const_byte_span& message) NOEXCEPT
{
    transcript_.insert(transcript_.cend(), message.begin(), message.end());
}

tls_client::secret tls_client::hash() NOEXCEPT
{
    return schedule{ suite_ }.hash(transcript_);
}

data_chunk tls_client::share(uint16_t group) const NOEXCEPT
{
    if (group == secp256r1_group)
    {
        secp256r1::point_t point{};
        secp256r1::public_key(point, secret256_);
        return to_chunk(point);
    }

    if (group == secp384r1_group)
    {
        secp384r1::point_t point{};
        secp384r1::public_key(point, secret384_);
        return to_chunk(point);
    }

    x25519::key point{};
    x25519::multiply(point, secret_);
    return to_chunk(point);
}

bool tls_client::agree(data_chunk& shared, uint16_t group, const const_byte_span& peer) const NOEXCEPT
{
    if (group == secp256r1_group)
    {
        secp256r1::point_t point{};
        secp256r1::shared_t value{};
        if (peer.size() != point.size())
            return false;

        std::copy(peer.begin(), peer.end(), point.begin());
        const auto valid = secp256r1::agree(value, secret256_, point);
        shared = to_chunk(value);
        return valid;
    }

    if (group == secp384r1_group)
    {
        secp384r1::point_t point{};
        secp384r1::shared_t value{};
        if (peer.size() != point.size())
            return false;

        std::copy(peer.begin(), peer.end(), point.begin());
        const auto valid = secp384r1::agree(value, secret384_, point);
        shared = to_chunk(value);
        return valid;
    }

    x25519::key point{}, value{};
    if (peer.size() != point.size())
        return false;

    std::copy(peer.begin(), peer.end(), point.begin());
    const auto valid = x25519::multiply(value, secret_, point);
    shared = to_chunk(value);
    return valid;
}

} // namespace test
