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
#ifndef LIBBITCOIN_NETWORK_RPC_BROADCASTER_IPP
#define LIBBITCOIN_NETWORK_RPC_BROADCASTER_IPP

#include <tuple>
#include <utility>
#include <variant>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/rpc/method.hpp>
#include <bitcoin/network/rpc/types.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

// make_notifiers
// ----------------------------------------------------------------------------
    
TEMPLATE
template <typename Method>
inline code CLASS::notify_one(subscriber_t<Method>& subscriber,
    const key_t& key, const parameters_t& params,
    const names_t<Method>& names) NOEXCEPT
{
    try
    {
        std::apply([&](auto&&... args) NOEXCEPT
        {
            subscriber.notify_one(std::forward<decltype(args)>(args)...);
        }, std::tuple_cat(std::tie(key),
            base::template preamble<Method>(),
            base::template extract<args_native_t<Method>>(params, names)));

        return error::success;
    }
    catch (const std::bad_any_cast&)
    {
        return error::unexpected_type;
    }
    catch (const std::bad_variant_access&)
    {
        return error::unexpected_type;
    }
    catch (const std::system_error& e)
    {
        return e.code();
    }
    catch (...)
    {
        return error::undefined_type;
    }
}

TEMPLATE
template <size_t Index>
inline code CLASS::one_functor(broadcaster& self, const key_t& key,
    const parameters_t& params) NOEXCEPT
{
    // Get method (type), suscriber, and parameter names from the index.
    using method = method_t<Index, methods_t>;
    auto& subscriber = std::get<Index>(self.subscribers_);
    const auto& names = std::get<Index>(Interface::methods).parameter_names();

    // Invoke subscriber.notify_one(key, error::success, parameters).
    return CLASS::notify_one<method>(subscriber, key, params, names);
}

TEMPLATE
template <size_t ...Index>
inline constexpr CLASS::notifiers_t CLASS::make_one_notifiers(
    std::index_sequence<Index...>) NOEXCEPT
{
    // Notifiers are declared statically (same for all distributors instances).
    return
    {
        std::make_pair
        (
            std::string{ method_t<Index, methods_t>::name },
            &CLASS::one_functor<Index>
        )...
    };
}

TEMPLATE
const typename CLASS::notifiers_t
CLASS::one_notifiers_ = CLASS::make_one_notifiers(
    std::make_index_sequence<Interface::size>{});

// desubscriber
// ----------------------------------------------------------------------------

TEMPLATE
template <typename Method>
inline void CLASS::notify_defaults(subscriber_t<Method>& subscriber,
    const key_t& key, const code& ec) NOEXCEPT
{
    std::apply([&](auto&&... args) NOEXCEPT
    {
        subscriber.notify_one(std::forward<decltype(args)>(args)...);
    }, std::tuple_cat(std::tie(key), base::template preamble<Method>(ec),
        externals_t<args_native_t<Method>>{}));
}

TEMPLATE
template <size_t Index>
inline void CLASS::desubscriber(const key_t& key) NOEXCEPT
{
    using method = method_t<Index, methods_t>;
    auto& subscriber = std::get<Index>(this->subscribers_);
    CLASS::notify_defaults<method>(subscriber, key, error::desubscribed);
}

TEMPLATE
template <size_t ...Index>
inline void CLASS::desubscribe(const key_t& key,
    std::index_sequence<Index...>) NOEXCEPT
{
    (CLASS::desubscriber<Index>(key), ...);
}

// public
// ----------------------------------------------------------------------------

TEMPLATE
inline code CLASS::notify(const request_t& request) NOEXCEPT
{
    return base::notify(request);
};

TEMPLATE
inline code CLASS::notify(const request_t& request, const key_t& key) NOEXCEPT
{
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

    // Search unordered map by method name for the notify_one() functor.
    const auto it = this->one_notifiers_.find(request.method);
    return it == this->one_notifiers_.end() ? error::unexpected_method :
        it->second(*this, key, request.params);

    BC_POP_WARNING()
};

TEMPLATE
inline void CLASS::unsubscribe(const key_t& key) NOEXCEPT
{
    // Unsubscribe all key-subscribed, passing code and default arguments.
    CLASS::desubscribe(key, std::make_index_sequence<Interface::size>{});
}

} // namespace rpc
} // namespace network
} // namespace libbitcoin

#endif
