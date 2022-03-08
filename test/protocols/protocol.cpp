/**
 * Copyright (c) 2011-2021 libbitcoin developers (see AUTHORS)
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

struct protocol_tests_setup_fixture
{
    protocol_tests_setup_fixture()
    {
        test::remove(TEST_NAME);
    }

    ~protocol_tests_setup_fixture()
    {
        test::remove(TEST_NAME);
    }
};

BOOST_FIXTURE_TEST_SUITE(protocol_tests, protocol_tests_setup_fixture)

using namespace bc::network;
using namespace bc::system::chain;
using namespace bc::network::messages;

class mock_channel
  : public channel
{
public:
    using channel::channel;

    void start() override
    {
        channel::start();
    }

    void stop(const code& ec) override
    {
        channel::stop(ec);
    }

    // Override protected base capture sent payload.
    void send_bytes(system::chunk_ptr payload,
        result_handler&& handler) override
    {
        payload_ = payload;
    }

    // Override protected base to notify subscribers.
    code notify(identifier id, uint32_t version, system::reader& source)
    {
        return notify(id, version, source);
    }

    // Get last captured payload.
    system::chunk_ptr sent() const
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
    mock_acceptor(asio::strand& strand, asio::io_context& service,
        const settings& settings)
      : acceptor(strand, service, settings), stopped_(false), port_(0)
    {
    }

    // Get captured port.
    uint16_t port() const
    {
        return port_;
    }

    // Get captured stopped.
    bool stopped() const
    {
        return stopped_;
    }

    // Capture port.
    code start(uint16_t port) override
    {
        port_ = port;
        return error::success;
    }

    // Capture stopped.
    void stop() override
    {
        stopped_ = true;
    }

    // Inject mock channel.
    void accept(accept_handler&& handler) override
    {
        const auto socket = std::make_shared<network::socket>(service_);
        const auto created = std::make_shared<mock_channel>(socket, settings_);
        handler(error::success, created);
    }

private:
    bool stopped_;
    uint16_t port_;
};

// Use mock connector to inject mock channel.
class mock_connector
  : public connector
{
public:
    mock_connector(asio::strand& strand, asio::io_context& service,
        const settings& settings)
      : connector(strand, service, settings), stopped_(false)
    {
    }

    // Get captured stopped.
    bool stopped() const
    {
        return stopped_;
    }

    // Capture stopped.
    void stop() override
    {
        stopped_ = true;
    }

    // Inject mock channel.
    void connect(const std::string& hostname, uint16_t port,
        connect_handler&& handler) override
    {
        const auto socket = std::make_shared<network::socket>(service_);
        const auto created = std::make_shared<mock_channel>(socket, settings_);
        handler(error::success, created);
    }

private:
    bool stopped_;
};

// Use mock p2p network to inject mock channels.
class mock_p2p
  : public p2p
{
public:
    using p2p::p2p;

    ////session_seed::ptr attach_seed_session() override
    ////{
    ////    return p2p::attach_seed_session();
    ////}
    ////
    ////session_manual::ptr attach_manual_session() override
    ////{
    ////    return p2p::attach_manual_session();
    ////}
    ////
    ////session_inbound::ptr attach_inbound_session() override
    ////{
    ////    return p2p::attach_inbound_session();
    ////}
    ////
    ////session_outbound::ptr attach_outbound_session() override
    ////{
    ////    return p2p::attach_outbound_session();
    ////}

    // Create mock acceptor to inject mock channel.
    acceptor::ptr create_acceptor() override
    {
        return std::make_shared<mock_acceptor>(strand(), service(),
            network_settings());
    }

    // Create mock connector to inject mock channel.
    connector::ptr create_connector() override
    {
        return std::make_shared<mock_connector>(strand(), service(),
            network_settings());
    }
};

BOOST_AUTO_TEST_CASE(protocol__run__one_connection__success)
{
    settings set(selection::mainnet);
    BOOST_REQUIRE(set.peers.empty());

    // This implies seeding would be required.
    set.host_pool_capacity = 1;

    // There are no seeds, so seeding would fail.
    set.seeds.clear();

    // Cache one address to preclude seeding.
    set.hosts_file = TEST_NAME;
    system::ofstream file(set.hosts_file);
    file << config::authority{ "1.2.3.4:42" } << std::endl;

    // Configure one connection with no batching.
    set.connect_batch_size = 5;
    set.outbound_connections = 1;

    mock_p2p net(set);

    std::promise<bool> promise_run;
    const auto run_handler = [&](const code& ec)
    {
        BOOST_REQUIRE_EQUAL(ec, error::success);
        promise_run.set_value(true);
    };

    const auto start_handler = [&](const code& ec)
    {
        BOOST_REQUIRE_EQUAL(ec, error::success);
        net.run(run_handler);
    };

    net.start(start_handler);
    BOOST_REQUIRE(promise_run.get_future().get());
}

BOOST_AUTO_TEST_SUITE_END()
