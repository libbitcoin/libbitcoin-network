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

#include <cmath>
#include <tuple>
#include <utility>
#include <variant>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/json/json.hpp>

namespace libbitcoin {
namespace network {

// make_dispatchers
// ----------------------------------------------------------------------------

TEMPLATE
inline bool CLASS::has_params(const optional_t& parameters) NOEXCEPT
{
    if (!parameters.has_value())
        return false;

    return std::visit(overload
    {
        [](const json::array_t& param) NOEXCEPT
        {
            return !param.empty();
        },
        [](const json::object_t& param) NOEXCEPT
        {
            return !param.empty();
        }
    }, parameters.value());
}

TEMPLATE
template <typename Type>
inline Type CLASS::extract(const json::value_t& value) THROWS
{
    if constexpr (is_same_type<Type, bool>)
        return std::get<json::boolean_t>(value.value());

    if constexpr (is_same_type<Type, std::string>)
        return std::get<json::string_t>(value.value());

    if constexpr (is_same_type<Type, double>)
        return std::get<json::number_t>(value.value());

    // Hack: this is not a native json-rpc type.
    if constexpr (is_same_type<Type, int>)
    {
        const auto& number = std::get<json::number_t>(value.value());
        if (number != std::floor(number) ||
            number < std::numeric_limits<int>::min() ||
            number > std::numeric_limits<int>::max()) {
            throw std::invalid_argument{ "int" };
        }

        return system::to_integer<int>(number);
    }

    ////throw std::bad_variant_access{};
}

TEMPLATE
template <typename Tuple>
inline Tuple CLASS::extractor(
    const optional_t& parameters,
    const std::array<std::string_view, std::tuple_size_v<Tuple>>& names) THROWS
{
    constexpr auto count = std::tuple_size_v<Tuple>;
    if (is_zero(count) && !has_params(parameters))
        return {};

    if (!parameters.has_value())
        throw std::invalid_argument{ "count" };

    const auto get_array = [&](const json::array_t& array) THROWS
    {
        if (array.size() != count) throw std::invalid_argument{ "count" };
        return [&]<size_t... Index>(std::index_sequence<Index...>)
        {
            return std::make_tuple(extract<std::tuple_element_t<Index, Tuple>>(
                array.at(Index))...);
        }(std::make_index_sequence<count>{});
    };

    const auto get_object = [&](const json::object_t& object) THROWS
    {
        if (object.size() != count) throw std::invalid_argument{ "count" };
        return [&]<size_t... Index>(std::index_sequence<Index...>)
        {
            return std::make_tuple(extract<std::tuple_element_t<Index, Tuple>>(
                object.at(std::string{ names.at(Index) }))...);
        }(std::make_index_sequence<count>{});
    };

    const auto& params = parameters.value();

    if constexpr (Interface::mode == rpc::group::positional)
    {
        if (!std::holds_alternative<json::array_t>(params))
            throw std::invalid_argument{ "positional" };

        return get_array(std::get<json::array_t>(params));
    }
    else if constexpr (Interface::mode == rpc::group::named)
    {
        if (!std::holds_alternative<json::object_t>(params))
            throw std::invalid_argument{ "named" };

        return get_object(std::get<json::object_t>(params));
    }
    else // if constexpr (Interface::mode == rpc::group::either)
    {
        if (std::holds_alternative<json::array_t>(params))
            return get_array(std::get<json::array_t>(params));

        if (std::holds_alternative<json::object_t>(params))
            return get_object(std::get<json::object_t>(params));

        throw std::invalid_argument{ "either" };
    }
}

TEMPLATE
template <typename Method>
inline code CLASS::notifier(subscriber_t<Method>& subscriber,
    const optional_t& parameters,
    const typename Method::names_t& names) NOEXCEPT
{
    try
    {
        std::apply([&](auto&&... args) THROWS
        {
           subscriber.notify(error::success, typename Method::tag{},
               std::forward<decltype(args)>(args)...);
        }, extractor<typename Method::args>(parameters, names));

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
    const optional_t& params) NOEXCEPT
{
    using method_t = std::tuple_element_t<Index, methods_t>;
    using tag = typename method_t::tag;
    auto& subscriber = std::get<Index>(self.subscribers_);
    const auto& names = std::get<Index>(Interface::methods).names();

    return notifier<method_t>(subscriber, params, names);
}

TEMPLATE
template <size_t ...Index>
inline constexpr CLASS::dispatch_t CLASS::make_dispatchers(
    std::index_sequence<Index...>) NOEXCEPT
{
    return dispatch_t
    {
        std::make_pair
        (
            std::string{ std::tuple_element_t<Index, methods_t>::name },
            &do_notify<Index>
        )...
    };
}

TEMPLATE
const typename CLASS::dispatch_t
CLASS::dispatch_ = make_dispatchers(sequence_t{});

// make_subscribers
// ----------------------------------------------------------------------------

TEMPLATE
template <size_t ...Index>
inline CLASS::subscribers_t CLASS::make_subscribers(asio::strand& strand,
    std::index_sequence<Index...>) NOEXCEPT
{
    return std::make_tuple
    (
        subscriber_t<std::tuple_element_t<Index, methods_t>>(strand)...
    );
}

// subscribe helpers
// ----------------------------------------------------------------------------

TEMPLATE
template <typename Methods, typename Tag, size_t Index>
inline constexpr size_t CLASS::find_tag_index() NOEXCEPT
{
    static_assert(Index < std::tuple_size_v<Methods>);
    using method_tag = typename std::tuple_element_t<Index, Methods>::tag;
    if constexpr (!is_same_type<method_tag, Tag>)
        return find_tag_index<Methods, Tag, add1(Index)>();

   return Index;
}

// public
// ----------------------------------------------------------------------------

TEMPLATE
template <typename Handler>
inline code CLASS::subscribe(Handler&& handler) NOEXCEPT
{
    using traits = traits<std::decay_t<Handler>>;
    using tag = typename traits::tag;
    using handler_args = typename traits::args;
    constexpr auto index = find_tag_index<methods_t, tag>();
    using method_t = std::tuple_element_t<index, methods_t>;
    using method_args = typename method_t::args;
    static_assert(is_same_type<handler_args, method_args>);
    auto& subscriber = std::get<index>(subscribers_);
    return subscriber.subscribe(std::forward<Handler>(handler));
}

TEMPLATE
inline CLASS::distributor_rpc(asio::strand& strand) NOEXCEPT
  : subscribers_(make_subscribers(strand, sequence_t{}))
{
}

TEMPLATE
inline code CLASS::notify(const json::request_t& request) NOEXCEPT
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
    std::apply([&ec](auto&&... subscriber) NOEXCEPT
    {
        (subscriber.stop_default(ec), ...);
    }, subscribers_);
}

} // namespace network
} // namespace libbitcoin
