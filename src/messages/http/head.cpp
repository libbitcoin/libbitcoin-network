/////**
//// * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
//// *
//// * This file is part of libbitcoin.
//// *
//// * This program is free software: you can redistribute it and/or modify
//// * it under the terms of the GNU Affero General Public License as published by
//// * the Free Software Foundation, either version 3 of the License, or
//// * (at your option) any later version.
//// *
//// * This program is distributed in the hope that it will be useful,
//// * but WITHOUT ANY WARRANTY; without even the implied warranty of
//// * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//// * GNU Affero General Public License for more details.
//// *
//// * You should have received a copy of the GNU Affero General Public License
//// * along with this program.  If not, see <http://www.gnu.org/licenses/>.
//// */
////#include <bitcoin/network/messages/http/head.hpp>
////
////#include <utility>
////#include <variant>
////#include <bitcoin/network/define.hpp>
////
////namespace libbitcoin {
////namespace network {
////namespace http {
////
////using namespace system;
////using namespace network::error;
////    
////// http::head::reader
////// ----------------------------------------------------------------------------
////
////void head::reader::init(const length_type& length, boost_code& ec) NOEXCEPT
////{
////    std::visit(overload
////    {
////        [&](request_reader& read) NOEXCEPT
////        {
////            try
////            {
////                if (length.has_value())
////                    read.header_limit(narrow_cast<uint32_t>(length.value()));
////
////                read.eager(true);
////                ec.clear();
////            }
////            catch (...)
////            {
////                ec = to_boost_code(boost_error_t::io_error);
////            }
////        },
////        [&](response_reader&) NOEXCEPT
////        {
////            // Server doesn't read responses.
////            ec = to_boost_code(boost_error_t::operation_not_supported);
////        }
////    }, reader_);
////}
////
////size_t head::reader::put(const buffer_type& buffer, boost_code& ec) NOEXCEPT
////{
////    return std::visit(overload
////    {
////        [&](request_reader& read) NOEXCEPT
////        {
////            try
////            {
////                const auto consumed = read.put(boost::asio::buffer(buffer), ec);
////                if (ec == boost::beast::http::error::need_more)
////                {
////                    ec.clear();
////                    return buffer.size();
////                }
////
////                if (ec)
////                    return zero;
////
////                if (read.is_header_done())
////                    value_.value().emplace<request_header>() = read.get();
////
////                return consumed;
////            }
////            catch (...)
////            {
////                ec = to_boost_code(boost_error_t::io_error);
////                return zero;
////            }
////        },
////        [&](response_reader&) NOEXCEPT
////        {
////            // Server doesn't read responses.
////            ec = to_boost_code(boost_error_t::operation_not_supported);
////            return zero;
////        }
////    }, reader_);
////}
////
////void head::reader::finish(boost_code& ec) NOEXCEPT
////{
////    std::visit(overload
////    {
////        [&] (request_reader& read) NOEXCEPT
////        {
////            try
////            {
////                if (read.is_header_done())
////                {
////                    ec.clear();
////                    return;
////                }
////
////                ec = boost::beast::http::error::partial_message;
////            }
////            catch (...)
////            {
////                ec = to_boost_code(boost_error_t::io_error);
////            }
////        },
////        [&](response_reader&) NOEXCEPT
////        {
////            // Server doesn't read responses.
////            ec = error::operation_failed;
////        }
////    }, reader_);
////}
////
////// http::head::writer
////// ----------------------------------------------------------------------------
////    
////void head::writer::init(boost_code& ec) NOEXCEPT
////{
////    return std::visit(overload
////    {
////        [&] (request_writer&) NOEXCEPT
////        {
////            // Server doesn't write requests.
////            ec = to_boost_code(boost_error_t::operation_not_supported);
////        },
////        [&](response_writer&) NOEXCEPT
////        {
////            ec.clear();
////        }
////    }, writer_);
////}
////
////head::writer::out_buffer head::writer::get(boost_code& ec) NOEXCEPT
////{
////    std::visit(overload
////    {
////        [&](request_writer&) NOEXCEPT -> out_buffer
////        {
////            // Server doesn't write requests.
////            ec = to_boost_code(boost_error_t::operation_not_supported);
////            return {};
////        },
////        [&](response_writer& write) NOEXCEPT -> out_buffer
////        {
////            try
////            {
////                if (write.is_done())
////                {
////                    ec = to_boost_code(boost_error_t::success);
////                    return {};
////                }
////
////                out_buffer buffer{};
////                write.next(ec, [&](boost_code&, const auto& buffers) NOEXCEPT
////                {
////                    constexpr auto more = true;
////                });
////
////                return buffer;
////            }
////            catch (...)
////            {
////                ec = to_boost_code(boost_error_t::io_error);
////                return {};
////            }
////        }
////    }, writer_);
////}
////
////} // namespace http
////} // namespace network
////} // namespace libbitcoin
