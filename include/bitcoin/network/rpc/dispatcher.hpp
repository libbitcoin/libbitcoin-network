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
#ifndef LIBBITCOIN_NETWORK_RPC_DISPATCHER_HPP
#define LIBBITCOIN_NETWORK_RPC_DISPATCHER_HPP

#include <tuple>
#include <unordered_map>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/rpc/method.hpp>
#include <bitcoin/network/rpc/types.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

/// Dispatches notifications to subscriber(s) of the method signature implied
/// by request. Subscribers and dispatch functors are fully defined at compile
/// time by the Interface template argument. The request_t parameter is the
/// request side of the rpc model. Requests are run-time generated (i.e. from
/// deserialization) and the request implies a signature that must match that
/// of one subscriber. Otherwise an error is returned from notify(request).
template <typename Interface>
class dispatcher
{
public:
    DELETE_COPY(dispatcher);
    DEFAULT_MOVE(dispatcher);

    /// If stopped, handler is invoked with error::subscriber_stopped.
    /// If key exists, handler is invoked with error::subscriber_exists.
    /// Otherwise handler retained. Subscription code is also returned here.
    template <typename Handler>
    inline code subscribe(Handler&& handler) NOEXCEPT;

    /// Create an instance of this class.
    inline dispatcher(asio::strand& strand) NOEXCEPT;
    virtual ~dispatcher() = default;

    /// Dispatch the request to the appropriate method's unsubscriber.
    virtual inline code notify(const request_t& request) NOEXCEPT;

    /// Stop all unsubscribers with the given code.
    virtual inline void stop(const code& ec) NOEXCEPT;

private:
    // make_subscribers
    // ------------------------------------------------------------------------

    using methods_t = typename Interface::type;

    template <typename Method>
    using subscriber_t = apply_t<Interface::template subscriber,
        args_t<Method>>;

    template <typename>
    struct subscribers;
    template <size_t ...Index>
    struct subscribers<std::index_sequence<Index...>>
    {
        using type = std::tuple<subscriber_t<
            std::tuple_element_t<Index, methods_t>>...>;
    };
    using subscribers_t = typename subscribers<std::make_index_sequence<
        Interface::size>>::type;

    template <typename Handler, size_t Index>
    static inline consteval bool subscriber_matches_handler() NOEXCEPT;

    template <typename Handler, size_t Index = zero>
    static inline consteval size_t find_subscriber_for_handler() NOEXCEPT;

    template <size_t ...Index>
    static inline subscribers_t make_subscribers(asio::strand& strand,
        std::index_sequence<Index...>) NOEXCEPT;

    // make_notifiers
    // ------------------------------------------------------------------------

    using parameters_t = params_option;
    using notifier_t = std::function<code(dispatcher&, const parameters_t&)>;
    using notifiers_t = std::unordered_map<std::string, notifier_t>;

    template <typename Argument>
    static inline external_t<Argument> get_required(
        const value_t& value) THROWS;
    template <typename Argument>
    static inline external_t<Argument> get_optional() THROWS;
    template <typename Argument>
    static inline external_t<Argument> get_nullable() THROWS;

    template <typename Argument>
    static inline external_t<Argument> get_positional(size_t& position,
        const array_t& array) THROWS;
    template <typename Argument>
    static inline external_t<Argument> get_named(
        const std::string_view& name, const object_t& object) THROWS;

    static inline array_t get_array(const parameters_t& params) THROWS;
    static inline object_t get_object(const parameters_t& params) THROWS;

    template <typename Arguments>
    static inline externals_t<Arguments> extract_positional(
        const parameters_t& params) THROWS;
    template <typename Arguments>
    static inline externals_t<Arguments> extract_named(
        const parameters_t& params, const names_t<Arguments>& names) THROWS;
    template <typename Arguments>
    static inline externals_t<Arguments> extract(
        const parameters_t& params, const names_t<Arguments>& names) THROWS;

    template <typename Method>
    static inline auto preamble() NOEXCEPT;
    template <typename Method>
    static inline code notify(subscriber_t<Method>& subscriber,
        const parameters_t& params, const names_t<Method>& names) NOEXCEPT;
    template <size_t Index>
    static inline code functor(dispatcher& self,
        const parameters_t& params) NOEXCEPT;
    template <size_t ...Index>
    static inline constexpr notifiers_t make_notifiers(
        std::index_sequence<Index...>) NOEXCEPT;

    /// Static map of handlers to functions.
    static const notifiers_t notifiers_;

    /// This is not thread safe.
    subscribers_t subscribers_;
};

} // namespace rpc
} // namespace network
} // namespace libbitcoin

#define TEMPLATE template <typename Interface>
#define CLASS dispatcher<Interface>

#include <bitcoin/network/impl/rpc/dispatcher.ipp>

#undef CLASS
#undef TEMPLATE

#endif
