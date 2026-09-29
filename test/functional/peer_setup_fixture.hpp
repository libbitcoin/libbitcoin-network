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
#ifndef LIBBITCOIN_NETWORK_TEST_FUNCTIONAL_PEER_SETUP_FIXTURE
#define LIBBITCOIN_NETWORK_TEST_FUNCTIONAL_PEER_SETUP_FIXTURE

#include "../test.hpp"
#include <fstream>
#include <future>

#define PEER_FUNCTIONAL_ENDPOINT "127.0.0.1:65008"

// Runs a real net instance accepting on loopback, with the test acting as
// the remote peer over a raw blocking socket (framing via peer messages).
struct peer_setup_fixture
{
    DELETE_COPY_MOVE(peer_setup_fixture);

    using configurator = std::function<void(network::settings&)>;
    explicit peer_setup_fixture(const configurator& configure={});
    ~peer_setup_fixture();

    /// Write a framed message to the node.
    void send(const std::string& command, const system::data_chunk& payload);

    /// Serialize and write a framed message to the node.
    template <class Message>
    void send(const Message& message, uint32_t version)
    {
        system::data_chunk payload(message.size(version));
        BOOST_REQUIRE(message.serialize(version, payload));
        send(Message::command, payload);
    }

    /// Read one framed message from the node.
    std::pair<std::string, system::data_chunk> receive();

    /// Read framed messages from the node until the command matches.
    system::data_chunk receive(const std::string& command);

    /// Perform the version handshake, retains the node's version message.
    bool handshake(uint64_t services=0,
        uint32_t version=messages::peer::level::maximum_protocol);

    /// The node's version message (set by handshake).
    messages::peer::version::cptr node_version{};

protected:
    network::settings settings_;
    network::logger log_{};
    network::net net_;

private:
    boost::asio::io_context io_{};
    boost::asio::ip::tcp::socket socket_{ io_ };
};

#define PEER_LISTEN_ENDPOINT "127.0.0.1:65031"

// The test as a remote peer over a raw blocking socket, either connecting to
// the node or accepting the node's connection (framing via peer messages).
class remote_peer
{
public:
    DELETE_COPY_MOVE(remote_peer);

    remote_peer(uint32_t identifier);
    ~remote_peer();

    /// Listen for a node connection on the endpoint.
    void listen(const network::config::authority& endpoint);

    /// Accept a node connection (requires listen).
    void accept();

    /// Connect to the node on the endpoint.
    void connect(const network::config::authority& endpoint);

    /// Close the connection and the listener.
    void disconnect();

    /// The local port of the connection.
    uint16_t port() const;

    /// Write a framed message to the node.
    void send(const std::string& command, const system::data_chunk& payload);

    /// Serialize and write a framed message to the node.
    template <class Message>
    void send(const Message& message, uint32_t version)
    {
        system::data_chunk payload(message.size(version));
        BOOST_REQUIRE(message.serialize(version, payload));
        send(Message::command, payload);
    }

    /// Read one framed message from the node.
    std::pair<std::string, system::data_chunk> receive();

    /// Read framed messages from the node until the command matches.
    system::data_chunk receive(const std::string& command);

    /// Read framed messages until the message type, and deserialize it.
    template <class Message>
    typename Message::cptr receive(uint32_t version)
    {
        return Message::deserialize(version, receive(Message::command));
    }

    /// Read (and discard) framed messages until the node drops the connection.
    bool dropped();

    /// Write a version message.
    void send_version(uint32_t value, uint64_t services, uint32_t timestamp);

    /// Read until the node's version and verack, retains the node's version.
    bool receive_handshake(uint32_t value);

    /// Perform the version handshake, retains the node's version message.
    bool handshake(uint32_t value=messages::peer::level::maximum_protocol,
        uint64_t services=0);

    /// The node's version message (set by handshake).
    messages::peer::version::cptr node_version{};

private:
    const uint32_t identifier_;
    boost::asio::io_context io_{};
    boost::asio::ip::tcp::acceptor acceptor_{ io_ };
    boost::asio::ip::tcp::socket socket_{ io_ };
};

// Runs a net instance (created upon start, so settings_ may be modified until
// then), accepting on PEER_FUNCTIONAL_ENDPOINT, with the test as remote peer.
template <class Net = network::net>
struct peer_net_setup_fixture
  : remote_peer
{
    DELETE_COPY_MOVE(peer_net_setup_fixture);

    peer_net_setup_fixture()
      : remote_peer(network::settings{ system::chain::selection::mainnet }.identifier),
        settings_{ system::chain::selection::mainnet }
    {
        test::clear(test::directory);
        settings_.path = TEST_DIRECTORY;
        settings_.inbound.connections = 1;
        settings_.inbound.binds.clear();
        settings_.inbound.binds.emplace_back(PEER_FUNCTIONAL_ENDPOINT);
        settings_.outbound.connections = 0;
        settings_.outbound.seeds.clear();
        listen({ PEER_LISTEN_ENDPOINT });
    }

    ~peer_net_setup_fixture()
    {
        disconnect();
        if (net_)
            net_->close();
    }

    /// Create and start the net, the future is set by the start handler.
    std::future<code> starting()
    {
        net_ = std::make_shared<Net>(settings_, log_);
        const auto promise = std::make_shared<std::promise<code>>();
        auto future = promise->get_future();
        net_->start([promise](const code& ec) NOEXCEPT
        {
            promise->set_value(ec);
        });

        return future;
    }

    /// Create and start the net.
    code start()
    {
        return starting().get();
    }

    /// Run the started net.
    code run()
    {
        std::promise<code> promise{};
        net_->run([&](const code& ec) NOEXCEPT
        {
            promise.set_value(ec);
        });

        return promise.get_future().get();
    }

    /// Start and run the net, and connect to its inbound endpoint.
    bool open()
    {
        if (start() || run())
            return false;

        connect(settings_.inbound.binds.back().to_endpoint());
        return true;
    }

    /// Start and run the net, and accept its manual connection.
    bool dial()
    {
        if (start() || run())
            return false;

        net_->connect({ PEER_LISTEN_ENDPOINT });
        accept();
        return true;
    }

    /// Complete all work posted to the network strand before this call.
    void synchronize()
    {
        std::promise<code> promise{};
        net_->fetch_totals([&](const code& ec, const auto&) NOEXCEPT
        {
            promise.set_value(ec);
        });

        BOOST_REQUIRE(!promise.get_future().get());
    }

    /// Write the address pool file (loaded upon start).
    void write_hosts(const messages::peer::address_items& items)
    {
        std::ofstream file{ settings_.file() };
        for (const auto& item: items)
            file << network::config::address{ item } << std::endl;
    }

protected:
    network::settings settings_;
    network::logger log_{};
    std::shared_ptr<Net> net_{};
};

#endif
