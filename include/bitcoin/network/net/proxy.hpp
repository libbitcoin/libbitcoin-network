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

/// This class is not thread safe:
/// pause/resume/paused should only be called from channel strand.
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

    template <class Message>
    void send(typename Message::ptr message, const result_handler& complete)
    {
        using namespace messages;
        send_bytes(serialize(*message, protocol_magic(), version()), complete);
    }

    virtual void stop(const code& ec);

    virtual void pause();
    virtual void resume();
    bool paused() const;

    void subscribe_stop(result_handler handler, result_handler complete);
    bool stopped() const;

    bool stranded() const;
    asio::strand& strand();
    const config::authority& authority() const;

protected:
    static std::string extract_command(const system::chunk_ptr& payload);

    // Protocols may subscribe<Message> and subscribe_stop from strand.
    friend class protocol;

    void subscribe_stop(result_handler handler);
    template <class Message, typename Handler = pump::handler<Message>>
    void subscribe(Handler&& handler)
    {
        BC_ASSERT_MSG(stranded(), "strand");
        pump_subscriber_.subscribe(std::forward<Handler>(handler));
    }

    proxy(socket::ptr socket);
    virtual ~proxy();

    virtual size_t maximum_payload() const = 0;
    virtual uint32_t protocol_magic() const = 0;
    virtual bool validate_checksum() const = 0;
    virtual bool verbose() const = 0;
    virtual uint32_t version() const = 0;
    virtual void signal_activity() = 0;

    virtual void send_bytes(system::chunk_ptr payload,
        result_handler&& handler);
    virtual void send_bytes(system::chunk_ptr payload,
        const result_handler& handler);
    virtual code notify(messages::identifier id, uint32_t version,
        system::reader& source);

private:
    typedef messages::heading::ptr heading_ptr;

    void do_stop(const code& ec);
    void do_subscribe_stop(result_handler handler, result_handler complete);

    void read_heading();
    void handle_read_heading(const code& ec, size_t heading_size);
    void handle_read_payload(const code& ec, size_t payload_size,
        const heading_ptr& head);
    void handle_send(const code& ec, size_t bytes, system::chunk_ptr payload,
        const result_handler& handler);

    // This is thread safe.
    socket::ptr socket_;

    // This is not thread safe.
    bool paused_;

    // These are protected by the strand.
    pump pump_subscriber_;
    stop_subscriber::ptr stop_subscriber_;

    // These are protected by read header/payload ordering (strand).
    system::data_chunk payload_buffer_;
    system::data_array<messages::heading::size()> heading_buffer_;
    system::read::bytes::copy heading_reader_;
};

} // namespace network
} // namespace libbitcoin

#endif
