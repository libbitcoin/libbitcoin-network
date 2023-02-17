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

BOOST_AUTO_TEST_SUITE(session_tests)

using namespace bc::network::messages;
using namespace bc::system::chain;

class mock_channel
  : public channel
{
public:
    using channel::channel;

    void resume() NOEXCEPT override
    {
        if (resumed_)
            reresumed_ = true;
        else
            resumed_ = true;

        channel::resume();
    }

    bool resumed() const NOEXCEPT
    {
        return resumed_;
    }

    bool reresumed() const NOEXCEPT
    {
        return reresumed_;
    }

    void stop(const code& ec) NOEXCEPT override
    {
        stop_code_ = ec;
        channel::stop(ec);
    }

    void stopper(const code& ec) NOEXCEPT
    {
        channel::stop(ec);
    }

    code stop_code() const NOEXCEPT
    {
        return stop_code_;
    }

protected:
    bool paused_{ false };
    bool resumed_{ false };
    bool reresumed_{ false };
    code stop_code_{ error::success };
};

class mock_channel_no_read
  : public mock_channel
{
public:
    using mock_channel::mock_channel;

    void resume() NOEXCEPT override
    {
        if (resumed_)
            reresumed_ = true;
        else
            resumed_ = true;

        ////channel::resume();
    }
};

class mock_session
  : public session
{
public:
    mock_session(p2p& network, size_t key, bool inbound, bool notify) NOEXCEPT
      : session(network, key), inbound_(inbound), notify_(notify)
    {
    }

    virtual ~mock_session() NOEXCEPT
    {
    }

    bool stopped() const NOEXCEPT override
    {
        return session::stopped();
    }

    bool stranded() const NOEXCEPT override
    {
        return session::stranded();
    }

    acceptor::ptr create_acceptor() NOEXCEPT override
    {
        return session::create_acceptor();
    }

    connector::ptr create_connector() NOEXCEPT override
    {
        return session::create_connector();
    }

    connectors_ptr create_connectors(size_t count) NOEXCEPT override
    {
        return session::create_connectors(count);
    }

    size_t address_count() const NOEXCEPT override
    {
        return session::address_count();
    }

    size_t channel_count() const NOEXCEPT override
    {
        return session::channel_count();
    }

    size_t inbound_channel_count() const NOEXCEPT override
    {
        return session::inbound_channel_count();
    }

    bool disabled(const config::address& address) const NOEXCEPT override
    {
        return session::disabled(address);
    }

    bool insufficient(const config::address& address) const NOEXCEPT override
    {
        return session::insufficient(address);
    }

    bool unsupported(const config::address& address) const NOEXCEPT override
    {
        return session::unsupported(address);
    }

    bool whitelisted(const config::authority& authority) const NOEXCEPT override
    {
        return session::whitelisted(authority);
    }

    bool blacklisted(const config::authority& authority) const NOEXCEPT override
    {
        return session::blacklisted(authority);
    }

    bool inbound() const NOEXCEPT override
    {
        return inbound_;
    }

    bool notify() const NOEXCEPT override
    {
        return notify_;
    }

    void start_channel(const channel::ptr& channel, result_handler&& started,
        result_handler&& stopped) NOEXCEPT override
    {
        session::start_channel(channel, std::move(started), std::move(stopped));
    }

    void attach_handshake(const channel::ptr& channel,
        result_handler&& handshake) const NOEXCEPT override
    {
        if (!handshaked_)
        {
            handshaked_ = true;
        }

        // Simulate handshake completion.
        handshake(channel->stopped() ? error::channel_stopped : error::success);
    }

    bool attached_handshake() const NOEXCEPT
    {
        return handshaked_;
    }

    void attach_protocols(const channel::ptr&) const NOEXCEPT override
    {
        if (!protocoled_)
        {
            protocoled_ = true;
            require_protocoled_.set_value(true);
        }
    }

    bool attached_protocol() const NOEXCEPT
    {
        return protocoled_;
    }

    bool require_attached_protocol() const NOEXCEPT
    {
        return require_protocoled_.get_future().get();
    }

private:
    const bool inbound_;
    const bool notify_;
    mutable bool handshaked_{ false };
    mutable bool protocoled_{ false };
    mutable std::promise<bool> require_protocoled_;
};

class mock_p2p
  : public p2p
{
public:
    using p2p::p2p;

    acceptor::ptr create_acceptor() NOEXCEPT override
    {
        ++acceptors_;
        return p2p::create_acceptor();
    }

    size_t acceptors() const NOEXCEPT
    {
        return acceptors_;
    }

    connector::ptr create_connector() NOEXCEPT override
    {
        ++connectors_;
        return p2p::create_connector();
    }

    size_t connectors() const NOEXCEPT
    {
        return connectors_;
    }

    void take(address_item_handler&& handler) NOEXCEPT override
    {
        handler(error::invalid_magic, {});
    }

    void fetch(address_handler&& handler) const NOEXCEPT override
    {
        handler(error::bad_stream, {});
    }

    void restore(const address_item_cptr& address,
        result_handler&& complete) NOEXCEPT override
    {
        restored_ = *address;
        complete(error::invalid_magic);
    }

    const address_item& restored() const NOEXCEPT
    {
        return restored_;
    }

    void save(const address_cptr& message,
        count_handler&& complete) NOEXCEPT override
    {
        saveds_ = message->addresses;
        complete(error::bad_stream, zero);
    }

    const address_items& saveds() const NOEXCEPT
    {
        return saveds_;
    }

    uint64_t pent_nonce() const NOEXCEPT
    {
        return pend_;
    }

    uint64_t unpent_nonce() const NOEXCEPT
    {
        return unpend_;
    }

    uint64_t stored_nonce() const NOEXCEPT
    {
        return store_nonce_;
    }

    bool stored_inbound() const NOEXCEPT
    {
        return store_inbound_;
    }

    bool stored_notify() const NOEXCEPT
    {
        return store_notify_;
    }

    code stored_result() const NOEXCEPT
    {
        return store_result_;
    }

    uint64_t unstored_nonce() const NOEXCEPT
    {
        return unstore_nonce_;
    }

    bool unstored_inbound() const NOEXCEPT
    {
        return unstore_inbound_;
    }

    code unstore_result() const NOEXCEPT
    {
        return unstore_result_;
    }

    session_seed::ptr attach_seed_session() NOEXCEPT override
    {
        return attach<mock_session_seed>(*this);
    }

protected:
    bool store_nonce(uint64_t nonce) NOEXCEPT override
    {
        BC_ASSERT(!is_zero(nonce));
        pend_ = nonce;
        return p2p::store_nonce(nonce);
    }

    bool unstore_nonce(uint64_t nonce) NOEXCEPT override
    {
        BC_ASSERT(!is_zero(nonce));
        unpend_ = nonce;
        return p2p::unstore_nonce(nonce);
    }

    code store_channel(const channel::ptr& channel, bool notify,
        bool inbound) NOEXCEPT override
    {
        BC_ASSERT(!is_zero(channel->nonce()));
        store_nonce_ = channel->nonce();
        store_notify_ = notify;
        store_inbound_ = inbound;
        return ((store_result_ = p2p::store_channel(channel, notify, inbound)));
    }

    code unstore_channel(const channel::ptr& channel, bool notify,
        bool inbound) NOEXCEPT override
    {
        BC_ASSERT(!is_zero(channel->nonce()));
        unstore_nonce_ = channel->nonce();
        unstore_inbound_ = inbound;
        return ((unstore_result_ = p2p::unstore_channel(channel, notify, inbound)));
    }

private:
    size_t acceptors_{ 0 };
    size_t connectors_{ 0 };
    address_item restored_{};
    address_items saveds_{};

    uint64_t pend_{ 0 };
    uint64_t unpend_{ 0 };

    uint64_t store_nonce_{ 0 };
    bool store_notify_{ false };
    bool store_inbound_{ false };
    code store_result_{ error::success };

    uint64_t unstore_nonce_{ 0 };
    bool unstore_inbound_{ false };
    code unstore_result_{ error::unknown };

    class mock_session_seed
      : public session_seed
    {
    public:
        using session_seed::session_seed;

        void start(result_handler&& handler) NOEXCEPT override
        {
            handler(error::success);
        }
    };
};

// construct/settings

BOOST_AUTO_TEST_CASE(session__construct__always__expected_settings)
{
    const logger log{ false };
    constexpr auto expected = 42u;
    settings set(selection::mainnet);
    set.threads = expected;
    p2p net(set, log);
    mock_session session(net, 1, false, true);
    BOOST_REQUIRE_EQUAL(session.settings().threads, expected);
}

// properties

BOOST_AUTO_TEST_CASE(session__properties__default__expected)
{
    const logger log{ false };
    settings set(selection::mainnet);
    p2p net(set, log);
    mock_session session(net, 1, false, true);
    BOOST_REQUIRE(session.stopped());
    BOOST_REQUIRE(!session.stranded());
    BOOST_REQUIRE(is_zero(session.address_count()));
    BOOST_REQUIRE(is_zero(session.channel_count()));
    BOOST_REQUIRE(is_zero(session.inbound_channel_count()));
    BOOST_REQUIRE(!session.blacklisted({ "[2001:db8::2]:42" }));
    BOOST_REQUIRE(session.whitelisted({ "[2001:db8::2]:42" }));
    BOOST_REQUIRE(!session.inbound());
    BOOST_REQUIRE(session.notify());
}

BOOST_AUTO_TEST_CASE(session__disabled__ipv4__false)
{
    const logger log{ false };
    settings set(selection::mainnet);
    p2p net(set, log);
    const mock_session session(net, 1, false, true);
    set.enable_ipv6 = false;
    BOOST_REQUIRE(!session.disabled({ "42.42.42.42" }));
    BOOST_REQUIRE(!session.disabled({ "42.42.42.42:42" }));
    set.enable_ipv6 = true;
    BOOST_REQUIRE(!session.disabled({ "42.42.42.42" }));
    BOOST_REQUIRE(!session.disabled({ "42.42.42.42:42" }));
}

BOOST_AUTO_TEST_CASE(session__disabled__ipv6__expected)
{
    const logger log{ false };
    settings set(selection::mainnet);
    p2p net(set, log);
    const mock_session session(net, 1, false, true);
    set.enable_ipv6 = false;
    BOOST_REQUIRE(session.disabled({ "[2001:db8::2]" }));
    BOOST_REQUIRE(session.disabled({ "[2001:db8::2]:42" }));
    set.enable_ipv6 = true;
    BOOST_REQUIRE(!session.disabled({ "[2001:db8::2]" }));
    BOOST_REQUIRE(!session.disabled({ "[2001:db8::2]:42" }));
}

BOOST_AUTO_TEST_CASE(session__insufficient__default__false)
{
    const logger log{ false };
    settings set(selection::mainnet);
    p2p net(set, log);
    const mock_session session(net, 1, false, true);
    constexpr uint64_t services = 0;
    constexpr messages::address_item loop{ 42, services, loopback_ip_address, 8333 };
    set.services_minimum = 0;
    BOOST_REQUIRE(!session.insufficient(loop));
    set.services_minimum = 1;
    BOOST_REQUIRE(session.insufficient(loop));
}

BOOST_AUTO_TEST_CASE(session__insufficient__match__expected)
{
    const logger log{ false };
    settings set(selection::mainnet);
    p2p net(set, log);
    const mock_session session(net, 1, false, true);
    constexpr uint64_t services = 0b01010101;
    constexpr messages::address_item loop{ 42, services, loopback_ip_address, 8333 };
    set.services_minimum = services;
    BOOST_REQUIRE(!session.insufficient(loop));
    set.services_minimum = services | 0b00000010;
    BOOST_REQUIRE(session.insufficient(loop));
    set.services_minimum = services & 0b11111110;
    BOOST_REQUIRE(!session.insufficient(loop));
}

BOOST_AUTO_TEST_CASE(session__unsupported__default__false)
{
    const logger log{ false };
    settings set(selection::mainnet);
    p2p net(set, log);
    const mock_session session(net, 1, false, true);
    constexpr uint64_t services = 0;
    constexpr messages::address_item loop{ 42, services, loopback_ip_address, 8333 };
    set.invalid_services = 0;
    BOOST_REQUIRE(!session.unsupported(loop));
    set.invalid_services = 1;
    BOOST_REQUIRE(!session.unsupported(loop));
}

BOOST_AUTO_TEST_CASE(session__unsupported__match__expected)
{
    const logger log{ false };
    settings set(selection::mainnet);
    p2p net(set, log);
    const mock_session session(net, 1, false, true);
    constexpr uint64_t services = 0b01010101;
    constexpr messages::address_item loop{ 42, services, loopback_ip_address, 8333 };
    set.invalid_services = services;
    BOOST_REQUIRE(session.unsupported(loop));
    set.invalid_services = services | 0b00000010;
    BOOST_REQUIRE(session.unsupported(loop));
    set.invalid_services = services & 0b11111110;
    BOOST_REQUIRE(session.unsupported(loop));
    set.invalid_services = 0b10101010;
    BOOST_REQUIRE(!session.unsupported(loop));
    set.invalid_services = 0;
    BOOST_REQUIRE(!session.unsupported(loop));
}

BOOST_AUTO_TEST_CASE(session__whitelisted__ipv4_subnet__expected)
{
    const logger log{ false };
    settings set(selection::mainnet);
    p2p net(set, log);
    const mock_session session(net, 1, false, true);
    set.whitelists.clear();
    BOOST_REQUIRE(session.whitelisted({ "42.42.42.42" }));
    set.whitelists.emplace_back("12.12.12.12");
    set.whitelists.emplace_back("24.24.24.24");
    BOOST_REQUIRE(!session.whitelisted({ "42.42.42.42" }));
    set.whitelists.emplace_back("42.42.42.0/24");
    BOOST_REQUIRE(session.whitelisted({ "42.42.42.42" }));
}

BOOST_AUTO_TEST_CASE(session__whitelisted__ipv4_host__expected)
{
    const logger log{ false };
    settings set(selection::mainnet);
    p2p net(set, log);
    const mock_session session(net, 1, false, true);
    set.whitelists.clear();
    BOOST_REQUIRE(session.whitelisted({ "24.24.24.24" }));
    set.whitelists.emplace_back("12.12.12.12");
    set.whitelists.emplace_back("42.42.42.0/24");
    BOOST_REQUIRE(!session.whitelisted({ "24.24.24.24" }));
    set.whitelists.emplace_back("24.24.24.24");
    BOOST_REQUIRE(session.whitelisted({ "24.24.24.24" }));
}

BOOST_AUTO_TEST_CASE(session__whitelisted__ipv6_subnet__expected)
{
    const logger log{ false };
    settings set(selection::mainnet);
    p2p net(set, log);
    const mock_session session(net, 1, false, true);
    set.whitelists.clear();
    BOOST_REQUIRE(session.whitelisted({ "[2020:db8::3]" }));
    set.whitelists.emplace_back("[2020:db8::1]");
    set.whitelists.emplace_back("[2020:db8::2]");
    BOOST_REQUIRE(!session.whitelisted({ "[2020:db8::3]" }));
    set.whitelists.emplace_back("[2020:db8::2]/64");
    BOOST_REQUIRE(session.whitelisted({ "[2020:db8::3]" }));
}

BOOST_AUTO_TEST_CASE(session__whitelisted__ipv6_host__expected)
{
    const logger log{ false };
    settings set(selection::mainnet);
    p2p net(set, log);
    const mock_session session(net, 1, false, true);
    set.whitelists.clear();
    BOOST_REQUIRE(session.whitelisted({ "[2020:db8::3]" }));
    set.whitelists.emplace_back("[2020:db8::1]");
    set.whitelists.emplace_back("[2020:db8::2]");
    BOOST_REQUIRE(!session.whitelisted({ "[2020:db8::3]" }));
    set.whitelists.emplace_back("[2020:db8::3]");
    BOOST_REQUIRE(session.whitelisted({ "[2020:db8::3]" }));
}

BOOST_AUTO_TEST_CASE(session__blacklisted__ipv4_subnet__expected)
{
    const logger log{ false };
    settings set(selection::mainnet);
    p2p net(set, log);
    const mock_session session(net, 1, false, true);
    set.whitelists.clear();
    BOOST_REQUIRE(!session.blacklisted({ "42.42.42.42" }));
    set.blacklists.emplace_back("12.12.12.12");
    set.blacklists.emplace_back("24.24.24.24");
    BOOST_REQUIRE(!session.blacklisted({ "42.42.42.42" }));
    set.blacklists.emplace_back("42.42.42.0/24");
    BOOST_REQUIRE(session.blacklisted({ "42.42.42.42" }));
}

BOOST_AUTO_TEST_CASE(session__blacklisted__ipv4_host__expected)
{
    const logger log{ false };
    settings set(selection::mainnet);
    p2p net(set, log);
    const mock_session session(net, 1, false, true);
    set.whitelists.clear();
    BOOST_REQUIRE(!session.blacklisted({ "24.24.24.24" }));
    set.blacklists.emplace_back("12.12.12.12");
    set.blacklists.emplace_back("42.42.42.0/24");
    BOOST_REQUIRE(!session.blacklisted({ "24.24.24.24" }));
    set.blacklists.emplace_back("24.24.24.24");
    BOOST_REQUIRE(session.blacklisted({ "24.24.24.24" }));
}

BOOST_AUTO_TEST_CASE(session__blacklisted__ipv6_subnet__expected)
{
    const logger log{ false };
    settings set(selection::mainnet);
    p2p net(set, log);
    const mock_session session(net, 1, false, true);
    set.whitelists.clear();
    BOOST_REQUIRE(!session.blacklisted({ "[2020:db8::3]" }));
    set.blacklists.emplace_back("[2020:db8::1]");
    set.blacklists.emplace_back("[2020:db8::2]");
    BOOST_REQUIRE(!session.blacklisted({ "[2020:db8::3]" }));
    set.blacklists.emplace_back("[2020:db8::2]/64");
    BOOST_REQUIRE(session.blacklisted({ "[2020:db8::3]" }));
}

BOOST_AUTO_TEST_CASE(session__blacklisted__ipv6_host__expected)
{
    const logger log{ false };
    settings set(selection::mainnet);
    p2p net(set, log);
    const mock_session session(net, 1, false, true);
    set.whitelists.clear();
    BOOST_REQUIRE(!session.blacklisted({ "[2020:db8::3]" }));
    set.blacklists.emplace_back("[2020:db8::1]");
    set.blacklists.emplace_back("[2020:db8::2]");
    BOOST_REQUIRE(!session.blacklisted({ "[2020:db8::3]" }));
    set.blacklists.emplace_back("[2020:db8::3]");
    BOOST_REQUIRE(session.blacklisted({ "[2020:db8::3]" }));
}

// factories

BOOST_AUTO_TEST_CASE(session__create_acceptor__always__expected)
{
    const logger log{ false };
    settings set(selection::mainnet);
    mock_p2p net(set, log);
    mock_session session(net, 1, false, true);
    BOOST_REQUIRE(session.create_acceptor());
    BOOST_REQUIRE_EQUAL(net.acceptors(), 1u);
}

BOOST_AUTO_TEST_CASE(session__create_connector__always__expected)
{
    const logger log{ false };
    settings set(selection::mainnet);
    mock_p2p net(set, log);
    mock_session session(net, 1, false, true);
    BOOST_REQUIRE(session.create_connector());
    BOOST_REQUIRE_EQUAL(net.connectors(), 1u);
}

BOOST_AUTO_TEST_CASE(session__create_connectors__always__expected)
{
    const logger log{ false };
    settings set(selection::mainnet);
    mock_p2p net(set, log);
    mock_session session(net, 1, false, true);
    constexpr auto expected = 42u;
    const auto connectors = session.create_connectors(expected);
    BOOST_REQUIRE(connectors);
    BOOST_REQUIRE_EQUAL(connectors->size(), expected);
    BOOST_REQUIRE_EQUAL(net.connectors(), expected);
}

// utilities

BOOST_AUTO_TEST_CASE(session__take__always__calls_network)
{
    const logger log{ false };
    settings set(selection::mainnet);
    mock_p2p net(set, log);
    mock_session session(net, 1, false, true);

    std::promise<code> taken;
    session.take([&](const code& ec, const address_item_cptr&)
    {
        taken.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(taken.get_future().get(), error::invalid_magic);
}

BOOST_AUTO_TEST_CASE(session__fetch__always__calls_network)
{
    const logger log{ false };
    settings set(selection::mainnet);
    mock_p2p net(set, log);
    mock_session session(net, 1, false, true);

    std::promise<code> fetched;
    session.fetch([&](const code& ec, const address_cptr&)
    {
        fetched.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(fetched.get_future().get(), error::bad_stream);
}

BOOST_AUTO_TEST_CASE(session__restore__always__calls_network_with_expected_address)
{
    const logger log{ false };
    settings set(selection::mainnet);
    mock_p2p net(set, log);
    mock_session session(net, 1, false, true);

    std::promise<code> save;
    const address_item item{ 42, 24, unspecified_ip_address, 4224u };
    session.restore(system::to_shared(item), [&](const code& ec)
    {
        save.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(save.get_future().get(), error::invalid_magic);

    const auto& saved = net.restored();
    BOOST_REQUIRE_EQUAL(saved.timestamp, 42u);
    BOOST_REQUIRE_EQUAL(saved.services, 24u);
    BOOST_REQUIRE_EQUAL(saved.ip, unspecified_ip_address);
    BOOST_REQUIRE_EQUAL(saved.port, 4224u);
}

BOOST_AUTO_TEST_CASE(session__save__always__calls_network_with_expected_addresses)
{
    const logger log{ false };
    settings set(selection::mainnet);
    mock_p2p net(set, log);
    mock_session session(net, 1, false, true);

    std::promise<code> save;
    const address_items items{ {}, { 42, 24, unspecified_ip_address, 4224u } };
    session.save(system::to_shared(address{ items }), [&](const code& ec, size_t)
    {
        save.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(save.get_future().get(), error::bad_stream);

    const auto& saveds = net.saveds();
    BOOST_REQUIRE_EQUAL(saveds.size(), 2u);
    BOOST_REQUIRE_EQUAL(saveds[1].timestamp, 42u);
    BOOST_REQUIRE_EQUAL(saveds[1].services, 24u);
    BOOST_REQUIRE_EQUAL(saveds[1].ip, unspecified_ip_address);
    BOOST_REQUIRE_EQUAL(saveds[1].port, 4224u);
}

// stop

BOOST_AUTO_TEST_CASE(session__stop__stopped__true)
{
    const logger log{ false };
    settings set(selection::mainnet);
    mock_p2p net(set, log);
    mock_session session(net, 1, false, true);
    BOOST_REQUIRE(session.stopped());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [&]()
    {
        session.stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
}

// start

BOOST_AUTO_TEST_CASE(session__start__restart__operation_failed)
{
    const logger log{ false };
    settings set(selection::mainnet);
    mock_p2p net(set, log);
    mock_session session(net, 1, false, true);

    std::promise<code> started;
    boost::asio::post(net.strand(), [&]()
    {
        session.start([&](const code& ec)
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);

    std::promise<code> restarted;
    boost::asio::post(net.strand(), [&]()
    {
        session.start([&](const code& ec)
        {
            restarted.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(restarted.get_future().get(), error::operation_failed);

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [&]()
    {
        session.stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session.stopped());
}

BOOST_AUTO_TEST_CASE(session__start__stop__success)
{
    const logger log{ false };
    settings set(selection::mainnet);
    mock_p2p net(set, log);
    mock_session session(net, 1, false, true);

    std::promise<code> started;
    boost::asio::post(net.strand(), [&]()
    {
        session.start([&](const code& ec)
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [&]()
    {
        session.stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session.stopped());
}

// channel sequence

BOOST_AUTO_TEST_CASE(session__start_channel__session_not_started__handlers_service_stopped_channel_service_stopped_not_pent_or_stored)
{
    const logger log{ false };
    settings set(selection::mainnet);
    mock_p2p net(set, log);
    auto session = std::make_shared<mock_session>(net, 1, false, true);
    BOOST_REQUIRE(session->stopped());

    const auto socket = std::make_shared<network::socket>(net.log(), net.service());
    const auto channel = std::make_shared<mock_channel>(net.log(), socket, session->settings(), 42);

    std::promise<code> started_channel;
    std::promise<code> stopped_channel;
    boost::asio::post(net.strand(), [=, &started_channel, &stopped_channel]()
    {
        session->start_channel(channel,
            [&](const code& ec)
            {
                started_channel.set_value(ec);
            },
            [&](const code& ec)
            {
                stopped_channel.set_value(ec);
            });
    });

    // Channel stopped early due to session being stopped (not started).
    BOOST_REQUIRE(!session->attached_handshake());
    BOOST_REQUIRE_EQUAL(started_channel.get_future().get(), error::service_stopped);
    BOOST_REQUIRE(!session->attached_handshake());
    BOOST_REQUIRE(!channel->resumed());
    BOOST_REQUIRE(!session->attached_protocol());
    BOOST_REQUIRE_EQUAL(stopped_channel.get_future().get(), error::service_stopped);
    BOOST_REQUIRE(channel->stopped());
    BOOST_REQUIRE_EQUAL(channel->stop_code(), error::service_stopped);

    // Channel was not pent or stored.
    BOOST_REQUIRE_EQUAL(net.pent_nonce(), 0u);
    BOOST_REQUIRE_EQUAL(net.stored_nonce(), 0u);

    // Channel was not unpent or unstored.
    BOOST_REQUIRE_EQUAL(net.unpent_nonce(), 0u);
    BOOST_REQUIRE_EQUAL(net.unstored_nonce(), 0u);
}

BOOST_AUTO_TEST_CASE(session__start_channel__channel_not_started__handlers_channel_stopped_channel_channel_stopped_pent_not_stored)
{
    const logger log{ false };
    settings set(selection::mainnet);
    mock_p2p net(set, log);
    auto session = std::make_shared<mock_session>(net, 1, false, true);

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]()
    {
        session->start([&](const code& ec)
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);

    const auto socket = std::make_shared<network::socket>(net.log(), net.service());
    const auto channel = std::make_shared<mock_channel>(net.log(), socket, session->settings(), 42);

    // Stop the channel (started by default).
    std::promise<bool> unstarted_channel;
    boost::asio::post(channel->strand(), [=, &unstarted_channel]()
    {
        channel->stopper(error::invalid_magic);
        unstarted_channel.set_value(true);
    });

    BOOST_REQUIRE(unstarted_channel.get_future().get());
    BOOST_REQUIRE(channel->stopped());

    std::promise<code> started_channel;
    std::promise<code> stopped_channel;
    boost::asio::post(net.strand(), [=, &started_channel, &stopped_channel]()
    {
        session->start_channel(channel,
            [&](const code& ec)
            {
                started_channel.set_value(ec);
            },
            [&](const code& ec)
            {
                stopped_channel.set_value(ec);
            });
    });

    BOOST_REQUIRE_EQUAL(started_channel.get_future().get(), error::channel_stopped);
    BOOST_REQUIRE(session->attached_handshake());
    BOOST_REQUIRE(channel->resumed());
    BOOST_REQUIRE(!session->attached_protocol());
    BOOST_REQUIRE(!channel->reresumed());
    BOOST_REQUIRE_EQUAL(stopped_channel.get_future().get(), error::channel_stopped);
    BOOST_REQUIRE(channel->stopped());
    BOOST_REQUIRE_EQUAL(channel->stop_code(), error::channel_stopped);

    // Channel was pent (handshake invoked) and not stored.
    BOOST_REQUIRE_EQUAL(net.pent_nonce(), channel->nonce());
    BOOST_REQUIRE_EQUAL(net.stored_nonce(), 0u);

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]()
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());

    // Channel unpend in do_handle_channel_stopped now, wait on network_.unpend().
    ////BOOST_REQUIRE_EQUAL(net.unpent_nonce(), channel->nonce());
    BOOST_REQUIRE_EQUAL(net.unstored_nonce(), channel->nonce());
    BOOST_REQUIRE(!net.unstored_inbound());
    BOOST_REQUIRE_EQUAL(net.unstore_result(), error::success);
}

BOOST_AUTO_TEST_CASE(session__start_channel__network_not_started__handlers_service_stopped_channel_service_stopped_pent_store_failed)
{
    const logger log{ false };
    settings set(selection::mainnet);
    mock_p2p net(set, log);
    auto session = std::make_shared<mock_session>(net, 1, false, true);

    std::promise<code> started;
    boost::asio::post(net.strand(), [&]()
    {
        session->start([&](const code& ec)
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);

    const auto socket = std::make_shared<network::socket>(net.log(), net.service());
    const auto channel = std::make_shared<mock_channel>(net.log(), socket, session->settings(), 42);

    std::promise<code> started_channel;
    std::promise<code> stopped_channel;
    boost::asio::post(net.strand(), [=, &started_channel, &stopped_channel]()
    {
        session->start_channel(channel,
            [&](const code& ec)
            {
                started_channel.set_value(ec);
            },
            [&](const code& ec)
            {
                stopped_channel.set_value(ec);
            });
    });

    // Channel stopped by network.store code then heading read fail (bad_stream).
    BOOST_REQUIRE_EQUAL(started_channel.get_future().get(), error::service_stopped);
    BOOST_REQUIRE(session->attached_handshake());
    BOOST_REQUIRE(channel->resumed());
    BOOST_REQUIRE(!session->attached_protocol());
    BOOST_REQUIRE(!channel->reresumed());
    BOOST_REQUIRE_EQUAL(stopped_channel.get_future().get(), error::service_stopped);
    BOOST_REQUIRE(channel->stopped());

    // Race between bad_stream and service_stopped.
    BOOST_REQUIRE(channel->stop_code());
    ////BOOST_REQUIRE_EQUAL(channel->stop_code(), error::bad_stream);
    ////BOOST_REQUIRE_EQUAL(channel->stop_code(), error::service_stopped);

    // Channel was pent (handshake invoked) and store failed.
    BOOST_REQUIRE_EQUAL(net.pent_nonce(), channel->nonce());
    BOOST_REQUIRE_EQUAL(net.stored_nonce(), channel->nonce());
    BOOST_REQUIRE_EQUAL(net.stored_result(), error::service_stopped);
    BOOST_REQUIRE(!net.stored_inbound());
    BOOST_REQUIRE(net.stored_notify());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [&]()
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());

    // Channel unpend in do_handle_channel_stopped now, wait on network_.unpend().
    ////BOOST_REQUIRE_EQUAL(net.unpent_nonce(), channel->nonce());
    BOOST_REQUIRE_EQUAL(net.unstored_nonce(), channel->nonce());
    BOOST_REQUIRE(!net.unstored_inbound());
    BOOST_REQUIRE_EQUAL(net.unstore_result(), error::success);
}

BOOST_AUTO_TEST_CASE(session__start_channel__all_started__handlers_expected_channel_service_stopped_pent_store_succeeded)
{
    const logger log{ false };
    settings set(selection::mainnet);
    set.host_pool_capacity = 0;
    mock_p2p net(set, log);
    auto session = std::make_shared<mock_session>(net, 1, false, true);

    std::promise<code> started_net;
    boost::asio::post(net.strand(), [&net, &started_net]()
    {
        net.start([&](const code& ec)
        {
            started_net.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started_net.get_future().get(), error::success);

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]()
    {
        session->start([&](const code& ec)
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);

    const auto socket = std::make_shared<network::socket>(net.log(), net.service());
    const auto channel = std::make_shared<mock_channel>(net.log(), socket, session->settings(), 42);
    
    std::promise<code> started_channel;
    std::promise<code> stopped_channel;
    boost::asio::post(net.strand(), [=, &started_channel, &stopped_channel]()
    {
        session->start_channel(channel,
            [&](const code& ec)
            {
                started_channel.set_value(ec);
            },
            [&](const code& ec)
            {
                stopped_channel.set_value(ec);
            });
    });

    // Channel stopped by heading read fail (bad_stream), stop method called by session.
    BOOST_REQUIRE_EQUAL(started_channel.get_future().get(), error::success);
    BOOST_REQUIRE(session->attached_handshake());
    BOOST_REQUIRE(channel->resumed());
    ////BOOST_REQUIRE(session->attached_protocol());
    ////BOOST_REQUIRE(channel->reresumed());

    // Race between bad_stream and service_stopped.
    BOOST_REQUIRE(channel->stopped());
    BOOST_REQUIRE(channel->stop_code());
    ////BOOST_REQUIRE_EQUAL(stopped_channel.get_future().get(), error::bad_stream);
    ////BOOST_REQUIRE_EQUAL(stopped_channel.get_future().get(), error::subscriber_stopped);

    // Channel is stopped before handshake completion, due to read failure.
    BOOST_REQUIRE_EQUAL(channel->stop_code(), error::bad_stream);

    // Channel pent and store succeeded.
    BOOST_REQUIRE_EQUAL(net.pent_nonce(), channel->nonce());
    BOOST_REQUIRE_EQUAL(net.stored_nonce(), channel->nonce());
    BOOST_REQUIRE_EQUAL(net.stored_result(), error::success);
    BOOST_REQUIRE(!net.stored_inbound());
    BOOST_REQUIRE(net.stored_notify());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]()
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());

    // Channel was unpent, found on unstore returns success.
    BOOST_REQUIRE_EQUAL(net.unpent_nonce(), channel->nonce());
    BOOST_REQUIRE_EQUAL(net.unstored_nonce(), channel->nonce());
    BOOST_REQUIRE(!net.unstored_inbound());
    BOOST_REQUIRE_EQUAL(net.unstore_result(), error::success);
}

BOOST_AUTO_TEST_CASE(session__start_channel__outbound_all_started__handlers_expected_channel_success_pent_store_succeeded)
{
    const logger log{ false };
    settings set(selection::mainnet);
    set.host_pool_capacity = 0;
    mock_p2p net(set, log);

    constexpr auto expected_inbound = false;
    constexpr auto expected_notify = true;
    auto session = std::make_shared<mock_session>(net, 1, expected_inbound, expected_notify);

    std::promise<code> started_net;
    boost::asio::post(net.strand(), [&net, &started_net]()
    {
        net.start([&](const code& ec)
        {
            started_net.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started_net.get_future().get(), error::success);

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]()
    {
        session->start([&](const code& ec)
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);

    const auto socket = std::make_shared<network::socket>(net.log(), net.service());
    const auto channel = std::make_shared<mock_channel_no_read>(net.log(), socket, session->settings(), 42);
    
    std::promise<code> started_channel;
    std::promise<code> stopped_channel;
    boost::asio::post(net.strand(), [=, &started_channel, &stopped_channel]()
    {
        session->start_channel(channel,
            [&](const code& ec)
            {
                started_channel.set_value(ec);
            },
            [&](const code& ec)
            {
                stopped_channel.set_value(ec);
            });
    });

    // Channel stopped by heading read fail, stop method called by session.
    BOOST_REQUIRE_EQUAL(started_channel.get_future().get(), error::success);
    BOOST_REQUIRE(session->attached_handshake());
    BOOST_REQUIRE(channel->resumed());
    BOOST_REQUIRE(session->require_attached_protocol());
    BOOST_REQUIRE(!channel->stopped());

    // Channel pent and store succeeded.
    BOOST_REQUIRE_EQUAL(net.pent_nonce(), channel->nonce());
    BOOST_REQUIRE_EQUAL(net.stored_nonce(), channel->nonce());
    BOOST_REQUIRE_EQUAL(net.stored_result(), error::success);
    BOOST_REQUIRE_EQUAL(net.stored_inbound(), expected_inbound);
    BOOST_REQUIRE_EQUAL(net.stored_notify(), expected_notify);

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]()
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
    BOOST_REQUIRE(channel->reresumed());
    BOOST_REQUIRE(!channel->stopped());

    // Channel unpend in do_handle_channel_stopped now, wait on network_.unpend().
    ////BOOST_REQUIRE_EQUAL(net.unpent_nonce(), channel->nonce());

    net.close();
    BOOST_REQUIRE_EQUAL(stopped_channel.get_future().get(), error::service_stopped);
    BOOST_REQUIRE(channel->stopped());
    BOOST_REQUIRE_EQUAL(channel->stop_code(), error::service_stopped);

    // Unstored on close, but may not be found because channels cleared in a race.
    ////BOOST_REQUIRE(!net.unstore_found());
    BOOST_REQUIRE_EQUAL(net.unstored_nonce(), channel->nonce());
    BOOST_REQUIRE_EQUAL(net.unstored_inbound(), expected_inbound);
}

BOOST_AUTO_TEST_CASE(session__start_channel__inbound_all_started__handlers_expected_channel_success_pent_store_succeeded)
{
    const logger log{ false };
    settings set(selection::mainnet);
    set.host_pool_capacity = 0;
    mock_p2p net(set, log);

    constexpr auto expected_inbound = true;
    constexpr auto expected_notify = false;
    auto session = std::make_shared<mock_session>(net, 1, expected_inbound, expected_notify);

    std::promise<code> started_net;
    boost::asio::post(net.strand(), [&net, &started_net]()
    {
        net.start([&](const code& ec)
        {
            started_net.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started_net.get_future().get(), error::success);

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]()
    {
        session->start([&](const code& ec)
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);

    const auto socket = std::make_shared<network::socket>(net.log(), net.service());
    const auto channel = std::make_shared<mock_channel_no_read>(net.log(), socket, session->settings(), 42);
    
    std::promise<code> started_channel;
    std::promise<code> stopped_channel;
    boost::asio::post(net.strand(), [=, &started_channel, &stopped_channel]()
    {
        session->start_channel(channel,
            [&](const code& ec)
            {
                started_channel.set_value(ec);
            },
            [&](const code& ec)
            {
                stopped_channel.set_value(ec);
            });
    });

    // Channel stopped by heading read fail, stop method called by session.
    BOOST_REQUIRE_EQUAL(started_channel.get_future().get(), error::success);
    BOOST_REQUIRE(session->attached_handshake());
    BOOST_REQUIRE(channel->resumed());
    BOOST_REQUIRE(session->require_attached_protocol());
    BOOST_REQUIRE(!channel->stopped());

    // Channel not pent (inbound) and store succeeded.
    BOOST_REQUIRE_EQUAL(net.pent_nonce(), 0u);
    BOOST_REQUIRE_EQUAL(net.stored_nonce(), channel->nonce());
    BOOST_REQUIRE_EQUAL(net.stored_result(), error::success);
    BOOST_REQUIRE_EQUAL(net.stored_inbound(), expected_inbound);
    BOOST_REQUIRE_EQUAL(net.stored_notify(), expected_notify);

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]()
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
    BOOST_REQUIRE(channel->reresumed());
    BOOST_REQUIRE(!channel->stopped());

    // Channel not unpent (inbound).
    BOOST_REQUIRE_EQUAL(net.unpent_nonce(), 0u);

    net.close();
    BOOST_REQUIRE_EQUAL(stopped_channel.get_future().get(), error::service_stopped);
    BOOST_REQUIRE(channel->stopped());
    BOOST_REQUIRE_EQUAL(channel->stop_code(), error::service_stopped);

    // Unstored on close, but may not be found because channels cleared in a race.
    ////BOOST_REQUIRE(!net.unstore_found());
    BOOST_REQUIRE_EQUAL(net.unstored_nonce(), channel->nonce());
    BOOST_REQUIRE_EQUAL(net.unstored_inbound(), expected_inbound);
}

BOOST_AUTO_TEST_SUITE_END()
