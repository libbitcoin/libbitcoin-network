/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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
#include "../test.hpp"

////struct protocol_tests_setup_fixture
////{
////    protocol_tests_setup_fixture()
////    {
////        test::remove(TEST_NAME);
////    }
////
////    ~protocol_tests_setup_fixture()
////    {
////        test::remove(TEST_NAME);
////    }
////};

BOOST_AUTO_TEST_SUITE(protocol_tests)

using namespace bc::system::chain;
using namespace bc::network::messages;

// settings (inject p2p)
// mock_p2p (inject connector)
// mock_sessions [mock_p2p] (bypass protocol attachments)
// mock_connector (inject channel)
// mock_channel (inject protocols, uses p2p/connector, mock send/receive)
// mock_protocol(s) (test)
// deconfigure inbound/outbound/seed, use manual for test (?)

class mock_channel
  : public channel
{
public:
    using channel::channel;

    // Capture last sent payload.
    void write(const system::chunk_ptr& payload,
        const result_handler&) NOEXCEPT override
    {
        payload_ = payload;
    }

    // Override protected base to notify subscribers.
    code notify(messages::identifier, uint32_t,
        const system::data_chunk&) NOEXCEPT override
    {
        return error::success;
        ////return channel::notify(id, version, source);
    }

    // Get last sent payload.
    system::chunk_ptr sent() const NOEXCEPT
    {
        return payload_;
    }

private:
    system::chunk_ptr payload_;
};

// Use mock acceptor to inject mock channel.
class mock_acceptor
  : public acceptor
{
public:
    mock_acceptor(const logger& log, asio::strand& strand,
        asio::io_context& service, const settings& settings) NOEXCEPT
      : acceptor(log, strand, service, settings, suspended_),
        stopped_(false), port_(0)
    {
    }

    // Get captured port.
    uint16_t port() const NOEXCEPT
    {
        return port_;
    }

    // Get captured stopped.
    bool stopped() const NOEXCEPT
    {
        return stopped_;
    }

    // Capture port.
    code start(uint16_t port) NOEXCEPT override
    {
        port_ = port;
        return error::success;
    }

    // Capture port.
    code start(const config::authority& local) NOEXCEPT override
    {
        port_ = local.port();
        return error::success;
    }

    // Capture stopped.
    void stop() NOEXCEPT override
    {
        stopped_ = true;
    }

    // Inject mock channel.
    void accept(socket_handler&& handler) NOEXCEPT override
    {
        const auto socket = std::make_shared<network::socket>(log, service_);

        // Must be asynchronous or is an infinite recursion.
        // This error code will set the re-listener timer and channel pointer is ignored.
        boost::asio::post(strand_, [=]() NOEXCEPT
        {
            handler(error::success, socket);
        });
    }

private:
    bool stopped_;
    uint16_t port_;
    std::atomic_bool suspended_{ false };
};

// Use mock connector to inject mock channel.
class mock_connector
  : public connector
{
public:
    mock_connector(const logger& log, asio::strand& strand,
        asio::io_context& service, const settings& settings) NOEXCEPT
      : connector(log, strand, service, settings, suspended_),
        stopped_(false)
    {
    }

    // Get captured stopped.
    bool stopped() const NOEXCEPT
    {
        return stopped_;
    }

    // Capture stopped.
    void stop() NOEXCEPT override
    {
        stopped_ = true;
    }

    // Inject mock channel.
    void start(const std::string&, uint16_t, const config::address&,
        socket_handler&& handler) NOEXCEPT override
    {
        const auto socket = std::make_shared<network::socket>(log, service_);
        handler(error::success, socket);
    }

private:
    bool stopped_;
    std::atomic_bool suspended_{ false };
};

// Use mock p2p network to inject mock channels.
class mock_p2p
  : public p2p
{
public:
    using p2p::p2p;

    // Create mock acceptor to inject mock channel.
    acceptor::ptr create_acceptor() NOEXCEPT override
    {
        return std::make_shared<mock_acceptor>(log, strand(), service(),
            network_settings());
    }

    // Create mock connector to inject mock channel.
    connector::ptr create_connector() NOEXCEPT override
    {
        return std::make_shared<mock_connector>(log, strand(), service(),
            network_settings());
    }
};

class mock_session
  : public session
{
public:
    using session::session;

    void start(result_handler&& handler) NOEXCEPT override
    {
        return session::start(std::move(handler));
    }

    void stop() NOEXCEPT override
    {
        return session::stop();
    }

    bool stopped() const NOEXCEPT override
    {
        return session::stopped();
    }

    void attach_handshake(const channel::ptr&,
        result_handler&&) NOEXCEPT override
    {
    }
};

class mock_protocol
  : public protocol
{
public:
    typedef std::shared_ptr<mock_protocol> ptr;

    mock_protocol(const session::ptr& session,
        const channel::ptr& channel) NOEXCEPT
      : protocol(session, channel)
    {
    }

    /// Start/Stop.
    /// -----------------------------------------------------------------------

    void start() NOEXCEPT override
    {
        protocol::start();
    }

    bool started() const NOEXCEPT override
    {
        return protocol::started();
    }

    bool stopped(const code& ec=error::success) const NOEXCEPT override
    {
        return protocol::stopped(ec);
    }

    void stop(const code& ec) NOEXCEPT override
    {
        protocol::stop(ec);
    }

    /// Properties.
    /// -----------------------------------------------------------------------

    config::authority authority() const NOEXCEPT override
    {
        return protocol::authority();
    }

    uint64_t nonce() const NOEXCEPT override
    {
        return protocol::nonce();
    }

    version::cptr peer_version() const NOEXCEPT override
    {
        return protocol::peer_version();
    }

    void set_peer_version(const version::cptr& value) NOEXCEPT override
    {
        protocol::set_peer_version(value);
    }

    uint32_t negotiated_version() const NOEXCEPT override
    {
        return protocol::negotiated_version();
    }

    void set_negotiated_version(uint32_t value) NOEXCEPT override
    {
        protocol::set_negotiated_version(value);
    }

    /// Addresses.
    /// -----------------------------------------------------------------------

    void fetch(address_handler&& handler) NOEXCEPT override
    {
        return protocol::fetch(std::move(handler));
    }

    void save(const messages::address::cptr& message,
        count_handler&& handler) NOEXCEPT override
    {
        return protocol::save(message, std::move(handler));
    }

    virtual void handle_send(const code& ec) NOEXCEPT override
    {
        return protocol::handle_send(ec);
    }
};

BOOST_AUTO_TEST_CASE(protocol_test)
{
    BOOST_REQUIRE(true);
}

BOOST_AUTO_TEST_SUITE_END()
