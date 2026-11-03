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
#include <bitcoin/network/io/context.hpp>

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/io/strand.hpp>
#include "scheduler.hpp"

namespace libbitcoin {
namespace network {
namespace io {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// executor
// ----------------------------------------------------------------------------

executor::executor(io::context& context) NOEXCEPT
  : context_(&context), strand_{}
{
}

executor::executor(io::context& context,
    const std::shared_ptr<detail::strand_state>& strand) NOEXCEPT
  : context_(&context), strand_(strand)
{
}

io::context& executor::context() const NOEXCEPT
{
    return *context_;
}

void executor::post(handler&& handler) const NOEXCEPT
{
    if (strand_)
        strand_->post(std::move(handler));
    else
        context_->post(std::move(handler));
}

void executor::dispatch(handler&& handler) const NOEXCEPT
{
    if (running_in_this_thread())
        handler();
    else if (strand_)
        strand_->dispatch(std::move(handler));
    else
        context_->post(std::move(handler));
}

bool executor::running_in_this_thread() const NOEXCEPT
{
    return strand_ ? strand_->running_in_this_thread() :
        context_->running_in_this_thread();
}

bool executor::operator==(const executor& other) const NOEXCEPT
{
    return context_ == other.context_ && strand_ == other.strand_;
}

// strand
// ----------------------------------------------------------------------------

strand::strand(const executor& executor) NOEXCEPT
  : strand(executor.context())
{
}

strand::strand(io::context& context) NOEXCEPT
  : executor(context, std::make_shared<detail::strand_state>(context))
{
}

// context
// ----------------------------------------------------------------------------

context::context() NOEXCEPT
  : scheduler_(std::make_unique<detail::scheduler>())
{
}

context::~context() NOEXCEPT
{
    scheduler_->shutdown();
}

size_t context::run() NOEXCEPT
{
    return scheduler_->run();
}

void context::stop() NOEXCEPT
{
    scheduler_->stop();
}

bool context::stopped() const NOEXCEPT
{
    return scheduler_->stopped();
}

void context::restart() NOEXCEPT
{
    scheduler_->restart();
}

context::executor_type context::get_executor() NOEXCEPT
{
    return { *this };
}

void context::post(handler&& handler) NOEXCEPT
{
    scheduler_->post(std::move(handler));
}

void context::dispatch(handler&& handler) NOEXCEPT
{
    if (running_in_this_thread())
        handler();
    else
        post(std::move(handler));
}

bool context::running_in_this_thread() const NOEXCEPT
{
    return scheduler_->running_in_this_thread();
}

detail::scheduler& context::scheduler() NOEXCEPT
{
    return *scheduler_;
}

// work_guard
// ----------------------------------------------------------------------------

work_guard::work_guard(io::context& context) NOEXCEPT
  : context_(&context)
{
    context_->scheduler().work_started();
}

work_guard::work_guard(work_guard&& other) NOEXCEPT
  : context_(other.context_)
{
    other.context_ = nullptr;
}

work_guard::~work_guard() NOEXCEPT
{
    reset();
}

void work_guard::reset() NOEXCEPT
{
    if (context_ != nullptr)
    {
        context_->scheduler().work_finished();
        context_ = nullptr;
    }
}

// Free functions.
// ----------------------------------------------------------------------------

void post(io::context& context, handler&& handler) NOEXCEPT
{
    context.post(std::move(handler));
}

void post(const executor& executor, handler&& handler) NOEXCEPT
{
    executor.post(std::move(handler));
}

void dispatch(io::context& context, handler&& handler) NOEXCEPT
{
    context.dispatch(std::move(handler));
}

void dispatch(const executor& executor, handler&& handler) NOEXCEPT
{
    executor.dispatch(std::move(handler));
}

BC_POP_WARNING()

} // namespace io
} // namespace network
} // namespace libbitcoin
