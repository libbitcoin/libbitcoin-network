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
#include <unordered_map>
#include <utility>
#include <variant>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/json/json.hpp>

namespace libbitcoin {
namespace network {

// macros
// ----------------------------------------------------------------------------
#define DISPATCH(name) { #name, &notify_##name }
#define PARAMS(...) std::make_tuple(__VA_ARGS__)
#define DEFINE_METHOD(name, mode, names, ...) \
code distributor_rpc::notify_##name(distributor_rpc& self, \
    const optional_t& params) { return notifier<__VA_ARGS__>( \
    self.name##_subscriber_, params, container::mode, names); }

// extract/invoke
// ----------------------------------------------------------------------------
// private/static

template <typename Type>
Type distributor_rpc::extract(const json::value_t&) THROWS
{
    throw std::bad_variant_access{};
}

template <>
bool distributor_rpc::extract<bool>(const json::value_t& value) THROWS
{
    return std::get<json::boolean_t>(value.value());
}

template <>
std::string distributor_rpc::extract<std::string>(
    const json::value_t& value) THROWS
{
    return std::get<json::string_t>(value.value());
}

template <>
double distributor_rpc::extract<double>(const json::value_t& value) THROWS
{
    return std::get<json::number_t>(value.value());
}

template <>
int distributor_rpc::extract<int>(const json::value_t& value) THROWS
{
    const auto& number = std::get<json::number_t>(value.value());
    if (number != std::floor(number) ||
        number < std::numeric_limits<int>::min() ||
        number > std::numeric_limits<int>::max())
    {
        throw std::invalid_argument{ "int" };
    }
    
    return system::to_integer<int>(number);
}

template <typename ...Args>
std::tuple<Args...> distributor_rpc::extractor(
    const optional_t& parameters, container mode,
    const names_t<Args...>& names) THROWS
{
    constexpr auto count = sizeof...(Args);
    if (is_zero(count) && !has_params(parameters))
        return {};

    if (!parameters.has_value())
        throw std::invalid_argument{ "count" };

    const auto get_array = [&](const json::array_t& array) THROWS
    {
        if (array.size() != count) throw std::invalid_argument{ "count" };
        return [&]<size_t... Index>(std::index_sequence<Index...>)
        {
            return std::make_tuple(extract<Args>(array.at(Index))...);
        }(std::make_index_sequence<count>{});
    };

    const auto get_object = [&](const json::object_t& object) THROWS
    {

        if (object.size() != count) throw std::invalid_argument{ "count" };
        return [&]<size_t... Index>(std::index_sequence<Index...>)
        {
            return std::make_tuple(extract<Args>(
                object.at(std::string{ names.at(Index) }))...);
        }(std::make_index_sequence<count>{});
    };

    const auto& params = parameters.value();
    switch (mode)
    {
        case container::positional:
            if (!std::holds_alternative<json::array_t>(params))
                throw std::invalid_argument{ "positional" };
            return get_array(std::get<json::array_t>(params));
        case container::named:
            if (!std::holds_alternative<json::object_t>(params))
                throw std::invalid_argument{ "named" };
            return get_object(std::get<json::object_t>(params));
        case container::either:
            if (std::holds_alternative<json::array_t>(params))
                return get_array(std::get<json::array_t>(params));
            if (std::holds_alternative<json::object_t>(params))
                return get_object(std::get<json::object_t>(params));
    }

    throw std::invalid_argument{ "container" };
}

template <typename ...Args>
code distributor_rpc::notifier(auto& subscriber, const optional_t& parameters,
    container mode, auto&& tuple) NOEXCEPT
{
    static auto names = std::apply([](auto&&... args) NOEXCEPT
    {
        return names_t<Args...>{ std::forward<decltype(args)>(args)... };
    }, std::forward<decltype(tuple)>(tuple));

    try
    {
        std::apply([&](auto&&... args) THROWS
        {
           subscriber.notify({}, std::forward<decltype(args)>(args)...);
        }, extractor<Args...>(parameters, mode, names));

        return error::success;
    }
    catch (...)
    {
        return error::not_found;
    }
}

bool distributor_rpc::has_params(const optional_t& parameters) NOEXCEPT
{
    if (!parameters.has_value())
        return false;

    return std::visit(overload
    {
        [] (const json::array_t& param) NOEXCEPT
        {
            return !param.empty();
        },
        [](const json::object_t& param) NOEXCEPT
        {
            return !param.empty();
        }
    }, parameters.value());
}

// define map
// ----------------------------------------------------------------------------

// Example methods.

// protected methods with parameter names and types
DEFINE_METHOD(get_version, either, PARAMS())
DEFINE_METHOD(add_element, either, PARAMS("a", "b"), int, int)

// private map
const distributor_rpc::dispatch_t distributor_rpc::dispatch_
{
    DISPATCH(get_version),
    DISPATCH(add_element)
};

// public
// ----------------------------------------------------------------------------

distributor_rpc::distributor_rpc(asio::strand& strand) NOEXCEPT
  : get_version_subscriber_{ strand },
    add_element_subscriber_{ strand }
{
}

void distributor_rpc::stop(const code& ec) NOEXCEPT
{
    get_version_subscriber_.stop_default(ec);
    add_element_subscriber_.stop_default(ec);
}

code distributor_rpc::notify(const json::request_t& request) NOEXCEPT
{
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    const auto it = dispatch_.find(request.method);
    if (it == dispatch_.end())
        return error::not_found;

    return it->second(*this, request.params);
    BC_POP_WARNING()
}

} // namespace network
} // namespace libbitcoin
