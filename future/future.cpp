//
// future.cpp
// future
//
// Created by Mathieu Garaud on 24/08/2017.
//
// MIT License
//
// Copyright © 2017 Pretty Simple
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include "future.hpp"

#include "system_error.hpp"

namespace ps
{
    
    // future_error_category
    
    class __attribute__((__visibility__("hidden"))) future_error_category : public do_message
    {
    public:
        const char* name() const noexcept override;
        std::string message(int ev) const override;
    };
    
    const char* future_error_category::name() const noexcept
    {
        return "future";
    }
    
    std::string future_error_category::message(int ev) const
    {
        switch (static_cast<future_errc>(ev))
        {
            case future_errc::broken_promise:
                return std::string("The associated promise has been destructed prior to the associated state becoming ready.");
            case future_errc::future_already_retrieved:
                return std::string("The future has already been retrieved from the promise or packaged_task.");
            case future_errc::promise_already_satisfied:
                return std::string("The state of the promise has already been set.");
            case future_errc::no_state:
                return std::string("Operation not permitted on an object without an associated state.");
        }
        return std::string("unspecified future_errc value\n");
    }
    
    // future_category
    
    const std::error_category& future_category() noexcept
    {
        static future_error_category f;
        return f;
    }
    
    // future_error
    
    future_error::future_error(std::error_code ec) : std::logic_error(ec.message()), _ec(ec)
    {
    }
    
    future_error::future_error(future_errc ev) : std::logic_error(future_category().message(static_cast<int>(ev))), _ec(make_error_code(ev))
    {
    }
    
    future_error::~future_error() noexcept
    {
    }
    
    // assoc_sub_state
    
    void assoc_sub_state::on_zero_shared() noexcept
    {
        delete this;
    }
    
    void assoc_sub_state::set_value()
    {
        bool continuation = false;
        {
            std::unique_lock<std::mutex> lk(_mut);
            if (has_value())
            {
                throw future_error(make_error_code(future_errc::promise_already_satisfied));
            }
            _status |= constructed | ready;
            _cv.notify_all();
            if (has_continuation())
            {
                continuation = true;
            }
        }
        if (continuation)
        {
            ps::invoke(std::move(_continuation), _exception);
        }
    }
    
    void assoc_sub_state::set_value_at_thread_exit()
    {
        std::unique_lock<std::mutex> lk(_mut);
        if (has_value())
        {
            throw future_error(make_error_code(future_errc::promise_already_satisfied));
        }
        _status |= constructed;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundefined-func-template"
        ASSERT(thread_local_data().get() != nullptr, "");
        thread_local_data()->make_ready_at_thread_exit(this);
#pragma clang diagnostic pop
    }
    
    void assoc_sub_state::set_exception(const std::exception_ptr& p)
    {
        bool continuation = false;
        {
            std::unique_lock<std::mutex> lk(_mut);
            if (has_value())
            {
                throw future_error(make_error_code(future_errc::promise_already_satisfied));
            }
            _exception = p;
            _status |= ready;
            _cv.notify_all();
            if (has_continuation())
            {
                continuation = true;
            }
        }
        if (continuation)
        {
            ps::invoke(std::move(_continuation), _exception);
        }
    }
    
    void assoc_sub_state::set_exception_at_thread_exit(const std::exception_ptr& p)
    {
        std::unique_lock<std::mutex> lk(_mut);
        if (has_value())
        {
            throw future_error(make_error_code(future_errc::promise_already_satisfied));
        }
        _exception = p;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundefined-func-template"
        thread_local_data()->make_ready_at_thread_exit(this);
#pragma clang diagnostic pop
    }
    
    void assoc_sub_state::then_error(fu2::unique_function<void(const std::exception_ptr&)>&& continuation)
    {
        bool ready = false;
        {
            std::unique_lock<std::mutex> lk(_mut);
            _status |= continuation_attached;
            if (is_ready())
            {
                ready = true;
            }
            else
            {
                _continuation = std::move(continuation);
            }
        }
        if (ready)
        {
            ps::invoke(std::move(continuation), _exception);
        }
    }
    
    void assoc_sub_state::make_ready()
    {
        {
            std::unique_lock<std::mutex> lk(_mut);
            _status |= ready;
        }
        if (has_continuation())
        {
            ps::invoke(std::move(_continuation), _exception);
        }
        _cv.notify_all();
    }
    
    void assoc_sub_state::copy()
    {
        std::unique_lock<std::mutex> lk(_mut);
        sub_wait(lk);
        if (_exception != nullptr)
        {
            std::rethrow_exception(_exception);
        }
    }
    
    void assoc_sub_state::wait()
    {
        std::unique_lock<std::mutex> lk(_mut);
        sub_wait(lk);
    }
    
    void assoc_sub_state::sub_wait(std::unique_lock<std::mutex>& lk)
    {
        if (!is_ready())
        {
            if ((_status & deferred) == deferred)
            {
                _status &= static_cast<std::uint8_t>(~deferred);
                lk.unlock();
                execute();
            }
            else
            {
                while (!is_ready())
                {
                    _cv.wait(lk);
                }
            }
        }
    }
    
    void assoc_sub_state::execute()
    {
        throw future_error(make_error_code(future_errc::no_state));
    }
    
    // future<void>
    
    future<void>::future(assoc_sub_state* state) : _state(state)
    {
        if (_state->has_future_attached())
        {
            throw future_error(make_error_code(future_errc::future_already_retrieved));
        }
        _state->add_shared();
        _state->set_future_attached();
    }
    
    future<void>::~future()
    {
        if (_state != nullptr)
        {
            _state->release_shared();
        }
    }
    
    void future<void>::get()
    {
        std::unique_ptr<shared_count, release_shared_count> __(_state);
        assoc_sub_state* s = _state;
        _state = nullptr;
        s->copy();
    }
    
    void future<void>::then_error(fu2::unique_function<void(const std::exception_ptr&)>&& continuation)
    {
        return _state->then_error(std::move(continuation));
    }
    
    future<void> make_ready_future()
    {
        std::unique_ptr<assoc_sub_state, release_shared_count> state{new assoc_sub_state()};
        state->set_value();
        return future<void>(state.get());
    }
    
    // promise<void>
    
    promise<void>::promise() : _state(new assoc_sub_state)
    {
    }
    
    promise<void>::~promise()
    {
        if (_state != nullptr)
        {
            if (!_state->has_value() && _state->use_count() > 1)
            {
                _state->set_exception(std::make_exception_ptr<future_error>(make_error_code(future_errc::broken_promise)));
            }
            _state->release_shared();
        }
    }
    
    future<void> promise<void>::get_future()
    {
        if (_state == nullptr)
        {
            throw future_error(make_error_code(future_errc::no_state));
        }
        return future<void>(_state);
    }
    
    void promise<void>::set_value()
    {
        if (_state == nullptr)
        {
            throw future_error(make_error_code(future_errc::no_state));
        }
        _state->set_value();
    }
    
    void promise<void>::set_exception(const std::exception_ptr& p)
    {
        if (_state == nullptr)
        {
            throw future_error(make_error_code(future_errc::no_state));
        }
        _state->set_exception(p);
    }
    
    void promise<void>::set_value_at_thread_exit()
    {
        if (_state == nullptr)
        {
            throw future_error(make_error_code(future_errc::no_state));
        }
        _state->set_value_at_thread_exit();
    }
    
    void promise<void>::set_exception_at_thread_exit(const std::exception_ptr& p)
    {
        if (_state == nullptr)
        {
            throw future_error(make_error_code(future_errc::no_state));
        }
        _state->set_exception_at_thread_exit(p);
    }
    
    // shared_future<void>
    
    shared_future<void>::~shared_future()
    {
        if (_state != nullptr)
        {
            _state->release_shared();
        }
    }
    
    void shared_future<void>::then_error(fu2::unique_function<void(const std::exception_ptr&)>&& continuation)
    {
        return _state->then_error(std::move(continuation));
    }
    
    // async_queued
    
    async_queued& get_async_queued()
    {
        static async_queued queue;
        return queue;
    }
    
    async_queued::async_queued() : _stop(false)
    {
        start();
    }
    
    async_queued::~async_queued()
    {
        stop();
    }
    
    void async_queued::post(assoc_sub_state* state)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        state->add_shared();
        _tasks.push(state);
        _cond.notify_all();
    }
    
    void async_queued::stop()
    {
        {
            std::lock_guard<std::mutex> _lock(_mutex);
            _stop = true;
            _cond.notify_all();
        }
        if (_thread.joinable())
        {
            _thread.join();
        }
    }
    
    void async_queued::start()
    {
        _thread = ps::thread([this] {
            std::unique_lock<std::mutex> lock(_mutex);
            while (!_stop)
            {
#ifdef __APPLE__
                @autoreleasepool {
#endif
                _cond.wait(lock, [this]() {
                    return _stop || !_tasks.empty();
                });
                if (_stop)
                {
                    break;
                }
                while (!_tasks.empty())
                {
                    auto state = _tasks.front();
                    _tasks.pop();
                    lock.unlock();
                    state->execute();
                    lock.lock();
                    state->release_shared();
                }
#ifdef __APPLE__
                }
#endif
            }
        });
    }
    
    // async_thread_pool
    
    async_thread_pool& get_async_thread_pool()
    {
        static async_thread_pool queue;
        return queue;
    }
    
    async_thread_worker::async_thread_worker()
    {
        _thread = ps::thread([this] {
            std::unique_lock<std::mutex> lock(_m);
            while (!_stop)
            {
#ifdef __APPLE__
                @autoreleasepool {
#endif
                _start_cond.wait(lock, [this] {
                    return _task != nullptr || _stop;
                });
                if (_stop)
                    break;
                lock.unlock();
                _task->execute();
                lock.lock();
                _task->release_shared();
                _task = nullptr;
                _has_task = false;
                ps::invoke(_completion_cb);
#ifdef __APPLE__
                }
#endif
            }
        });
    }
    
    async_thread_worker::~async_thread_worker()
    {
        stop();
    }
    
    void async_thread_worker::stop()
    {
        {
            std::lock_guard<std::mutex> lock(_m);
            _stop = true;
            _start_cond.notify_all();
        }
        if (_thread.joinable())
        {
            _thread.join();
        }
    }
    
    void async_thread_worker::post(assoc_sub_state* task, fu2::unique_function<void()>&& completion_cb)
    {
        std::unique_lock<std::mutex> lock(_m);
        if (_task != nullptr)
        {
            throw std::logic_error("Worker already has a pending task");
        }
        start_task(task, std::move(completion_cb));
    }
    
    void async_thread_worker::start_task(assoc_sub_state* task, fu2::unique_function<void()>&& completion_cb)
    {
        _has_task = true;
        _task = task;
        _completion_cb = std::move(completion_cb);
        _start_cond.notify_one();
    }
    
    async_thread_pool::async_thread_pool() : _tp(ps::thread::hardware_concurrency())
    , _available_count(ps::thread::hardware_concurrency())
    {
        _manager_thread = ps::thread([this] {
            std::unique_lock<std::mutex> lock(_mutex);
            while (!_stop)
            {
#ifdef __APPLE__
                @autoreleasepool {
#endif
                _cond.wait(lock, [this] {
                    return _stop || (!_task_queue.empty() && _available_count > 0);
                });
                while (!_task_queue.empty() && _available_count > 0)
                {
                    if (_stop)
                    {
                        break;
                    }
                    auto ready_it = std::find_if(_tp.begin(), _tp.end(), [](const async_thread_worker& worker) {
                        return worker.available();
                    });
                    if (ready_it == _tp.end())
                    {
                        break;
                    }
                    auto task = _task_queue.front();
                    _task_queue.pop();
                    --_available_count;
                    ready_it->post(task, [this] {
                        ++_available_count;
                        _cond.notify_one();
                    });
                }
#ifdef __APPLE__
                }
#endif
            }
        });
    }
    
    async_thread_pool::~async_thread_pool()
    {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _stop = true;
            _cond.notify_all();
        }
        if (_manager_thread.joinable())
        {
            _manager_thread.join();
        }
        for (auto& worker : _tp)
        {
            worker.stop();
        }
    }
    
    void async_thread_pool::post(assoc_sub_state* task)
    {
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _task_queue.push(task);
            task->add_shared();
        }
        _cond.notify_one();
    }
    
} // namespace ps
