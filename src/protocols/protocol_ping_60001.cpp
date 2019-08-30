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
#include <bitcoin/network/protocols/protocol_ping_60001.hpp>

#include <cstdint>
#include <functional>
#include <string>
#include <bitcoin/system.hpp>
#include <bitcoin/network/channel.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/p2p.hpp>
#include <bitcoin/network/protocols/protocol_ping_31402.hpp>
#include <bitcoin/network/protocols/protocol_timer.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_ping_60001

using namespace bc::system;
using namespace bc::system::message;
using namespace std::placeholders;

protocol_ping_60001::protocol_ping_60001(p2p& network, channel::ptr channel)
  : protocol_ping_31402(network, channel),
    nonce(0),
    pending_(0),
    CONSTRUCT_TRACK(protocol_ping_60001)
{
}

// This is fired by the callback (i.e. base timer and stop handler).
void protocol_ping_60001::send_ping(const code& ec)
{
    LOG_DEBUG(LOG_NETWORK)
    << "send_ping callback on channel with [" << authority() << "]";

    if (stopped(ec))
        return;

    if (ec && ec != error::channel_timeout)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failure in protocol_ping_60001 timer for [" << authority() << "] "
            << ec.message();
        stop(ec);
        return;
    }

    // active_ is set to true on each inbound message received, so we stay connected; some
    // peers send frequent periodic ping (e.g. original recommendation was every 2 mins)
    if (!channel_->active())
    {
        // pending_ means we have already sent a ping and we're waiting for the pong reply
        // if pending_ then policy is don't burden the overloaded peer with new messages.
        // but don't disconnect just because they have a single-minded message queue loop.
        // do disconnect if channel has been inactive for length of two full ping timers.
        // see https://github.com/bitcoin/bitcoin/pull/932/files

        // pending_ is set to 1 in handle_send_ping() after a ping has been sent on the wire following the preflight queue state
        if (pending_ > 0)
        {
            // Bitcoin Core will, by default, disconnect from any clients which have not responded to a ping message within 20 minutes.
            // see https://bitcoin.org/en/developer-reference#pong
            // here we determine whether ping response time limit has been exceeded, and give up on the peer if yes.
            // the ping response time limit was retrieved (in protocol_ping_31402 base clase) by calling settings_.channel_heartbeat()
            // ping timeout config file setting is channel_heartbeat_minutes
            if (pending_ == 2)
            {
                LOG_DEBUG(LOG_NETWORK)
                    << "Ping latency limit exceeded on inactive channel [" << authority() << "]";
                stop(error::channel_timeout);
                return;
            }
            else
            {
                // we want to wait for one complete channel heartbeat, plus time already elapsed since callback handle_send_ping occurred
                // when channel_heartbeat_minutes == 5 this results in minimum of 5 minutes, maximum of 10 minutes for ping latency timeout.
                // now that first channel heartbeat has occurred subsequent to handle_send_ping we trigger latency timeout on next heartbeat
                pending_ = 2;
                
                LOG_DEBUG(LOG_NETWORK)
                << "protocol_ping_60001 pending_ now = 2";
            }
        }
        else if (pending_ == -1)
        {
            pending_ = 0; // on the next heartbeat callback, queue another ping message.
        }
        else
        {
            // nonzero nonce serves as preflight indicator, when a ping is waiting in the queue, started but not yet sent.
            // note: BIP31 recommends sending zero nonce if the peer is never going to send multiple overlapping pings.
            // libbitcoin may not send overlapping pings today, but this code is ready to be used in that way in the future.
            if (nonce == 0)
            {
                while (nonce == 0)
                {
                    nonce = pseudo_random::next();
                }
                
                LOG_DEBUG(LOG_NETWORK)
                << "protocol_ping_60001 nonce now = " << nonce;
                
                SUBSCRIBE3(pong, handle_receive_pong, _1, _2, nonce);
                SEND2(ping{ nonce }, handle_send_ping, _1, ping::command);
            }
            else
            {
                // many channel heartbeats may occur during the preflight state, while waiting for a queued ping to be sent such as
                // in the condition where a channel is receiving a large number of blocks from a peer, typically during first sync.
                // this is not a failure condition, because channel_inactivity_minutes and channel_expiration_minutes also govern.
                // however, excessive preflight delay might be unwanted, particularly if the peer is providing useless fork blocks
                // or unwanted transactions that won't remain canonical. a Byzantine peer seeking to cause a DoS condition may be
                // contributing to preflight delay, but we allow this because we queue outgoing while processing incoming messages.
                // disconnecting honest peers because of preflight delay should be avoided, so we allow channel_expiration_minutes
                // before terminating a busy channel that is so filled with incoming messages that a ping message can never be sent.
                // obviously, a Byzantine peer would simply respond with a pong in any case, if our ping message ever did get sent.
                // thus better detection of unwanted low-quality messages coming from a Byzantine peer is needed here in the future.
                
                LOG_DEBUG(LOG_NETWORK)
                << "protocol_ping_60001 heartbeat callback, nonzero nonce, still = " << nonce;
            }
        }
    }
    else
    {
        // activity on the channel before timeout will set this flag to true again
        channel_->set_active(false);
    }
}

void protocol_ping_60001::handle_send_ping(const code& ec,
    const std::string&)
{
    LOG_DEBUG(LOG_NETWORK)
    << "handle_send_ping callback on channel with [" << authority() << "]";

    if (stopped(ec))
        return;

    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failure sending ping to [" << authority() << "] "
            << ec.message();
        stop(ec);
        return;
    }

    channel_->set_ponged(false);

    LOG_DEBUG(LOG_NETWORK)
    << "protocol_ping_60001 pending_ now = 1";
    
    pending_ = 1;
}

bool protocol_ping_60001::handle_receive_ping(const code& ec,
    ping_const_ptr message)
{
    LOG_DEBUG(LOG_NETWORK)
    << "handle_receive_ping callback on channel with [" << authority() << "]";

    if (stopped(ec))
        return false;

    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failure getting ping from [" << authority() << "] "
            << ec.message();
        stop(ec);
        return false;
    }

    SEND2(pong{ message->nonce() }, handle_send_pong, _1, pong::command);

    channel_->set_pinged(true);

    // RESUBSCRIBE
    return true;
}

void protocol_ping_60001::handle_send_pong(const code& ec,
                                        const std::string&)
{
    LOG_DEBUG(LOG_NETWORK)
    << "handle_send_pong callback on channel with [" << authority() << "]";
        
    if (stopped(ec))
        return;
        
    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
        << "Failure sending pong to [" << authority() << "] "
        << ec.message();
        stop(ec);
        return;
    }

    // TODO: this isn't correct if we have overlapped inbound pings pending on the channel
    channel_->set_pinged(false);

    // active channels don't need to be pinged, nor disconnected due to ping timeout
    // if we have sent a pong already without first sending a ping, that's unexpected.
    // sending a pong before we send a ping will probably result in no ping being sent
    // until and unless the channel becomes inactive. when first created the inactive
    // state of the channel is intended to cause a ping to send at first timer event.
    channel_->set_active(true);
}

bool protocol_ping_60001::handle_receive_pong(const code& ec,
    pong_const_ptr message, uint64_t nonce_in)
{
    LOG_DEBUG(LOG_NETWORK)
    << "handle_receive_pong callback on channel with [" << authority() << "]";

    if (stopped(ec))
        return false;

    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failure getting pong from [" << authority() << "] "
            << ec.message();
        stop(ec);
        return false;
    }

    if (message->nonce() != nonce_in)
    {
        // TODO: this presumes that the peer will always reply to pings in the sequence
        // they were sent. that's not necessarily true, and we should be willing to accept
        // any of the nonces we have sent out, if we do send out overlapping pings.
        // NOTE: with the latest revision to this code we no longer send overlapped pings.
        LOG_WARNING(LOG_NETWORK)
            << "Invalid pong nonce = " << message->nonce() << " expecting " << nonce_in << " from [" << authority() << "]";
        stop(error::bad_stream);
        return false;
    }
    else
    {
        LOG_WARNING(LOG_NETWORK)
        << "Received valid pong nonce " << nonce_in << " from [" << authority() << "]";
    }

    channel_->set_ponged(true);
    nonce = 0; // pong received, ready to preflight another ping
    pending_ = -1; // don't spam the peer with unnecessry pings. skip one channel heartbeat before we queue a new ping message.
    
    LOG_DEBUG(LOG_NETWORK)
    << "protocol_ping_60001 reset nonce, now = " << nonce;

    // do not RESUBSCRIBE because each handler functor contains the pong's unique nonce.
    // false return value from SUBSCRIBE3'd handler tells resubscriber not to resubscribe.
    // see: do_invoke() in resubscriber.ipp
    return false;
}

} // namespace network
} // namespace libbitcoin
