/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
#include "../test.hpp"

BC_AUTO_TEST_SUITE(io_context_tests)

BC_AUTO_TEST_CASE(io_context__run__no_work__zero_stopped)
{
    io::context context{};
    BC_REQUIRE_EQUAL(context.run(), 0u);
    BC_REQUIRE(context.stopped());
}

BC_AUTO_TEST_CASE(io_context__run__posted__all_run_in_order)
{
    io::context context{};
    std::vector<size_t> order{};
    for (size_t index{}; index < 5u; ++index)
        context.post([&order, index]() NOEXCEPT { order.push_back(index); });

    BC_REQUIRE_EQUAL(context.run(), 5u);
    BC_REQUIRE_EQUAL(order, (std::vector<size_t>{ 0, 1, 2, 3, 4 }));
    BC_REQUIRE(context.stopped());
}

BC_AUTO_TEST_CASE(io_context__run__nested_post__run)
{
    io::context context{};
    auto inner = false;
    context.post([&]() NOEXCEPT
    {
        context.post([&]() NOEXCEPT { inner = true; });
    });

    BC_REQUIRE_EQUAL(context.run(), 2u);
    BC_REQUIRE(inner);
}

BC_AUTO_TEST_CASE(io_context__dispatch__running__invoked_inline)
{
    io::context context{};
    std::vector<size_t> order{};
    context.post([&]() NOEXCEPT
    {
        context.dispatch([&]() NOEXCEPT { order.push_back(1); });
        order.push_back(2);
    });

    context.run();
    BC_REQUIRE_EQUAL(order, (std::vector<size_t>{ 1, 2 }));
}

BC_AUTO_TEST_CASE(io_context__dispatch__not_running__posted)
{
    io::context context{};
    auto invoked = false;
    context.dispatch([&]() NOEXCEPT { invoked = true; });
    BC_REQUIRE(!invoked);
    BC_REQUIRE(!context.running_in_this_thread());
    context.run();
    BC_REQUIRE(invoked);
}

BC_AUTO_TEST_CASE(io_context__running_in_this_thread__in_handler__true)
{
    io::context context{};
    auto running = false;
    context.post([&]() NOEXCEPT { running = context.running_in_this_thread(); });
    context.run();
    BC_REQUIRE(running);
}

BC_AUTO_TEST_CASE(io_context__stop__in_handler__remaining_retained)
{
    io::context context{};
    size_t count{};
    context.post([&]() NOEXCEPT { ++count; context.stop(); });
    context.post([&]() NOEXCEPT { ++count; });
    BC_REQUIRE_EQUAL(context.run(), 1u);
    BC_REQUIRE_EQUAL(count, 1u);
    BC_REQUIRE(context.stopped());

    context.restart();
    BC_REQUIRE(!context.stopped());
    BC_REQUIRE_EQUAL(context.run(), 1u);
    BC_REQUIRE_EQUAL(count, 2u);
}

BC_AUTO_TEST_CASE(io_context__work_guard__reset_from_other_thread__run_returns)
{
    io::context context{};
    io::work_guard guard{ context };
    std::promise<void> started{};
    context.post([&]() NOEXCEPT { started.set_value(); });

    std::thread runner{ [&]() NOEXCEPT { context.run(); } };
    started.get_future().wait();
    guard.reset();
    runner.join();
    BC_REQUIRE(context.stopped());
}

BC_AUTO_TEST_CASE(io_context__run__multiple_threads__all_run)
{
    io::context context{};
    io::work_guard guard{ context };
    std::atomic<size_t> count{};
    std::vector<std::thread> threads{};
    for (size_t index{}; index < 4u; ++index)
        threads.emplace_back([&]() NOEXCEPT { context.run(); });

    for (size_t index{}; index < 1000u; ++index)
        io::post(context, [&]() NOEXCEPT { ++count; });

    // The guard is released by the last handler so all handlers run first.
    std::promise<void> drained{};
    io::post(context, [&]() NOEXCEPT { drained.set_value(); });
    drained.get_future().wait();
    guard.reset();

    for (auto& thread: threads)
        thread.join();

    BC_REQUIRE_EQUAL(count.load(), 1000u);
}

// strand

BC_AUTO_TEST_CASE(io_strand__post__ordered_and_not_concurrent)
{
    io::context context{};
    io::work_guard guard{ context };
    const io::strand strand{ context };
    std::vector<size_t> order{};
    std::atomic<size_t> active{};
    std::atomic<bool> overlapped{};
    std::vector<std::thread> threads{};
    for (size_t index{}; index < 4u; ++index)
        threads.emplace_back([&]() NOEXCEPT { context.run(); });

    std::promise<void> drained{};
    for (size_t index{}; index < 500u; ++index)
    {
        io::post(strand, [&, index]() NOEXCEPT
        {
            if (++active != 1u) overlapped = true;
            order.push_back(index);
            --active;
        });
    }

    io::post(strand, [&]() NOEXCEPT { drained.set_value(); });
    drained.get_future().wait();
    guard.reset();
    for (auto& thread: threads)
        thread.join();

    std::vector<size_t> expected(500);
    std::iota(expected.begin(), expected.end(), size_t{});
    BC_REQUIRE(!overlapped.load());
    BC_REQUIRE_EQUAL(order, expected);
}

BC_AUTO_TEST_CASE(io_strand__running_in_this_thread__expected)
{
    io::context context{};
    const io::strand strand{ context };
    const io::strand other{ context };
    auto in_strand = false;
    auto in_other = true;
    io::post(strand, [&]() NOEXCEPT
    {
        in_strand = strand.running_in_this_thread();
        in_other = other.running_in_this_thread();
    });

    context.run();
    BC_REQUIRE(in_strand);
    BC_REQUIRE(!in_other);
    BC_REQUIRE(!strand.running_in_this_thread());
}

BC_AUTO_TEST_CASE(io_strand__dispatch__in_strand__inline)
{
    io::context context{};
    const io::strand strand{ context };
    std::vector<size_t> order{};
    io::post(strand, [&]() NOEXCEPT
    {
        io::dispatch(strand, [&]() NOEXCEPT { order.push_back(1); });
        order.push_back(2);
    });

    context.run();
    BC_REQUIRE_EQUAL(order, (std::vector<size_t>{ 1, 2 }));
}

BC_AUTO_TEST_CASE(io_strand__copy__same_strand)
{
    io::context context{};
    const io::strand strand{ context };
    const io::strand copy{ strand };
    const io::strand other{ context };
    BC_REQUIRE(copy == strand);
    BC_REQUIRE(!(other == strand));
}

// steady_timer

BC_AUTO_TEST_CASE(io_steady_timer__past_expiry__success)
{
    io::context context{};
    io::steady_timer timer{ context };
    code result{ error::unknown };
    timer.expires_after(std::chrono::seconds(-1));
    timer.async_wait([&](const code& ec) NOEXCEPT { result = ec; });
    context.run();
    BC_REQUIRE_EQUAL(result, error::success);
}

BC_AUTO_TEST_CASE(io_steady_timer__short_expiry__success)
{
    io::context context{};
    io::steady_timer timer{ context };
    code result{ error::unknown };
    timer.expires_after(std::chrono::milliseconds(1));
    timer.async_wait([&](const code& ec) NOEXCEPT { result = ec; });
    context.run();
    BC_REQUIRE_EQUAL(result, error::success);
}

BC_AUTO_TEST_CASE(io_steady_timer__cancel__operation_canceled)
{
    io::context context{};
    io::steady_timer timer{ context };
    code result{};
    timer.expires_after(std::chrono::hours(1));
    timer.async_wait([&](const code& ec) NOEXCEPT { result = ec; });
    BC_REQUIRE_EQUAL(timer.cancel(), 1u);
    context.run();
    BC_REQUIRE_EQUAL(result, error::operation_canceled);
}

BC_AUTO_TEST_CASE(io_steady_timer__expires_at__pending_canceled)
{
    io::context context{};
    io::steady_timer timer{ context };
    code first{};
    code second{ error::unknown };
    timer.expires_after(std::chrono::hours(1));
    timer.async_wait([&](const code& ec) NOEXCEPT { first = ec; });
    BC_REQUIRE_EQUAL(timer.expires_after(std::chrono::seconds(-1)), 1u);
    timer.async_wait([&](const code& ec) NOEXCEPT { second = ec; });
    context.run();
    BC_REQUIRE_EQUAL(first, error::operation_canceled);
    BC_REQUIRE_EQUAL(second, error::success);
}

BC_AUTO_TEST_CASE(io_steady_timer__expiries__all_completed)
{
    io::context context{};
    io::steady_timer later{ context };
    io::steady_timer sooner{ context };
    const auto now = io::steady_timer::clock_type::now();
    std::vector<size_t> order{};
    later.expires_at(now - std::chrono::seconds(1));
    sooner.expires_at(now - std::chrono::seconds(2));
    later.async_wait([&](const code&) NOEXCEPT { order.push_back(2); });
    sooner.async_wait([&](const code&) NOEXCEPT { order.push_back(1); });
    context.run();
    BC_REQUIRE(order == (std::vector<size_t>{ 1, 2 }) || order == (std::vector<size_t>{ 2, 1 }));
}

BC_AUTO_TEST_CASE(io_steady_timer__strand__completes_in_strand)
{
    io::context context{};
    const io::strand strand{ context };
    io::steady_timer timer{ strand };
    auto stranded = false;
    timer.expires_after(std::chrono::seconds(-1));
    timer.async_wait([&](const code&) NOEXCEPT { stranded = strand.running_in_this_thread(); });
    context.run();
    BC_REQUIRE(stranded);
}

BC_AUTO_TEST_SUITE_END()
