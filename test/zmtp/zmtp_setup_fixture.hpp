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
#ifndef LIBBITCOIN_NETWORK_TEST_ZMTP_ZMTP_SETUP_FIXTURE
#define LIBBITCOIN_NETWORK_TEST_ZMTP_ZMTP_SETUP_FIXTURE

#include <future>
#include "../test.hpp"

// The socket runs on the fixture pool, and the peer is a plain socket driven
// synchronously from the test thread. Each step owns one completion, awaited
// before the step returns, and the pool is joined before teardown, so no
// completion outlives the case that armed it.

using zmtp_stream = network::zmtp::stream;
using zmtp_context = network::zmtp::context;
using zmtp_role = network::zmtp::role;
using peer_socket = network::asio::socket;

// The role harness servers bind these ports for their libzmq peers.
#define ZMTP_PULLER_PORT 65031
#define ZMTP_REPLIER_PORT 65032
#define ZMTP_ROUTER_PORT 65033

// Wire (peer side).
// ----------------------------------------------------------------------------

/// Build a command frame with the given name and trailing content.
system::data_chunk command(const std::string& name, const system::data_chunk& content);

/// Build one metadata property (name u8-length, value u32be-length).
system::data_chunk property(const std::string& key, const std::string& value);

/// Build a READY command advertising the socket type (and identity if any).
system::data_chunk ready(const std::string& type, const std::string& identity={});

/// Parse a command frame body into its name and content.
void parse_command(const system::data_chunk& body, std::string& name, system::data_chunk& content);

/// Synchronously write the whole buffer to the peer socket.
void peer_write(peer_socket& peer, const system::data_chunk& data);

/// Synchronously read one short frame (flags and body) from the peer socket.
void peer_read_frame(peer_socket& peer, uint8_t& flags, system::data_chunk& body);

/// Synchronously read one whole (multipart) message from the peer socket.
system::data_stack peer_read_message(peer_socket& peer);

/// Synchronously read one command frame, returning its name and content.
std::string peer_read_command(peer_socket& peer, system::data_chunk& content);

/// Synchronously perform the peer side of the handshake as the given socket
/// type, returning the name of the first command received (READY or ERROR).
std::string peer_handshake(peer_socket& peer, const std::string& type, const std::string& identity={}, uint8_t minor=1);

// Rpc (message side).
// ----------------------------------------------------------------------------

/// Positional params are byte chunks.
system::data_chunk param_of(const rpc::request& request, size_t index);
size_t params_of(const rpc::request& request);

/// The prefix and stop params of a subscription.
system::data_chunk prefix_of(const rpc::request& request);
bool stop_of(const rpc::request& request);

/// A byte chunk of the given text.
system::data_chunk chunk(const std::string& text);

// Socket (rpc by role, against a synchronous peer).
// ----------------------------------------------------------------------------

/// A server socket in the given role and a peer of the given socket type.
struct socket_setup_fixture
{
    DELETE_COPY_MOVE(socket_setup_fixture);

    socket_setup_fixture(zmtp_role value, const std::string& type, size_t maximum=4096, const std::string& identity={});
    ~socket_setup_fixture();

    /// Each blocks until the socket completes the operation.
    code read(rpc::request& request);
    code notify(rpc::request&& notification);
    code respond(rpc::response&& response);

    const logger log{};
    threadpool pool;
    const zmtp_context configuration{};
    socket::parameters params;
    socket::ptr sock;
    asio::strand strand;
    asio::acceptor acceptor;
    boost::asio::io_context peer_context{};
    peer_socket peer;
    http::flat_buffer buffer{};
    std::string first{};
    code accepted{};
};

struct puller_fixture
  : socket_setup_fixture
{
    puller_fixture() : socket_setup_fixture(zmtp_role::puller, "PUSH") {}
};

struct bounded_fixture
  : socket_setup_fixture
{
    bounded_fixture() : socket_setup_fixture(zmtp_role::puller, "PUSH", 16) {}
};

struct incompatible_fixture
  : socket_setup_fixture
{
    incompatible_fixture() : socket_setup_fixture(zmtp_role::puller, "SUB") {}
};

struct replier_fixture
  : socket_setup_fixture
{
    replier_fixture() : socket_setup_fixture(zmtp_role::replier, "REQ") {}
};

struct router_fixture
  : socket_setup_fixture
{
    router_fixture() : socket_setup_fixture(zmtp_role::router, "DEALER") {}
};

struct identified_router_fixture
  : socket_setup_fixture
{
    identified_router_fixture() : socket_setup_fixture(zmtp_role::router, "DEALER", 4096, "peer1") {}
};

// Proxy (control absorption over a publisher socket).
// ----------------------------------------------------------------------------

class mock_proxy
  : public proxy
{
public:
    mock_proxy(const socket::ptr& socket) NOEXCEPT
      : proxy(socket, 0)
    {
    }

    // Call must be stranded.
    void read1(http::flat_buffer& buffer, rpc::request& request, count_handler&& handler) NOEXCEPT
    {
        proxy::read(buffer, request, std::move(handler));
    }

    void write1(rpc::request&& notification, count_handler&& handler) NOEXCEPT
    {
        proxy::write(std::move(notification), std::move(handler));
    }

    void write1(rpc::response&& response, count_handler&& handler) NOEXCEPT
    {
        proxy::write(std::move(response), std::move(handler));
    }
};

/// A publisher socket with its proxy and a handshaken subscriber peer.
/// The proxy absorbs control only while a read is armed, so a case arms the
/// read, writes to the peer, and then awaits the armed read.
struct publisher_setup_fixture
{
    DELETE_COPY_MOVE(publisher_setup_fixture);

    publisher_setup_fixture();
    ~publisher_setup_fixture();

    /// Arm one rpc read, completed by await_read (fixture owns the promise).
    void arm_read(rpc::request& request);
    code await_read();

    /// Each blocks until the proxy completes the write.
    code notify(rpc::request&& notification);
    code respond(rpc::response&& response);

    const logger log{};
    threadpool pool;
    const zmtp_context configuration{};
    socket::parameters params;
    socket::ptr sock;
    std::shared_ptr<mock_proxy> prx;
    asio::strand strand;
    asio::acceptor acceptor;
    boost::asio::io_context peer_context{};
    peer_socket peer;
    http::flat_buffer buffer{};
    std::promise<code> armed{};
};

// Role (servers for the pyzmq harness, see test/pyzmq/zmtp_roles.py).
// ----------------------------------------------------------------------------

/// A server socket in the given role, accepted from the harness peer. The
/// fixture binds only under ZMTP_HARNESS, where the peer is guaranteed.
struct role_setup_fixture
{
    DELETE_COPY_MOVE(role_setup_fixture);

    role_setup_fixture(zmtp_role value, uint16_t port);
    ~role_setup_fixture();

    /// Each blocks until the socket completes the operation.
    code read(rpc::request& request);
    code notify(rpc::request&& notification);
    code respond(rpc::response&& response);

    const bool enabled;
    const logger log{};
    threadpool pool;
    const zmtp_context configuration{};
    socket::parameters params;
    socket::ptr sock;
    asio::strand strand;
    asio::acceptor acceptor;
    http::flat_buffer buffer{};
};

struct role_puller_fixture
  : role_setup_fixture
{
    role_puller_fixture() : role_setup_fixture(zmtp_role::puller, ZMTP_PULLER_PORT) {}
};

struct role_replier_fixture
  : role_setup_fixture
{
    role_replier_fixture() : role_setup_fixture(zmtp_role::replier, ZMTP_REPLIER_PORT) {}
};

struct role_router_fixture
  : role_setup_fixture
{
    role_router_fixture() : role_setup_fixture(zmtp_role::router, ZMTP_ROUTER_PORT) {}
};

#endif
