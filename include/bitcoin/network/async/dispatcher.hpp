/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_NETWORK_ASYNC_DISPATCHER_HPP
#define LIBBITCOIN_NETWORK_ASYNC_DISPATCHER_HPP

#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/asio.hpp>
#include <bitcoin/network/async/deadline.hpp>
#include <bitcoin/network/async/delegates.hpp>
#include <bitcoin/network/async/threadpool.hpp>
#include <bitcoin/network/async/work.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

/// dispatcher->()
/// dispatcher->work->[service|strand|sequence->service]
/// dispatcher->delegate->work->[service|strand|sequence->service]

/// This class is thread safe.
/// If the ios service is stopped jobs will not be dispatched.
/// This is the originator of work executed by the network.
class BCT_API dispatcher
  : system::noncopyable
{
public:
    typedef std::function<void(const code&)> delay_handler;

    dispatcher(threadpool& pool, const std::string& name);

    /// Executes a job immediately on the current thread.
    template <typename... Args>
    static void bound(Args&&... args)
    {
        std::bind(FORWARD_ARGS(args))();
    }

    /// Executes a job after specified delay, on the timer thread.
    /// The timer cannot be canceled as no reference is retained.
    /// This is used for delayed retry network operations.
    inline void delayed(const duration& delay, delay_handler handler)
    {
        auto timer = std::make_shared<deadline>(pool_, delay);
        timer->start([handler, timer](const code& ec)
        {
            handler(ec);
            timer->stop();
        });
    }

    /// Posts a job to the service. Concurrent and not ordered.
    template <typename... Args>
    void concurrent(Args&&... args)
    {
        heap_->concurrent(std::bind(FORWARD_ARGS(args)));
    }

    /// Post a job to the strand. Ordered and not concurrent.
    template <typename... Args>
    void ordered(Args&&... args)
    {
        heap_->ordered(std::bind(FORWARD_ARGS(args)));
    }

    /// Posts a strand-wrapped job to the service. Not ordered or concurrent.
    /// The wrap provides non-concurrency, order is precluded by service post.
    template <typename... Args>
    void unordered(Args&&... args)
    {
        heap_->unordered(std::bind(FORWARD_ARGS(args)));
    }

    /// Posts an asynchronous job to the sequencer. Ordered and not concurrent.
    /// The sequencer provides both non-concurrency and ordered execution.
    /// Succesive calls to lock are enqueued until the next unlock call.
    template <typename... Args>
    void lock(Args&&... args)
    {
        heap_->lock(std::bind(FORWARD_ARGS(args)));
    }

    /// Complete sequential execution.
    inline void unlock()
    {
        heap_->unlock();
    }

    /// Returns a delegate that will execute the job on the current thread.
    template <typename... Args>
    static auto bound_delegate(Args&&... args) ->
        delegates::bound<decltype(std::bind(FORWARD_ARGS(args)))>
    {
        return
        {
            std::bind(FORWARD_ARGS(args))
        };
    }

    /// Returns a delegate that will post the job via the service.
    template <typename... Args>
    auto concurrent_delegate(Args&&... args) ->
        delegates::concurrent<decltype(std::bind(FORWARD_ARGS(args)))>
    {
        return
        {
            std::bind(FORWARD_ARGS(args)),
            heap_
        };
    }

    /// Returns a delegate that will post the job via the strand.
    template <typename... Args>
    auto ordered_delegate(Args&&... args) ->
        delegates::ordered<decltype(std::bind(FORWARD_ARGS(args)))>
    {
        return
        {
            std::bind(FORWARD_ARGS(args)),
            heap_
        };
    }

    /// Returns a delegate that will post a wrapped job via the service.
    template <typename... Args>
    auto unordered_delegate(Args&&... args) ->
        delegates::unordered<decltype(std::bind(FORWARD_ARGS(args)))>
    {
        return
        {
            std::bind(FORWARD_ARGS(args)),
            heap_
        };
    }

    /// Returns a delegate that will post a job via the sequencer.
    template <typename... Args>
    auto sequence_delegate(Args&&... args) ->
        delegates::sequence<decltype(std::bind(FORWARD_ARGS(args)))>
    {
        return
        {
            BIND_ARGS(args),
            heap_
        };
    }

    /// The size of the dispatcher's threadpool at the time of calling.
    inline size_t size() const
    {
        return pool_.size();
    }

private:

    // These are thread safe.
    work::ptr heap_;
    threadpool& pool_;
};

} // namespace network
} // namespace libbitcoin

#endif

////#include <bitcoin/network/async/synchronizer.hpp>
////#include <vector>
////
////// Collection dispatch doesn't forward args as move args can only forward once.
////#define BIND_RACE(args, call) \
////    std::bind(args..., call)
////#define BIND_ELEMENT(args, element, call) \
////    std::bind(args..., element, call)
////
/////// Executes multiple identical jobs concurrently until one completes.
////template <typename Count, typename Handler, typename... Args>
////void race(Count count, const std::string& name, Handler&& handler,
////    Args... args)
////{
////    // The first fail will also terminate race and return the code.
////    static const size_t clearance_count = 1;
////    const auto call = synchronize(FORWARD_HANDLER(handler),
////        clearance_count, name, false);
////
////    for (Count iteration = 0; iteration < count; ++iteration)
////        concurrent(BIND_RACE(args, call));
////}
////
/////// Executes the job against each member of a collection concurrently.
////template <typename Element, typename Handler, typename... Args>
////void parallel(const std::vector<Element>& collection,
////    const std::string& name, Handler&& handler, Args... args)
////{
////    // Failures are suppressed, success always returned to handler.
////    const auto call = synchronize(FORWARD_HANDLER(handler),
////        collection.size(), name, true);
////
////    for (const auto& element: collection)
////        concurrent(BIND_ELEMENT(args, element, call));
////}
////
/////// Disperses the job against each member of a collection without order.
////template <typename Element, typename Handler, typename... Args>
////void disperse(const std::vector<Element>& collection,
////    const std::string& name, Handler&& handler, Args... args)
////{
////    // Failures are suppressed, success always returned to handler.
////    const auto call = synchronize(FORWARD_HANDLER(handler),
////        collection.size(), name, true);
////
////    for (const auto& element: collection)
////        unordered(BIND_ELEMENT(args, element, call));
////}
////
/////// Disperses the job against each member of a collection with order.
////template <typename Element, typename Handler, typename... Args>
////void serialize(const std::vector<Element>& collection,
////    const std::string& name, Handler&& handler, Args... args)
////{
////    // Failures are suppressed, success always returned to handler.
////    const auto call = synchronize(FORWARD_HANDLER(handler),
////        collection.size(), name, true);
////
////    for (const auto& element: collection)
////        ordered(BIND_ELEMENT(args, element, call));
////}
////
/////// Sequences the job against each member of a collection with order.
////template <typename Element, typename Handler, typename... Args>
////void sequential(const std::vector<Element>& collection,
////    const std::string& name, Handler&& handler, Args... args)
////{
////    // Failures are suppressed, success always returned to handler.
////    const auto call = synchronize(FORWARD_HANDLER(handler),
////        collection.size(), name, true);
////
////    for (const auto& element: collection)
////        sequence(BIND_ELEMENT(args, element, call));
////}