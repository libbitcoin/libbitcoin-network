/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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
#include <iostream>

struct p2p_tests_setup_fixture
{
    p2p_tests_setup_fixture()
    {
        test::remove(TEST_NAME);
    }

    ~p2p_tests_setup_fixture()
    {
        test::remove(TEST_NAME);
    }
};

BOOST_FIXTURE_TEST_SUITE(p2p_tests, p2p_tests_setup_fixture)

using namespace bc::system::chain;
using namespace bc::network::messages;

template <error::error_t ManualCode = error::success,
    error::error_t SeedCode = error::success>
class mock_p2p_session_start
  : public p2p
{
public:
    mock_p2p_session_start(const settings& settings, const logger& log,
        const code& hosts_start=error::success) NOEXCEPT
      : p2p(settings, log), hosts_start_(hosts_start)
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
class mock_p2p_session_run
  : public p2p
{
public:
    mock_p2p_session_run(const settings& settings, const logger& log,
        bool started) NOEXCEPT
      : p2p(settings, log), closed_(!started)
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

BOOST_AUTO_TEST_CASE(p2p__network_settings__unstarted__expected)
{
    const logger log{};
    settings set(selection::mainnet);
    BOOST_REQUIRE_EQUAL(set.threads, 1u);

    p2p net(set, log);
    BOOST_REQUIRE_EQUAL(net.network_settings().threads, 1u);
}

BOOST_AUTO_TEST_CASE(p2p__address_count__unstarted__zero)
{
    const logger log{};
    const settings set(selection::mainnet);
    p2p net(set, log);
    BOOST_REQUIRE_EQUAL(net.address_count(), 0u);
}

BOOST_AUTO_TEST_CASE(p2p__channel_count__unstarted__zero)
{
    const logger log{};
    const settings set(selection::mainnet);
    p2p net(set, log);
    BOOST_REQUIRE_EQUAL(net.channel_count(), 0u);
}

BOOST_AUTO_TEST_CASE(p2p__connect__unstarted__service_stopped)
{
    const logger log{};
    const settings set(selection::mainnet);
    p2p net(set, log);

    std::promise<bool> promise;
    const auto handler = [&](const code& ec, const channel::ptr& channel)
    {
        BOOST_REQUIRE(!channel);
        BOOST_REQUIRE_EQUAL(ec, error::service_stopped);
        promise.set_value(true);
        return true;
    };

    net.connect({ "truckers.ca" });
    net.connect({ "truckers.ca", 42 });
    net.connect({ "truckers.ca", 42 }, handler);
    BOOST_REQUIRE(promise.get_future().get());
}

BOOST_AUTO_TEST_CASE(p2p__subscribe_connect__unstopped__success)
{
    const logger log{};
    const settings set(selection::mainnet);
    p2p net(set, log);

    std::promise<bool> promise_handler;
    const auto handler = [&](const code& ec, const channel::ptr& channel)
    {
        BOOST_REQUIRE(!channel);
        BOOST_REQUIRE_EQUAL(ec, error::service_stopped);
        promise_handler.set_value(true);
        return false;
    };

    std::promise<bool> promise_complete;
    const auto complete = [&](const code& ec, size_t key)
    {
        // First key is ++0;
        BOOST_REQUIRE_EQUAL(key, one);
        BOOST_REQUIRE_EQUAL(ec, error::success);
        promise_complete.set_value(true);
    };

    net.subscribe_connect(handler, complete);
    BOOST_REQUIRE(promise_complete.get_future().get());

    // Close (or ~p2p) required to clear subscription.
    net.close();
    BOOST_REQUIRE(promise_handler.get_future().get());
}

BOOST_AUTO_TEST_CASE(p2p__subscribe_close__unstopped__success)
{
    const logger log{};
    const settings set(selection::mainnet);
    p2p net(set, log);

    std::promise<bool> promise_handler;
    const auto handler = [&](const code& ec)
    {
        BOOST_REQUIRE_EQUAL(ec, error::service_stopped);
        promise_handler.set_value(true);
        return true;
    };

    std::promise<bool> promise_complete;
    const auto complete = [&](const code& ec, size_t key)
    {
        // First key is ++0;
        BOOST_REQUIRE_EQUAL(key, one);
        BOOST_REQUIRE_EQUAL(ec, error::success);
        promise_complete.set_value(true);
    };

    net.subscribe_close(handler, complete);
    BOOST_REQUIRE(promise_complete.get_future().get());

    // Close (or ~p2p) required to clear subscription.
    net.close();
    BOOST_REQUIRE(promise_handler.get_future().get());
}

BOOST_AUTO_TEST_CASE(p2p__start__outbound_connections_but_no_peers_no_seeds__seeding_unsuccessful)
{
    const logger log{};
    settings set(selection::mainnet);
    set.seeds.clear();
    BOOST_REQUIRE(set.peers.empty());

    p2p net(set, log);
    BOOST_REQUIRE(net.network_settings().peers.empty());
    BOOST_REQUIRE(net.network_settings().seeds.empty());

    std::promise<bool> promise;
    const auto handler = [&](const code& ec)
    {
        BOOST_REQUIRE_EQUAL(ec, error::seeding_unsuccessful);
        promise.set_value(true);
    };

    net.start(handler);
    BOOST_REQUIRE(promise.get_future().get());
}

BOOST_AUTO_TEST_CASE(p2p__run__not_started__service_stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    p2p net(set, log);

    std::promise<bool> promise;
    const auto handler = [&](const code& ec)
    {
        BOOST_REQUIRE_EQUAL(ec, error::service_stopped);
        promise.set_value(true);
    };

    net.run(handler);
    BOOST_REQUIRE(promise.get_future().get());
}

BOOST_AUTO_TEST_CASE(p2p__run__started_no_outbound_connections__success)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound_connections = 0;
    set.seeds.clear();
    BOOST_REQUIRE(set.peers.empty());

    p2p net(set, log);
    BOOST_REQUIRE(net.network_settings().peers.empty());
    BOOST_REQUIRE(net.network_settings().seeds.empty());

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

BOOST_AUTO_TEST_CASE(p2p__run__started_no_peers_no_seeds_one_connection_one_batch__success)
{
    const logger log{};
    mock_settings set(selection::mainnet);
    BOOST_REQUIRE(set.peers.empty());

    // This implies seeding would be required.
    set.host_pool_capacity = 1;

    // There are no seeds, so seeding would fail.
    set.seeds.clear();

    // Cache one address to preclude seeding.
    set.path = TEST_NAME;
    system::ofstream file(set.file());
    file << config::authority{ "1.2.3.4:42" } << std::endl;

    // Configure one connection with one batch.
    set.connect_batch_size = 1;
    set.outbound_connections = 1;

    p2p net(set, log);

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

// start

BOOST_AUTO_TEST_CASE(p2p__start__success_success__success)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p_session_start<error::success, error::success> net(set, log);

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec)
    {
        start.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::success);
}

BOOST_AUTO_TEST_CASE(p2p__start__unknown_success__unknown)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p_session_start<error::unknown, error::success> net(set, log);

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec)
    {
        start.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::unknown);
}

BOOST_AUTO_TEST_CASE(p2p__start__success_unknown__unknown)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p_session_start<error::success, error::unknown> net(set, log);

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec)
    {
        start.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::unknown);
}

BOOST_AUTO_TEST_CASE(p2p__start__unknown_unknown__unknown)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p_session_start<error::unknown, error::unknown> net(set, log);

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec)
    {
        start.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::unknown);
}

// manual does not bypass
BOOST_AUTO_TEST_CASE(p2p__start__bypassed_success__bypassed)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p_session_start<error::bypassed, error::success> net(set, log);

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec)
    {
        start.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::bypassed);
}

BOOST_AUTO_TEST_CASE(p2p__start__success_bypassed__success)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p_session_start<error::success, error::bypassed> net(set, log);

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec)
    {
        start.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::success);
}

BOOST_AUTO_TEST_CASE(p2p__start__unknown_bypassed__unknown)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p_session_start<error::unknown, error::bypassed> net(set, log);

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec)
    {
        start.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::unknown);
}

// manual does not bypass
BOOST_AUTO_TEST_CASE(p2p__start__bypassed_bypassed__bypassed)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p_session_start<error::bypassed, error::bypassed> net(set, log);

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec)
    {
        start.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::bypassed);
}

BOOST_AUTO_TEST_CASE(p2p__start__file_load_success_success__file_load)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p_session_start<error::success, error::success> net(set, log, error::file_load);

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec)
    {
        start.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::file_load);
}

BOOST_AUTO_TEST_CASE(p2p__start__file_load_unknown_success__unknown)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p_session_start<error::unknown, error::success> net(set, log, error::file_load);

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec)
    {
        start.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::unknown);
}

BOOST_AUTO_TEST_CASE(p2p__start__file_load_success_unknown__file_load)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p_session_start<error::success, error::unknown> net(set, log, error::file_load);

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec)
    {
        start.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::file_load);
}

BOOST_AUTO_TEST_CASE(p2p__start__file_load_unknown_unknown__unknown)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p_session_start<error::unknown, error::unknown> net(set, log, error::file_load);

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec)
    {
        start.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::unknown);
}

// run

BOOST_AUTO_TEST_CASE(p2p__run__stopped_success_success__service_stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p_session_run<error::success, error::success> net(set, log, false);

    std::promise<code> run;
    net.run([&](const code& ec)
    {
        run.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::service_stopped);
}

BOOST_AUTO_TEST_CASE(p2p__run__started_success_success__success)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p_session_run<error::success, error::success> net(set, log, true);

    std::promise<code> run;
    net.run([&](const code& ec)
    {
        run.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::success);
}

BOOST_AUTO_TEST_CASE(p2p__run__stopped_unknown_success__service_stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p_session_run<error::unknown, error::success> net(set, log, false);

    std::promise<code> run;
    net.run([&](const code& ec)
    {
        run.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::service_stopped);
}

BOOST_AUTO_TEST_CASE(p2p__run__started_unknown_success__unknown)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p_session_run<error::unknown, error::success> net(set, log, true);

    std::promise<code> run;
    net.run([&](const code& ec)
    {
        run.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::unknown);
}

BOOST_AUTO_TEST_CASE(p2p__run__stopped_success_unknown__service_stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p_session_run<error::success, error::unknown> net(set, log, false);

    std::promise<code> run;
    net.run([&](const code& ec)
    {
        run.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::service_stopped);
}

BOOST_AUTO_TEST_CASE(p2p__run__started_success_unknown__unknown)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p_session_run<error::success, error::unknown> net(set, log, true);

    std::promise<code> run;
    net.run([&](const code& ec)
    {
        run.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::unknown);
}

BOOST_AUTO_TEST_CASE(p2p__run__stopped_unknown_unknown__service_stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p_session_run<error::unknown, error::unknown> net(set, log, false);

    std::promise<code> run;
    net.run([&](const code& ec)
    {
        run.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::service_stopped);
}

BOOST_AUTO_TEST_CASE(p2p__run__started_unknown_unknown__unknown)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p_session_run<error::unknown, error::unknown> net(set, log, true);

    std::promise<code> run;
    net.run([&](const code& ec)
    {
        run.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::unknown);
}

BOOST_AUTO_TEST_CASE(p2p__run__stopped_bypassed_success__service_stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p_session_run<error::bypassed, error::success> net(set, log, false);

    std::promise<code> run;
    net.run([&](const code& ec)
    {
        run.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::service_stopped);
}

BOOST_AUTO_TEST_CASE(p2p__run__started_bypassed_success__success)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p_session_run<error::bypassed, error::success> net(set, log, true);

    std::promise<code> run;
    net.run([&](const code& ec)
    {
        run.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::success);
}

BOOST_AUTO_TEST_CASE(p2p__run__stopped_success_bypassed__service_stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p_session_run<error::success, error::bypassed> net(set, log, false);

    std::promise<code> run;
    net.run([&](const code& ec)
    {
        run.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::service_stopped);
}

BOOST_AUTO_TEST_CASE(p2p__run__started_success_bypassed__success)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p_session_run<error::success, error::bypassed> net(set, log, true);

    std::promise<code> run;
    net.run([&](const code& ec)
    {
        run.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::success);
}

BOOST_AUTO_TEST_CASE(p2p__run__stopped_bypassed_bypassed__service_stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p_session_run<error::bypassed, error::bypassed> net(set, log, false);

    std::promise<code> run;
    net.run([&](const code& ec)
    {
        run.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::service_stopped);
}

BOOST_AUTO_TEST_CASE(p2p__run__started_bypassed_bypassed__success)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p_session_run<error::bypassed, error::bypassed> net(set, log, true);

    std::promise<code> run;
    net.run([&](const code& ec)
    {
        run.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::success);
}

BOOST_AUTO_TEST_SUITE_END()
