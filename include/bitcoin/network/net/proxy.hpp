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
#ifndef LIBBITCOIN_NETWORK_NET_PROXY_HPP
#define LIBBITCOIN_NETWORK_NET_PROXY_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/pump.hpp>
#include <bitcoin/network/net/socket.hpp>

namespace libbitcoin {
namespace network {

/// Thread safe.
/// Virtual base for all channel communication, error handling and logging.
class BCT_API proxy
  : public enable_shared_from_base<proxy>, system::noncopyable
{
public:
    typedef std::shared_ptr<proxy> ptr;
    typedef subscriber<code> stop_subscriber;
    typedef std::function<void(const code&)> result_handler;

    template <class Message>
    void send(typename Message::ptr message, result_handler&& complete)
    {
        using namespace messages;
        send_bytes(serialize(*message, protocol_magic(), version()),
            std::move(complete));
    }

    template <class Message, typename Handler = pump::handler<Message>>
    void subscribe(Handler&& handler, result_handler&& complete)
    {
        const auto self = shared_from_this();

        // C++14: std::move handlers into closure.
        boost::asio::post(strand(), [=]()
        {
            self->pump_subscriber_.subscribe(std::move(handler));
            complete(error::success);
        });
    }

    virtual void start();
    virtual void stop(const code& ec);

    bool stopped() const;
    void subscribe_stop(result_handler&& handler, result_handler&& complete);

    bool stranded() const;
    asio::strand& strand();
    const config::authority& authority() const;

protected:
    proxy(socket::ptr socket);
    virtual ~proxy();

    virtual size_t maximum_payload() const = 0;
    virtual uint32_t protocol_magic() const = 0;
    virtual bool validate_checksum() const = 0;
    virtual bool verbose() const = 0;
    virtual uint32_t version() const = 0;
    virtual void signal_activity() = 0;

    virtual void send_bytes(system::chunk_ptr payload, result_handler&& handler);
    virtual code notify(messages::identifier id, uint32_t version,
        system::reader& source);

private:
    typedef messages::heading::ptr heading_ptr;

    static std::string extract_command(system::chunk_ptr payload);

    void do_stop(const code& ec);
    void do_subscribe_stop(result_handler handler, result_handler complete);

    void read_heading();
    void handle_read_heading(const code& ec, size_t heading_size);
    void handle_read_payload(const code& ec, size_t payload_size,
        heading_ptr head);
    void handle_send(const code& ec, size_t bytes, system::chunk_ptr payload,
        const result_handler& handler);

    // This is thread safe.
    socket::ptr socket_;

    // These are protected by the strand.
    pump pump_subscriber_;
    stop_subscriber::ptr stop_subscriber_;

    // These are protected by read header/payload ordering.
    system::data_chunk payload_buffer_;
    system::data_array<messages::heading::size()> heading_buffer_;
    system::read::bytes::copy heading_reader_;
};

} // namespace network
} // namespace libbitcoin

#endif
