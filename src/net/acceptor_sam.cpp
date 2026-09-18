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
#include <bitcoin/network/net/acceptor_sam.hpp>

#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/error.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/acceptor.hpp>
#include <bitcoin/network/net/connector.hpp>
#include <bitcoin/network/net/socket.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

using namespace system;
using namespace messages::peer;
using namespace std::placeholders;

// geti2p.net/en/docs/api/samv3
namespace sam
{
    // Lines are newline terminated, length bounded only as a memory guard.
    constexpr auto terminator = '\n';
    constexpr auto maximum_line = 65'536_size;

    // I2P has no ports.
    constexpr uint16_t port = 0;

    // Streams are forwarded to the loopback listener.
    constexpr auto forward_host = "127.0.0.1";

    // Credentials require version 3.2.
    constexpr auto version = "3.1";
    constexpr auto version_authenticated = "3.2";

    // Transient session creation (ed25519 destination).
    constexpr auto transient = "TRANSIENT";
    constexpr auto signature_type = "7";
    constexpr auto options = "i2cp.leaseSetEncType=4,0";

    // Reply tokens.
    constexpr auto hello = "HELLO";
    constexpr auto reply = "REPLY";
    constexpr auto session = "SESSION";
    constexpr auto stream = "STREAM";
    constexpr auto status = "STATUS";
    constexpr auto result = "RESULT";
    constexpr auto destination = "DESTINATION";

    // Result values.
    constexpr auto ok = "OK";
    constexpr auto no_version = "NOVERSION";
    constexpr auto cant_reach_peer = "CANT_REACH_PEER";
    constexpr auto duplicated_dest = "DUPLICATED_DEST";
    constexpr auto duplicated_id = "DUPLICATED_ID";
    constexpr auto i2p_error = "I2P_ERROR";
    constexpr auto invalid_id = "INVALID_ID";
    constexpr auto invalid_key = "INVALID_KEY";
    constexpr auto key_not_found = "KEY_NOT_FOUND";
    constexpr auto peer_not_found = "PEER_NOT_FOUND";
    constexpr auto timeout = "TIMEOUT";
}

// static
code acceptor_sam::sam_result(const std::string& value) NOEXCEPT
{
    if (value == sam::ok) return error::success;
    if (value == sam::no_version) return error::sam_no_version;
    if (value == sam::cant_reach_peer) return error::sam_cant_reach_peer;
    if (value == sam::duplicated_dest) return error::sam_duplicated_dest;
    if (value == sam::duplicated_id) return error::sam_duplicated_id;
    if (value == sam::i2p_error) return error::sam_i2p_error;
    if (value == sam::invalid_id) return error::sam_invalid_id;
    if (value == sam::invalid_key) return error::sam_invalid_key;
    if (value == sam::key_not_found) return error::sam_key_not_found;
    if (value == sam::peer_not_found) return error::sam_peer_not_found;
    if (value == sam::timeout) return error::sam_timeout;
    return error::sam_unassigned_failure;
}

// Utilities.
// ----------------------------------------------------------------------------

// Split a reply into tokens, retaining quoted values as single tokens.
static string_list to_tokens(const std::string& line) NOEXCEPT
{
    string_list tokens{};
    std::string token{};
    auto quoted = false;
    auto escaped = false;

    for (const auto character: line)
    {
        if (escaped)
        {
            token.push_back(character);
            escaped = false;
        }
        else if (quoted && character == '\\')
        {
            escaped = true;
        }
        else if (character == '"')
        {
            quoted = !quoted;
        }
        else if (character == ' ' && !quoted)
        {
            if (!token.empty())
                tokens.push_back(token);

            token.clear();
        }
        else
        {
            token.push_back(character);
        }
    }

    if (!token.empty())
        tokens.push_back(token);

    return tokens;
}

// The value of a key=value token, empty if the key is not present.
static std::string to_value(const string_list& tokens,
    const std::string& key) NOEXCEPT
{
    const auto prefix = key + "=";

    for (const auto& token: tokens)
        if (token.starts_with(prefix))
            return token.substr(prefix.length());

    return {};
}

// True if the first two tokens are as specified (e.g. HELLO REPLY).
static bool is_reply(const string_list& tokens, const std::string& major,
    const std::string& minor) NOEXCEPT
{
    return tokens.size() >= two && tokens.at(0) == major &&
        tokens.at(1) == minor;
}

// Values containing spaces must be quoted, with inner quotes escaped.
static std::string to_quoted(const std::string& value) NOEXCEPT
{
    return "\"" + replace_copy(value, "\"", "\\\"") + "\"";
}

// The i2p base64 alphabet substitutes -~ for +/ (padding is retained).
static std::string to_standard_base64(std::string i2p) NOEXCEPT
{
    std::replace(i2p.begin(), i2p.end(), '-', '+');
    std::replace(i2p.begin(), i2p.end(), '~', '/');
    return i2p;
}

static std::string to_i2p_base64(std::string standard) NOEXCEPT
{
    std::replace(standard.begin(), standard.end(), '+', '-');
    std::replace(standard.begin(), standard.end(), '/', '~');
    return standard;
}

// The address of a destination (sha256 of its serialization).
static config::address to_address(const data_chunk& destination) NOEXCEPT
{
    return address_item
    {
        unix_time(), service::node_none, i2p_t{ sha256_hash(destination) },
        sam::port
    };
}

static std::string to_session_id() NOEXCEPT
{
    data_array<8> entropy{};
    maybe_random::fill(entropy);
    return encode_base16(entropy);
}

// Requests.
// ----------------------------------------------------------------------------

static std::string to_hello(const std::string& username,
    const std::string& password) NOEXCEPT
{
    if (username.empty() && password.empty())
        return std::string{ "HELLO VERSION MIN=" } + sam::version +
            " MAX=" + sam::version + sam::terminator;

    return std::string{ "HELLO VERSION MIN=" } + sam::version_authenticated +
        " MAX=" + sam::version_authenticated +
        " USER=" + to_quoted(username) +
        " PASSWORD=" + to_quoted(password) + sam::terminator;
}

static std::string to_session_create(const std::string& id,
    const std::string& key) NOEXCEPT
{
    const auto destination = key.empty() ? std::string{ sam::transient } +
        " SIGNATURE_TYPE=" + sam::signature_type + " " + sam::options : key;

    return "SESSION CREATE STYLE=STREAM ID=" + id + " DESTINATION=" +
        destination + sam::terminator;
}

static std::string to_stream_forward(const std::string& id,
    uint16_t port) NOEXCEPT
{
    return "STREAM FORWARD ID=" + id + " PORT=" + system::serialize(port) +
        " HOST=" + sam::forward_host + " SILENT=false" + sam::terminator;
}

// Construct/start/stop.
// ----------------------------------------------------------------------------

acceptor_sam::acceptor_sam(const logger& log, asio::strand& strand,
    asio::context& service, std::atomic_bool& suspended,
    parameters&& parameters, const settings::sam& sam) NOEXCEPT
  : acceptor(log, strand, service, suspended, std::move(parameters)),
    sam_(sam),
    tracker<acceptor_sam>(log)
{
}

// The bridge is contacted upon accept, forwarding to the loopback listener.
code acceptor_sam::start(const config::authority&) NOEXCEPT
{
    const config::authority loopback
    {
        asio::address{ boost::asio::ip::address_v4::loopback() }, 0
    };

    if (const auto ec = acceptor::start(loopback))
        return ec;

    auto params = parameters_;
    connector_ = emplace_shared<connector>(log, strand_, service_,
        suspended_, std::move(params));

    return error::success;
}

void acceptor_sam::stop() NOEXCEPT
{
    BC_ASSERT(stranded());

    if (connector_)
        connector_->stop();

    do_teardown();
    acceptor::stop();
}

// Properties.
// ----------------------------------------------------------------------------

bool acceptor_sam::proxied() const NOEXCEPT
{
    return true;
}

// Methods.
// ----------------------------------------------------------------------------

void acceptor_sam::accept(socket_handler&& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (stopped_)
    {
        handler(error::service_stopped, nullptr);
        return;
    }

    if (suspended_.load())
    {
        handler(error::service_suspended, nullptr);
        return;
    }

    if (forward_)
    {
        acceptor::accept(std::move(handler));
        return;
    }

    // A partial establishment is torn down, the session is created afresh.
    do_teardown();
    id_ = to_session_id();

    connector_->connect(sam_.bridge,
        std::bind(&acceptor_sam::handle_session_connect,
            shared_from_base<acceptor_sam>(), _1, _2, std::move(handler)));
}

// sam line protocol
// ----------------------------------------------------------------------------

// Write the request and then read the reply line.
void acceptor_sam::do_sam_request(const socket::ptr& socket,
    const std::string& request, line_handler&& handler) NOEXCEPT
{
    const auto out = emplace_shared<std::string>(request);

    socket->tcp_write({ out->data(), out->size() },
        std::bind(&acceptor_sam::handle_sam_write,
            shared_from_base<acceptor_sam>(),
                _1, _2, socket, out, std::move(handler)));
}

void acceptor_sam::handle_sam_write(const code& ec, size_t size,
    const socket::ptr& socket, const line_ptr& request,
    const line_handler& handler) NOEXCEPT
{
    BC_ASSERT(socket->stranded());

    if (const auto result = (socket->stopped() ? error::channel_stopped : ec))
    {
        handler(result, socket, {});
        return;
    }

    if (size != request->size())
    {
        handler(error::operation_failed, socket, {});
        return;
    }

    do_sam_read(socket, emplace_shared<std::string>(), handler);
}

// Read one byte at a time, as bytes following the line belong to the peer.
void acceptor_sam::do_sam_read(const socket::ptr& socket,
    const line_ptr& line, const line_handler& handler) NOEXCEPT
{
    const auto byte = emplace_shared<std::array<char, 1>>();

    socket->tcp_read({ byte->data(), byte->size() },
        std::bind(&acceptor_sam::handle_sam_read,
            shared_from_base<acceptor_sam>(),
                _1, _2, socket, byte, line, handler));
}

void acceptor_sam::handle_sam_read(const code& ec, size_t size,
    const socket::ptr& socket, const char_ptr& byte, const line_ptr& line,
    const line_handler& handler) NOEXCEPT
{
    BC_ASSERT(socket->stranded());

    if (const auto result = (socket->stopped() ? error::channel_stopped : ec))
    {
        handler(result, socket, {});
        return;
    }

    if (size != byte->size())
    {
        handler(error::operation_failed, socket, {});
        return;
    }

    if (line->size() >= sam::maximum_line)
    {
        handler(error::sam_response_invalid, socket, {});
        return;
    }

    if (byte->front() != sam::terminator)
    {
        line->push_back(byte->front());
        do_sam_read(socket, line, handler);
        return;
    }

    handler(error::success, socket, line);
}

// sam session (control socket)
// ----------------------------------------------------------------------------

void acceptor_sam::handle_session_connect(const code& ec,
    const socket::ptr& socket, const socket_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (ec)
    {
        handler(ec, nullptr);
        return;
    }

    do_sam_request(socket, to_hello(sam_.username, sam_.password),
        std::bind(&acceptor_sam::handle_session_hello,
            shared_from_base<acceptor_sam>(), _1, _2, _3, handler));
}

void acceptor_sam::handle_session_hello(const code& ec,
    const socket::ptr& socket, const line_ptr& line,
    const socket_handler& handler) NOEXCEPT
{
    BC_ASSERT(socket->stranded());

    if (ec)
    {
        acceptor::handle_accept(ec, socket, handler);
        return;
    }

    const auto tokens = to_tokens(*line);
    if (!is_reply(tokens, sam::hello, sam::reply))
    {
        acceptor::handle_accept(error::sam_response_invalid, socket, handler);
        return;
    }

    if (const auto result = sam_result(to_value(tokens, sam::result)))
    {
        acceptor::handle_accept(result, socket, handler);
        return;
    }

    // A stored key is base64, the bridge expects the i2p base64 alphabet.
    const auto transient = sam_.key.empty();

    do_sam_request(socket, to_session_create(id_, to_i2p_base64(sam_.key)),
        std::bind(&acceptor_sam::handle_session_create,
            shared_from_base<acceptor_sam>(), _1, _2, _3, transient, handler));
}

void acceptor_sam::handle_session_create(const code& ec,
    const socket::ptr& socket, const line_ptr& line, bool transient,
    const socket_handler& handler) NOEXCEPT
{
    BC_ASSERT(socket->stranded());

    if (ec)
    {
        acceptor::handle_accept(ec, socket, handler);
        return;
    }

    const auto tokens = to_tokens(*line);
    if (!is_reply(tokens, sam::session, sam::status))
    {
        acceptor::handle_accept(error::sam_response_invalid, socket, handler);
        return;
    }

    if (const auto result = sam_result(to_value(tokens, sam::result)))
    {
        acceptor::handle_accept(result, socket, handler);
        return;
    }

    // The reply destination is the private key (created or as provided).
    const auto key = to_standard_base64(to_value(tokens, sam::destination));
    if (!settings::sam::to_self(key))
    {
        acceptor::handle_accept(error::sam_invalid_key, socket, handler);
        return;
    }

    // A transient destination is persisted so that the address is retained.
    if (transient && !save_key(key))
    {
        acceptor::handle_accept(error::file_save, socket, handler);
        return;
    }

    boost::asio::post(strand_,
        std::bind(&acceptor_sam::do_session_ready,
            shared_from_base<acceptor_sam>(), socket, handler));
}

void acceptor_sam::do_session_ready(const socket::ptr& socket,
    const socket_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (stopped_)
    {
        socket->stop();
        handler(error::service_stopped, nullptr);
        return;
    }

    session_ = socket;

    // The bridge closes the control socket when the session is destroyed.
    session_->watch(
        std::bind(&acceptor_sam::handle_close,
            shared_from_base<acceptor_sam>(), _1));

    connector_->connect(sam_.bridge,
        std::bind(&acceptor_sam::handle_forward_connect,
            shared_from_base<acceptor_sam>(), _1, _2, handler));
}

// sam forward (control socket)
// ----------------------------------------------------------------------------

void acceptor_sam::handle_forward_connect(const code& ec,
    const socket::ptr& socket, const socket_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (ec)
    {
        do_teardown();
        handler(ec, nullptr);
        return;
    }

    do_sam_request(socket, to_hello(sam_.username, sam_.password),
        std::bind(&acceptor_sam::handle_forward_hello,
            shared_from_base<acceptor_sam>(), _1, _2, _3, local().port(),
                handler));
}

void acceptor_sam::handle_forward_hello(const code& ec,
    const socket::ptr& socket, const line_ptr& line, uint16_t port,
    const socket_handler& handler) NOEXCEPT
{
    BC_ASSERT(socket->stranded());

    if (ec)
    {
        acceptor::handle_accept(ec, socket, handler);
        return;
    }

    const auto tokens = to_tokens(*line);
    if (!is_reply(tokens, sam::hello, sam::reply))
    {
        acceptor::handle_accept(error::sam_response_invalid, socket, handler);
        return;
    }

    if (const auto result = sam_result(to_value(tokens, sam::result)))
    {
        acceptor::handle_accept(result, socket, handler);
        return;
    }

    do_sam_request(socket, to_stream_forward(id_, port),
        std::bind(&acceptor_sam::handle_forward_status,
            shared_from_base<acceptor_sam>(), _1, _2, _3, handler));
}

void acceptor_sam::handle_forward_status(const code& ec,
    const socket::ptr& socket, const line_ptr& line,
    const socket_handler& handler) NOEXCEPT
{
    BC_ASSERT(socket->stranded());

    if (ec)
    {
        acceptor::handle_accept(ec, socket, handler);
        return;
    }

    const auto tokens = to_tokens(*line);
    if (!is_reply(tokens, sam::stream, sam::status))
    {
        acceptor::handle_accept(error::sam_response_invalid, socket, handler);
        return;
    }

    if (const auto result = sam_result(to_value(tokens, sam::result)))
    {
        acceptor::handle_accept(result, socket, handler);
        return;
    }

    boost::asio::post(strand_,
        std::bind(&acceptor_sam::do_forward_ready,
            shared_from_base<acceptor_sam>(), socket, handler));
}

void acceptor_sam::do_forward_ready(const socket::ptr& socket,
    const socket_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (stopped_)
    {
        socket->stop();
        handler(error::service_stopped, nullptr);
        return;
    }

    forward_ = socket;
    LOGN("Forwarding sam session [" << sam_.self << "] to [" << local() << "].");

    // The bridge closes the control socket when forwarding is stopped.
    forward_->watch(
        std::bind(&acceptor_sam::handle_close,
            shared_from_base<acceptor_sam>(), _1));

    acceptor::accept(socket_handler{ handler });
}

// sam teardown
// ----------------------------------------------------------------------------

void acceptor_sam::handle_close(const code& ec) NOEXCEPT
{
    boost::asio::post(strand_,
        std::bind(&acceptor_sam::do_close,
            shared_from_base<acceptor_sam>(), ec));
}

// The session is recreated (with the same key) upon the next accept.
void acceptor_sam::do_close(const code& LOG_ONLY(ec)) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (!session_ && !forward_)
        return;

    LOGN("Closed sam session [" << sam_.self << "] " << ec.message());
    do_teardown();
}

void acceptor_sam::do_teardown() NOEXCEPT
{
    BC_ASSERT(stranded());

    if (forward_)
        forward_->stop();

    if (session_)
        session_->stop();

    forward_.reset();
    session_.reset();
}

// sam accept (forwarded data socket)
// ----------------------------------------------------------------------------

void acceptor_sam::handle_accept(const code& ec, const socket::ptr& socket,
    const socket_handler& handler) NOEXCEPT
{
    if (ec)
    {
        acceptor::handle_accept(ec, socket, handler);
        return;
    }

    // The peer destination is the first line of each forwarded stream.
    do_sam_read(socket, emplace_shared<std::string>(),
        std::bind(&acceptor_sam::handle_peer,
            shared_from_base<acceptor_sam>(), _1, _2, _3, handler));
}

void acceptor_sam::handle_peer(const code& ec, const socket::ptr& socket,
    const line_ptr& line, const socket_handler& handler) NOEXCEPT
{
    BC_ASSERT(socket->stranded());

    if (ec)
    {
        acceptor::handle_accept(ec, socket, handler);
        return;
    }

    data_chunk destination{};
    const auto tokens = to_tokens(*line);
    if (tokens.empty() ||
        !decode_base64(destination, to_standard_base64(tokens.front())))
    {
        acceptor::handle_accept(error::sam_response_invalid, socket, handler);
        return;
    }

    socket->set_address(to_address(destination));

    // The handshake is deferred by the proxied socket until peer identification.
    socket->handshake(
        std::bind(&acceptor_sam::handle_handshake,
            shared_from_base<acceptor_sam>(), _1, socket, handler));
}

void acceptor_sam::handle_handshake(const code& ec,
    const socket::ptr& socket, const socket_handler& handler) NOEXCEPT
{
    acceptor::handle_accept(ec, socket, handler);
}

// key persistence
// ----------------------------------------------------------------------------

bool acceptor_sam::save_key(const std::string& in) const NOEXCEPT
{
    if (sam_.key_path.empty())
        return true;

    try
    {
        ofstream file{ sam_.key_path, ofstream::out };
        if (!file.good())
            return false;

        file << in << std::endl;
        return !file.bad();
    }
    catch (const std::exception&)
    {
        return false;
    }
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
