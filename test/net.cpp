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
#include "test.hpp"

#include <cstdio>
#include <future>

struct net_tests_setup_fixture
{
    net_tests_setup_fixture()
    {
        test::remove(TEST_NAME);
    }

    ~net_tests_setup_fixture()
    {
        test::remove(TEST_NAME);
    }
};

BOOST_FIXTURE_TEST_SUITE(net_tests, net_tests_setup_fixture)

using namespace bc::system::chain;
using namespace network::messages::peer;

template <error::error_t ManualCode = error::success,
    error::error_t SeedCode = error::success>
class mock_net_session_start
  : public net
{
public:
    mock_net_session_start(const settings& settings, const logger& log,
        const code& hosts_start=error::success) NOEXCEPT
      : net(settings, log), hosts_start_(hosts_start)
    {
    }

    code start_hosts() NOEXCEPT override
    {
        return hosts_start_;
    }

    session_manual::ptr attach_manual_session() NOEXCEPT override
    {
        return attach<mock_session_manual>(*this);
    }

    session_seed::ptr attach_seed_session() NOEXCEPT override
    {
        return attach<mock_session_seed>(*this);
    }

private:
    const code hosts_start_;

    class mock_session_manual
      : public session_manual
    {
    public:
        using session_manual::session_manual;

        void start(result_handler&& handler) NOEXCEPT override
        {
            handler(ManualCode);
        }
    };

    class mock_session_seed
      : public session_seed
    {
    public:
        using session_seed::session_seed;

        void start(result_handler&& handler) NOEXCEPT override
        {
            handler(SeedCode);
        }
    };
};

template <error::error_t InboundCode = error::success,
    error::error_t OutboundCode = error::success>
class mock_net_session_run
  : public net
{
public:
    mock_net_session_run(const settings& settings, const logger& log,
        bool started) NOEXCEPT
      : net(settings, log), closed_(!started)
    {
    }

    bool closed() const NOEXCEPT override
    {
        return closed_;
    }

    session_inbound::ptr attach_inbound_session() NOEXCEPT override
    {
        return attach<mock_session_inbound>(*this);
    }

    session_outbound::ptr attach_outbound_session() NOEXCEPT override
    {
        return attach<mock_session_outbound>(*this);
    }

private:
    const bool closed_;

    class mock_session_inbound
      : public session_inbound
    {
    public:
        using session_inbound::session_inbound;

        void start(result_handler&& handler) NOEXCEPT override
        {
            handler(InboundCode);
        }
    };

    class mock_session_outbound
      : public session_outbound
    {
    public:
        using session_outbound::session_outbound;

        void start(result_handler&& handler) NOEXCEPT override
        {
            handler(OutboundCode);
        }
    };
};

BOOST_AUTO_TEST_CASE(net__network_settings__unstarted__expected)
{
    const logger log{};
    settings set(selection::mainnet);
    BOOST_REQUIRE_EQUAL(set.threads, 0u);

    net net(set, log);
    BOOST_REQUIRE_EQUAL(net.network_settings().threads, 0u);
}

BOOST_AUTO_TEST_CASE(net__address_count__unstarted__zero)
{
    const logger log{};
    const settings set(selection::mainnet);
    net net(set, log);
    BOOST_REQUIRE_EQUAL(net.address_count(), 0u);
}

BOOST_AUTO_TEST_CASE(net__address_counts__unstarted__zeros)
{
    const logger log{};
    const settings set(selection::mainnet);
    net net(set, log);
    BOOST_REQUIRE(net.address_counts() == config::address_counts{});
}

BOOST_AUTO_TEST_CASE(net__dump_addresses__unstarted__service_stopped)
{
    const logger log{};
    const settings set(selection::mainnet);
    net net(set, log);

    std::promise<std::pair<code, address_cptr>> promise{};
    net.dump_addresses([&](const code& ec, const address_cptr& message) NOEXCEPT
    {
        promise.set_value({ ec, message });
    });

    const auto result = promise.get_future().get();
    BOOST_REQUIRE_EQUAL(result.first, error::service_stopped);
    BOOST_REQUIRE(!result.second);
}

BOOST_AUTO_TEST_CASE(net__channel_count__unstarted__zero)
{
    const logger log{};
    const settings set(selection::mainnet);
    net net(set, log);
    BOOST_REQUIRE_EQUAL(net.channel_count(), 0u);
}

BOOST_AUTO_TEST_CASE(net__connect__unstarted__service_stopped)
{
    const logger log{};
    const settings set(selection::mainnet);
    net net(set, log);

    std::promise<std::pair<code, channel::ptr>> promise{};
    const auto handler = [&](const code& ec, const channel::ptr& channel) NOEXCEPT
    {
        promise.set_value({ ec, channel });
        return true;
    };

    net.connect({ "truckers.ca" });
    net.connect({ "truckers.ca", 42 });
    net.connect({ "truckers.ca", 42 }, handler);
    const auto result = promise.get_future().get();
    BOOST_REQUIRE_EQUAL(result.first, error::service_stopped);
    BOOST_REQUIRE(!result.second);
}

BOOST_AUTO_TEST_CASE(net__subscribe_connect__closed___service_stopped)
{
    const logger log{};
    const settings set(selection::mainnet);
    net net(set, log);
    net.close();

    std::promise<std::pair<code, channel::ptr>> promise_handler{};
    const auto handler = [&](const code& ec, const channel::ptr& channel) NOEXCEPT
    {
        promise_handler.set_value({ ec, channel });
        return false;
    };

    std::promise<std::pair<code, net::object_key>> promise_complete{};
    const auto complete = [&](const code& ec, net::object_key key) NOEXCEPT
    {
        // First key is ++0;
        promise_complete.set_value({ ec, key });
    };

    net.subscribe_connect(handler, complete);
    const auto result1 = promise_complete.get_future().get();
    BOOST_REQUIRE_EQUAL(result1.first, error::service_stopped);
    BOOST_REQUIRE_EQUAL(result1.second, zero);

    const auto result2 = promise_handler.get_future().get();
    BOOST_REQUIRE_EQUAL(result2.first, error::service_stopped);
    BOOST_REQUIRE(!result2.second);
}

BOOST_AUTO_TEST_CASE(net__subscribe_connect__unclosed___success)
{
    const logger log{};
    const settings set(selection::mainnet);
    net net(set, log);

    std::promise<std::pair<code, channel::ptr>> promise_handler{};
    const auto handler = [&](const code& ec, const channel::ptr& channel) NOEXCEPT
    {
        promise_handler.set_value({ ec, channel });
        return false;
    };

    std::promise<std::pair<code, net::object_key>> promise_complete{};
    const auto complete = [&](const code& ec, net::object_key key) NOEXCEPT
    {
        // First key is ++0;
        promise_complete.set_value({ ec, key });
    };

    net.subscribe_connect(handler, complete);
    const auto result1 = promise_complete.get_future().get();
    BOOST_REQUIRE_EQUAL(result1.first, error::success);
    BOOST_REQUIRE_EQUAL(result1.second, one);

    // Close (or ~net) required to clear subscription.
    net.close();
    const auto result2 = promise_handler.get_future().get();
    BOOST_REQUIRE_EQUAL(result2.first, error::service_stopped);
    BOOST_REQUIRE(!result2.second);
}

BOOST_AUTO_TEST_CASE(net__subscribe_close__closed__service_stopped)
{
    const logger log{};
    const settings set(selection::mainnet);
    net net(set, log);
    net.close();

    std::promise<code> promise_handler;
    const auto handler = [&](const code& ec) NOEXCEPT
    {
        promise_handler.set_value(ec);
        return true;
    };

    std::promise<std::pair<code, net::object_key>> promise_complete;
    const auto complete = [&](const code& ec, net::object_key key) NOEXCEPT
    {
        // First key is ++0;
        promise_complete.set_value({ ec, key });
    };

    net.subscribe_close(handler, complete);
    const auto result = promise_complete.get_future().get();
    BOOST_REQUIRE_EQUAL(result.first, error::service_stopped);
    BOOST_REQUIRE_EQUAL(result.second, zero);
    BOOST_REQUIRE_EQUAL(promise_handler.get_future().get(), error::service_stopped);
}

BOOST_AUTO_TEST_CASE(net__subscribe_close__unclosed___success)
{
    const logger log{};
    const settings set(selection::mainnet);
    net net(set, log);

    std::promise<code> promise_handler;
    const auto handler = [&](const code& ec) NOEXCEPT
    {
        promise_handler.set_value(ec);
        return true;
    };

    std::promise<std::pair<code, net::object_key>> promise_complete;
    const auto complete = [&](const code& ec, net::object_key key) NOEXCEPT
    {
        // First key is ++0;
        promise_complete.set_value({ ec, key });
    };

    net.subscribe_close(handler, complete);
    const auto result = promise_complete.get_future().get();
    BOOST_REQUIRE_EQUAL(result.first, error::success);
    BOOST_REQUIRE_EQUAL(result.second, one);

    // Close (or ~net) required to clear subscription.
    net.close();
    BOOST_REQUIRE_EQUAL(promise_handler.get_future().get(), error::service_stopped);
}

BOOST_AUTO_TEST_CASE(net__start__outbound_connections_but_no_peers_no_seeds__seeding_unsuccessful)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound.seeds.clear();
    BOOST_REQUIRE(set.manual.peers.empty());

    net net(set, log);
    BOOST_REQUIRE(net.network_settings().manual.peers.empty());
    BOOST_REQUIRE(net.network_settings().outbound.seeds.empty());

    std::promise<code> promise;
    const auto handler = [&](const code& ec) NOEXCEPT
    {
        promise.set_value(ec);
    };

    net.start(handler);
    BOOST_REQUIRE_EQUAL(promise.get_future().get(), error::seeding_unsuccessful);
}

BOOST_AUTO_TEST_CASE(net__run__closed__service_stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    net net(set, log);
    net.close();

    std::promise<code> promise;
    const auto handler = [&](const code& ec) NOEXCEPT
    {
        promise.set_value(ec);
    };

    net.run(handler);
    BOOST_REQUIRE_EQUAL(promise.get_future().get(), error::service_stopped);
}

BOOST_AUTO_TEST_CASE(net__run__started_no_outbound_connections__success)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound.connections = 0;
    set.outbound.seeds.clear();
    BOOST_REQUIRE(set.manual.peers.empty());

    net net(set, log);
    BOOST_REQUIRE(net.network_settings().manual.peers.empty());
    BOOST_REQUIRE(net.network_settings().outbound.seeds.empty());

    std::promise<code> promise_run;
    const auto run_handler = [&](const code& ec) NOEXCEPT
    {
        promise_run.set_value(ec);
    };

    std::promise<code> promise_start;
    const auto start_handler = [&](const code& ec) NOEXCEPT
    {
        promise_start.set_value(ec);
        net.run(run_handler);
    };

    net.start(start_handler);
    BOOST_REQUIRE_EQUAL(promise_start.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(promise_run.get_future().get(), error::success);
}

class mock_settings final
  : public settings
{
public:
    using settings::settings;

    // Override derivative name, using directory as file.
    std::filesystem::path file() const NOEXCEPT override
    {
        return path;
    }
};

BOOST_AUTO_TEST_CASE(net__run__started_no_peers_no_seeds_one_connection_one_batch__success)
{
    const logger log{};
    mock_settings set(selection::mainnet);
    BOOST_REQUIRE(set.manual.peers.empty());

    // This implies seeding would be required.
    set.outbound.host_pool_capacity = 1;

    // There are no seeds, so seeding would fail.
    set.outbound.seeds.clear();

    // Cache one address to preclude seeding.
    set.path = TEST_NAME;
    system::ofstream file(set.file());
    file << config::authority{ "1.2.3.4:42" } << std::endl;

    // Configure one connection with one batch.
    set.outbound.connect_batch_size = 1;
    set.outbound.connections = 1;

    net net(set, log);

    std::promise<code> promise_run;
    const auto run_handler = [&](const code& ec) NOEXCEPT
    {
        promise_run.set_value(ec);
    };

    std::promise<code> promise_start;
    const auto start_handler = [&](const code& ec) NOEXCEPT
    {
        promise_start.set_value(ec);
        net.run(run_handler);
    };

    net.start(start_handler);
    BOOST_REQUIRE_EQUAL(promise_start.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(promise_run.get_future().get(), error::success);
}

// start

BOOST_AUTO_TEST_CASE(net__start__success_success__success)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_net_session_start<error::success, error::success> net(set, log);

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec) NOEXCEPT
    {
        start.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::success);
}

BOOST_AUTO_TEST_CASE(net__start__unknown_success__unknown)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_net_session_start<error::unknown, error::success> net(set, log);

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec) NOEXCEPT
    {
        start.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::unknown);
}

BOOST_AUTO_TEST_CASE(net__start__success_unknown__unknown)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_net_session_start<error::success, error::unknown> net(set, log);

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec) NOEXCEPT
    {
        start.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::unknown);
}

BOOST_AUTO_TEST_CASE(net__start__unknown_unknown__unknown)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_net_session_start<error::unknown, error::unknown> net(set, log);

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec) NOEXCEPT
    {
        start.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::unknown);
}

BOOST_AUTO_TEST_CASE(net__start__file_load_success_success__file_load)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_net_session_start<error::success, error::success> net(set, log, error::file_load);

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec) NOEXCEPT
    {
        start.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::file_load);
}

BOOST_AUTO_TEST_CASE(net__start__file_load_unknown_success__unknown)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_net_session_start<error::unknown, error::success> net(set, log, error::file_load);

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec) NOEXCEPT
    {
        start.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::unknown);
}

BOOST_AUTO_TEST_CASE(net__start__file_load_success_unknown__file_load)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_net_session_start<error::success, error::unknown> net(set, log, error::file_load);

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec) NOEXCEPT
    {
        start.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::file_load);
}

BOOST_AUTO_TEST_CASE(net__start__file_load_unknown_unknown__unknown)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_net_session_start<error::unknown, error::unknown> net(set, log, error::file_load);

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec) NOEXCEPT
    {
        start.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::unknown);
}

// run

BOOST_AUTO_TEST_CASE(net__run__stopped_success_success__service_stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_net_session_run<error::success, error::success> net(set, log, false);

    std::promise<code> run;
    net.run([&](const code& ec) NOEXCEPT
    {
        run.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::service_stopped);
}

BOOST_AUTO_TEST_CASE(net__run__started_success_success__success)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_net_session_run<error::success, error::success> net(set, log, true);

    std::promise<code> run;
    net.run([&](const code& ec) NOEXCEPT
    {
        run.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::success);
}

BOOST_AUTO_TEST_CASE(net__run__stopped_unknown_success__service_stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_net_session_run<error::unknown, error::success> net(set, log, false);

    std::promise<code> run;
    net.run([&](const code& ec) NOEXCEPT
    {
        run.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::service_stopped);
}

BOOST_AUTO_TEST_CASE(net__run__started_unknown_success__unknown)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_net_session_run<error::unknown, error::success> net(set, log, true);

    std::promise<code> run;
    net.run([&](const code& ec) NOEXCEPT
    {
        run.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::unknown);
}

BOOST_AUTO_TEST_CASE(net__run__stopped_success_unknown__service_stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_net_session_run<error::success, error::unknown> net(set, log, false);

    std::promise<code> run;
    net.run([&](const code& ec) NOEXCEPT
    {
        run.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::service_stopped);
}

BOOST_AUTO_TEST_CASE(net__run__started_success_unknown__unknown)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_net_session_run<error::success, error::unknown> net(set, log, true);

    std::promise<code> run;
    net.run([&](const code& ec) NOEXCEPT
    {
        run.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::unknown);
}

BOOST_AUTO_TEST_CASE(net__run__stopped_unknown_unknown__service_stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_net_session_run<error::unknown, error::unknown> net(set, log, false);

    std::promise<code> run;
    net.run([&](const code& ec) NOEXCEPT
    {
        run.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::service_stopped);
}

BOOST_AUTO_TEST_CASE(net__run__started_unknown_unknown__unknown)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_net_session_run<error::unknown, error::unknown> net(set, log, true);

    std::promise<code> run;
    net.run([&](const code& ec) NOEXCEPT
    {
        run.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::unknown);
}

// protected

class net_accessor
  : public net
{
public:
    using net::net;
    using net::create_service;
    using net::create_acceptor_sam;
    using net::to_connector;
    using net::fetch;
    using net::save;
    using net::store_nonce;
    using net::unstore_nonce;
    using net::count_channel;
    using net::uncount_channel;

    void subscribe_close_stranded(stop_handler&& handler) NOEXCEPT
    {
        net::subscribe_close(std::move(handler));
    }
};

class closed_net_accessor
  : public net_accessor
{
public:
    using net_accessor::net_accessor;
    using net::do_run;

    bool closed() const NOEXCEPT override
    {
        return true;
    }
};

template <typename Function>
static auto on_strand(net& instance, Function&& function)
{
    std::promise<decltype(function())> promise{};
    boost::asio::post(instance.strand(), [&]() NOEXCEPT
    {
        promise.set_value(function());
    });

    return promise.get_future().get();
}

static const settings::tcp_server peer_options{ "test" };

static channel_peer::ptr make_inbound(net& instance, const logger& log)
{
    const network::socket::parameters params{ .maximum_request = 42, .maximum_buffer = peer_options.maximum_buffer };
    const auto socket = std::make_shared<network::socket>(log, instance.service(), params);
    return std::make_shared<channel_peer>(log, socket, 1, instance.network_settings(), peer_options);
}

static channel_peer::ptr make_outbound(net& instance, const logger& log)
{
    const network::socket::parameters params{ .maximum_request = 42, .maximum_buffer = peer_options.maximum_buffer };
    const config::endpoint endpoint{ "1.2.3.4", 42 };
    const auto socket = std::make_shared<network::socket>(log, instance.service(), params, config::address{}, endpoint, false);
    return std::make_shared<channel_peer>(log, socket, 2, instance.network_settings(), peer_options);
}

BOOST_AUTO_TEST_CASE(net__suspend__resume__expected)
{
    const logger log{};
    const settings set(selection::mainnet);
    net net(set, log);
    BOOST_REQUIRE(!net.suspended());

    net.suspend(error::unknown);
    BOOST_REQUIRE(net.suspended());

    BOOST_REQUIRE(net.resume());
    BOOST_REQUIRE(!net.suspended());
}

BOOST_AUTO_TEST_CASE(net__reserved_count__unstarted__zero)
{
    const logger log{};
    const settings set(selection::mainnet);
    net net(set, log);
    BOOST_REQUIRE_EQUAL(net.reserved_count(), 0u);
}

BOOST_AUTO_TEST_CASE(net__fetch_totals__unstarted__success_empty)
{
    const logger log{};
    const settings set(selection::mainnet);
    net net(set, log);

    std::promise<std::pair<code, net::totals>> promise{};
    net.fetch_totals([&](const code& ec, const net::totals& totals) NOEXCEPT
    {
        promise.set_value({ ec, totals });
    });

    const auto result = promise.get_future().get();
    BOOST_REQUIRE_EQUAL(result.first, error::success);
    BOOST_REQUIRE_EQUAL(result.second.sent, 0u);
    BOOST_REQUIRE_EQUAL(result.second.received, 0u);
    BOOST_REQUIRE(result.second.actives.empty());
}

BOOST_AUTO_TEST_CASE(net__fetch_totals__closed__service_stopped)
{
    const logger log{};
    const settings set(selection::mainnet);
    net net(set, log);
    net.close();

    std::promise<code> promise{};
    net.fetch_totals([&](const code& ec, const net::totals&) NOEXCEPT
    {
        promise.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(promise.get_future().get(), error::service_stopped);
}

BOOST_AUTO_TEST_CASE(net__unsubscribe_connect__subscribed__desubscribed)
{
    const logger log{};
    const settings set(selection::mainnet);
    net net(set, log);

    std::promise<code> promise_handler{};
    const auto handler = [&](const code& ec, const channel::ptr&) NOEXCEPT
    {
        promise_handler.set_value(ec);
        return false;
    };

    std::promise<net::object_key> promise_complete{};
    const auto complete = [&](const code&, net::object_key key) NOEXCEPT
    {
        promise_complete.set_value(key);
    };

    net.subscribe_connect(handler, complete);
    const auto key = promise_complete.get_future().get();
    BOOST_REQUIRE_EQUAL(on_strand(net, [&]() NOEXCEPT { return net.connect_subscriber_count(); }), 1u);

    net.unsubscribe_connect(key);
    BOOST_REQUIRE_EQUAL(promise_handler.get_future().get(), error::desubscribed);
    BOOST_REQUIRE_EQUAL(on_strand(net, [&]() NOEXCEPT { return net.connect_subscriber_count(); }), 0u);
    BOOST_REQUIRE_EQUAL(on_strand(net, [&]() NOEXCEPT { return net.stop_subscriber_count(); }), 0u);
}

BOOST_AUTO_TEST_CASE(net__create_service__always__acceptor)
{
    const logger log{};
    const settings set(selection::mainnet);
    net_accessor net(set, log);
    BOOST_REQUIRE(net.create_service({ .maximum_request = 42 }));
}

BOOST_AUTO_TEST_CASE(net__create_acceptor_sam__privacy__acceptor_sam)
{
    const logger log{};
    const settings set(selection::mainnet);
    net_accessor net(set, log, service::node_none, service::node_encrypted_transport);
    BOOST_REQUIRE(net.create_acceptor_sam());
}

BOOST_AUTO_TEST_CASE(net__to_connector__proxied_privacy__connector_socks)
{
    const logger log{};
    const settings set(selection::mainnet);
    net_accessor net(set, log, service::node_none, service::node_encrypted_transport);
    settings::socks5 socks{};
    socks.socks = { "127.0.0.1:65140" };
    const auto instance = net.to_connector(socks, set.outbound, seconds(1), 0);
    BOOST_REQUIRE(std::dynamic_pointer_cast<connector_socks>(instance));
}

BOOST_AUTO_TEST_CASE(net__to_connector__unproxied__connector)
{
    const logger log{};
    const settings set(selection::mainnet);
    net_accessor net(set, log);
    const auto instance = net.to_connector(settings::socks5{}, set.outbound, seconds(1), 0);
    BOOST_REQUIRE(instance);
    BOOST_REQUIRE(!std::dynamic_pointer_cast<connector_socks>(instance));
}

BOOST_AUTO_TEST_CASE(net__save__unstarted__service_stopped)
{
    const logger log{};
    const settings set(selection::mainnet);
    net_accessor net(set, log);

    std::promise<code> promise{};
    net.save(system::to_shared(address{}), [&](const code& ec, size_t) NOEXCEPT
    {
        promise.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(promise.get_future().get(), error::service_stopped);
}

BOOST_AUTO_TEST_CASE(net__save__closed__service_stopped)
{
    const logger log{};
    const settings set(selection::mainnet);
    closed_net_accessor net(set, log);

    std::promise<code> promise{};
    net.save(system::to_shared(address{}), [&](const code& ec, size_t) NOEXCEPT
    {
        promise.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(promise.get_future().get(), error::service_stopped);
}

BOOST_AUTO_TEST_CASE(net__fetch__closed__service_stopped)
{
    const logger log{};
    const settings set(selection::mainnet);
    closed_net_accessor net(set, log);

    std::promise<code> promise{};
    net.fetch([&](const code& ec, const address_cptr&) NOEXCEPT
    {
        promise.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(promise.get_future().get(), error::service_stopped);
}

BOOST_AUTO_TEST_CASE(net__do_run__closed__service_stopped)
{
    const logger log{};
    const settings set(selection::mainnet);
    closed_net_accessor net(set, log);

    std::promise<code> promise{};
    boost::asio::post(net.strand(), [&]() NOEXCEPT
    {
        net.do_run([&](const code& ec) NOEXCEPT
        {
            promise.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(promise.get_future().get(), error::service_stopped);
}

BOOST_AUTO_TEST_CASE(net__store_nonce__outbound__stored_once)
{
    const logger log{};
    const settings set(selection::mainnet);
    net_accessor net(set, log);
    const auto channel = make_outbound(net, log);

    BOOST_REQUIRE(on_strand(net, [&]() NOEXCEPT { return net.store_nonce(*channel); }));
    BOOST_REQUIRE(!on_strand(net, [&]() NOEXCEPT { return net.store_nonce(*channel); }));
    BOOST_REQUIRE_EQUAL(on_strand(net, [&]() NOEXCEPT { return net.nonces_count(); }), 1u);
    BOOST_REQUIRE(on_strand(net, [&]() NOEXCEPT { return net.unstore_nonce(*channel); }));
    BOOST_REQUIRE(!on_strand(net, [&]() NOEXCEPT { return net.unstore_nonce(*channel); }));
    BOOST_REQUIRE_EQUAL(on_strand(net, [&]() NOEXCEPT { return net.nonces_count(); }), 0u);
    channel->stop(error::service_stopped);
}

BOOST_AUTO_TEST_CASE(net__count_channel__outbound_duplicate__address_in_use)
{
    const logger log{};
    const settings set(selection::mainnet);
    net_accessor net(set, log);
    const auto channel = make_outbound(net, log);

    BOOST_REQUIRE_EQUAL(on_strand(net, [&]() NOEXCEPT { return net.count_channel(*channel); }), error::success);
    BOOST_REQUIRE_EQUAL(net.channel_count(), 1u);
    BOOST_REQUIRE_EQUAL(net.reserved_count(), 1u);
    BOOST_REQUIRE_EQUAL(on_strand(net, [&]() NOEXCEPT { return net.count_channel(*channel); }), error::address_in_use);
    BOOST_REQUIRE_EQUAL(net.channel_count(), 1u);

    on_strand(net, [&]() NOEXCEPT { net.uncount_channel(*channel); return true; });
    BOOST_REQUIRE_EQUAL(net.channel_count(), 0u);
    BOOST_REQUIRE_EQUAL(net.reserved_count(), 0u);
    channel->stop(error::service_stopped);
}

BOOST_AUTO_TEST_CASE(net__count_channel__inbound_own_nonce__accept_failed)
{
    const logger log{};
    const settings set(selection::mainnet);
    net_accessor net(set, log);
    const auto outbound = make_outbound(net, log);
    const auto inbound = make_inbound(net, log);

    const auto version = std::make_shared<messages::peer::version>();
    version->nonce = outbound->nonce();
    std::promise<bool> promise{};
    boost::asio::post(inbound->strand(), [&]() NOEXCEPT
    {
        inbound->set_peer_version(version);
        promise.set_value(true);
    });

    BOOST_REQUIRE(promise.get_future().get());
    BOOST_REQUIRE(on_strand(net, [&]() NOEXCEPT { return net.store_nonce(*outbound); }));
    BOOST_REQUIRE_EQUAL(on_strand(net, [&]() NOEXCEPT { return net.count_channel(*inbound); }), error::accept_failed);
    BOOST_REQUIRE_EQUAL(net.inbound_channel_count(), 0u);
    outbound->stop(error::service_stopped);
    inbound->stop(error::service_stopped);
}

BOOST_AUTO_TEST_CASE(net__connect_handled__closed__service_stopped)
{
    const logger log{};
    const settings set(selection::mainnet);
    net net(set, log);
    net.close();

    code result{};
    net.connect({ "truckers.ca", 42 }, [&](const code& ec, const channel::ptr&) NOEXCEPT
    {
        result = ec;
        return false;
    });

    BOOST_REQUIRE_EQUAL(result, error::service_stopped);
}

BOOST_AUTO_TEST_CASE(net__subscribe_close__stranded__subscribed_then_stopped)
{
    const logger log{};
    const settings set(selection::mainnet);
    net_accessor net(set, log);

    std::promise<code> promise{};
    on_strand(net, [&]() NOEXCEPT
    {
        net.subscribe_close_stranded([&](const code& ec) NOEXCEPT
        {
            promise.set_value(ec);
            return false;
        });

        return true;
    });

    BOOST_REQUIRE_EQUAL(on_strand(net, [&]() NOEXCEPT { return net.stop_subscriber_count(); }), 1u);
    net.close();
    BOOST_REQUIRE_EQUAL(promise.get_future().get(), error::service_stopped);
}

BOOST_AUTO_TEST_SUITE_END()
