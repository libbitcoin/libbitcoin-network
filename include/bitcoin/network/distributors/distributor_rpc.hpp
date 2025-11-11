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

// Macro for declaring method subscribers.
#define DECLARE_METHOD(name_, ...) \
using name_##_handler = std::function<bool( \
    const code& __VA_OPT__(,) __VA_ARGS__)>; \
using name_##_subscriber_type = unsubscriber<__VA_ARGS__>; \
name_##_subscriber_type name_##_subscriber_; \
static code notify_##name_(distributor_rpc& self, \
    const json::params_option& params); \
code subscribe_##name_(name_##_handler&& handler) NOEXCEPT \
{ return name_##_subscriber_.subscribe(std::move(handler)); }
    
/// Not thread safe.
class BCT_API distributor_rpc
{
public:
    DELETE_COPY_MOVE_DESTRUCT(distributor_rpc);

    /// Create an instance of this class.
    distributor_rpc(asio::strand& strand) NOEXCEPT;

    /// Stop all unsubscribers with the given code.
    virtual void stop(const code& ec) NOEXCEPT;

    /// Dispatch the request to the appropriate method's unsubscriber.
    virtual code notify(const json::request_t& request) NOEXCEPT;

protected:
    /// Example methods.
    DECLARE_METHOD(get_version)
    DECLARE_METHOD(add_element, int, int)

private:
    template <typename ...Args>
    using names_t = std::array<std::string_view, sizeof...(Args)>;
    using optional_t = json::params_option;
    using functor_t = std::function<code(distributor_rpc&, const optional_t&)>;
    using dispatch_t = std::unordered_map<std::string, functor_t>;
    enum class container{ positional, named, either };

    template <typename Type>
    static Type extract(const json::value_t& value) THROWS;
    template <typename ...Args>
    static std::tuple<Args...> extractor(const optional_t& aparameters,
        container mode, const names_t<Args...>& names) THROWS;
    template <typename ...Args>
    static code notifier(auto& subscriber, const optional_t& parameters,
        container mode, auto&& tuple) NOEXCEPT;
    static bool has_params(const optional_t& parameters) NOEXCEPT;

    // Function map, find name and dispatch with request.
    static const dispatch_t dispatch_;
};

} // namespace network
} // namespace libbitcoin

#undef DECLARE_METHOD

#endif
