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
#include <bitcoin/network/messages/json/json.hpp>

namespace libbitcoin {
namespace network {

template <size_t Size>
using to_sequence = std::make_index_sequence<Size>;

// make_dispatchers
// ----------------------------------------------------------------------------

TEMPLATE
inline bool CLASS::has_params(const optional_t& parameters) NOEXCEPT
{
    if (!parameters.has_value())
        return false;

    return std::visit(overload
    {
        [](const rpc::array_t& param) NOEXCEPT
        {
            return !param.empty();
        },
        [](const rpc::object_t& param) NOEXCEPT
        {
            return !param.empty();
        }
    }, parameters.value());
}

TEMPLATE
template <typename Type>
inline Type CLASS::extract(const rpc::value_t& value) THROWS
{
    using namespace rpc;
    if constexpr (is_same_type<Type, boolean_t>)
        return std::get<boolean_t>(value.value());

    if constexpr (is_same_type<Type, string_t>)
        return std::get<string_t>(value.value());

    if constexpr (is_same_type<Type, number_t>)
        return std::get<number_t>(value.value());

    if constexpr (is_same_type<Type, array_t>)
        return std::get<array_t>(value.value());

    if constexpr (is_same_type<Type, object_t>)
        return std::get<object_t>(value.value());

    throw std::invalid_argument{ "type" };
}

TEMPLATE
template <typename Arguments>
inline Arguments CLASS::extractor(const optional_t& parameters,
    const rpc::names_t<Arguments>& names) THROWS
{
    constexpr auto count = std::tuple_size_v<Arguments>;
    if (is_zero(count) && !has_params(parameters))
        return {};

    if (!parameters.has_value())
        throw std::invalid_argument{ "count" };

    const auto get_array = [&](const rpc::array_t& array) THROWS
    {
        if (array.size() != count)
            throw std::invalid_argument{ "count" };

        return [&]<size_t... Index>(std::index_sequence<Index...>)
        {
            return std::make_tuple
            (
                extract<std::tuple_element_t<Index, Arguments>>(
                    array.at(Index))...
            );
        }(to_sequence<count>{});
    };

    const auto get_object = [&](const rpc::object_t& object) THROWS
    {
        if (object.size() != count)
            throw std::invalid_argument{ "count" };

        return [&]<size_t... Index>(std::index_sequence<Index...>)
        {
            return std::make_tuple
            (
                // TODO: std::string removal with rpc::method change (gcc14).
                extract<std::tuple_element_t<Index, Arguments>>(
                    object.at(std::string{ names.at(Index) }))...
            );
        }(to_sequence<count>{});
    };

    const auto& params = parameters.value();
    constexpr auto mode = Interface::mode;

    if constexpr (mode == rpc::group::positional)
    {
        if (!std::holds_alternative<rpc::array_t>(params))
            throw std::invalid_argument{ "positional" };

        return get_array(std::get<rpc::array_t>(params));
    }
    else if constexpr (mode == rpc::group::named)
    {
        if (!std::holds_alternative<rpc::object_t>(params))
            throw std::invalid_argument{ "named" };

        return get_object(std::get<rpc::object_t>(params));
    }
    else // if constexpr (mode == rpc::group::either)
    {
        if (std::holds_alternative<rpc::array_t>(params))
            return get_array(std::get<rpc::array_t>(params));

        if (std::holds_alternative<rpc::object_t>(params))
            return get_object(std::get<rpc::object_t>(params));

        throw std::invalid_argument{ "either" };
    }
}

TEMPLATE
template <typename Method>
inline code CLASS::notifier(subscriber_t<Method>& subscriber,
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
            extractor<rpc::args_t<Method>>(parameters, names)
        );

        return error::success;
    }
    catch (...)
    {
        return error::not_found;
    }
}

TEMPLATE
template <size_t Index>
inline code CLASS::do_notify(distributor_rpc& self,
    const optional_t& parameters) NOEXCEPT
{
    using method = rpc::method_t<Index, methods_t>;
    auto& subscriber = std::get<Index>(self.subscribers_);
    const auto& names = std::get<Index>(Interface::methods).parameter_names();
    return notifier<method>(subscriber, parameters, names);
}

TEMPLATE
template <size_t ...Index>
inline constexpr CLASS::dispatch_t CLASS::make_dispatchers(
    std::index_sequence<Index...>) NOEXCEPT
{
    return
    {
        std::make_pair
        (
            // TODO: std::string removal with rpc::method change (gcc14).
            std::string{ rpc::method_t<Index, methods_t>::name },
            &do_notify<Index>
        )...
    };
}

TEMPLATE
const typename CLASS::dispatch_t
CLASS::dispatch_ = make_dispatchers(to_sequence<Interface::size>{});

// make_subscribers
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

// subscribe helpers
// ----------------------------------------------------------------------------

TEMPLATE
template <typename Tag, size_t Index>
inline constexpr size_t CLASS::find_tag_index() NOEXCEPT
{
    static_assert(Index < std::tuple_size_v<methods_t>);
    using tag = rpc::tag_t<rpc::method_t<Index, methods_t>>;
    if constexpr (!is_same_type<tag, Tag>)
        return find_tag_index<Tag, add1(Index)>();

   return Index;
}

// public
// ----------------------------------------------------------------------------

TEMPLATE
template <typename Handler>
inline code CLASS::subscribe(Handler&& handler) NOEXCEPT
{
    using traits = rpc::traits<Handler>;
    constexpr auto index = find_tag_index<typename traits::tag>();

    // is_same_type decays individual types but not tuple elements.
    using handle_args = typename traits::args;
    using method_args = rpc::args_t<rpc::method_t<index, methods_t>>;
    using decayed_handle_args = typename decay_tuple<handle_args>::type;
    using decayed_method_args = typename decay_tuple<method_args>::type;
    static_assert(is_same_type<decayed_handle_args, decayed_method_args>);

    auto& subscriber = std::get<index>(subscribers_);
    return subscriber.subscribe(std::forward<Handler>(handler));
}

TEMPLATE
inline CLASS::distributor_rpc(asio::strand& strand) NOEXCEPT
  : subscribers_(make_subscribers(strand, to_sequence<Interface::size>{}))
{
}

TEMPLATE
inline code CLASS::notify(const rpc::request_t& request) NOEXCEPT
{
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    const auto it = dispatch_.find(request.method);
    if (it == dispatch_.end())
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
