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
#ifndef LIBBITCOIN_NETWORK_DISTRIBUTORS_DISTRIBUTOR_RPC_HPP
#define LIBBITCOIN_NETWORK_DISTRIBUTORS_DISTRIBUTOR_RPC_HPP

#include <tuple>
#include <unordered_map>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/distributors/distributor.hpp>
#include <bitcoin/network/messages/rpc/rpc.hpp>

namespace libbitcoin {
namespace network {

/// Not thread safe.
template <typename Interface>
class distributor_rpc
{
public:
    DELETE_COPY(distributor_rpc);
    DEFAULT_MOVE(distributor_rpc);

    /// If stopped, handler is invoked with error::subscriber_stopped.
    /// If key exists, handler is invoked with error::subscriber_exists.
    /// Otherwise handler retained. Subscription code is also returned here.
    template <typename Handler>
    inline code subscribe(Handler&& handler) NOEXCEPT;

    /// Create an instance of this class.
    inline distributor_rpc(asio::strand& strand) NOEXCEPT;
    virtual ~distributor_rpc() = default;

    /// Dispatch the request to the appropriate method's unsubscriber.
    virtual inline code notify(const rpc::request_t& request) NOEXCEPT;

    /// Stop all unsubscribers with the given code.
    virtual inline void stop(const code& ec) NOEXCEPT;

private:
    // make_subscribers
    // ------------------------------------------------------------------------

    template <typename Method>
    struct subscriber_type;

    template <text_t Text, typename ...Args>
    struct subscriber_type<rpc::method<Text, Args...>>
    {
        using tag = typename rpc::method<Text, Args...>::tag;
        using type = network::unsubscriber<tag, Args...>;
    };

    template <typename Method>
    using subscriber_t = typename subscriber_type<Method>::type;

    template <typename Method>
    struct subscribers_type;

    template <typename ...Method>
    struct subscribers_type<std::tuple<Method...>>
    {
        using type = std::tuple<subscriber_t<Method>...>;
    };

    using methods_t = std::remove_const_t<typename Interface::type>;
    using subscribers_t = typename subscribers_type<methods_t>::type;

    template <size_t Index, typename Handler>
    static inline consteval bool is_handler_type() NOEXCEPT;

    template <typename Tag, size_t Index = zero>
    static inline consteval size_t find_handler_index() NOEXCEPT;

    template <size_t ...Index>
    static inline subscribers_t make_subscribers(asio::strand& strand,
        std::index_sequence<Index...>) NOEXCEPT;

    // make_notifiers
    // ------------------------------------------------------------------------

    using optional_t = rpc::params_option;
    using notifier_t = std::function<code(distributor_rpc&, const optional_t&)>;
    using notifiers_t = std::unordered_map<std::string, notifier_t>;

    template <typename Argument>
    static inline rpc::external_t<Argument> get_required(
        const rpc::value_t& value) THROWS;
    template <typename Argument>
    static inline rpc::external_t<Argument> get_optional() THROWS;
    template <typename Argument>
    static inline rpc::external_t<Argument> get_nullable() THROWS;

    template <typename Argument>
    static inline rpc::external_t<Argument> get_positional(size_t& position,
        const rpc::array_t& array) THROWS;
    template <typename Argument>
    static inline rpc::external_t<Argument> get_named(
        const std::string_view& name, const rpc::object_t& object) THROWS;

    static inline rpc::array_t get_array(const optional_t& params) THROWS;
    static inline rpc::object_t get_object(const optional_t& params) THROWS;

    template <typename Arguments>
    static inline Arguments extract_positional(
        const optional_t& params) THROWS;
    template <typename Arguments>
    static inline Arguments extract_named(const optional_t& params,
        const rpc::names_t<Arguments>& names) THROWS;

    static inline void require_empty(const optional_t& params) THROWS;
    template <typename Arguments>
    static inline Arguments extract(const optional_t& params,
        const rpc::names_t<Arguments>& names) THROWS;

    template <typename Method>
    static inline code notify(subscriber_t<Method>& subscriber,
        const optional_t& params, const rpc::names_t<Method>& names) NOEXCEPT;
    template <size_t Index>
    static inline code notifier(distributor_rpc& self,
        const optional_t& params) NOEXCEPT;
    template <size_t ...Index>
    static inline constexpr notifiers_t make_notifiers(
        std::index_sequence<Index...>) NOEXCEPT;

    /// Static map of handlers to functions.
    static const notifiers_t notifiers_;

    /// This is not thread safe.
    subscribers_t subscribers_;
};

} // namespace network
} // namespace libbitcoin

#define TEMPLATE template <typename Interface>
#define CLASS distributor_rpc<Interface>

#include <bitcoin/network/impl/distributors/distributor_rpc.ipp>

#undef CLASS
#undef TEMPLATE

#endif
