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
#include <bitcoin/network/messages/json/json.hpp>

namespace libbitcoin {
namespace network {

// do_notify_[name] is a closure over container mode and names tuple. It
/// is static so the dispatch map can be statically defined, which in turn
/// requires self referential invocation for dispatch to the unsubscriber.
#define SUBSCRIBER(name) name##_subscriber_
#define SUBSCRIBER_TYPE(name) name##_subscriber
#define NAMES(...) std::make_tuple(__VA_ARGS__)
#define DECLARE_SUBSCRIBER(name, mode, tuple, ...) \
public: \
    struct name{}; \
private: \
    using SUBSCRIBER_TYPE(name) = unsubscriber<name __VA_OPT__(,) __VA_ARGS__>; \
    SUBSCRIBER_TYPE(name) SUBSCRIBER(name); \
    inline code do_subscribe(SUBSCRIBER_TYPE(name)::handler&& handler) NOEXCEPT \
        { return SUBSCRIBER(name).subscribe(std::move(handler)); } \
    static inline code do_notify_##name( \
        distributor_rpc& self, const optional_t& params) \
        { return notifier<name, SUBSCRIBER_TYPE(name) __VA_OPT__(,) __VA_ARGS__>( \
          self.SUBSCRIBER(name), params, container::mode, tuple); }

/// Not thread safe.
class BCT_API distributor_rpc
{
public:
    DELETE_COPY_MOVE_DESTRUCT(distributor_rpc);

    /// Create an instance of this class.
    distributor_rpc(asio::strand& strand) NOEXCEPT;

    /// If stopped, handler is invoked with error::subscriber_stopped.
    /// If key exists, handler is invoked with error::subscriber_exists.
    /// Otherwise handler retained. Subscription code is also returned here.
    template <typename Handler>
    inline code subscribe(Handler&& handler) NOEXCEPT
    {
        return do_subscribe(std::forward<Handler>(handler));
    }

    /// Dispatch the request to the appropriate method's unsubscriber.
    virtual code notify(const json::request_t& request) NOEXCEPT;

    /// Stop all unsubscribers with the given code.
    virtual void stop(const code& ec) NOEXCEPT;

private:
    template <typename ...Args>
    using names_t = std::array<std::string, sizeof...(Args)>;
    using optional_t = json::params_option;
    using functor_t = std::function<code(distributor_rpc&, const optional_t&)>;
    using dispatch_t = std::unordered_map<std::string, functor_t>;
    enum class container{ positional, named, either };

    template <typename Type>
    static Type extract(const json::value_t& value) THROWS;
    template <typename ...Args>
    static std::tuple<Args...> extractor(const optional_t& parameters,
        container mode, const names_t<Args...>& names) THROWS;
    template <typename Method, typename Subscriber, typename ...Args>
    static code notifier(Subscriber& subscriber, const optional_t& parameters,
        container mode, auto&& tuple) NOEXCEPT;
    static bool has_params(const optional_t& parameters) NOEXCEPT;
    
    // These are thread safe.
    DECLARE_SUBSCRIBER(get_version, either, NAMES())
    DECLARE_SUBSCRIBER(add_element, either, NAMES("a", "b"), int, int)

    // Function map, find name and dispatch with request.
    static const dispatch_t dispatch_;
};

#undef SUBSCRIBER
#undef SUBSCRIBER_TYPE
#undef NAMES
#undef DECLARE_SUBSCRIBER

} // namespace network
} // namespace libbitcoin

#endif
