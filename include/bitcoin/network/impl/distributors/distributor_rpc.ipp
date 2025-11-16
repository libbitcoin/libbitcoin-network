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
#include <bitcoin/network/distributors/distributor_rpc.hpp>

#include <tuple>
#include <utility>
#include <variant>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/rpc/rpc.hpp>

namespace libbitcoin {
namespace network {

// make_notifiers
// ----------------------------------------------------------------------------

TEMPLATE
template <typename Type>
inline Type CLASS::get(const rpc::value_t& value) THROWS
{
    const auto& inner = value.value();
    if (!std::holds_alternative<Type>(inner))
        throw std::system_error{ error::not_found, "invalid type" };

    return std::get<Type>(inner);
}

TEMPLATE
template <typename Argument>
inline rpc::outer_t<Argument> CLASS::get_value(const rpc::value_t& value) THROWS
{
    if constexpr (rpc::is_nullable<Argument>::value)
    {
        return { get<rpc::inner_t<Argument>>(value) };
    }
    else
    {
        return get<rpc::inner_t<Argument>>(value);
    }
}

TEMPLATE
template <typename Argument>
inline rpc::outer_t<Argument> CLASS::get_positional(size_t& position,
    const rpc::array_t& array) THROWS
{
    using namespace rpc;
    if (position >= array.size())
    {
        if constexpr (is_required<Argument>::value)
            throw std::system_error{ error::not_found, "missing positional" };

        if constexpr (is_optional<Argument>::value)
            return Argument::value;
        else
            return outer_t<Argument>{};
    }

    const auto& value = array.at(position++);
    if (std::holds_alternative<null_t>(value.value()))
    {
        if constexpr (is_required<Argument>::value)
            throw std::system_error{ error::not_found, "null positional" };

        if constexpr (is_optional<Argument>::value)
            return Argument::value;
        else
            return outer_t<Argument>{};
    }

    return get_value<Argument>(value);
}

TEMPLATE
template <typename Argument>
inline rpc::outer_t<Argument> CLASS::get_named(
    const std::string_view& name, const rpc::object_t& object) THROWS
{
    using namespace rpc;
    const auto it = object.find(std::string{ name });
    if (it == object.end())
    {
        if constexpr (is_required<Argument>::value)
            throw std::system_error{ error::not_found, "missing named" };

        if constexpr (is_optional<Argument>::value)
            return Argument::value;
        else
            return outer_t<Argument>{};
    }

    const auto& value = it->second;
    if (std::holds_alternative<null_t>(value.value()))
    {
        if constexpr (is_required<Argument>::value)
            throw std::system_error{ error::not_found, "null named" };

        if constexpr (is_optional<Argument>::value)
            return Argument::value;
        else
            return outer_t<Argument>{};
    }

    return get_value<Argument>(value);
}

TEMPLATE
template <typename Arguments>
inline Arguments CLASS::extract_positional(const optional_t& parameters) THROWS
{
    using namespace rpc;

    array_t array{};
    if (parameters.has_value())
    {
        const auto& params = parameters.value();
        if (!std::holds_alternative<array_t>(params))
            throw std::system_error{ error::not_found, "missing array" };

        array = std::get<array_t>(params);
    }

    size_t position{};
    Arguments values{};
    constexpr auto count = std::tuple_size_v<Arguments>;

    // Sequence via comma expansion to preseve order.
    [&] <size_t... Index>(std::index_sequence<Index...>)
    {
        ((
            std::get<Index>(values) = get_positional<std::tuple_element_t<
            Index, Arguments>>(position, array)
        ), ...);
    }(std::make_index_sequence<count>{});

    if (position < array.size())
        throw std::system_error{ error::not_found, "extra positional" };

    return values;
}

TEMPLATE
template <typename Arguments>
inline Arguments CLASS::extract_named(const optional_t& parameters,
    const rpc::names_t<Arguments>& names) THROWS
{
    using namespace rpc;

    object_t object{};
    if (parameters.has_value())
    {
        const auto& params = parameters.value();
        if (!std::holds_alternative<object_t>(params))
            throw std::system_error{ error::not_found, "missing object" };

        object = std::get<object_t>(params);
    }

    constexpr auto count = std::tuple_size_v<Arguments>;
     if (object.size() > count)
         throw std::system_error{ error::not_found, "extra named" };

    return [&]<size_t... Index>(std::index_sequence<Index...>)
    {
        return std::make_tuple
        (
            get_named<std::tuple_element_t<Index, Arguments>>(
                names.at(Index), object)...
        );
    }(std::make_index_sequence<count>{});
}

TEMPLATE
inline void CLASS::disallow_params(const optional_t& parameters) THROWS
{
    if (!parameters.has_value())
        return;

    std::visit(overload
    {
        [](const rpc::array_t& params) THROWS
        {
            if (!params.empty())
                throw std::system_error{ error::not_found,
                    "extra positional params" };
        },
        [](const rpc::object_t& params) THROWS
        {
            if (!params.empty())
                throw std::system_error{ error::not_found,
                    "extra named params" };
        }
    }, parameters.value());
}

TEMPLATE
template <typename Arguments>
inline Arguments CLASS::extract(const optional_t& parameters,
    const rpc::names_t<Arguments>& names) THROWS
{
    constexpr auto count = std::tuple_size_v<Arguments>;
    if constexpr (is_zero(count))
    {
        disallow_params(parameters);
        return {};
    }

    constexpr auto mode = Interface::mode;
    if constexpr (mode == rpc::grouping::positional)
    {
        return extract_positional<Arguments>(parameters);
    }
    else if constexpr (mode == rpc::grouping::named)
    {
        return extract_named<Arguments>(parameters, names);
    }
    else // rpc::grouping::either
    {
        const auto specified_params = parameters.has_value();
        const auto positional_params = specified_params && 
            std::holds_alternative<rpc::array_t>(parameters.value());

        if (!specified_params || positional_params)
            return extract_positional<Arguments>(parameters);
        else
            return extract_named<Arguments>(parameters, names);
    }
}

TEMPLATE
template <typename Method>
inline code CLASS::notify(subscriber_t<Method>& subscriber,
    const optional_t& parameters, const rpc::names_t<Method>& names) NOEXCEPT
{
    try
    {
        std::apply
        (
            [&](auto&&... args) NOEXCEPT
            {
                subscriber.notify({}, rpc::tag_t<Method>{},
                    std::forward<decltype(args)>(args)...);
            },
            extract<rpc::args_t<Method>>(parameters, names)
        );

        return error::success;
    }
    catch (const std::system_error& e)
    {
        // TODO: pass specific codes above.
        return e.code();
    }
    catch (const std::bad_variant_access&)
    {
        return error::operation_failed;
    }
    catch (...)
    {
        return error::not_found;
    }
}

TEMPLATE
template <size_t Index>
inline code CLASS::notifier(distributor_rpc& self,
    const optional_t& parameters) NOEXCEPT
{
    using method = rpc::method_t<Index, methods_t>;
    auto& subscriber = std::get<Index>(self.subscribers_);
    const auto& names = std::get<Index>(Interface::methods).parameter_names();
    return notify<method>(subscriber, parameters, names);
}

TEMPLATE
template <size_t ...Index>
inline constexpr CLASS::notifiers_t CLASS::make_notifiers(
    std::index_sequence<Index...>) NOEXCEPT
{
    return
    {
        std::make_pair
        (
            std::string{ rpc::method_t<Index, methods_t>::name },
            &notifier<Index>
        )...
    };
}

// make_subscribers/subscribe
// ----------------------------------------------------------------------------

TEMPLATE
template <size_t ...Index>
inline CLASS::subscribers_t CLASS::make_subscribers(asio::strand& strand,
    std::index_sequence<Index...>) NOEXCEPT
{
    return std::make_tuple
    (
        subscriber_t<rpc::method_t<Index, methods_t>>(strand)...
    );
}

TEMPLATE
template <size_t Index, typename Handler>
inline consteval bool CLASS::is_handler_type() NOEXCEPT
{
    // is_same_type decays individual types but not tuple elements.
    using handle_args = typename rpc::traits<Handler>::args;
    using method_args = rpc::args_t<rpc::method_t<Index, methods_t>>;
    using decayed_handle_args = typename decay_tuple<handle_args>::type;
    using decayed_method_args = typename decay_tuple<method_args>::type;
    return is_same_type<decayed_handle_args, decayed_method_args>;
}

TEMPLATE
template <typename Tag, size_t Index>
inline consteval size_t CLASS::find_handler_index() NOEXCEPT
{
    static_assert(Index < std::tuple_size_v<methods_t>);
    using tag = rpc::tag_t<rpc::method_t<Index, methods_t>>;
    if constexpr (!is_same_type<tag, Tag>)
        return find_handler_index<Tag, add1(Index)>();

    return Index;
}

// public
// ----------------------------------------------------------------------------

TEMPLATE
template <typename Handler>
inline code CLASS::subscribe(Handler&& handler) NOEXCEPT
{
    using tag = typename rpc::traits<Handler>::tag;
    constexpr auto index = find_handler_index<tag>();
    static_assert(is_handler_type<index, Handler>());

    auto& subscriber = std::get<index>(subscribers_);
    return subscriber.subscribe(std::forward<Handler>(handler));
}

// private/static
TEMPLATE
const typename CLASS::notifiers_t
CLASS::notifiers_ = make_notifiers(
    std::make_index_sequence<Interface::size>{});

TEMPLATE
inline CLASS::distributor_rpc(asio::strand& strand) NOEXCEPT
  : subscribers_(make_subscribers(strand,
      std::make_index_sequence<Interface::size>{}))
{
}

TEMPLATE
inline code CLASS::notify(const rpc::request_t& request) NOEXCEPT
{
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    const auto it = notifiers_.find(request.method);
    if (it == notifiers_.end())
        return error::not_found;

    return it->second(*this, request.params);
    BC_POP_WARNING()
}

TEMPLATE
inline void CLASS::stop(const code& ec) NOEXCEPT
{
    std::apply
    (
        [&ec](auto&&... subscriber) NOEXCEPT
        {
            (subscriber.stop_default(ec), ...);
        },
        subscribers_
    );
}

} // namespace network
} // namespace libbitcoin
