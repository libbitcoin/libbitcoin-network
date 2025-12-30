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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_RPC_DISPATCHER_IPP
#define LIBBITCOIN_NETWORK_MESSAGES_RPC_DISPATCHER_IPP

#include <any>
#include <tuple>
#include <utility>
#include <variant>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/rpc/enums/grouping.hpp>
#include <bitcoin/network/messages/rpc/method.hpp>
#include <bitcoin/network/messages/rpc/types.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

// make_notifiers
// ----------------------------------------------------------------------------

TEMPLATE
template <typename Argument>
inline external_t<Argument> CLASS::get_missing() THROWS
{
    if constexpr (is_required<Argument>)
        throw std::system_error{ error::missing_parameter };
    else if constexpr (is_optional<Argument>)
        return Argument::default_value();
    else
        return external_t<Argument>{};
}

TEMPLATE
template <typename Argument>
inline external_t<Argument> CLASS::get_nullified() THROWS
{
    if constexpr (!is_nullable<Argument>)
        throw std::system_error{ error::missing_parameter };
    else
        return external_t<Argument>{};
}

TEMPLATE
template <typename Argument>
inline external_t<Argument> CLASS::get_valued(const value_t& value) THROWS
{
    // Get contained variant value_t(inner_t).
    const auto& internal = value.value();
    using type = internal_t<Argument>;

    if constexpr (is_same_type<Argument, value_t> ||
        is_same_type<type, value_t>)
        return value;
    else if constexpr (is_shared_ptr<type>)
        return std::get<any_t>(internal).as<pointer_t<type>>();
    else if constexpr (is_nullable<Argument>)
        return { std::get<type>(internal) };
    else
        return std::get<type>(internal);
}

TEMPLATE
template <typename Argument>
inline external_t<Argument> CLASS::get_positional(size_t& position,
    const array_t& array) THROWS
{
    // Only optional can be missing.
    if (position >= array.size())
        return CLASS::get_missing<Argument>();

    // Get contained variant value_t(inner_t).
    const auto& internal = array.at(position++);

    // value_t(null_t) implies nullable.
    if (std::holds_alternative<null_t>(internal.value()))
        return CLASS::get_nullified<Argument>();

    // Otherwise value_t(inner_t) is required.
    return CLASS::get_valued<Argument>(internal);
}

TEMPLATE
template <typename Argument>
inline external_t<Argument> CLASS::get_named(
    const std::string_view& name, const object_t& object) THROWS
{
    // Only optional can be missing.
    const auto it = object.find(std::string{ name });
    if (it == object.end())
        return CLASS::get_missing<Argument>();

    // Get contained variant value_t(inner_t).
    const auto& internal = it->second;

    // value_t(null_t) implies nullable.
    if (std::holds_alternative<null_t>(internal.value()))
        return CLASS::get_nullified<Argument>();

    // Otherwise value_t(inner_t) is required.
    return CLASS::get_valued<Argument>(internal);
}

TEMPLATE
inline array_t CLASS::get_array(const parameters_t& params) THROWS
{
    if (!params.has_value())
        return {};

    if (!std::holds_alternative<array_t>(params.value()))
        throw std::system_error{ error::missing_array };

    return std::get<array_t>(params.value());
}

TEMPLATE
inline object_t CLASS::get_object(const parameters_t& params) THROWS
{
    if (!params.has_value())
        return {};

    if (!std::holds_alternative<object_t>(params.value()))
        throw std::system_error{ error::missing_object };

    return std::get<object_t>(params.value());
}

TEMPLATE
template <typename Arguments>
inline externals_t<Arguments> CLASS::extract_positional(
    const parameters_t& params) THROWS
{
    const auto array = get_array(params);
    constexpr auto count = std::tuple_size_v<Arguments>;

    size_t position{};
    externals_t<Arguments> values{};
    [&] <size_t... Index>(std::index_sequence<Index...>) THROWS
    {
        // Sequence via comma expansion is required to preserve order.
        ((
            std::get<Index>(values) = CLASS::get_positional<
            std::tuple_element_t<Index, Arguments>>(position, array)
        ), ...);
    }(std::make_index_sequence<count>{});

    if (position < array.size())
        throw std::system_error{ error::extra_positional };

    return values;
}

TEMPLATE
template <typename Arguments>
inline externals_t<Arguments> CLASS::extract_named(
    const parameters_t& params, const names_t<Arguments>& names) THROWS
{
    const auto object = get_object(params);
    constexpr auto count = std::tuple_size_v<Arguments>;

    // This doesn't catch duplicate names (allowed by json-rpc).
    if (object.size() > count)
        throw std::system_error{ error::extra_named };

    return [&]<size_t... Index>(std::index_sequence<Index...>) THROWS
    {
        return std::make_tuple
        (
            CLASS::get_named<std::tuple_element_t<Index, Arguments>>(
                names.at(Index), object)...
        );
    }(std::make_index_sequence<count>{});
}

TEMPLATE
template <typename Arguments>
inline externals_t<Arguments> CLASS::extract(const parameters_t& params,
    const names_t<Arguments>& names) THROWS
{
    if constexpr (Interface::mode == grouping::positional)
    {
        return CLASS::extract_positional<Arguments>(params);
    }
    else if constexpr (Interface::mode == grouping::named)
    {
        return CLASS::extract_named<Arguments>(params, names);
    }
    else // grouping::either
    {
        if (!params.has_value() ||
            std::holds_alternative<array_t>(params.value()))
            return CLASS::extract_positional<Arguments>(params);
        else
            return CLASS::extract_named<Arguments>(params, names);
    }
}

TEMPLATE
template <typename Method>
inline auto CLASS::preamble(const code& ec) NOEXCEPT
{
    if constexpr (Method::native)
        return std::make_tuple(ec);
    else
        return std::make_tuple(ec, Method{});
}

TEMPLATE
template <typename Method>
inline code CLASS::notify(subscriber_t<Method>& subscriber,
    const parameters_t& params, const names_t<Method>& names) NOEXCEPT
{
    try
    {
        using native = args_native_t<Method>;
        std::apply([&](auto&&... args) NOEXCEPT
        {
            subscriber.notify(std::forward<decltype(args)>(args)...);
        }, std::tuple_cat(CLASS::preamble<Method>(),
            CLASS::extract<native>(params, names)));

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
inline code CLASS::functor(dispatcher& self,
    const parameters_t& params) NOEXCEPT
{
    // Get method (type), suscriber, and parameter names from the index.
    using method = method_t<Index, methods_t>;
    auto& subscriber = std::get<Index>(self.subscribers_);
    const auto& names = std::get<Index>(Interface::methods).parameter_names();

    // Invoke subscriber.notify(error::success, ordered-or-named-parameters).
    return notify<method>(subscriber, params, names);
}

TEMPLATE
template <size_t ...Index>
inline constexpr CLASS::notifiers_t CLASS::make_notifiers(
    std::index_sequence<Index...>) NOEXCEPT
{
    // Notifiers are declared statically (same for all distributors instances).
    return
    {
        std::make_pair
        (
            std::string{ method_t<Index, methods_t>::name },
            &CLASS::functor<Index>
        )...
    };
}

TEMPLATE
const typename CLASS::notifiers_t
CLASS::notifiers_ = CLASS::make_notifiers(
    std::make_index_sequence<Interface::size>{});

// make_subscribers/subscribe
// ----------------------------------------------------------------------------

TEMPLATE
template <size_t ...Index>
inline CLASS::subscribers_t CLASS::make_subscribers(
    std::index_sequence<Index...>) NOEXCEPT
{
    // Subscribers declared dynamically (tuple for each distributor/channel).
    return std::make_tuple
    (
        subscriber_t<method_t<Index, methods_t>>{}...
    );
}

TEMPLATE
template <typename Handler, size_t Index>
inline consteval bool CLASS::subscriber_matches_handler() NOEXCEPT
{
    static_assert(Index < std::tuple_size_v<methods_t>, "subscriber missing");

    // This guard limits compilation error noise when a handler is ill-defined.
    if constexpr (Index < std::tuple_size_v<methods_t>)
    {
        using handler = typename subscriber_t<method_t<Index, methods_t>>::handler;
        return std::is_convertible_v<Handler, handler>;
    }
}

TEMPLATE
template <typename Handler, size_t Index>
inline consteval size_t CLASS::find_subscriber_for_handler() NOEXCEPT
{
    // Find the index of the subscriber that matches Handler (w/o code arg).
    if constexpr (!CLASS::subscriber_matches_handler<Handler, Index>())
        return CLASS::find_subscriber_for_handler<Handler, add1(Index)>();
    else
        return Index;
}

// public
// ----------------------------------------------------------------------------

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

TEMPLATE
template <typename Handler, typename ...Args>
inline code CLASS::subscribe(Handler&& handler, Args&&... args) NOEXCEPT
{
    // Iterate methods_t in order to find the matching function signature.
    // The index of each method correlates to its defined subscriber index.
    constexpr auto index = CLASS::find_subscriber_for_handler<Handler>();
    auto& subscriber = std::get<index>(this->subscribers_);
    return subscriber.subscribe(std::forward<Handler>(handler),
        std::forward<Args>(args)...);
}

TEMPLATE
inline CLASS::dispatcher() NOEXCEPT
  : subscribers_(CLASS::make_subscribers(
      std::make_index_sequence<Interface::size>{}))
{
}

TEMPLATE
inline code CLASS::notify(const request_t& request) NOEXCEPT
{
    // Search unordered map by method name for the notify() functor.
    const auto it = this->notifiers_.find(request.method);
    return it == this->notifiers_.end() ? error::unexpected_method :
        it->second(*this, request.params);
}

BC_POP_WARNING()

TEMPLATE
inline void CLASS::stop(const code& ec) NOEXCEPT
{
    // Stop all subscribers, passing code and default arguments.
    std::apply
    (
        [&ec](auto&... subscriber) NOEXCEPT
        {
            (subscriber.stop_default(ec), ...);
        },
        this->subscribers_
    );
}

} // namespace rpc
} // namespace network
} // namespace libbitcoin

#endif
