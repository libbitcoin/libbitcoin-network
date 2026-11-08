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

BC_AUTO_TEST_SUITE(acceptor_sam_tests)

class accessor
  : public acceptor_sam
{
public:
    using acceptor_sam::acceptor_sam;
    using acceptor_sam::sam_result;
    using acceptor_sam::proxied;
};

// The acceptor runs on the fixture pool and the bridge is a plain socket
// driven synchronously from the test thread, so each line exchange is ordered
// by the bridge's own reads and writes.
struct sam_setup_fixture
{
    DELETE_COPY_MOVE(sam_setup_fixture);

    sam_setup_fixture()
      : pool(2), strand(pool.service().get_executor()),
        listener(context), bridge(context), forward(context), peer(context)
    {
        log.stop();
        REQUIRE_SUCCESS(listener, open, io::tcp::v4());
        REQUIRE_SUCCESS(listener, bind, test::loopback());
        REQUIRE_SUCCESS(listener, listen);
        sam_settings.bridge = { listener.local_endpoint() };
    }

    ~sam_setup_fixture()
    {
        if (instance)
        {
            std::promise<bool> stopped{};
            io::post(strand, [this, &stopped]() NOEXCEPT
            {
                instance->stop();
                stopped.set_value(true);
            });

            stopped.get_future().get();
        }

        peer.close();
        forward.close();
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
        io::post(strand, [this, &started]() NOEXCEPT
        {
            started.set_value(instance->start({ "127.0.0.1:65022" }));
        });

        return started.get_future().get();
    }

    // Start an accept, returns the future accept result.
    std::future<code> accept()
    {
        io::post(strand, [this]() NOEXCEPT
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

    // The port of the loopback listener to which streams are forwarded.
    uint16_t local_port()
    {
        std::promise<uint16_t> port{};
        io::post(strand, [this, &port]() NOEXCEPT
        {
            port.set_value(instance->local().port());
        });

        return port.get_future().get();
    }

    // Point the bridge at an endpoint that refuses connections.
    void refuse()
    {
        std::promise<bool> pointed{};
        io::post(strand, [this, &pointed]() NOEXCEPT
        {
            sam_settings.bridge = { test::refuser() };
            pointed.set_value(true);
        });

        pointed.get_future().get();
    }

    // Accept the acceptor's connection to the bridge.
    void connect()
    {
        REQUIRE_SUCCESS(listener, accept, bridge);
    }

    // Read one newline-terminated line from the bridge socket.
    std::string read_line()
    {
        return read_line(bridge);
    }

    void write_line(const std::string& line)
    {
        write_line(bridge, line);
    }

    // Bytes are read singly so that none beyond the line are consumed.
    static std::string read_line(io::socket& socket)
    {
        std::string line{};
        char byte{};
        while (is_one(test::read(socket, io::buffer(&byte, one))) && byte != '\n')
            line.push_back(byte);

        return line;
    }

    static void write_line(io::socket& socket, const std::string& line)
    {
        const auto text = line + "\n";
        test::write(socket, io::buffer(text));
    }

    // True if the socket is closed by the remote.
    static bool closed(io::socket& socket)
    {
        code ec{};
        std::array<char, 1> byte{};
        io::read(socket, io::buffer(byte), ec);
        return ec == error::peer_disconnect;
    }

    // Complete the session, returning the session create request.
    std::string create_session(const std::string& key)
    {
        connect();
        read_line();
        write_line("HELLO REPLY RESULT=OK VERSION=3.1");
        const auto request = read_line();
        write_line("SESSION STATUS RESULT=OK DESTINATION=" + key);
        return request;
    }

    // Accept the forward control connection and complete its hello.
    void forward_hello()
    {
        REQUIRE_SUCCESS(listener, accept, forward);
        read_line(forward);
        write_line(forward, "HELLO REPLY RESULT=OK VERSION=3.1");
    }

    // Complete session and forwarding, returning the stream forward request.
    std::string establish()
    {
        create_session(destination);
        forward_hello();
        const auto request = read_line(forward);
        write_line(forward, "STREAM STATUS RESULT=OK");
        return request;
    }

    // Connect the peer stream to the forwarding listener.
    void connect_peer()
    {
        REQUIRE_SUCCESS(peer, connect, test::loopback(local_port()));
    }

    // An ed25519 destination with an empty certificate (387 bytes).
    static inline const std::string destination = system::encode_base64(system::splice(system::data_chunk(385, 0x42), system::data_chunk(2, 0x00)));

    logger log{};
    threadpool pool;
    std::atomic_bool suspended{ false };
    io::strand strand;
    settings::sam sam_settings{};
    acceptor_sam::ptr instance{};
    io::context context{};
    io::acceptor listener;
    io::socket bridge;
    io::socket forward;
    io::socket peer;
    std::promise<code> accepted{};
};

struct sam_directory_setup_fixture
  : test::directory_setup_fixture, sam_setup_fixture
{
};

BC_AUTO_TEST_CASE(acceptor_sam__sam_result__result_values__expected)
{
    BC_REQUIRE_EQUAL(accessor::sam_result("OK"), error::success);
    BC_REQUIRE_EQUAL(accessor::sam_result("NOVERSION"), error::sam_no_version);
    BC_REQUIRE_EQUAL(accessor::sam_result("CANT_REACH_PEER"), error::sam_cant_reach_peer);
    BC_REQUIRE_EQUAL(accessor::sam_result("DUPLICATED_DEST"), error::sam_duplicated_dest);
    BC_REQUIRE_EQUAL(accessor::sam_result("DUPLICATED_ID"), error::sam_duplicated_id);
    BC_REQUIRE_EQUAL(accessor::sam_result("I2P_ERROR"), error::sam_i2p_error);
    BC_REQUIRE_EQUAL(accessor::sam_result("INVALID_ID"), error::sam_invalid_id);
    BC_REQUIRE_EQUAL(accessor::sam_result("INVALID_KEY"), error::sam_invalid_key);
    BC_REQUIRE_EQUAL(accessor::sam_result("KEY_NOT_FOUND"), error::sam_key_not_found);
    BC_REQUIRE_EQUAL(accessor::sam_result("PEER_NOT_FOUND"), error::sam_peer_not_found);
    BC_REQUIRE_EQUAL(accessor::sam_result("TIMEOUT"), error::sam_timeout);
    BC_REQUIRE_EQUAL(accessor::sam_result("UNKNOWN"), error::sam_unassigned_failure);
    BC_REQUIRE_EQUAL(accessor::sam_result(""), error::sam_unassigned_failure);
}

BC_FIXTURE_TEST_CASE(acceptor_sam__start__always__success_proxied, sam_setup_fixture)
{
    BC_REQUIRE_EQUAL(create(), error::success);
}

BC_FIXTURE_TEST_CASE(acceptor_sam__accept__stopped__service_stopped, sam_setup_fixture)
{
    BC_REQUIRE_EQUAL(create(), error::success);

    std::promise<bool> stopped{};
    io::post(strand, [this, &stopped]() NOEXCEPT
    {
        instance->stop();
        stopped.set_value(true);
    });

    stopped.get_future().get();
    BC_REQUIRE_EQUAL(accept().get(), error::service_stopped);
}

BC_FIXTURE_TEST_CASE(acceptor_sam__accept__suspended__service_suspended, sam_setup_fixture)
{
    BC_REQUIRE_EQUAL(create(), error::success);
    suspended.store(true);
    BC_REQUIRE_EQUAL(accept().get(), error::service_suspended);
}

BC_FIXTURE_TEST_CASE(acceptor_sam__accept__hello_unversioned__sam_no_version, sam_setup_fixture)
{
    BC_REQUIRE_EQUAL(create(), error::success);
    auto result = accept();
    connect();

    const auto hello = read_line();
    BC_REQUIRE(hello.starts_with("HELLO VERSION MIN=3.1"));
    write_line("HELLO REPLY RESULT=NOVERSION");

    BC_REQUIRE_EQUAL(result.get(), error::sam_no_version);
}

BC_FIXTURE_TEST_CASE(acceptor_sam__accept__authenticated_hello__expected_request, sam_setup_fixture)
{
    sam_settings.username = "user";
    sam_settings.password = "pass";
    BC_REQUIRE_EQUAL(create(), error::success);
    auto result = accept();
    connect();

    const auto hello = read_line();
    BC_REQUIRE_EQUAL(hello, "HELLO VERSION MIN=3.2 MAX=3.2 USER=\"user\" PASSWORD=\"pass\"");
    write_line("HELLO REPLY RESULT=I2P_ERROR");

    BC_REQUIRE_EQUAL(result.get(), error::sam_i2p_error);
}

BC_FIXTURE_TEST_CASE(acceptor_sam__accept__hello_not_a_reply__sam_response_invalid, sam_setup_fixture)
{
    BC_REQUIRE_EQUAL(create(), error::success);
    auto result = accept();
    connect();

    read_line();
    write_line("PING PONG RESULT=OK");

    BC_REQUIRE_EQUAL(result.get(), error::sam_response_invalid);
}

BC_FIXTURE_TEST_CASE(acceptor_sam__accept__session_duplicated_id__sam_duplicated_id, sam_setup_fixture)
{
    BC_REQUIRE_EQUAL(create(), error::success);
    auto result = accept();
    connect();

    read_line();
    write_line("HELLO REPLY RESULT=OK VERSION=3.1");

    const auto create_session = read_line();
    BC_REQUIRE(create_session.starts_with("SESSION CREATE STYLE=STREAM ID="));
    write_line("SESSION STATUS RESULT=DUPLICATED_ID");

    BC_REQUIRE_EQUAL(result.get(), error::sam_duplicated_id);
}

BC_FIXTURE_TEST_CASE(acceptor_sam__accept__session_not_a_status__sam_response_invalid, sam_setup_fixture)
{
    BC_REQUIRE_EQUAL(create(), error::success);
    auto result = accept();
    connect();

    read_line();
    write_line("HELLO REPLY RESULT=OK VERSION=3.1");
    read_line();
    write_line("SESSION REPLY RESULT=OK");

    BC_REQUIRE_EQUAL(result.get(), error::sam_response_invalid);
}

BC_FIXTURE_TEST_CASE(acceptor_sam__accept__session_invalid_destination__sam_invalid_key, sam_setup_fixture)
{
    BC_REQUIRE_EQUAL(create(), error::success);
    auto result = accept();
    connect();

    read_line();
    write_line("HELLO REPLY RESULT=OK VERSION=3.1");
    read_line();
    write_line("SESSION STATUS RESULT=OK DESTINATION=not~a~key");

    BC_REQUIRE_EQUAL(result.get(), error::sam_invalid_key);
}

BC_AUTO_TEST_CASE(acceptor_sam__proxied__always__true)
{
    const logger log{};
    threadpool pool(1);
    std::atomic_bool suspended{ false };
    io::strand strand(pool.service().get_executor());
    const settings::sam sam{};
    const auto instance = std::make_shared<accessor>(log, strand, pool.service(), suspended, acceptor::parameters{}, sam);
    BC_REQUIRE(instance->proxied());
    pool.stop();
    BC_REQUIRE(pool.join());
}

BC_FIXTURE_TEST_CASE(acceptor_sam__start__started__operation_failed, sam_setup_fixture)
{
    BC_REQUIRE_EQUAL(create(), error::success);

    std::promise<code> restarted{};
    io::post(strand, [this, &restarted]() NOEXCEPT
    {
        restarted.set_value(instance->start({ "127.0.0.1:65022" }));
    });

    BC_REQUIRE_EQUAL(restarted.get_future().get(), error::operation_failed);
}

BC_FIXTURE_TEST_CASE(acceptor_sam__accept__no_bridge__connection_refused, sam_setup_fixture)
{
    BC_REQUIRE_EQUAL(create(), error::success);
    refuse();
    BC_REQUIRE_EQUAL(accept().get(), error::connection_refused);
}

BC_FIXTURE_TEST_CASE(acceptor_sam__accept__hello_closed__peer_disconnect, sam_setup_fixture)
{
    BC_REQUIRE_EQUAL(create(), error::success);
    auto result = accept();
    connect();

    read_line();
    bridge.close();

    BC_REQUIRE_EQUAL(result.get(), error::peer_disconnect);
}

BC_FIXTURE_TEST_CASE(acceptor_sam__accept__hello_quoted_values__result_outside_quotes, sam_setup_fixture)
{
    BC_REQUIRE_EQUAL(create(), error::success);
    auto result = accept();
    connect();

    read_line();
    write_line("HELLO REPLY MESSAGE=\"quoted RESULT=OK \\\"escaped RESULT=OK\\\"\" RESULT=NOVERSION");

    BC_REQUIRE_EQUAL(result.get(), error::sam_no_version);
}

BC_FIXTURE_TEST_CASE(acceptor_sam__accept__line_overflow__sam_response_invalid, sam_setup_fixture)
{
    BC_REQUIRE_EQUAL(create(), error::success);
    auto result = accept();
    connect();

    read_line();
    test::write(bridge, io::buffer(std::string(65'537, 'x')));

    BC_REQUIRE_EQUAL(result.get(), error::sam_response_invalid);
}

BC_FIXTURE_TEST_CASE(acceptor_sam__accept__session_closed__peer_disconnect, sam_setup_fixture)
{
    BC_REQUIRE_EQUAL(create(), error::success);
    auto result = accept();
    connect();

    read_line();
    write_line("HELLO REPLY RESULT=OK VERSION=3.1");
    read_line();
    bridge.close();

    BC_REQUIRE_EQUAL(result.get(), error::peer_disconnect);
}

BC_FIXTURE_TEST_CASE(acceptor_sam__accept__transient__expected_session_create, sam_setup_fixture)
{
    BC_REQUIRE_EQUAL(create(), error::success);
    auto result = accept();
    connect();

    read_line();
    write_line("HELLO REPLY RESULT=OK VERSION=3.1");

    const auto request = read_line();
    BC_REQUIRE_EQUAL(request.substr(0, 31), "SESSION CREATE STYLE=STREAM ID=");
    BC_REQUIRE_EQUAL(request.substr(47), " DESTINATION=TRANSIENT SIGNATURE_TYPE=7 i2cp.leaseSetEncType=4,0");
    write_line("SESSION STATUS RESULT=DUPLICATED_DEST");

    BC_REQUIRE_EQUAL(result.get(), error::sam_duplicated_dest);
}

BC_FIXTURE_TEST_CASE(acceptor_sam__accept__configured_key__i2p_alphabet_destination, sam_setup_fixture)
{
    sam_settings.key = "+/+/QkJC";
    BC_REQUIRE_EQUAL(create(), error::success);
    auto result = accept();
    connect();

    read_line();
    write_line("HELLO REPLY RESULT=OK VERSION=3.1");

    const auto request = read_line();
    BC_REQUIRE(request.ends_with(" DESTINATION=-~-~QkJC"));
    write_line("SESSION STATUS RESULT=INVALID_KEY");

    BC_REQUIRE_EQUAL(result.get(), error::sam_invalid_key);
}

BC_FIXTURE_TEST_CASE(acceptor_sam__accept__key_path_directory__file_save, sam_directory_setup_fixture)
{
    sam_settings.key_path = { TEST_DIRECTORY };
    BC_REQUIRE_EQUAL(create(), error::success);
    auto result = accept();
    create_session(destination);
    BC_REQUIRE_EQUAL(result.get(), error::file_save);
}

BC_FIXTURE_TEST_CASE(acceptor_sam__accept__forward_refused__connection_refused, sam_setup_fixture)
{
    BC_REQUIRE_EQUAL(create(), error::success);
    auto result = accept();
    connect();

    read_line();
    write_line("HELLO REPLY RESULT=OK VERSION=3.1");
    read_line();
    refuse();
    write_line("SESSION STATUS RESULT=OK DESTINATION=" + destination);

    BC_REQUIRE_EQUAL(result.get(), error::connection_refused);
    BC_REQUIRE(closed(bridge));
}

BC_FIXTURE_TEST_CASE(acceptor_sam__accept__forward_hello__expected_request, sam_setup_fixture)
{
    BC_REQUIRE_EQUAL(create(), error::success);
    auto result = accept();
    create_session(destination);
    REQUIRE_SUCCESS(listener, accept, forward);

    BC_REQUIRE_EQUAL(read_line(forward), "HELLO VERSION MIN=3.1 MAX=3.1");
    write_line(forward, "HELLO REPLY RESULT=NOVERSION");

    BC_REQUIRE_EQUAL(result.get(), error::sam_no_version);
}

BC_FIXTURE_TEST_CASE(acceptor_sam__accept__forward_hello_closed__peer_disconnect, sam_setup_fixture)
{
    BC_REQUIRE_EQUAL(create(), error::success);
    auto result = accept();
    create_session(destination);
    REQUIRE_SUCCESS(listener, accept, forward);

    read_line(forward);
    forward.close();

    BC_REQUIRE_EQUAL(result.get(), error::peer_disconnect);
}

BC_FIXTURE_TEST_CASE(acceptor_sam__accept__forward_hello_not_a_reply__sam_response_invalid, sam_setup_fixture)
{
    BC_REQUIRE_EQUAL(create(), error::success);
    auto result = accept();
    create_session(destination);
    REQUIRE_SUCCESS(listener, accept, forward);

    read_line(forward);
    write_line(forward, "SESSION STATUS RESULT=OK");

    BC_REQUIRE_EQUAL(result.get(), error::sam_response_invalid);
}

BC_FIXTURE_TEST_CASE(acceptor_sam__accept__stream_forward__expected_request, sam_setup_fixture)
{
    BC_REQUIRE_EQUAL(create(), error::success);
    auto result = accept();
    const auto session = create_session(destination);
    forward_hello();

    const auto id = session.substr(31, 16);
    const auto port = std::to_string(local_port());
    BC_REQUIRE_EQUAL(read_line(forward), "STREAM FORWARD ID=" + id + " PORT=" + port + " HOST=127.0.0.1 SILENT=false");
    write_line(forward, "STREAM STATUS RESULT=CANT_REACH_PEER");

    BC_REQUIRE_EQUAL(result.get(), error::sam_cant_reach_peer);
}

BC_FIXTURE_TEST_CASE(acceptor_sam__accept__stream_status_closed__peer_disconnect, sam_setup_fixture)
{
    BC_REQUIRE_EQUAL(create(), error::success);
    auto result = accept();
    create_session(destination);
    forward_hello();

    read_line(forward);
    forward.close();

    BC_REQUIRE_EQUAL(result.get(), error::peer_disconnect);
}

BC_FIXTURE_TEST_CASE(acceptor_sam__accept__stream_not_a_status__sam_response_invalid, sam_setup_fixture)
{
    BC_REQUIRE_EQUAL(create(), error::success);
    auto result = accept();
    create_session(destination);
    forward_hello();

    read_line(forward);
    write_line(forward, "STREAM REPLY RESULT=OK");

    BC_REQUIRE_EQUAL(result.get(), error::sam_response_invalid);
}

BC_FIXTURE_TEST_CASE(acceptor_sam__accept__forwarded_peer_closed__peer_disconnect, sam_setup_fixture)
{
    BC_REQUIRE_EQUAL(create(), error::success);
    auto result = accept();
    establish();
    connect_peer();
    peer.close();

    BC_REQUIRE_EQUAL(result.get(), error::peer_disconnect);
}

BC_FIXTURE_TEST_CASE(acceptor_sam__accept__forwarded_peer_empty_line__sam_response_invalid, sam_setup_fixture)
{
    BC_REQUIRE_EQUAL(create(), error::success);
    auto result = accept();
    establish();
    connect_peer();
    write_line(peer, "");

    BC_REQUIRE_EQUAL(result.get(), error::sam_response_invalid);
}

BC_FIXTURE_TEST_CASE(acceptor_sam__accept__forwarded_peer_invalid_destination__sam_response_invalid, sam_setup_fixture)
{
    BC_REQUIRE_EQUAL(create(), error::success);
    auto result = accept();
    establish();
    connect_peer();
    write_line(peer, "!!!!");

    BC_REQUIRE_EQUAL(result.get(), error::sam_response_invalid);
}

BC_FIXTURE_TEST_CASE(acceptor_sam__accept__forwarded_peer_destination__success, sam_setup_fixture)
{
    BC_REQUIRE_EQUAL(create(), error::success);
    auto result = accept();
    establish();
    connect_peer();
    write_line(peer, destination + " FROM_PORT=0 TO_PORT=0");

    BC_REQUIRE(result.wait_for(seconds(10)) == std::future_status::ready);
    BC_REQUIRE_EQUAL(result.get(), error::success);
}

BC_FIXTURE_TEST_CASE(acceptor_sam__accept__forward_closed__session_closed, sam_setup_fixture)
{
    BC_REQUIRE_EQUAL(create(), error::success);
    auto result = accept();
    establish();
    forward.close();

    BC_REQUIRE(closed(bridge));
}

BC_FIXTURE_TEST_CASE(acceptor_sam__accept__transient_key_path__key_saved, sam_directory_setup_fixture)
{
    const auto key = system::encode_base64(system::build_chunk({ system::base16_chunk("fbffbf"), system::data_chunk(382, 0x42), system::data_chunk(2, 0x00) }));
    const auto i2p = "-~-~" + key.substr(4);
    sam_settings.key_path = { TEST_PATH };
    BC_REQUIRE_EQUAL(create(), error::success);
    auto result = accept();
    create_session(i2p);
    forward_hello();

    std::ifstream file{ TEST_PATH };
    std::string saved{};
    std::getline(file, saved);
    BC_REQUIRE_EQUAL(saved, key);
    BC_REQUIRE(key.starts_with("+/+/"));
}

BC_AUTO_TEST_SUITE_END()
