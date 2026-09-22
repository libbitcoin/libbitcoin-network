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
#include <future>
#include "../test.hpp"

BOOST_AUTO_TEST_SUITE(acceptor_sam_tests)

#define SAM_BRIDGE_ENDPOINT "127.0.0.1:65021"

class accessor
  : public acceptor_sam
{
public:
    using acceptor_sam::acceptor_sam;
    using acceptor_sam::sam_result;
};

// The acceptor runs on the fixture pool and the bridge is a plain socket
// driven synchronously from the test thread, so each line exchange is ordered
// by the bridge's own reads and writes.
struct sam_setup_fixture
{
    DELETE_COPY_MOVE(sam_setup_fixture);

    sam_setup_fixture()
      : pool(2), strand(pool.service().get_executor()),
        listener(context), bridge(context)
    {
        log.stop();
        sam_settings.bridge = { SAM_BRIDGE_ENDPOINT };
        const config::authority listen{ SAM_BRIDGE_ENDPOINT };
        listener.open(boost::asio::ip::tcp::v4());
        listener.set_option(boost::asio::socket_base::reuse_address(true));
        listener.bind(listen.to_endpoint());
        listener.listen();
    }

    ~sam_setup_fixture()
    {
        if (instance)
        {
            std::promise<bool> stopped{};
            boost::asio::post(strand, [this, &stopped]() NOEXCEPT
            {
                instance->stop();
                stopped.set_value(true);
            });

            stopped.get_future().get();
        }

        bridge.close();
        listener.close();
        pool.stop();
        pool.join();
    }

    // Construct and start the acceptor, after any settings adjustment.
    code create()
    {
        acceptor::parameters params
        {
            .connect_timeout = seconds(10),
            .maximum_request = 42
        };
        instance = std::make_shared<acceptor_sam>(log, strand,
            pool.service(), suspended, std::move(params), sam_settings);

        std::promise<code> started{};
        boost::asio::post(strand, [this, &started]() NOEXCEPT
        {
            started.set_value(instance->start({ "127.0.0.1:65022" }));
        });

        return started.get_future().get();
    }

    // Start an accept, returns the future accept result.
    std::future<code> accept()
    {
        boost::asio::post(strand, [this]() NOEXCEPT
        {
            instance->accept(
                [this](const code& ec, const socket::ptr& socket) NOEXCEPT
                {
                    if (socket)
                        socket->stop();

                    accepted.set_value(ec);
                });
        });

        return accepted.get_future();
    }

    // Accept the acceptor's connection to the bridge.
    void connect()
    {
        listener.accept(bridge);
    }

    // Read one newline-terminated line from the bridge socket.
    std::string read_line()
    {
        boost::asio::streambuf buffer{};
        boost::asio::read_until(bridge, buffer, '\n');
        std::istream stream{ &buffer };
        std::string line{};
        std::getline(stream, line);
        return line;
    }

    void write_line(const std::string& line)
    {
        const auto text = line + "\n";
        boost::asio::write(bridge, boost::asio::buffer(text));
    }

    logger log{};
    threadpool pool;
    std::atomic_bool suspended{ false };
    asio::strand strand;
    settings::sam sam_settings{};
    acceptor_sam::ptr instance{};
    boost::asio::io_context context{};
    boost::asio::ip::tcp::acceptor listener;
    boost::asio::ip::tcp::socket bridge;
    std::promise<code> accepted{};
};

BOOST_AUTO_TEST_CASE(acceptor_sam__sam_result__result_values__expected)
{
    BOOST_REQUIRE_EQUAL(accessor::sam_result("OK"), error::success);
    BOOST_REQUIRE_EQUAL(accessor::sam_result("NOVERSION"), error::sam_no_version);
    BOOST_REQUIRE_EQUAL(accessor::sam_result("CANT_REACH_PEER"), error::sam_cant_reach_peer);
    BOOST_REQUIRE_EQUAL(accessor::sam_result("DUPLICATED_DEST"), error::sam_duplicated_dest);
    BOOST_REQUIRE_EQUAL(accessor::sam_result("DUPLICATED_ID"), error::sam_duplicated_id);
    BOOST_REQUIRE_EQUAL(accessor::sam_result("I2P_ERROR"), error::sam_i2p_error);
    BOOST_REQUIRE_EQUAL(accessor::sam_result("INVALID_ID"), error::sam_invalid_id);
    BOOST_REQUIRE_EQUAL(accessor::sam_result("INVALID_KEY"), error::sam_invalid_key);
    BOOST_REQUIRE_EQUAL(accessor::sam_result("KEY_NOT_FOUND"), error::sam_key_not_found);
    BOOST_REQUIRE_EQUAL(accessor::sam_result("PEER_NOT_FOUND"), error::sam_peer_not_found);
    BOOST_REQUIRE_EQUAL(accessor::sam_result("TIMEOUT"), error::sam_timeout);
    BOOST_REQUIRE_EQUAL(accessor::sam_result("UNKNOWN"), error::sam_unassigned_failure);
    BOOST_REQUIRE_EQUAL(accessor::sam_result(""), error::sam_unassigned_failure);
}

BOOST_FIXTURE_TEST_CASE(acceptor_sam__start__always__success_proxied, sam_setup_fixture)
{
    BOOST_REQUIRE_EQUAL(create(), error::success);
}

BOOST_FIXTURE_TEST_CASE(acceptor_sam__accept__stopped__service_stopped, sam_setup_fixture)
{
    BOOST_REQUIRE_EQUAL(create(), error::success);

    std::promise<bool> stopped{};
    boost::asio::post(strand, [this, &stopped]() NOEXCEPT
    {
        instance->stop();
        stopped.set_value(true);
    });

    stopped.get_future().get();
    BOOST_REQUIRE_EQUAL(accept().get(), error::service_stopped);
}

BOOST_FIXTURE_TEST_CASE(acceptor_sam__accept__suspended__service_suspended, sam_setup_fixture)
{
    BOOST_REQUIRE_EQUAL(create(), error::success);
    suspended.store(true);
    BOOST_REQUIRE_EQUAL(accept().get(), error::service_suspended);
}

BOOST_FIXTURE_TEST_CASE(acceptor_sam__accept__hello_unversioned__sam_no_version, sam_setup_fixture)
{
    BOOST_REQUIRE_EQUAL(create(), error::success);
    auto result = accept();
    connect();

    const auto hello = read_line();
    BOOST_REQUIRE(hello.starts_with("HELLO VERSION MIN=3.1"));
    write_line("HELLO REPLY RESULT=NOVERSION");

    BOOST_REQUIRE_EQUAL(result.get(), error::sam_no_version);
}

BOOST_FIXTURE_TEST_CASE(acceptor_sam__accept__authenticated_hello__expected_request, sam_setup_fixture)
{
    sam_settings.username = "user";
    sam_settings.password = "pass";
    BOOST_REQUIRE_EQUAL(create(), error::success);
    auto result = accept();
    connect();

    const auto hello = read_line();
    BOOST_REQUIRE_EQUAL(hello, "HELLO VERSION MIN=3.2 MAX=3.2 USER=\"user\" PASSWORD=\"pass\"");
    write_line("HELLO REPLY RESULT=I2P_ERROR");

    BOOST_REQUIRE_EQUAL(result.get(), error::sam_i2p_error);
}

BOOST_FIXTURE_TEST_CASE(acceptor_sam__accept__hello_not_a_reply__sam_response_invalid, sam_setup_fixture)
{
    BOOST_REQUIRE_EQUAL(create(), error::success);
    auto result = accept();
    connect();

    read_line();
    write_line("PING PONG RESULT=OK");

    BOOST_REQUIRE_EQUAL(result.get(), error::sam_response_invalid);
}

BOOST_FIXTURE_TEST_CASE(acceptor_sam__accept__session_duplicated_id__sam_duplicated_id, sam_setup_fixture)
{
    BOOST_REQUIRE_EQUAL(create(), error::success);
    auto result = accept();
    connect();

    read_line();
    write_line("HELLO REPLY RESULT=OK VERSION=3.1");

    const auto create_session = read_line();
    BOOST_REQUIRE(create_session.starts_with("SESSION CREATE STYLE=STREAM ID="));
    write_line("SESSION STATUS RESULT=DUPLICATED_ID");

    BOOST_REQUIRE_EQUAL(result.get(), error::sam_duplicated_id);
}

BOOST_FIXTURE_TEST_CASE(acceptor_sam__accept__session_not_a_status__sam_response_invalid, sam_setup_fixture)
{
    BOOST_REQUIRE_EQUAL(create(), error::success);
    auto result = accept();
    connect();

    read_line();
    write_line("HELLO REPLY RESULT=OK VERSION=3.1");
    read_line();
    write_line("SESSION REPLY RESULT=OK");

    BOOST_REQUIRE_EQUAL(result.get(), error::sam_response_invalid);
}

BOOST_FIXTURE_TEST_CASE(acceptor_sam__accept__session_invalid_destination__sam_invalid_key, sam_setup_fixture)
{
    BOOST_REQUIRE_EQUAL(create(), error::success);
    auto result = accept();
    connect();

    read_line();
    write_line("HELLO REPLY RESULT=OK VERSION=3.1");
    read_line();
    write_line("SESSION STATUS RESULT=OK DESTINATION=not~a~key");

    BOOST_REQUIRE_EQUAL(result.get(), error::sam_invalid_key);
}

BOOST_AUTO_TEST_SUITE_END()
