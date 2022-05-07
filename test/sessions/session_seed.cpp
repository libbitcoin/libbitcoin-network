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

BOOST_AUTO_TEST_SUITE(session_seed_tests)

using namespace bc::network;
using namespace bc::network::messages;
using namespace bc::system::chain;

class mock_connector_connect_fail
  : public connector
{
public:
    typedef std::shared_ptr<mock_connector_connect_fail> ptr;

    using connector::connector;

    void connect(const std::string&, uint16_t,
        connect_handler&& handler) override
    {
        handler(error::invalid_magic, nullptr);
    }
};

class mock_session_seed
  : public session_seed
{
public:
    using session_seed::session_seed;

    bool inbound() const noexcept override
    {
        return session_seed::inbound();
    }

    bool notify() const noexcept override
    {
        return session_seed::notify();
    }

    bool stopped() const
    {
        return session_seed::stopped();
    }

    // Capture first start_connect call.
    void start_seed(const config::endpoint& seed,
        connector::ptr connector, channel_handler handler) noexcept override
    {
        // Must be first to ensure connector::connect() preceeds promise release.
        session_seed::start_seed(seed, connector, handler);

        if (!seeded_)
            seed_.set_value(true);
    }

    bool seeded() const
    {
        return seeded_;
    }

    bool require_seeded() const
    {
        return seed_.get_future().get();
    }

    void attach_handshake(const channel::ptr&,
        result_handler handshake) const noexcept override
    {
        if (!handshaked_)
        {
            handshaked_ = true;
            handshake_.set_value(true);
        }

        // Simulate handshake successful completion.
        handshake(error::success);
    }

    bool attached_handshake() const
    {
        return handshaked_;
    }

    bool require_attached_handshake() const
    {
        return handshake_.get_future().get();
    }

protected:
    mutable bool handshaked_{ false };
    mutable std::promise<bool> handshake_;

private:
    bool seeded_{ false };
    mutable std::promise<bool> seed_;
};

template <class Connector = network::connector>
class mock_p2p
  : public p2p
{
public:
    using p2p::p2p;

    // Get last created connector.
    typename Connector::ptr get_connector() const
    {
        return connector_;
    }

    // Create mock connector to inject mock channel.
    connector::ptr create_connector() noexcept override
    {
        return ((connector_ = std::make_shared<Connector>(strand(), service(),
            network_settings())));
    }

private:
    typename Connector::ptr connector_;
};

// start via network (not required for coverage)

BOOST_AUTO_TEST_SUITE_END()
