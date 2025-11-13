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
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/distributors/distributor.hpp>
#include <bitcoin/network/messages/json/json.hpp>

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

    template <rpc::method_name Unique, typename... Args>
    struct subscriber_type<rpc::method<Unique, Args...>>
    {
        // The subscriber signature/handler includes the tag.
        using tag = typename rpc::method<Unique, Args...>::tag;
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

    template <size_t ...Index>
    static inline subscribers_t make_subscribers(asio::strand& strand,
        std::index_sequence<Index...>) NOEXCEPT;

    subscribers_t subscribers_;

    // make_dispatchers
    // ------------------------------------------------------------------------

    template <typename Type>
    using names_t = typename rpc::parameter_names<Type>::type;

    template <typename Method>
    using args_t = typename Method::args;

    template <typename Method>
    using tag_t = typename Method::tag;

    template <size_t Index, typename Methods>
    using method_t = std::tuple_element_t<Index, methods_t>;

    template <size_t Size>
    using sequence_t = std::make_index_sequence<Size>;

    using optional_t = rpc::params_option;
    using functor_t = std::function<code(distributor_rpc&, const optional_t&)>;
    using dispatch_t = std::unordered_map<std::string, functor_t>;

    static inline bool has_params(const optional_t& parameters) NOEXCEPT;

    template <typename Type>
    static inline Type extract(const rpc::value_t& value) THROWS;

    template <typename Arguments>
    static inline Arguments extractor(const optional_t& parameters,
        const names_t<Arguments>& names) THROWS;

    template <typename Method>
    static inline code notifier(subscriber_t<Method>& subscriber,
        const optional_t& parameters, const names_t<Method>& names) NOEXCEPT;

    template <size_t Index>
    static inline code do_notify(distributor_rpc& self,
        const optional_t& parameters) NOEXCEPT;

    template <size_t ...Index>
    static inline constexpr dispatch_t make_dispatchers(
        std::index_sequence<Index...>) NOEXCEPT;

    static const dispatch_t dispatch_;

    // find_tag_index
    // ------------------------------------------------------------------------

    template <typename Handler, typename = void>
    struct traits;

    template <typename Handler>
    struct traits<Handler, std::void_t<decltype(&Handler::operator())>>
      : traits<decltype(&Handler::operator())> {};

    template <typename Return, typename Class, typename Tag, typename ...Args>
    struct traits<Return (Class::*)(const code&, Tag, Args...) const NOEXCEPT>
    {
        using tag = Tag;
        using args = std::tuple<Args...>;
    };
    
    template <typename Methods, typename Tag, size_t Index = zero>
    static inline constexpr size_t find_tag_index() NOEXCEPT;
};

} // namespace network
} // namespace libbitcoin

#define TEMPLATE template <typename Interface>
#define CLASS distributor_rpc<Interface>

#include <bitcoin/network/impl/distributors/distributor_rpc.ipp>

#undef CLASS
#undef TEMPLATE

#endif
