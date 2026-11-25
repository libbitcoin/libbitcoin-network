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

#include <future>

BC_AUTO_TEST_SUITE(channel_sv2_tests)

using namespace std::chrono_literals;
using system::data_chunk;
using system::base16_chunk;
using system::pointer_cast;
using namespace network::messages::sv2;

static const network::settings sv2_settings{ test::network_settings() };
static const channel_sv2::options_t sv2_options{ "test" };

// setup_connection, setup_connection_success{ 2, 0 } and set_target frames.
static data_chunk setup_frame() NOEXCEPT
{
    return base16_chunk("0000002a0000011202130304030205057465787435160605746578743705746578743805746578743906746578743130");
}

static data_chunk success_frame() NOEXCEPT
{
    return base16_chunk("000001060000020000000000");
}

static data_chunk target_frame() NOEXCEPT
{
    return base16_chunk("00802124000001030202202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f");
}

template <typename Value>
static Value await(std::promise<Value>& promise) NOEXCEPT
{
    auto future = promise.get_future();
    BC_REQUIRE(future.wait_for(5s) == std::future_status::ready);
    return future.get();
}

// A channel accepted on loopback, answering setup with success.
struct sv2_channel_fixture
{
    DELETE_COPY_MOVE(sv2_channel_fixture);

    sv2_channel_fixture() NOEXCEPT
      : pool_(2), strand_(pool_.service().get_executor()), acceptor_(strand_),
        client_(client_service_), websocket_(client_)
    {
        const auto local = test::loopback();
        code ec{};
        acceptor_.open(io::tcp::of(local), ec);
        BC_REQUIRE(!ec);
        acceptor_.bind(local, ec);
        BC_REQUIRE(!ec);
        acceptor_.listen(ec);
        BC_REQUIRE(!ec);

        network::socket::parameters params
        {
            .maximum_request = 1'000'000,
            .minimum_buffer = settings::tcp_server{ "test" }.minimum_buffer,
            .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer
        };

        const auto server = std::make_shared<network::socket>(log_,
            pool_.service(), std::move(params));

        std::promise<code> accepted{};
        server->accept(acceptor_, [&](const code& accept_ec) NOEXCEPT
        {
            accepted.set_value(accept_ec);
        });

        client_.connect(acceptor_.local_endpoint(), ec);
        BC_REQUIRE(!ec);
        BC_REQUIRE_EQUAL(await(accepted), error::success);

        channel_ = std::make_shared<channel_sv2>(log_, server, 42,
            sv2_settings, sv2_options);

        std::promise<code> started{};
        io::post(channel_->strand(), [&]() NOEXCEPT
        {
            channel_->subscribe_stop([&](const code& stop_ec) NOEXCEPT
            {
                stopped.set_value(stop_ec);
            }, [](const code&) NOEXCEPT {});

            channel_->subscribe<setup_connection>([&](const code& message_ec,
                const setup_connection::cptr& message) NOEXCEPT
            {
                if (message_ec)
                    return false;

                channel_->set_handshaked();
                channel_->send(setup_connection_success{ 2, 0 },
                    [](const code&) NOEXCEPT {});

                setup.set_value(message);
                return true;
            });

            channel_->resume();
            started.set_value(error::success);
        });

        BC_REQUIRE_EQUAL(await(started), error::success);
    }

    ~sv2_channel_fixture() NOEXCEPT
    {
        code ignore{};
        client_.close(ignore);
        channel_->stop(error::service_stopped);
        pool_.stop();
        BC_REQUIRE(pool_.join());
    }

    void upgrade() NOEXCEPT
    {
        BC_REQUIRE_EQUAL(websocket_.handshake("127.0.0.1", "/"), error::success);
    }

    void send(const data_chunk& data) NOEXCEPT
    {
        const auto text = pointer_cast<const char>(data.data());
        BC_REQUIRE_EQUAL(websocket_.write({ text, data.size() }, true), error::success);
    }

    void send_text(const std::string& text) NOEXCEPT
    {
        code ec{};
        io::write(client_, io::buffer(text), ec);
        BC_REQUIRE(!ec);
    }

    data_chunk receive() NOEXCEPT
    {
        http::flat_buffer buffer{};
        BC_REQUIRE_EQUAL(websocket_.read(buffer), error::success);
        BC_REQUIRE(websocket_.binary());
        const auto data = pointer_cast<const uint8_t>(buffer.data().data());
        return { data, std::next(data, buffer.size()) };
    }

    http::status receive_status() NOEXCEPT
    {
        code ec{};
        http::flat_buffer buffer{};
        http::message<false, http::string_body> response{};
        http::read(client_, buffer, response, ec);
        BC_REQUIRE(!ec);
        return response.result();
    }

    std::promise<code> stopped{};
    std::promise<setup_connection::cptr> setup{};

private:
    const logger log_{};
    threadpool pool_;
    io::strand strand_;
    io::acceptor acceptor_;
    io::context client_service_{};
    io::socket client_;
    ws::client<io::socket> websocket_;
    channel_sv2::ptr channel_{};
};

BC_FIXTURE_TEST_CASE(channel_sv2__dispatch__setup_connection__subscriber_notified, sv2_channel_fixture)
{
    upgrade();
    send(setup_frame());
    const auto message = await(setup);
    BC_REQUIRE(message);
    BC_REQUIRE_EQUAL(message->min_version, 0x0212u);
    BC_REQUIRE_EQUAL(message->device_id, "text10");
}

BC_FIXTURE_TEST_CASE(channel_sv2__send__setup_connection_success__binary_frame, sv2_channel_fixture)
{
    upgrade();
    send(setup_frame());
    BC_REQUIRE_EQUAL(receive(), success_frame());
}

BC_FIXTURE_TEST_CASE(channel_sv2__dispatch__message_before_handshake__protocol_violation, sv2_channel_fixture)
{
    upgrade();
    send(target_frame());
    BC_REQUIRE_EQUAL(await(stopped), error::protocol_violation);
}

BC_FIXTURE_TEST_CASE(channel_sv2__dispatch__handshake_repeated__protocol_violation, sv2_channel_fixture)
{
    upgrade();
    send(setup_frame());
    BC_REQUIRE_EQUAL(receive(), success_frame());
    send(setup_frame());
    BC_REQUIRE_EQUAL(await(stopped), error::protocol_violation);
}

BC_FIXTURE_TEST_CASE(channel_sv2__dispatch__unregistered_message_type__unknown_message, sv2_channel_fixture)
{
    upgrade();
    send(base16_chunk("00007f000000"));
    BC_REQUIRE_EQUAL(await(stopped), error::unknown_message);
}

BC_FIXTURE_TEST_CASE(channel_sv2__dispatch__extension_message__discarded, sv2_channel_fixture)
{
    upgrade();
    send(base16_chunk("010001060000020000000000"));
    send(setup_frame());
    BC_REQUIRE(await(setup));
}

BC_FIXTURE_TEST_CASE(channel_sv2__receive__message_below_heading__invalid_heading, sv2_channel_fixture)
{
    upgrade();
    send(base16_chunk("000001"));
    BC_REQUIRE_EQUAL(await(stopped), error::invalid_heading);
}

BC_FIXTURE_TEST_CASE(channel_sv2__receive__length_above_message__invalid_heading, sv2_channel_fixture)
{
    upgrade();
    send(base16_chunk("0000010600000200000000"));
    BC_REQUIRE_EQUAL(await(stopped), error::invalid_heading);
}

BC_FIXTURE_TEST_CASE(channel_sv2__receive__two_frames_in_message__invalid_heading, sv2_channel_fixture)
{
    upgrade();
    send(base16_chunk("000001060000020000000000000001060000020000000000"));
    BC_REQUIRE_EQUAL(await(stopped), error::invalid_heading);
}

BC_FIXTURE_TEST_CASE(channel_sv2__receive__invalid_payload__invalid_message, sv2_channel_fixture)
{
    upgrade();
    send(base16_chunk("0000000100002a"));
    BC_REQUIRE_EQUAL(await(stopped), error::invalid_message);
}

BC_FIXTURE_TEST_CASE(channel_sv2__dispatch__request_without_upgrade__upgrade_required, sv2_channel_fixture)
{
    send_text("GET / HTTP/1.1\r\nHost: example.com\r\n\r\n");
    BC_REQUIRE(receive_status() == http::status::upgrade_required);
    BC_REQUIRE_EQUAL(await(stopped), error::channel_dropped);
}

BC_AUTO_TEST_SUITE_END()
