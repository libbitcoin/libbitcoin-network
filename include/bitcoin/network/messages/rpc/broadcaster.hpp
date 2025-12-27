/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_RPC_BROADCASTER_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_RPC_BROADCASTER_HPP

#include <tuple>
#include <unordered_map>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/rpc/dispatcher.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

/// Broadcasts notifications to subscriber(s) of the method signature implied
/// by request. Adds a notify_one(key, ...) method for targeted desubscribe.
template <typename Interface>
class broadcaster
  : public dispatcher<Interface>
{
public:
    /// Interface requires key type when using desubscriber.
    using key_t = typename Interface::key;

    /// Create an instance of this class.
    inline broadcaster() NOEXCEPT = default;
    virtual ~broadcaster() = default;

    /// Dispatch request, optionally to key-subscribed method handler(s).
    /// Success if subscription key found (subscribed). Requires desubscriber.
    inline code notify(const request_t& request) NOEXCEPT override;
    virtual inline code notify(const request_t& request,
        const key_t& key) NOEXCEPT;

    /// Unsubscribe key-subscribed method handler(s) from all subscribers.
    virtual inline void unsubscribe(const key_t& key) NOEXCEPT;

private:
    // make_notifiers
    // ------------------------------------------------------------------------
    using base = dispatcher<Interface>;

    template <typename Method>
    using subscriber_t = base::template subscriber_t<Method>;
    using methods_t = base::methods_t;
    using parameters_t = params_option;
    using notifier_t = std::function<code(broadcaster&, const key_t&,
        const parameters_t&)>;
    using notifiers_t = std::unordered_map<std::string, notifier_t>;

    template <typename Method>
    static inline code notify_one(subscriber_t<Method>& subscriber,
        const key_t& key, const parameters_t& params,
        const names_t<Method>& names) NOEXCEPT;
    template <size_t Index>
    static inline code one_functor(broadcaster& self, const key_t& key,
        const parameters_t& params) NOEXCEPT;
    template <size_t ...Index>
    static inline constexpr notifiers_t make_one_notifiers(
        std::index_sequence<Index...>) NOEXCEPT;

    // desubscriber
    // ------------------------------------------------------------------------

    template <typename Method>
    static inline void notify_defaults(subscriber_t<Method>& subscriber,
        const key_t& key, const code& ec) NOEXCEPT;
    template <size_t Index>
    inline void desubscriber(const key_t& key) NOEXCEPT;
    template <size_t ...Index>
    inline void desubscribe(const key_t& key,
        std::index_sequence<Index...>) NOEXCEPT;

protected:
    /// Static map of handlers to one_functors.
    static const notifiers_t one_notifiers_;
};

} // namespace rpc
} // namespace network
} // namespace libbitcoin

#define TEMPLATE template <typename Interface>
#define CLASS broadcaster<Interface>

#include <bitcoin/network/impl/messages/rpc/broadcaster.ipp>

#undef CLASS
#undef TEMPLATE

#endif
