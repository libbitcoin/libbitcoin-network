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
#include "../test.hpp"
#include "../functional/peer_setup_fixture.hpp"
#include <numeric>

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
using namespace network::messages::peer;

// settings (inject net)
// mock_net (inject connector)
// mock_sessions [mock_net] (bypass protocol attachments)
// mock_connector (inject channel)
// mock_channel (inject protocols, uses net/connector, mock send/receive)
// mock_protocol(s) (test)
// deconfigure inbound/outbound/seed, use manual for test (?)

class mock_channel
  : public channel_peer
{
public:
    using channel_peer::channel_peer;
};

// Use mock acceptor to inject mock channel.
class mock_acceptor
  : public acceptor
{
public:
    mock_acceptor(const logger& log, asio::strand& strand,
        asio::context& service, acceptor::parameters&& params) NOEXCEPT
      : acceptor(log, strand, service, suspended_, std::move(params)),
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
        const auto socket = std::make_shared<network::socket>(log, service_,
            parameters_);

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
        asio::context& service, connector::parameters&& params) NOEXCEPT
      : connector(log, strand, service, suspended_, std::move(params)),
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
        const config::endpoint&, socket_handler&& handler) NOEXCEPT override
    {
        const auto socket = std::make_shared<network::socket>(log, service_,
            parameters_);
        handler(error::success, socket);
    }

private:
    bool stopped_;
    std::atomic_bool suspended_{ false };
};

// Use mock net network to inject mock channels.
class mock_net
  : public net
{
public:
    using net::net;

    // Create mock acceptor to inject mock channel.
    acceptor::ptr create_acceptor(const socket::context& context) NOEXCEPT override
    {
        acceptor::parameters params
        {
            .maximum_request = network_settings().inbound.maximum_request,
            .context = context
        };

        return std::make_shared<mock_acceptor>(log, strand(), service(), std::move(params));
    }

    // Create mock connector to inject mock channel.
    connector::ptr to_connector(const settings::socks5& ,
        const settings::tcp_server& options,
        const steady_clock::duration& timeout, size_t) NOEXCEPT override
    {
        connector::parameters params
        {
            .connect_timeout = timeout,
            .maximum_request = options.maximum_request,
            .minimum_buffer = options.minimum_buffer,
            .maximum_buffer = options.maximum_buffer
        };

        return std::make_shared<mock_connector>(log, strand(), service(), std::move(params));
    }
};

// network::session namespace avoids xcode global namespace pollution.
class mock_session
  : public network::session
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
  : public protocol_peer
{
public:
    typedef std::shared_ptr<mock_protocol> ptr;

    // network::session namespace avoids xcode global namespace pollution.
    mock_protocol(const network::session::ptr& session,
        const channel::ptr& channel) NOEXCEPT
      : protocol_peer(session, channel)
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

    config::endpoint opposite() const NOEXCEPT override
    {
        return protocol::opposite();
    }

    uint64_t nonce() const NOEXCEPT override
    {
        return protocol::nonce();
    }

    version::cptr peer_version() const NOEXCEPT override
    {
        return protocol_peer::peer_version();
    }

    void set_peer_version(const version::cptr& value) NOEXCEPT override
    {
        protocol_peer::set_peer_version(value);
    }

    uint32_t negotiated_version() const NOEXCEPT override
    {
        return protocol_peer::negotiated_version();
    }

    void set_negotiated_version(uint32_t value) NOEXCEPT override
    {
        protocol_peer::set_negotiated_version(value);
    }

    /// Addresses.
    /// -----------------------------------------------------------------------

    void fetch(address_handler&& handler) NOEXCEPT override
    {
        return protocol_peer::fetch(std::move(handler));
    }

    void save(const messages::peer::address::cptr& message,
        count_handler&& handler) NOEXCEPT override
    {
        return protocol_peer::save(message, std::move(handler));
    }

    virtual void handle_send(const code& ec) NOEXCEPT override
    {
        // Causes channel stop on ec.
        return protocol::handle_send(ec);
    }
};

// Protocol properties captured on the channel strand by probe_protocol.
struct probe_record
{
    bool encrypted{ true };
    size_t start_height{ max_size_t };
    version::cptr peer_version{};
    uint32_t negotiated_version{};
    bool wants_address_v2{};
    address selfs{};
    bool gated{};
    size_t remaining{};
    uint16_t opposite_port{};
    uint16_t binding_port{};
    bool terminal{ true };
    uint64_t nonce{};
    uint64_t sent{};
    uint64_t received{};
    uint64_t sent_by_message{};
    uint64_t received_by_message{};
    uint32_t created{};
    uint32_t last_read{};
    uint32_t last_write{};
    uint64_t identifier{};
    uint64_t minimum_fee{};
    steady_clock::duration ping_time{};
    steady_clock::duration minimum_ping_time{ steady_clock::duration::max() };
    steady_clock::duration pending_ping_time{ steady_clock::duration::max() };
    uint64_t sender{};
    code save_ec{ error::unknown };
    size_t accepted{};
    size_t address_count{};
    code fetch_ec{ error::unknown };
    address::cptr fetched{};
};

using probe_promise = std::promise<probe_record>;

using namespace std::placeholders;

#define CLASS probe_protocol

class probe_protocol
  : public protocol_peer
{
public:
    typedef std::shared_ptr<probe_protocol> ptr;

    probe_protocol(const network::session::ptr& session,
        const channel::ptr& channel, probe_promise& promise,
        bool drop) NOEXCEPT
      : protocol_peer(session, channel), promise_(promise), drop_(drop)
    {
    }

    void start() NOEXCEPT override
    {
        if (started())
            return;

        record_.encrypted = encrypted();
        record_.start_height = start_height();
        set_peer_version(peer_version());
        record_.peer_version = peer_version();
        set_negotiated_version(negotiated_version());
        record_.negotiated_version = negotiated_version();
        set_wants_address_v2();
        record_.wants_address_v2 = wants_address_v2();
        record_.selfs = selfs();
        record_.gated = !is_null(gate());
        record_.remaining = remaining();
        record_.opposite_port = opposite().port();
        record_.binding_port = binding().port();
        record_.terminal = is_terminal(zero);
        record_.nonce = nonce();
        record_.sent = sent();
        record_.received = received();
        record_.sent_by_message = std::accumulate(sent_by_message().begin(), sent_by_message().end(), 0_u64);
        record_.received_by_message = std::accumulate(received_by_message().begin(), received_by_message().end(), 0_u64);
        record_.created = created();
        record_.last_read = last_read();
        record_.last_write = last_write();
        record_.identifier = identifier();
        set_minimum_fee(42);
        record_.minimum_fee = minimum_fee();
        set_ping();
        set_pong();
        record_.pending_ping_time = pending_ping_time();
        record_.ping_time = ping_time();
        record_.minimum_ping_time = minimum_ping_time();

        SUBSCRIBE_BROADCAST(address, handle_broadcast, _1, _2, _3);
        BROADCAST(address, system::to_shared<address>(record_.selfs));
        protocol::start();

        if (drop_)
            stop(error::channel_timeout);
    }

private:
    bool handle_broadcast(const code&, const address::cptr& message,
        uint64_t sender) NOEXCEPT
    {
        record_.sender = sender;
        UNSUBSCRIBE_BROADCAST();
        save(message, BIND(handle_save, _1, _2));
        return false;
    }

    void handle_save(const code& ec, size_t accepted) NOEXCEPT
    {
        record_.save_ec = ec;
        record_.accepted = accepted;
        record_.address_count = address_count();
        fetch(BIND(handle_fetch, _1, _2));
    }

    void handle_fetch(const code& ec, const address::cptr& message) NOEXCEPT
    {
        record_.fetch_ec = ec;
        record_.fetched = message;
        promise_.set_value(record_);
    }

    probe_promise& promise_;
    probe_record record_{};
    const bool drop_;
};

#undef CLASS

class probe_session
  : public session_inbound
{
public:
    probe_session(net& network, uint64_t identifier,
        probe_promise& promise, const bool& drop) NOEXCEPT
      : session_inbound(network, identifier), promise_(promise), drop_(drop)
    {
    }

protected:
    void attach_protocols(const channel::ptr& channel) NOEXCEPT override
    {
        session_inbound::attach_protocols(channel);
        channel->attach<probe_protocol>(shared_from_this(), promise_, drop_)->start();
    }

private:
    probe_promise& promise_;
    const bool& drop_;
};

class probe_net
  : public net
{
public:
    probe_net(const network::settings& settings,
        const network::logger& log) NOEXCEPT
      : net(settings, log)
    {
    }

    probe_promise promise{};
    bool drop{};

protected:
    session_inbound::ptr attach_inbound_session() NOEXCEPT override
    {
        return attach<probe_session>(*this, promise, drop);
    }
};

struct protocol_probe_setup_fixture
  : peer_net_setup_fixture<probe_net>
{
    protocol_probe_setup_fixture()
    {
        settings_.outbound.host_pool_capacity = 100;
        settings_.address_lower = 1;
        settings_.address_upper = 1;
        settings_.inbound.selfs.emplace_back("1.2.3.4:8333");
        settings_.inbound.selfs.emplace_back("[2001:db8::1]:8333");
        settings_.inbound.self = network::config::address{ "5.6.7.8:8333" };
    }
};

BOOST_FIXTURE_TEST_CASE(protocol__properties__handshaken_inbound__expected, protocol_probe_setup_fixture)
{
    settings_.inbound.inactivity_minutes = 10;
    settings_.inbound.expiration_minutes = 60;
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::bip61));
    const auto record = net_->promise.get_future().get();
    const auto now = network::unix_time();

    BOOST_REQUIRE(!record.encrypted);
    BOOST_REQUIRE_EQUAL(record.start_height, node_version->start_height);
    BOOST_REQUIRE(record.peer_version);
    BOOST_REQUIRE_EQUAL(record.peer_version->value, level::bip61);
    BOOST_REQUIRE_EQUAL(record.peer_version->user_agent, "/test/");
    BOOST_REQUIRE_EQUAL(record.negotiated_version, level::bip61);
    BOOST_REQUIRE(record.wants_address_v2);
    BOOST_REQUIRE_EQUAL(record.selfs.addresses.size(), 2u);
    BOOST_REQUIRE(settings_.inbound.selfs.front() == record.selfs.addresses.front());
    BOOST_REQUIRE(settings_.inbound.self == record.selfs.addresses.back());
    BOOST_REQUIRE_EQUAL(record.selfs.addresses.front().services, service::node_none);
    BOOST_REQUIRE_GT(record.remaining, 0u);
    BOOST_REQUIRE_LE(record.remaining, settings_.inbound.expiration_minutes * 60u);
    BOOST_REQUIRE_EQUAL(record.opposite_port, port());
    BOOST_REQUIRE_EQUAL(record.binding_port, settings_.inbound.binds.back().port());
    BOOST_REQUIRE(!record.terminal);
    BOOST_REQUIRE_EQUAL(record.nonce, node_version->nonce);
    BOOST_REQUIRE_GT(record.sent, 0u);
    BOOST_REQUIRE_GT(record.received, 0u);
    BOOST_REQUIRE_GT(record.sent_by_message, 0u);
    BOOST_REQUIRE_GT(record.received_by_message, 0u);
    BOOST_REQUIRE_GT(record.created, 0u);
    BOOST_REQUIRE_LE(record.created, now);
    BOOST_REQUIRE_LE(record.created, record.last_read);
    BOOST_REQUIRE_LE(record.last_read, now);
    BOOST_REQUIRE_LE(record.created, record.last_write);
    BOOST_REQUIRE_LE(record.last_write, now);
    BOOST_REQUIRE_NE(record.identifier, 0u);
    BOOST_REQUIRE_EQUAL(record.minimum_fee, 42u);
    BOOST_REQUIRE(record.pending_ping_time == steady_clock::duration::zero());
    BOOST_REQUIRE(record.minimum_ping_time <= record.ping_time);
    BOOST_REQUIRE_EQUAL(record.sender, record.identifier);
    BOOST_REQUIRE_EQUAL(record.save_ec, error::success);
    BOOST_REQUIRE_EQUAL(record.accepted, 2u);
    BOOST_REQUIRE_EQUAL(record.address_count, 2u);
    BOOST_REQUIRE_EQUAL(record.fetch_ec, error::success);
    BOOST_REQUIRE(record.fetched);
    BOOST_REQUIRE_EQUAL(record.fetched->addresses.size(), 2u);
}

BOOST_FIXTURE_TEST_CASE(protocol__stop__handshaken_inbound__dropped, protocol_probe_setup_fixture)
{
    BOOST_REQUIRE(!start());
    net_->drop = true;
    BOOST_REQUIRE(!run());
    connect(settings_.inbound.binds.back().to_endpoint());
    BOOST_REQUIRE(handshake(level::bip61));
    BOOST_REQUIRE(dropped());
}

BOOST_AUTO_TEST_SUITE_END()
