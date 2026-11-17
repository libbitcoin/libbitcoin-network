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
#ifndef LIBBITCOIN_NETWORK_TEST_TLS_FIXTURES_HPP
#define LIBBITCOIN_NETWORK_TEST_TLS_FIXTURES_HPP

#include "../test.hpp"

constexpr uint64_t now = 1767225600;
const auto server_key = system::base16_array("c9afa9d845ba75166b5c215767b1d6934e50c3db36e89b127b8a622b120f6721");
const auto client_key = system::base16_array("0000000000000000000000000000000000000000000000000000000000000002");
const auto other_key = system::base16_array("0000000000000000000000000000000000000000000000000000000000000003");

struct identity
{
    std::string chain;
    std::string key;
    system::x509::certificate certificate;
};

inline identity make_identity(const system::x509::secret& key,
    const std::string& name)
{
    system::x509::subject subject{};
    subject.common_name = name;
    subject.dns_names = { "localhost" };
    subject.not_before = 1735689600;
    subject.not_after = 2524608000;

    system::data_chunk der{};
    system::x509::build_self_signed(der, key, subject);

    system::x509::certificate certificate{};
    system::x509::parse(certificate, der);
    const auto chain = system::x509::encode_certificate(der);
    const auto secret = system::x509::encode_private_key(key);
    return { chain, secret, certificate };
}

inline const identity& server_identity()
{
    static const auto value = make_identity(server_key, "server");
    return value;
}

// Server context with server identity, optionally requesting client.
struct server_setup
{
    server_setup(bool request=false, bool require=false)
    {
        context.set_chain(server_identity().chain);
        context.set_key(server_identity().key, {});
        context.set_verify(request, require);
        context.set_time(now);
    }

    tls::context context{};
};

// Client context verifying server identity, or given chain.
struct client_setup
{
    client_setup(const std::string& anchors=server_identity().chain)
    {
        context.add_anchors(anchors);
        context.set_time(now);
    }

    tls::context context{};
};

// Move bytes between peers until neither has output.
inline void exchange(tls::client& client, tls::server& server)
{
    while (!client.output().empty() || !server.output().empty())
    {
        const auto to_server = std::move(client.output());
        client.output().clear();
        server.receive(to_server);

        const auto to_client = std::move(server.output());
        server.output().clear();
        client.receive(to_client);
    }
}

inline system::data_chunk read_all(tls::connection& connection)
{
    system::data_chunk out(connection.readable());
    connection.read(out);
    return out;
}

// Handshake messages.

inline system::data_chunk message_of(uint8_t type,
    const system::data_chunk& body)
{
    tls::writer out{};
    out.write_8(type);
    out.write_vector_24(body);
    return out.data();
}

inline system::data_chunk entry_of(const system::data_chunk& der)
{
    tls::writer out{};
    out.write_vector_24(der);
    out.write_vector_16(system::data_chunk{});
    return out.data();
}

inline system::data_chunk certificate_of(
    const system::data_chunk& request_context,
    const system::data_chunk& entries)
{
    tls::writer out{};
    out.write_vector_8(request_context);
    out.write_vector_24(entries);
    return out.data();
}

inline system::data_chunk verify_of(uint16_t scheme,
    const system::data_chunk& signature)
{
    tls::writer out{};
    out.write_16(scheme);
    out.write_vector_16(signature);
    return out.data();
}

#endif
