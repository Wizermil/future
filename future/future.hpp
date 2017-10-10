//
// future.hpp
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

#ifndef FUTURE_FUTURE_HPP
#define FUTURE_FUTURE_HPP

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include "cxx_function.hpp"
#pragma clang diagnostic pop
#include "memory.hpp"
#include "thread.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <functional>
#include <iterator>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace ps
{
    
    enum struct future_errc
    {
        future_already_retrieved = 1,
        promise_already_satisfied,
        no_state,
        broken_promise
    };
    
    enum struct launch
    {
        async = 1,
        deferred = 2,
        any = async | deferred,
        queued = 4,
        thread_pool = 8,
    };
    using launch_underlying_type = std::underlying_type<launch>::type;
    inline constexpr launch operator&(launch x, launch y)
    {
        return static_cast<launch>(static_cast<launch_underlying_type>(x) & static_cast<launch_underlying_type>(y));
    }
    inline constexpr launch operator|(launch x, launch y)
    {
        return static_cast<launch>(static_cast<launch_underlying_type>(x) | static_cast<launch_underlying_type>(y));
    }
    inline constexpr launch operator^(launch x, launch y)
    {
        return static_cast<launch>(static_cast<launch_underlying_type>(x) ^ static_cast<launch_underlying_type>(y));
    }
    inline constexpr launch operator~(launch x)
    {
        return static_cast<launch>(~static_cast<launch_underlying_type>(x) & 15);
    }
    inline launch& operator&=(launch& x, launch y)
    {
        x = x & y;
        return x;
    }
    inline launch& operator|=(launch& x, launch y)
    {
        x = x | y;
        return x;
    }
    inline launch& operator^=(launch& x, launch y)
    {
        x = x ^ y;
        return x;
    }
    
    enum struct future_status
    {
        ready,
        timeout,
        deferred
    };
    
    // future_error
    
    const std::error_category& future_category() noexcept;
    
    inline std::error_code make_error_code(future_errc e) noexcept
    {
        return std::error_code(static_cast<int>(e), future_category());
    }
    
    inline std::error_condition make_error_condition(future_errc e) noexcept
    {
        return std::error_condition(static_cast<int>(e), future_category());
    }
    
    class future_error : public std::logic_error
    {
        std::error_code _ec;
    public:
        future_error(std::error_code ec);
        explicit future_error(future_errc ev);
        future_error(const future_error&) =default;
        future_error& operator=(const future_error&) = default;
        future_error(future_error&&) noexcept = default;
        future_error& operator=(future_error&&) noexcept = default;
        ~future_error() noexcept override;
        
        inline const std::error_code& code() const noexcept
        {
            return _ec;
        }
    };
    
    [[noreturn]] inline void throw_future_error(future_errc ev)
    {
        throw future_error(make_error_code(ev));
    }
    
    // assoc_sub_state
    
    template<class T>
    class assoc_state;
    
    template<class T>
    class future;
    template<class T>
    class shared_future;
    
    template<class T>
    class promise;
    
    template<class T>
    struct __attribute__((__visibility__("hidden"))) is_future : public std::false_type
    {
    };
    
    template<class T>
    struct __attribute__((__visibility__("hidden"))) is_future<future<T>> : public std::true_type
    {
    };
    
    template<typename T>
    struct __attribute__((__visibility__("hidden"))) future_held
    {
        using type = T;
    };
    
    template<typename T>
    struct __attribute__((__visibility__("hidden"))) future_held<future<T>>
    {
        using type = std::decay_t<T>;
    };
    
    template<class T, class F, class Arg = future<T>>
    using future_then_ret_t = std::result_of_t<std::decay_t<F>(Arg)>;
    
    template<typename T, typename F, class Arg = future<T>>
    using future_then_t = std::conditional_t<is_future<future_then_ret_t<T, F, Arg>>::value, future_then_ret_t<T, F, Arg>, future<future_then_ret_t<T, F, Arg>>>;
    
    class assoc_sub_state : public shared_count
    {
    protected:
        std::exception_ptr _exception { nullptr};
        mutable std::mutex _mut;
        mutable std::condition_variable _cv;
        std::atomic<unsigned> _status {0};
        cxx_function::unique_function<void(const std::exception_ptr&)> _continuation {nullptr};
        
        void on_zero_shared() noexcept override;
        void sub_wait(std::unique_lock<std::mutex>& lk);

        template<class, class>
        friend class async_assoc_state;
        template<class, class>
        friend class deferred_assoc_state;
    public:
        enum
        {
            constructed = 1,
            future_attached = 2,
            ready = 4,
            deferred = 8,
            queued = 16,
            thread_pool = 32,
            continuation_attached = 64,
        };
        
        inline assoc_sub_state() = default;
        virtual ~assoc_sub_state() = default;
        
        inline bool has_value() const
        {
            return ((_status & constructed) == constructed) || (_exception != nullptr);
        }
        
        inline void set_future_attached()
        {
            std::lock_guard<std::mutex> lk(_mut);
            _status |= future_attached;
        }
        
        inline bool has_future_attached() const
        {
            return (_status & future_attached) != 0;
        }
        
        inline void set_deferred()
        {
            _status |= deferred;
        }
        
        inline void set_queued()
        {
            _status |= queued;
        }
        
        inline void set_thread_pool()
        {
            _status |= thread_pool;
        }
        
        void make_ready();
        inline bool is_ready() const
        {
            return (_status & ready) != 0;
        }
        
        void set_value();
        void set_value_at_thread_exit();
        
        void set_exception(const std::exception_ptr& p);
        void set_exception_at_thread_exit(const std::exception_ptr& p);
        
        void then_error(cxx_function::unique_function<void(const std::exception_ptr&)>&& continuation);
        template<class T, class F, class Arg = future<T>>
        future_then_t<T, F, Arg> then(Arg&& future, F&& func);
        inline bool has_continuation() const
        {
            return (_status & continuation_attached) != 0;
        }
        
        void copy();
        
        void wait();
        template<class Rep, class Period>
        inline future_status wait_for(const std::chrono::duration<Rep, Period>& rel_time) const;
        template<class Clock, class Duration>
        future_status wait_until(const std::chrono::time_point<Clock, Duration>& abs_time) const;
        
        virtual void execute();
    };
    
    template<class Rep, class Period>
    inline future_status assoc_sub_state::wait_for(const std::chrono::duration<Rep, Period>& rel_time) const
    {
        return wait_until(std::chrono::steady_clock::now() + rel_time);
    }
    
    template<class Clock, class Duration>
    future_status assoc_sub_state::wait_until(const std::chrono::time_point<Clock, Duration>& abs_time) const
    {
        std::unique_lock<std::mutex> lk(_mut);
        if (_status & deferred)
        {
            return future_status::deferred;
        }
        while (!(_status & ready) && Clock::now() < abs_time)
        {
            _cv.wait_until(lk, abs_time);
        }
        if (_status & ready)
        {
            return future_status::ready;
        }
        return future_status::timeout;
    }
    
    template<class T, class F, class Arg>
    future_then_t<T, F, Arg> assoc_sub_state::then(Arg&& future, F&& func)
    {
        using R = typename future_held<future_then_ret_t<T, F, Arg>>::type;
        promise<R> prom;
        auto ret = prom.get_future();
        
        then_error([fut = std::forward<Arg>(future), p = std::move(prom), f = std::forward<F>(func)](const std::exception_ptr& exception) mutable {
            if (exception != nullptr)
            {
                p.set_exception(exception);
            }
            else
            {
                try
                {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++1z-extensions"
                    if constexpr(std::is_void<R>::value && is_future<future_then_ret_t<T, F, Arg>>::value)
                    {
                        ps::invoke(std::forward<F>(f), std::forward<Arg>(fut)).then_error([prom_fut = std::move(p)](const std::exception_ptr& except) mutable {
                            if (except == nullptr)
                            {
                                prom_fut.set_value();
                            }
                            else
                            {
                                prom_fut.set_exception(except);
                            }
                        });
                    }
                    else if constexpr(std::is_void<R>::value && !is_future<future_then_ret_t<T, F, Arg>>::value)
                    {
                        ps::invoke(std::forward<F>(f), std::forward<Arg>(fut));
                        p.set_value();
                    }
                    else if constexpr(!std::is_void<R>::value && !is_future<future_then_ret_t<T, F, Arg>>::value)
                    {
                        p.set_value(ps::invoke(std::forward<F>(f), std::forward<Arg>(fut)));
                    }
                    else if constexpr(!std::is_void<R>::value && is_future<future_then_ret_t<T, F, Arg>>::value)
                    {
                        auto fut_then = ps::invoke(std::forward<F>(f), std::forward<Arg>(fut));
                        fut_then._state->add_shared();
                        fut_then.then_error([prom_fut = std::move(p), t = fut_then._state](const std::exception_ptr& except) mutable {
                            if (except == nullptr)
                            {
                                t->_status &= ~future_attached;
                                future_then_ret_t<T, F, Arg> fut_arg(t);
                                prom_fut.set_value(fut_arg.get());
                            }
                            else
                            {
                                prom_fut.set_exception(except);
                            }
                            t->release_shared();
                        });
                    }
#pragma clang diagnostic pop
                }
                catch(...)
                {
                    p.set_exception(std::current_exception());
                }
            }
        });
        return ret;
    }
    
    // assoc_state
    
    template<class T>
    class assoc_state : public assoc_sub_state
    {
        using base = assoc_sub_state;
        using U = typename std::aligned_storage<sizeof(T), std::alignment_of<T>::value>::type;
    protected:
        U _value;
        
        void on_zero_shared() noexcept override;
    public:
        
        template<class Arg>
        void set_value(Arg&& arg);
        template<class Arg>
        void set_value_at_thread_exit(Arg&& arg);
        
        T move();
        typename std::add_lvalue_reference<T>::type copy();
    };
    
    template<class T>
    void assoc_state<T>::on_zero_shared() noexcept
    {
        if (_status & base::constructed)
        {
            reinterpret_cast<T*>(&_value)->~T();
        }
        delete this;
    }
    
    template<class T>
    template<class Arg>
    void assoc_state<T>::set_value(Arg&& arg)
    {
        bool continuation = false;
        {
            std::unique_lock<std::mutex> lk(_mut);
            if (has_value())
            {
                throw_future_error(future_errc::promise_already_satisfied);
            }
            new (&_value) T(std::forward<Arg>(arg));
            _status |= base::constructed | base::ready;
            _cv.notify_all();
            if (has_continuation())
            {
                continuation = true;
            }
        }
        if (continuation)
        {
            _continuation(_exception);
        }
    }
    
    template<class T>
    template<class Arg>
    void assoc_state<T>::set_value_at_thread_exit(Arg&& arg)
    {
        std::unique_lock<std::mutex> lk(_mut);
        if (has_value())
        {
            throw_future_error(future_errc::promise_already_satisfied);
        }
        new (&_value) T(std::forward<Arg>(arg));
        _status |= base::constructed;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundefined-func-template"
        ASSERT(thread_local_data().get() != nullptr, "");
        thread_local_data()->make_ready_at_thread_exit(this);
#pragma clang diagnostic pop
    }
    
    template<class T>
    T assoc_state<T>::move()
    {
        std::unique_lock<std::mutex> lk(_mut);
        sub_wait(lk);
        if (_exception != nullptr)
        {
            std::rethrow_exception(_exception);
        }
        return std::move(*reinterpret_cast<T*>(&_value));
    }
    
    template<class T>
    typename std::add_lvalue_reference<T>::type assoc_state<T>::copy()
    {
        std::unique_lock<std::mutex> lk(_mut);
        sub_wait(lk);
        if (_exception != nullptr)
        {
            std::rethrow_exception(_exception);
        }
        return *reinterpret_cast<T*>(&_value);
    }
    
    // assoc_state<T&>
    
    template<class T>
    class assoc_state<T&> : public assoc_sub_state
    {
        using base = assoc_sub_state;
        using U = T*;
    protected:
        U _value;
        
        void on_zero_shared() noexcept override;
    public:
        
        void set_value(T& arg);
        void set_value_at_thread_exit(T& arg);
        
        T& copy();
    };
    
    template<class T>
    void assoc_state<T&>::on_zero_shared() noexcept
    {
        delete this;
    }
    
    template<class T>
    void assoc_state<T&>::set_value(T& arg)
    {
        bool continuation = false;
        {
            std::unique_lock<std::mutex> lk(this->_mut);
            if (this->has_value())
            {
                throw_future_error(future_errc::promise_already_satisfied);
            }
            _value = std::addressof(arg);
            _status |= base::constructed | base::ready;
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
    
    template<class T>
    void assoc_state<T&>::set_value_at_thread_exit(T& arg)
    {
        std::unique_lock<std::mutex> lk(this->_mut);
        if (this->has_value())
        {
            throw_future_error(future_errc::promise_already_satisfied);
        }
        _value = std::addressof(arg);
        _status |= base::constructed;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundefined-func-template"
        ASSERT(thread_local_data().get() != nullptr, "");
        thread_local_data()->make_ready_at_thread_exit(this);
#pragma clang diagnostic pop
    }
    
    template<class T>
    T& assoc_state<T&>::copy()
    {
        std::unique_lock<std::mutex> lk(this->_mut);
        this->sub_wait(lk);
        if (this->_exception != nullptr)
        {
            rethrow_exception(this->_exception);
        }
        return *_value;
    }
    
    // assoc_state_alloc
    
    template<class T, class Alloc>
    class assoc_state_alloc : public assoc_state<T>
    {
        using base = assoc_state<T>;
        Alloc _alloc;
        
        virtual void on_zero_shared() noexcept;
    public:
        inline explicit assoc_state_alloc(const Alloc& a) : _alloc(a)
        {
        }
    };
    
    template<class T, class Alloc>
    void assoc_state_alloc<T, Alloc>::on_zero_shared() noexcept
    {
        if (this->_state & base::constructed)
        {
            reinterpret_cast<T*>(std::addressof(this->_value))->~T();
        }
        
        using Al = typename allocator_traits_rebind<Alloc, assoc_state_alloc>::type;
        using ATraits = std::allocator_traits<Al> ;
        using PTraits = std::pointer_traits<typename ATraits::pointer>;
        Al a(_alloc);
        ~assoc_state_alloc();
        a.deallocate(PTraits::pointer_to(*this), 1);
    }
    
    template<class T, class Alloc>
    class assoc_state_alloc<T&, Alloc> : public assoc_state<T&>
    {
        using base = assoc_state<T&> ;
        Alloc _alloc;
        
        virtual void on_zero_shared() noexcept;
    public:
        inline explicit assoc_state_alloc(const Alloc& a) : _alloc(a)
        {
        }
    };
    
    template<class T, class Alloc>
    void assoc_state_alloc<T&, Alloc>::on_zero_shared() noexcept
    {
        using Al = typename allocator_traits_rebind<Alloc, assoc_state_alloc>::type;
        using ATraits = std::allocator_traits<Al>;
        using PTraits = std::pointer_traits<typename ATraits::pointer>;
        Al a(_alloc);
        ~assoc_state_alloc();
        a.deallocate(PTraits::pointer_to(*this), 1);
    }
    
    // assoc_sub_state_alloc
    
    template<class Alloc>
    class assoc_sub_state_alloc : public assoc_sub_state
    {
        using base = assoc_sub_state;
        Alloc _alloc;
        
        void on_zero_shared() noexcept override;
    public:
        inline explicit assoc_sub_state_alloc(const Alloc& a) : _alloc(a)
        {
        }
    };
    
    template<class Alloc>
    void assoc_sub_state_alloc<Alloc>::on_zero_shared() noexcept
    {
        using Al = typename allocator_traits_rebind<Alloc, assoc_sub_state_alloc>::type;
        using ATraits = std::allocator_traits<Al>;
        using PTraits = std::pointer_traits<typename ATraits::pointer>;
        Al a(_alloc);
        ~assoc_sub_state_alloc();
        a.deallocate(PTraits::pointer_to(*this), 1);
    }
    
    // deferred_assoc_state
    
    template<class T, class F>
    class deferred_assoc_state : public assoc_state<T>
    {
        using base = assoc_state<T>;
        
        F _func;
        
    public:
        inline explicit deferred_assoc_state(F&& f) : _func(std::forward<F>(f))
        {
            this->set_deferred();
        }
        
        void execute() override;
    };
    
    template<class T, class F>
    void deferred_assoc_state<T, F>::execute()
    {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++1z-extensions"
        if constexpr(is_future<invoke_of_t<std::decay_t<F>>>::value)
        {
            auto fut = _func();
            fut._state->add_shared();
            this->add_shared();
            fut.then_error([this, state = fut._state](const std::exception_ptr& exception) mutable {
                if (exception == nullptr)
                {
                    state->_status &= ~assoc_sub_state::future_attached;
                    future<T> fut_arg(state);
                    this->set_value(fut_arg.get());
                }
                else
                {
                    this->set_exception(exception);
                }
                state->release_shared();
                this->release_shared();
            });
        }
        else
        {
            try
            {
                this->set_value(_func());
            }
            catch (...)
            {
                this->set_exception(std::current_exception());
            }
        }
#pragma clang diagnostic pop
    }
    
    template<class F>
    class deferred_assoc_state<void, F> : public assoc_sub_state
    {
        using base = assoc_sub_state;
        
        F _func;
        
    public:
        inline explicit deferred_assoc_state(F&& f) : _func(std::forward<F>(f))
        {
            set_deferred();
        }
        
        void execute() override;
    };
    
    template<class F>
    void deferred_assoc_state<void, F>::execute()
    {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++1z-extensions"
        if constexpr(is_future<invoke_of_t<std::decay_t<F>>>::value)
        {
            auto fut = _func();
            fut._state->add_shared();
            this->add_shared();
            fut.then_error([this, state = fut._state](const std::exception_ptr& exception) mutable {
                if (exception == nullptr)
                {
                    this->set_value();
                }
                else
                {
                    this->set_exception(exception);
                }
                state->release_shared();
                this->release_shared();
            });
        }
        else
        {
            try
            {
                _func();
                set_value();
            }
            catch (...)
            {
                set_exception(std::current_exception());
            }
        }
#pragma clang diagnostic pop
    }
    
    // async_assoc_state
    
    template<class T, class F>
    class async_assoc_state : public assoc_state<T>
    {
        using base = assoc_state<T>;
        
        F _func;
        
        void on_zero_shared() noexcept override;
    public:
        inline explicit async_assoc_state(F&& f) : _func(std::forward<F>(f))
        {
        }
        
        void execute() override;
    };
    
    template<class T, class F>
    void async_assoc_state<T, F>::execute()
    {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++1z-extensions"
        if constexpr(is_future<invoke_of_t<std::decay_t<F>>>::value)
        {
            auto fut = _func();
            fut._state->add_shared();
            this->add_shared();
            fut.then_error([this, state = fut._state](const std::exception_ptr& exception) mutable {
                if (exception == nullptr)
                {
                    state->_status &= ~assoc_sub_state::future_attached;
                    future<T> fut_arg(state);
                    this->set_value(fut_arg.get());
                }
                else
                {
                    this->set_exception(exception);
                }
                state->release_shared();
                this->release_shared();
            });
        }
        else
        {
            try
            {
                this->set_value(_func());
            }
            catch (...)
            {
                this->set_exception(std::current_exception());
            }
        }
#pragma clang diagnostic pop
    }
    
    template<class T, class F>
    void async_assoc_state<T, F>::on_zero_shared() noexcept
    {
        base::on_zero_shared();
    }
    
    template<class F>
    class async_assoc_state<void, F> : public assoc_sub_state
    {
        using base = assoc_sub_state;
        
        F _func;
        
        void on_zero_shared() noexcept override;
    public:
        inline explicit async_assoc_state(F&& f) : _func(std::forward<F>(f))
        {
        }
        
        void execute() override;
    };
    
    template<class F>
    void async_assoc_state<void, F>::execute()
    {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++1z-extensions"
        if constexpr(is_future<invoke_of_t<std::decay_t<F>>>::value)
        {
            auto fut = _func();
            fut._state->add_shared();
            this->add_shared();
            fut.then_error([this, state = fut._state](const std::exception_ptr& exception) mutable {
                if (exception == nullptr)
                {
                    this->set_value();
                }
                else
                {
                    this->set_exception(exception);
                }
                state->release_shared();
                this->release_shared();
            });
        }
        else
        {
            try
            {
                _func();
                set_value();
            }
            catch (...)
            {
                set_exception(std::current_exception());
            }
        }
#pragma clang diagnostic pop
    }
    
    template<class F>
    void async_assoc_state<void, F>::on_zero_shared() noexcept
    {
        base::on_zero_shared();
    }
    
    // queued_assoc_state
    
    class async_queued
    {
        std::queue<assoc_sub_state*> _tasks;
        std::mutex _mutex;
        std::condition_variable _cond;
        ps::thread _thread;
        bool _stop;
    public:
        async_queued();
        ~async_queued();
        
        void post(assoc_sub_state* state);
        
        void stop();
        
    private:
        void start();
    };
    
    // thread_pool_assoc_state
    
    class async_thread_worker
    {
        ps::thread _thread;
        mutable std::mutex _m;
        bool _stop {false};
        bool _has_task {false};
        std::condition_variable _start_cond;
        assoc_sub_state* _task {nullptr};
        cxx_function::function<void()> _completion_cb {nullptr};
    public:
        async_thread_worker();
        ~async_thread_worker();
        
        void stop();
        
        inline bool available() const
        {
            return !_has_task;
        }
        
        void post(assoc_sub_state* task, const cxx_function::function<void()>& completion_cb);
        
    private:
        void start_task(assoc_sub_state* task, const cxx_function::function<void()>& completion_cb);
    };
    
    class async_thread_pool
    {
        using tp_type = std::vector<async_thread_worker>;
        
        tp_type _tp;
        std::mutex _mutex;
        std::queue<assoc_sub_state*> _task_queue;
        ps::thread _manager_thread;
        bool _stop {false};
        std::size_t _available_count;
        std::condition_variable _cond;
        
    public:
        async_thread_pool();
        ~async_thread_pool();
        
        inline std::size_t available() const
        {
            return _available_count;
        }
        
        void post(assoc_sub_state* task);
    };
    
    // future<T>
    
    template<typename Sequence>
    struct when_any_result;
    
    template<class T, class F>
    future<T> make_deferred_assoc_state(F&& f);
    template<class T, class F>
    future<T> make_async_assoc_state(F&& f);
    template<class T, class F>
    future<T> make_queued_assoc_state(F&& f);
    template<class T, class F>
    future<T> make_thread_pool_assoc_state(F&& f);
    template<class T>
    std::conditional_t<is_reference_wrapper<std::decay_t<T>>::value, future<std::decay_t<T>&>, future<std::decay_t<T>>> make_ready_future(T&& value);
    
    template<class T>
    class future
    {
        assoc_state<T>* _state {nullptr};
        
        explicit future(assoc_state<T>* state);
        
        template<class>
        friend class promise;
        template<class>
        friend class shared_future;
        
        template<class R, class F>
        friend future<R> make_deferred_assoc_state(F&& f);
        template<class R, class F>
        friend future<R> make_async_assoc_state(F&& f);
        template<class R, class F>
        friend future<R> make_queued_assoc_state(F&& f);
        template<class R, class F>
        friend future<R> make_thread_pool_assoc_state(F&& f);
        template<typename InputIt>
        friend auto when_all(InputIt first, InputIt last) -> future<std::vector<typename std::iterator_traits<InputIt>::value_type>>;
        template<std::size_t I, typename Context, typename Future>
        friend void __attribute__((__visibility__("hidden"))) when_inner_helper(Context* context, Future&& f);
        template<typename InputIt>
        friend auto when_any(InputIt first, InputIt last) -> future<when_any_result<std::vector<typename std::iterator_traits<InputIt>::value_type>>>;
        template<size_t I, typename Context>
        friend void __attribute__((__visibility__("hidden"))) when_any_inner_helper(Context* context);
        template<class R>
        friend std::conditional_t<is_reference_wrapper<std::decay_t<R>>::value, future<std::decay_t<R>&>, future<std::decay_t<R>>> make_ready_future(R&& value);
        friend assoc_sub_state;
        template<class, class>
        friend class async_assoc_state;
        template<class, class>
        friend class deferred_assoc_state;
        
        void then_error(cxx_function::unique_function<void(const std::exception_ptr&)>&& continuation);
        
    public:
        inline future() noexcept = default;
        inline future(future&& rhs) noexcept : _state(rhs._state)
        {
            rhs._state = nullptr;
        }
        future(const future&) = delete;
        future& operator=(const future&) = delete;
        inline future& operator=(future&& rhs) noexcept
        {
            future(std::move(rhs)).swap(*this);
            return *this;
        }
        ~future();
        
        inline void swap(future& rhs) noexcept
        {
            std::swap(_state, rhs._state);
        }
        
        inline shared_future<T> share() noexcept;
        T get();
        
        inline bool valid() const noexcept
        {
            return _state != nullptr;
        }
        inline bool is_ready() const
        {
            if (_state)
            {
                return _state->is_ready();
            }
            return false;
        }
        
        template<class F>
        future_then_t<T, F> then(F&& func);
        
        inline void wait() const
        {
            _state->wait();
        }
        template<class Rep, class Period>
        inline future_status wait_for(const std::chrono::duration<Rep, Period>& rel_time) const
        {
            return _state->wait_for(rel_time);
        }
        template<class Clock, class Duration>
        inline future_status wait_until(const std::chrono::time_point<Clock, Duration>& abs_time) const
        {
            return _state->wait_until(abs_time);
        }
        
    };
    
    template<class T>
    future<T>::future(assoc_state<T>* state) : _state(state)
    {
        if (_state->has_future_attached())
        {
            throw_future_error(future_errc::future_already_retrieved);
        }
        _state->add_shared();
        _state->set_future_attached();
    }
    
    template<class T>
    future<T>::~future()
    {
        if (_state)
        {
            _state->release_shared();
        }
    }
    
    template<class T>
    T future<T>::get()
    {
        std::unique_ptr<shared_count, release_shared_count> __(_state);
        assoc_state<T>* s = _state;
        _state = nullptr;
        return s->move();
    }
    
    template<class T>
    template<class F>
    future_then_t<T, F> future<T>::then(F&& func)
    {
        return _state->template then<T, F>(std::move(*this), std::forward<F>(func));
    }
    
    template<class T>
    void future<T>::then_error(cxx_function::unique_function<void(const std::exception_ptr&)>&& continuation)
    {
        return _state->then_error(std::move(continuation));
    }
    
    // future<T&>
    
    template<class T>
    class future<T&>
    {
        assoc_state<T&>* _state {nullptr};
        
        explicit future(assoc_state<T&>* state);
        
        template<class>
        friend class promise;
        template<class>
        friend class shared_future;
        
        template<class R, class F>
        friend future<R> make_deferred_assoc_state(F&& f);
        template<class R, class F>
        friend future<R> make_async_assoc_state(F&& f);
        template<class R, class F>
        friend future<R> make_queued_assoc_state(F&& f);
        template<class R, class F>
        friend future<R> make_thread_pool_assoc_state(F&& f);
        template<typename InputIt>
        friend auto when_all(InputIt first, InputIt last) -> future<std::vector<typename std::iterator_traits<InputIt>::value_type>>;
        template<std::size_t I, typename Context, typename Future>
        friend void __attribute__((__visibility__("hidden"))) when_inner_helper(Context* context, Future&& f);
        template<typename InputIt>
        friend auto when_any(InputIt first, InputIt last) -> future<when_any_result<std::vector<typename std::iterator_traits<InputIt>::value_type>>>;
        template<size_t I, typename Context>
        friend void __attribute__((__visibility__("hidden"))) when_any_inner_helper(Context* context);
        template<class R>
        friend std::conditional_t<is_reference_wrapper<std::decay_t<R>>::value, future<std::decay_t<R>&>, future<std::decay_t<R>>> make_ready_future(R&& value);
        friend assoc_sub_state;
        template<class, class>
        friend class async_assoc_state;
        template<class, class>
        friend class deferred_assoc_state;
        
        void then_error(cxx_function::unique_function<void(const std::exception_ptr&)>&& continuation);
        
    public:
        inline future() noexcept = default;
        inline future(future&& rhs) noexcept : _state(rhs._state)
        {
            rhs._state = nullptr;
        }
        future(const future&) = delete;
        future& operator=(const future&) = delete;
        inline future& operator=(future&& rhs) noexcept
        {
            future(std::move(rhs)).swap(*this);
            return *this;
        }
        ~future();
        inline void swap(future& rhs) noexcept
        {
            std::swap(_state, rhs._state);
        }
        
        inline shared_future<T&> share() noexcept;
        T& get();
        
        inline bool valid() const noexcept
        {
            return _state != nullptr;
        }
        inline bool is_ready() const
        {
            if (_state)
            {
                return _state->is_ready();
            }
            return false;
        }
        
        template<class F>
        future_then_t<T&, F> then(F&& func);
        
        inline void wait() const
        {
            _state->wait();
        }
        
        template<class Rep, class Period>
        inline future_status wait_for(const std::chrono::duration<Rep, Period>& rel_time) const
        {
            return _state->wait_for(rel_time);
        }
        
        template<class Clock, class Duration>
        inline future_status wait_until(const std::chrono::time_point<Clock, Duration>& abs_time) const
        {
            return _state->wait_until(abs_time);
        }
    };
    
    template<class T>
    future<T&>::future(assoc_state<T&>* state) : _state(state)
    {
        if (_state->has_future_attached())
        {
            throw_future_error(future_errc::future_already_retrieved);
        }
        _state->add_shared();
        _state->set_future_attached();
    }
    
    template<class T>
    future<T&>::~future()
    {
        if (_state)
        {
            _state->release_shared();
        }
    }
    
    template<class T>
    T& future<T&>::get()
    {
        std::unique_ptr<shared_count, release_shared_count> __(_state);
        assoc_state<T&>* s = _state;
        _state = nullptr;
        return s->copy();
    }
    
    template<class T>
    template<class F>
    future_then_t<T&, F> future<T&>::then(F&& func)
    {
        return _state->template then<T&, F>(std::move(*this), std::forward<F>(func));
    }
    
    template<class T>
    void future<T&>::then_error(cxx_function::unique_function<void(const std::exception_ptr&)>&& continuation)
    {
        return _state->then_error(std::move(continuation));
    }
    
    // future<void>
    
    template<>
    class future<void>
    {
        assoc_sub_state* _state {nullptr};
        
        explicit future(assoc_sub_state* state);
        
        template<class>
        friend class promise;
        template<class>
        friend class shared_future;
        
        template<class R, class F>
        friend future<R> make_deferred_assoc_state(F&& f);
        template<class R, class F>
        friend future<R> make_async_assoc_state(F&& f);
        template<class R, class F>
        friend future<R> make_queued_assoc_state(F&& f);
        template<class R, class F>
        friend future<R> make_thread_pool_assoc_state(F&& f);
        template<typename InputIt>
        friend auto when_all(InputIt first, InputIt last) -> future<std::vector<typename std::iterator_traits<InputIt>::value_type>>;
        template<std::size_t I, typename Context, typename Future>
        friend void __attribute__((__visibility__("hidden"))) when_inner_helper(Context* context, Future&& f);
        template<typename InputIt>
        friend auto when_any(InputIt first, InputIt last) -> future<when_any_result<std::vector<typename std::iterator_traits<InputIt>::value_type>>>;
        template<size_t I, typename Context>
        friend void __attribute__((__visibility__("hidden"))) when_any_inner_helper(Context* context);
        friend future<void> make_ready_future();
        friend assoc_sub_state;
        template<class, class>
        friend class async_assoc_state;
        template<class, class>
        friend class deferred_assoc_state;
        
        void then_error(cxx_function::unique_function<void(const std::exception_ptr&)>&& continuation);
        
    public:
        inline future() noexcept = default;
        inline future(future&& rhs) noexcept : _state(rhs._state)
        {
            rhs._state = nullptr;
        }
        future(const future&) = delete;
        future& operator=(const future&) = delete;
        inline future& operator=(future&& rhs) noexcept
        {
            future(std::move(rhs)).swap(*this);
            return *this;
        }
        ~future();
        
        inline void swap(future& rhs) noexcept
        {
            std::swap(_state, rhs._state);
        }
        
        inline shared_future<void> share() noexcept;
        void get();
        
        inline bool valid() const noexcept
        {
            return _state != nullptr;
        }
        inline bool is_ready() const
        {
            if (_state != nullptr)
            {
                return _state->is_ready();
            }
            return false;
        }
        
        template<class F>
        future_then_t<void, F> then(F&& func);
        
        inline void wait() const
        {
            _state->wait();
        }
        
        template<class Rep, class Period>
        inline future_status wait_for(const std::chrono::duration<Rep, Period>& rel_time) const
        {
            return _state->wait_for(rel_time);
        }
        
        template<class Clock, class Duration>
        inline future_status wait_until(const std::chrono::time_point<Clock, Duration>& abs_time) const
        {
            return _state->wait_until(abs_time);
        }
    };
    
    template<class F>
    future_then_t<void, F> future<void>::then(F&& func)
    {
        return _state->template then<void, F>(std::move(*this), std::forward<F>(func));
    }
    
    template<class T>
    inline void swap(future<T>& x, future<T>& y) noexcept
    {
        x.swap(y);
    }
    
    // promise<T>
    
    template<class>
    class packaged_task;
    
    template<class T>
    class promise
    {
        assoc_state<T>* _state {nullptr};
        
        inline explicit promise(std::nullptr_t) noexcept
        {
        }
        
        template<class>
        friend class packaged_task;

    public:
        promise();
        template<class Alloc>
        promise(std::allocator_arg_t /*unused*/, const Alloc& a0);
        inline promise(promise&& rhs) noexcept : _state(rhs._state)
        {
            rhs._state = nullptr;
        }
        promise(const promise& rhs) = delete;
        ~promise();
        
        inline promise& operator=(promise&& rhs) noexcept
        {
            promise(std::move(rhs)).swap(*this);
            return *this;
        }
        promise& operator=(const promise& rhs) = delete;
        inline void swap(promise& rhs) noexcept
        {
            std::swap(_state, rhs._state);
        }
        
        future<T> get_future();
        
        void set_value(const T& r);
        void set_value(T&& r);
        void set_exception(const std::exception_ptr& p);
        
        void set_value_at_thread_exit(const T& r);
        void set_value_at_thread_exit(T&& r);
        void set_exception_at_thread_exit(const std::exception_ptr& p);
    };
    
    template<class T>
    promise<T>::promise() : _state(new assoc_state<T>)
    {
    }
    
    template<class T>
    template<class Alloc>
    promise<T>::promise(std::allocator_arg_t /*unused*/, const Alloc& a0)
    {
        using State = assoc_state_alloc<T, Alloc>;
        using A2 = typename allocator_traits_rebind<Alloc, State>::type;
        using D2 = allocator_destructor<A2>;
        A2 a(a0);
        std::unique_ptr<State, D2> hold(a.allocate(1), D2(a, 1));
        new (static_cast<void*>(std::addressof(*hold.get()))) State(a0);
        _state = std::addressof(*hold.release());
    }
    
    template<class T>
    promise<T>::~promise()
    {
        if (_state)
        {
            if (!_state->has_value() && _state->use_count() > 1)
            {
                _state->set_exception(std::make_exception_ptr(future_error(make_error_code(future_errc::broken_promise))));
            }
            _state->release_shared();
        }
    }
    
    template<class T>
    future<T> promise<T>::get_future()
    {
        if (_state == nullptr)
        {
            throw_future_error(future_errc::no_state);
        }
        return future<T>(_state);
    }
    
    template<class T>
    void promise<T>::set_value(const T& r)
    {
        if (_state == nullptr)
        {
            throw_future_error(future_errc::no_state);
        }
        _state->set_value(r);
    }
    
    template<class T>
    void promise<T>::set_value(T&& r)
    {
        if (_state == nullptr)
        {
            throw_future_error(future_errc::no_state);
        }
        _state->set_value(std::forward<T>(r));
    }
    
    template<class T>
    void promise<T>::set_exception(const std::exception_ptr& p)
    {
        ASSERT(p != nullptr, "promise::set_exception: received nullptr");
        if (_state == nullptr)
        {
            throw_future_error(future_errc::no_state);
        }
        _state->set_exception(p);
    }
    
    template<class T>
    void promise<T>::set_value_at_thread_exit(const T& r)
    {
        if (_state == nullptr)
        {
            throw_future_error(future_errc::no_state);
        }
        _state->set_value_at_thread_exit(r);
    }
    
    template<class T>
    void promise<T>::set_value_at_thread_exit(T&& r)
    {
        if (_state == nullptr)
        {
            throw_future_error(future_errc::no_state);
        }
        _state->set_value_at_thread_exit(std::forward<T>(r));
    }
    
    template<class T>
    void promise<T>::set_exception_at_thread_exit(const std::exception_ptr& p)
    {
        ASSERT(p != nullptr, "promise::set_exception_at_thread_exit: received nullptr");
        if (_state == nullptr)
        {
            throw_future_error(future_errc::no_state);
        }
        _state->set_exception_at_thread_exit(p);
    }
    
    // promise<T&>
    
    template<class T>
    class promise<T&>
    {
        assoc_state<T&>* _state {nullptr};
        
        inline explicit promise(std::nullptr_t) noexcept
        {
        }
        
        template<class>
        friend class packaged_task;
        
    public:
        promise();
        template<class Allocator>
        promise(std::allocator_arg_t /*unused*/, const Allocator& a0);
        inline promise(promise&& rhs) noexcept : _state(rhs._state)
        {
            rhs._state = nullptr;
        }
        promise(const promise& rhs) = delete;
        ~promise();
        
        inline promise& operator=(promise&& rhs) noexcept
        {
            promise(std::move(rhs)).swap(*this);
            return *this;
        }
        promise& operator=(const promise& rhs) = delete;
        inline void swap(promise& rhs) noexcept
        {
            std::swap(_state, rhs._state);
        }
        
        future<T&> get_future();
        
        void set_value(T& r);
        void set_exception(const std::exception_ptr& p);
        
        void set_value_at_thread_exit(T& r);
        void set_exception_at_thread_exit(const std::exception_ptr& p);
    };
    
    template<class T>
    promise<T&>::promise() : _state(new assoc_state<T&>)
    {
    }
    
    template<class T>
    template<class Alloc>
    promise<T&>::promise(std::allocator_arg_t /*unused*/, const Alloc& a0)
    {
        using State = assoc_state_alloc<T&, Alloc>;
        using A2 = typename allocator_traits_rebind<Alloc, State>::type;
        using D2 = allocator_destructor<A2>;
        A2 a(a0);
        std::unique_ptr<State, D2> hold(a.allocate(1), D2(a, 1));
        new (static_cast<void*>(std::addressof(*hold.get()))) State(a0);
        _state = std::addressof(*hold.release());
    }
    
    template<class T>
    promise<T&>::~promise()
    {
        if (_state)
        {
            if (!_state->has_value() && _state->use_count() > 1)
            {
                _state->set_exception(std::make_exception_ptr(future_error(make_error_code(future_errc::broken_promise))));
            }
            _state->release_shared();
        }
    }
    
    template<class T>
    future<T&> promise<T&>::get_future()
    {
        if (_state == nullptr)
        {
            throw_future_error(future_errc::no_state);
        }
        return future<T&>(_state);
    }
    
    template<class T>
    void promise<T&>::set_value(T& r)
    {
        if (_state == nullptr)
        {
            throw_future_error(future_errc::no_state);
        }
        _state->set_value(r);
    }
    
    template<class T>
    void promise<T&>::set_exception(const std::exception_ptr& p)
    {
        ASSERT(p != nullptr, "promise::set_exception: received nullptr");
        if (_state == nullptr)
        {
            throw_future_error(future_errc::no_state);
        }
        _state->set_exception(p);
    }
    
    template<class T>
    void promise<T&>::set_value_at_thread_exit(T& r)
    {
        if (_state == nullptr)
        {
            throw_future_error(future_errc::no_state);
        }
        _state->set_value_at_thread_exit(r);
    }
    
    template<class T>
    void promise<T&>::set_exception_at_thread_exit(const std::exception_ptr& p)
    {
        ASSERT(p != nullptr, "promise::set_exception_at_thread_exit: received nullptr");
        if (_state == nullptr)
        {
            throw_future_error(future_errc::no_state);
        }
        _state->set_exception_at_thread_exit(p);
    }
    
    // promise<void>
    
    template<>
    class promise<void>
    {
        assoc_sub_state* _state {nullptr};
        
        inline explicit promise(std::nullptr_t) noexcept
        {
        }
        
        template<class>
        friend class packaged_task;
        
    public:
        promise();
        template<class Allocator>
        promise(std::allocator_arg_t /*unused*/, const Allocator& a0);
        inline promise(promise&& rhs) noexcept : _state(rhs._state)
        {
            rhs._state = nullptr;
        }
        promise(const promise& rhs) = delete;
        ~promise();
        
        inline promise& operator=(promise&& rhs) noexcept
        {
            promise(std::move(rhs)).swap(*this);
            return *this;
        }
        promise& operator=(const promise& rhs) = delete;
        inline void swap(promise& rhs) noexcept
        {
            std::swap(_state, rhs._state);
        }
        
        future<void> get_future();
        
        void set_value();
        void set_exception(const std::exception_ptr& p);
        
        void set_value_at_thread_exit();
        void set_exception_at_thread_exit(const std::exception_ptr& p);
    };
    
    template<class Alloc>
    promise<void>::promise(std::allocator_arg_t /*unused*/, const Alloc& a0)
    {
        using State = assoc_sub_state_alloc<Alloc>;
        using A2 = typename allocator_traits_rebind<Alloc, State>::type;
        using D2 = allocator_destructor<A2>;
        A2 a(a0);
        std::unique_ptr<State, D2> hold(a.allocate(1), D2(a, 1));
        new (static_cast<void*>(std::addressof(*hold.get()))) State(a0);
        _state = std::addressof(*hold.release());
    }
    
    template<class T>
    inline void swap(promise<T>& x, promise<T>& y) noexcept
    {
        x.swap(y);
    }
    
    // make_ready_future
    
    template<class T>
    std::conditional_t<is_reference_wrapper<std::decay_t<T>>::value, future<std::decay_t<T>&>, future<std::decay_t<T>>> make_ready_future(T&& value)
    {
        using X = std::decay_t<T>;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++1z-extensions"
        if constexpr(is_reference_wrapper<X>::value)
        {
            auto state = new assoc_state<X&>();
            state->set_value(std::forward<T>(value));
            return future<X&>(state);
        }
        else
        {
            auto state = new assoc_state<X>();
            state->set_value(std::forward<T>(value));
            return future<X>(state);
        }
#pragma clang diagnostic pop
    }
    
    future<void> make_ready_future();
    
    // make_exceptional_future
    
    template<class T>
    future<T> make_exceptional_future(std::exception_ptr ex)
    {
        promise<T> p;
        p.set_exception(ex);
        return p.get_future();
    }
    
    template<class T, class E>
    future<T> make_exceptional_future(E ex)
    {
        promise<T> p;
        p.set_exception(std::make_exception_ptr(ex));
        return p.get_future();
    }
    
    // packaged_task_function
    
    template<class F> class packaged_task_base;
    
    template<class T, class ...ArgTypes>
    class packaged_task_base<T(ArgTypes...)>
    {
    public:
        inline packaged_task_base() = default;
        packaged_task_base(const packaged_task_base&) = delete;
        packaged_task_base& operator=(const packaged_task_base&) = delete;
        packaged_task_base(packaged_task_base&&) noexcept = delete;
        packaged_task_base& operator=(packaged_task_base&&) noexcept = delete;
        inline virtual ~packaged_task_base() = default;
        virtual void move_to(packaged_task_base*) noexcept = 0;
        virtual void destroy() = 0;
        virtual void destroy_deallocate() = 0;
        virtual T operator()(ArgTypes&& ...) = 0;
    };
    
    template<class, class, class>
    class packaged_task_func;
    
    template<class F, class Alloc, class R, class ...ArgTypes>
    class packaged_task_func<F, Alloc, R(ArgTypes...)> : public packaged_task_base<R(ArgTypes...)>
    {
        compressed_pair<F, Alloc> _f;
    public:
        inline explicit packaged_task_func(const F& f) : _f(f)
        {
        }
        inline explicit packaged_task_func(F&& f) : _f(std::move(f))
        {
        }
        inline packaged_task_func(const F& f, const Alloc& a) : _f(f, a)
        {
        }
        inline packaged_task_func(F&& f, const Alloc& a) : _f(std::move(f), a)
        {
        }
        virtual void move_to(packaged_task_base<R(ArgTypes...)>* p) noexcept;
        virtual void destroy();
        virtual void destroy_deallocate();
        virtual R operator()(ArgTypes&& ... args);
    };
    
    template<class F, class Alloc, class R, class ...ArgTypes>
    void packaged_task_func<F, Alloc, R(ArgTypes...)>::move_to(packaged_task_base<R(ArgTypes...)>* p) noexcept
    {
        new (p) packaged_task_func(std::move(_f.first()), std::move(_f.second()));
    }
    
    template<class F, class Alloc, class R, class ...ArgTypes>
    void packaged_task_func<F, Alloc, R(ArgTypes...)>::destroy()
    {
        _f.~compressed_pair<F, Alloc>();
    }
    
    template<class F, class Alloc, class R, class ...ArgTypes>
    void packaged_task_func<F, Alloc, R(ArgTypes...)>::destroy_deallocate()
    {
        using A = typename allocator_traits_rebind<Alloc, packaged_task_func>::type;
        using ATraits = std::allocator_traits<A>;
        using PTraits = std::pointer_traits<typename ATraits::pointer>;
        A a(_f.second());
        _f.~compressed_pair<F, Alloc>();
        a.deallocate(PTraits::pointer_to(*this), 1);
    }
    
    template<class F, class Alloc, class R, class ...ArgTypes>
    R packaged_task_func<F, Alloc, R(ArgTypes...)>::operator()(ArgTypes&& ... args)
    {
        return ps::invoke(_f.first(), std::forward<ArgTypes>(args)...);
    }
    
    template<class Callable>
    class packaged_task_function;
    
    template<class R, class ...ArgTypes>
    class packaged_task_function<R(ArgTypes...)>
    {
        using base = packaged_task_base<R(ArgTypes...)>;
        typename std::aligned_storage<3*sizeof(void*)>::type _buf;
        base* _f;
        
    public:
        using result_type = R;
        
        inline packaged_task_function() noexcept : _f(nullptr)
        {
        }
        template<class F>
        packaged_task_function(F&& f);
        template<class F, class Alloc>
        packaged_task_function(std::allocator_arg_t /*unused*/, const Alloc& a0, F&& f);
        
        packaged_task_function(packaged_task_function&& f) noexcept;
        packaged_task_function& operator=(packaged_task_function&& f) noexcept;
        
        packaged_task_function(const packaged_task_function&) =  delete;
        packaged_task_function& operator=(const packaged_task_function&) =  delete;
        
        ~packaged_task_function();
        
        void swap(packaged_task_function& f) noexcept;
        
        inline R operator()(ArgTypes... args) const;
    };
    
    template<class R, class ...ArgTypes>
    packaged_task_function<R(ArgTypes...)>::packaged_task_function(packaged_task_function&& f) noexcept
    {
        if (f._f == nullptr)
        {
            _f = nullptr;
        }
        else if (f._f == static_cast<base*>(&f._buf))
        {
            _f = static_cast<base*>(&_buf);
            f._f->move_to(_f);
        }
        else
        {
            _f = f._f;
            f._f = nullptr;
        }
    }
    
    template<class R, class ...ArgTypes>
    template<class F>
    packaged_task_function<R(ArgTypes...)>::packaged_task_function(F&& f) : _f(nullptr)
    {
        using FR = typename std::remove_reference<typename std::decay<F>::type>::type;
        using FF = packaged_task_func<FR, std::allocator<FR>, R(ArgTypes...)>;
        if (sizeof(FF) <= sizeof(_buf))
        {
            _f = reinterpret_cast<base*>(&_buf);
            new (_f) FF(std::forward<F>(f));
        }
        else
        {
            using A = std::allocator<FF>;
            A a;
            using D = allocator_destructor<A>;
            std::unique_ptr<base, D> hold(a.allocate(1), D(a, 1));
            new (hold.get()) FF(std::forward<F>(f), std::allocator<FR>(a));
            _f = hold.release();
        }
    }
    
    template<class R, class ...ArgTypes>
    template<class F, class Alloc>
    packaged_task_function<R(ArgTypes...)>::packaged_task_function(std:: allocator_arg_t /*unused*/, const Alloc& a0, F&& f) : _f(nullptr)
    {
        using FR = typename std::remove_reference<typename std::decay<F>::type>::type;
        using FF = packaged_task_func<FR, Alloc, R(ArgTypes...)>;
        if (sizeof(FF) <= sizeof(_buf))
        {
            _f = static_cast<base*>(&_buf);
            new (_f) FF(std::forward<F>(f));
        }
        else
        {
            using A = typename allocator_traits_rebind<Alloc, FF>::type;
            A a(a0);
            using D = allocator_destructor<A>;
            std::unique_ptr<base, D> hold(a.allocate(1), D(a, 1));
            new (static_cast<void*>(std::addressof(*hold.get()))) FF(std::forward<F>(f), Alloc(a));
            _f = std::addressof(*hold.release());
        }
    }
    
    template<class R, class ...ArgTypes>
    packaged_task_function<R(ArgTypes...)>& packaged_task_function<R(ArgTypes...)>::operator=(packaged_task_function&& f) noexcept
    {
        if (_f == reinterpret_cast<base*>(&_buf))
        {
            _f->destroy();
        }
        else if (_f)
        {
            _f->destroy_deallocate();
        }
        
        _f = nullptr;
        if (f._f == nullptr)
        {
            _f = nullptr;
        }
        else if (f._f == reinterpret_cast<base*>(&f._buf))
        {
            _f = reinterpret_cast<base*>(&_buf);
            f._f->move_to(_f);
        }
        else
        {
            _f = f._f;
            f._f = nullptr;
        }
        return *this;
    }
    
    template<class R, class ...ArgTypes>
    packaged_task_function<R(ArgTypes...)>::~packaged_task_function()
    {
        if (_f == reinterpret_cast<base*>(&_buf))
        {
            _f->destroy();
        }
        else if (_f)
        {
            _f->destroy_deallocate();
        }
    }
    
    template<class R, class ...ArgTypes>
    void packaged_task_function<R(ArgTypes...)>::swap(packaged_task_function& f) noexcept
    {
        if (_f == static_cast<base*>(&_buf) && f._f == static_cast<base*>(&f._buf))
        {
            typename std::aligned_storage<sizeof(_buf)>::type tempbuf;
            auto t = static_cast<base*>(&tempbuf);
            _f->move_to(t);
            _f->destroy();
            _f = nullptr;
            f._f->move_to(static_cast<base*>(&_buf));
            f._f->destroy();
            f._f = nullptr;
            _f = static_cast<base*>(&_buf);
            t->move_to(static_cast<base*>(&f._buf));
            t->destroy();
            f._f = static_cast<base*>(&f._buf);
        }
        else if (_f == static_cast<base*>(&_buf))
        {
            _f->move_to(static_cast<base*>(&f._buf));
            _f->destroy();
            _f = f._f;
            f._f = static_cast<base*>(&f._buf);
        }
        else if (f._f == static_cast<base*>(&f._buf))
        {
            f._f->move_to(static_cast<base*>(&_buf));
            f._f->destroy();
            f._f = _f;
            _f = static_cast<base*>(&_buf);
        }
        else
        {
            std::swap(_f, f._f);
        }
    }
    
    template<class R, class ...ArgTypes>
    inline R packaged_task_function<R(ArgTypes...)>::operator()(ArgTypes... args) const
    {
        return (*_f)(std::forward<ArgTypes>(args)...);
    }
    
    // packaged_task
    
    template<class R, class ...ArgTypes>
    class packaged_task<R(ArgTypes...)>
    {
    public:
        using result_type = R;
        
    private:
        packaged_task_function<result_type(ArgTypes...)> _f;
        promise<result_type> _p;
        
    public:
        inline packaged_task() noexcept : _p(nullptr)
        {
        }
        template<class F, class = typename std::enable_if<!std::is_same<typename std::decay<F>::type, packaged_task>::value>::type>
        inline explicit packaged_task(F&& f) : _f(std::forward<F>(f))
        {
        }
        template<class F, class Allocator, class = typename std::enable_if<!std::is_same<typename std::decay<F>::type, packaged_task>::value>::type>
        inline packaged_task(std::allocator_arg_t /*unused*/, const Allocator& a, F&& f) : _f(std::allocator_arg, a, std::forward<F>(f)), _p(std::allocator_arg, a)
        {
        }
        ~packaged_task() = default;
        
        packaged_task(const packaged_task&) = delete;
        packaged_task& operator=(const packaged_task&) = delete;
        
        inline packaged_task(packaged_task&& other) noexcept : _f(std::move(other._f)), _p(std::move(other._p))
        {
        }
        inline packaged_task& operator=(packaged_task&& other) noexcept
        {
            _f = std::move(other._f);
            _p = std::move(other._p);
            return *this;
        }
        inline void swap(packaged_task& other) noexcept
        {
            _f.swap(other._f);
            _p.swap(other._p);
        }
        
        inline bool valid() const noexcept
        {
            return _p._state != nullptr;
        }
        
        inline future<result_type> get_future()
        {
            return _p.get_future();
        }
        
        void operator()(ArgTypes... args);
        void make_ready_at_thread_exit(ArgTypes... args);
        
        void reset();
    };
    
    template<class R, class ...ArgTypes>
    void packaged_task<R(ArgTypes...)>::operator()(ArgTypes... args)
    {
        if (_p._state == nullptr)
        {
            throw_future_error(future_errc::no_state);
        }
        if (_p._state->has_value())
        {
            throw_future_error(future_errc::promise_already_satisfied);
        }
        try
        {
            _p.set_value(_f(std::forward<ArgTypes>(args)...));
        }
        catch (...)
        {
            _p.set_exception(std::current_exception());
        }
    }
    
    template<class R, class ...ArgTypes>
    void packaged_task<R(ArgTypes...)>::make_ready_at_thread_exit(ArgTypes... args)
    {
        if (_p._state == nullptr)
        {
            throw_future_error(future_errc::no_state);
        }
        if (_p._state->has_value())
        {
            throw_future_error(future_errc::promise_already_satisfied);
        }
        try
        {
            _p.set_value_at_thread_exit(_f(std::forward<ArgTypes>(args)...));
        }
        catch (...)
        {
            _p.set_exception_at_thread_exit(std::current_exception());
        }
    }
    
    template<class R, class ...ArgTypes>
    void packaged_task<R(ArgTypes...)>::reset()
    {
        if (!valid())
        {
            throw_future_error(future_errc::no_state);
        }
        _p = promise<result_type>();
    }
    
    template<class ...ArgTypes>
    class packaged_task<void(ArgTypes...)>
    {
    public:
        using result_type = void;
        
    private:
        packaged_task_function<result_type(ArgTypes...)> _f;
        promise<result_type> _p;
        
    public:
        inline packaged_task() noexcept : _p(nullptr)
        {
        }
        template<class F, class = typename std::enable_if<!std::is_same<typename std::decay<F>::type, packaged_task>::value>::type>
        inline explicit packaged_task(F&& f) : _f(std::forward<F>(f))
        {
        }
        template<class F, class Allocator, class = typename std::enable_if<!std::is_same<typename std::decay<F>::type, packaged_task>::value>::type>
        inline packaged_task(std::allocator_arg_t /*unused*/, const Allocator& a, F&& f) : _f(std::allocator_arg, a, std::forward<F>(f)), _p(std::allocator_arg, a)
        {
        }
        ~packaged_task() = default;
        
        packaged_task(const packaged_task&) = delete;
        packaged_task& operator=(const packaged_task&) = delete;
        
        inline packaged_task(packaged_task&& other) noexcept : _f(std::move(other._f)), _p(std::move(other._p))
        {
        }
        inline packaged_task& operator=(packaged_task&& other) noexcept
        {
            _f = std::move(other._f);
            _p = std::move(other._p);
            return *this;
        }
        inline void swap(packaged_task& other) noexcept
        {
            _f.swap(other._f);
            _p.swap(other._p);
        }
        
        inline bool valid() const noexcept
        {
            return _p._state != nullptr;
        }
        
        inline future<result_type> get_future()
        {
            return _p.get_future();
        }
        
        void operator()(ArgTypes... args);
        void make_ready_at_thread_exit(ArgTypes... args);
        
        void reset();
    };
    
    template<class ...ArgTypes>
    void packaged_task<void(ArgTypes...)>::operator()(ArgTypes... args)
    {
        if (_p._state == nullptr)
        {
            throw_future_error(future_errc::no_state);
        }
        if (_p._state->has_value())
        {
            throw_future_error(future_errc::promise_already_satisfied);
        }
        try
        {
            _f(std::forward<ArgTypes>(args)...);
            _p.set_value();
        }
        catch (...)
        {
            _p.set_exception(std::current_exception());
        }
    }
    
    template<class ...ArgTypes>
    void packaged_task<void(ArgTypes...)>::make_ready_at_thread_exit(ArgTypes... args)
    {
        if (_p._state == nullptr)
        {
            throw_future_error(future_errc::no_state);
        }
        if (_p._state->has_value())
        {
            throw_future_error(future_errc::promise_already_satisfied);
        }
        try
        {
            _f(std::forward<ArgTypes>(args)...);
            _p.set_value_at_thread_exit();
        }
        catch (...)
        {
            _p.set_exception_at_thread_exit(std::current_exception());
        }
    }
    
    template<class ...ArgTypes>
    void packaged_task<void(ArgTypes...)>::reset()
    {
        if (!valid())
        {
            throw_future_error(future_errc::no_state);
        }
        _p = promise<result_type>();
    }
    
    template<class Callable>
    inline void swap(packaged_task<Callable>& x, packaged_task<Callable>& y) noexcept
    {
        x.swap(y);
    }
    
    // async
    
    template<class T, class F>
    future<T> make_deferred_assoc_state(F&& f)
    {
        std::unique_ptr<deferred_assoc_state<T, F>, release_shared_count> h(new deferred_assoc_state<T, F>(std::forward<F>(f)));
        h->execute();
        return future<T>(h.get());
    }
    
    template<class T, class F>
    future<T> make_async_assoc_state(F&& f)
    {
        std::unique_ptr<async_assoc_state<T, F>, release_shared_count> h(new async_assoc_state<T, F>(std::forward<F>(f)));
        ps::thread(&async_assoc_state<T, F>::execute, h.get()).detach();
        return future<T>(h.get());
    }
    
    async_queued& get_async_queued();
    
    template<class T, class F>
    future<T> make_queued_assoc_state(F&& f)
    {
        auto& queue = get_async_queued();
        std::unique_ptr<async_assoc_state<T, F>, release_shared_count> h(new async_assoc_state<T, F>(std::forward<F>(f)));
        h->set_queued();
        queue.post(h.get());
        return future<T>(h.get());
    }
    
    async_thread_pool& get_async_thread_pool();
    
    template<class T, class F>
    future<T> make_thread_pool_assoc_state(F&& f)
    {
        auto& queue = get_async_thread_pool();
        std::unique_ptr<async_assoc_state<T, F>, release_shared_count> h(new async_assoc_state<T, F>(std::forward<F>(f)));
        h->set_thread_pool();
        queue.post(h.get());
        return future<T>(h.get());
    }
    
    template<class F, class... Args>
    using future_async_ret_t = invoke_of_t<std::decay_t<F>, std::decay_t<Args>...>;
    
    template<class F, class... Args>
    using future_async_t = std::conditional_t<is_future<future_async_ret_t<F, Args...>>::value, future_async_ret_t<F, Args...>, future<future_async_ret_t<F, Args...>>>;
    
    template<class F, class... Args>
    class async_func
    {
        std::tuple<F, Args...> _f;
        
    public:
        inline explicit async_func(F&& f, Args&&... args) : _f(std::move(f), std::move(args)...)
        {
        }
        async_func(const async_func& f) = delete;
        async_func& operator=(const async_func& f) = delete;
        inline async_func(async_func&& f) noexcept : _f(std::move(f._f))
        {
        }
        inline async_func& operator=(async_func&& f) noexcept
        {
            _f = std::move(f._f);
            return *this;
        }
        ~async_func() = default;
        
        
        future_async_ret_t<F, Args...> operator()()
        {
            using Index = typename make_tuple_indices<1+sizeof...(Args), 1>::type;
            return execute(Index());
        }
    private:
        template<std::size_t ...Indices>
        future_async_ret_t<F, Args...> execute(tuple_indices<Indices...> /*unused*/)
        {
            return ps::invoke(std::move(std::get<0>(_f)), std::move(std::get<Indices>(_f))...);
        }
    };
    
    inline bool does_policy_contain(launch policy, launch value)
    {
        return (static_cast<launch_underlying_type>(policy) & static_cast<launch_underlying_type>(value)) != 0;
    }
    
    template<class F, class... Args>
    future_async_t<F, Args...> async(ps::launch policy, F&& f, Args&&... args)
    {
        using R = typename future_held<future_async_ret_t<F, Args...>>::type;
        using BF = async_func<std::decay_t<F>, std::decay_t<Args>...>;
        
        if (does_policy_contain(policy, launch::queued))
        {
            return make_queued_assoc_state<R>(BF(decay_copy(f), decay_copy(args)...));
        }
        else if (does_policy_contain(policy, launch::thread_pool))
        {
            return make_thread_pool_assoc_state<R>(BF(decay_copy(f), decay_copy(args)...));
        }
        
        try
        {
            if (does_policy_contain(policy, launch::async))
            {
                return make_async_assoc_state<R>(BF(decay_copy(f), decay_copy(args)...));
            }
        }
        catch (...)
        {
            if (policy == launch::async)
            {
                throw;
            }
        }
        
        if (does_policy_contain(policy, launch::deferred))
        {
            return make_deferred_assoc_state<R>(BF(decay_copy(f), decay_copy(args)...));
        }
        return future<R>{};
    }
    
    template<class F, class... Args>
    inline future_async_t<F, Args...> async(F&& f, Args&&... args)
    {
        return async(ps::launch::any, std::forward<F>(f), std::forward<Args>(args)...);
    }
    
    // when_all
    
    template<typename InputIt>
    auto when_all(InputIt first, InputIt last) -> future<std::vector<typename std::iterator_traits<InputIt>::value_type>>
    {
        using result_inner_type = std::vector<typename std::iterator_traits<InputIt>::value_type>;
        
        struct context_all
        {
            std::size_t total_futures = 0;
            std::size_t ready_futures = 0;
            result_inner_type result;
            std::mutex mutex;
            promise<result_inner_type> p;
            std::exception_ptr e = nullptr;
        };
        
        auto shared_context = new context_all;
        auto result_future = shared_context->p.get_future();
        shared_context->total_futures = std::distance(first, last);
        shared_context->result.reserve(shared_context->total_futures);
        size_t index = 0;
        
        for (; first != last; ++first, ++index)
        {
            shared_context->result.push_back(std::move(*first));
            shared_context->result[index].then_error([shared_context, index](const std::exception_ptr exception) {
                std::lock_guard<std::mutex> lock(shared_context->mutex);
                ++shared_context->ready_futures;
                if (exception != nullptr)
                {
                    shared_context->e = exception;
                }
                
                if (shared_context->ready_futures == shared_context->total_futures)
                {
                    if (shared_context->e != nullptr)
                    {
                        shared_context->p.set_exception(shared_context->e);
                    }
                    else
                    {
                        shared_context->p.set_value(std::move(shared_context->result));
                    }
                    delete shared_context;
                }
            });
        }
        
        return result_future;
    }
    
    template<std::size_t I, typename Context, typename Future>
    void __attribute__((__visibility__("hidden"))) when_inner_helper(Context* context, Future&& f)
    {
        std::get<I>(context->result) = std::forward<Future>(f);
        std::get<I>(context->result).then_error([context](const std::exception_ptr exception) {
            std::lock_guard<std::mutex> lock(context->mutex);
            ++context->ready_futures;
            if (exception != nullptr)
            {
                context->e = exception;
            }
            if (context->ready_futures == context->total_futures)
            {
                if (context->e != nullptr)
                {
                    context->p.set_exception(context->e);
                }
                else
                {
                    context->p.set_value(std::move(context->result));
                }
                delete context;
            }
        });
    }
    
    template<std::size_t I, typename Context>
    void __attribute__((__visibility__("hidden"))) apply_helper(Context* /*unused*/)
    {
    }
    
    template<std::size_t I, typename Context, typename FirstFuture, typename... Futures>
    void __attribute__((__visibility__("hidden"))) apply_helper(Context* context, FirstFuture&& f, Futures&&... fs)
    {
        when_inner_helper<I>(context, std::forward<FirstFuture>(f));
        apply_helper<I+1>(context, std::forward<Futures>(fs)...);
    }
    
    template<typename... Futures>
    auto when_all(Futures&&... futures) -> future<std::tuple<std::decay_t<Futures>...>>
    {
        using result_inner_type = std::tuple<std::decay_t<Futures>...>;
        struct context_all
        {
            std::size_t total_futures = 0;
            std::size_t ready_futures = 0;
            result_inner_type result;
            promise<result_inner_type> p;
            std::mutex mutex;
            std::exception_ptr e = nullptr;
        };
        auto shared_context = new context_all;
        auto ret = shared_context->p.get_future();
        shared_context->total_futures = sizeof...(futures);
        apply_helper<0>(shared_context, std::forward<Futures>(futures)...);
        return ret;
    }
    
    // when_any
    
    template<typename Sequence>
    struct when_any_result
    {
        size_t index;
        Sequence sequence;
    };
    
    template<typename InputIt>
    auto when_any(InputIt first, InputIt last) -> future<when_any_result<std::vector<typename std::iterator_traits<InputIt>::value_type>>>
    {
        using result_inner_type = std::vector<typename std::iterator_traits<InputIt>::value_type>;
        using future_inner_type = when_any_result<result_inner_type>;
        
        struct context_any
        {
            std::size_t total_futures = 0;
            std::size_t ready_futures = 0;
            std::size_t processed = 0;
            std::size_t failled = 0;
            future_inner_type result;
            std::vector<assoc_sub_state*> result_sub_state;
            promise<future_inner_type> p;
            bool ready = false;
            bool result_moved = false;
            std::mutex mutex;
            std::exception_ptr e = nullptr;
        };
        
        auto shared_context = new context_any;
        auto result_future = shared_context->p.get_future();
        shared_context->total_futures = std::distance(first, last);
        shared_context->result.sequence.reserve(shared_context->total_futures);
        shared_context->result_sub_state.reserve(shared_context->total_futures);
        
        for (size_t index = 0; first != last; ++first, ++index)
        {
            first->_state->add_shared();
            shared_context->result.sequence.push_back(std::move(*first));
            shared_context->result.sequence[index].then_error([shared_context, index](const std::exception_ptr exception) {
                std::lock_guard<std::mutex> lock(shared_context->mutex);
                ++shared_context->ready_futures;
                if (exception != nullptr)
                {
                    ++shared_context->failled;
                    shared_context->e = exception;
                }
                if (!shared_context->ready)
                {
                    if (exception == nullptr)
                    {
                        shared_context->result.index = index;
                        shared_context->ready = true;
                    }
                    else
                    {
                        shared_context->result.index = index;
                    }
                    
                    if (shared_context->processed == shared_context->total_futures && !shared_context->result_moved && (exception == nullptr || shared_context->failled == shared_context->total_futures))
                    {
                        shared_context->result_moved = true;
                        if (shared_context->failled == shared_context->total_futures)
                        {
                            shared_context->p.set_exception(shared_context->e);
                        }
                        else
                        {
                            shared_context->p.set_value(std::move(shared_context->result));
                        }
                    }
                }
                
                if (shared_context->processed == shared_context->total_futures && shared_context->ready_futures == shared_context->total_futures)
                {
                    for(auto& it : shared_context->result_sub_state)
                    {
                        it->release_shared();
                    }
                    delete shared_context;
                }
            });
            ++shared_context->processed;
        }
        
        {
            std::lock_guard<std::mutex> lock(shared_context->mutex);
            if ((shared_context->ready || shared_context->failled == shared_context->total_futures) && !shared_context->result_moved)
            {
                shared_context->result_moved = true;
                if (shared_context->failled == shared_context->total_futures)
                {
                    shared_context->p.set_exception(shared_context->e);
                }
                else
                {
                    shared_context->p.set_value(std::move(shared_context->result));
                }
                if (shared_context->ready_futures == shared_context->total_futures)
                {
                    for(auto& it : shared_context->result_sub_state)
                    {
                        it->release_shared();
                    }
                    delete shared_context;
                }
            }
        }
        
        return result_future;
    }
    
    template<size_t I, typename Context>
    void __attribute__((__visibility__("hidden"))) when_any_inner_helper(Context* context)
    {
        std::get<I>(context->result.sequence)._state->add_shared();
        context->result_sub_state.emplace_back(static_cast<assoc_sub_state*>(std::get<I>(context->result.sequence)._state));
        std::get<I>(context->result.sequence).then_error([context](const std::exception_ptr exception) {
            std::lock_guard<std::mutex> lock(context->mutex);
            ++context->ready_futures;
            if (exception != nullptr)
            {
                ++context->failled;
                context->e = exception;
            }
            
            if (!context->ready)
            {
                if (exception == nullptr)
                {
                    context->ready = true;
                    context->result.index = I;
                }
                else
                {
                    context->result.index = I;
                }
                
                if (context->processed == context->total_futures && !context->result_moved && (exception == nullptr || context->failled == context->total_futures))
                {
                    context->result_moved = true;
                    if (context->failled == context->total_futures)
                    {
                        context->p.set_exception(context->e);
                    }
                    else
                    {
                        context->p.set_value(std::move(context->result));
                    }
                }
            }
            if (context->processed == context->total_futures && context->ready_futures == context->total_futures)
            {
                for(auto& it : context->result_sub_state)
                {
                    it->release_shared();
                }
                delete context;
            }
        });
    }
    
    template<size_t I, size_t S>
    struct __attribute__((__visibility__("hidden"))) when_any_helper_struct
    {
        template<typename Context, typename... Futures>
        static void apply(Context* context, std::tuple<Futures...>& t)
        {
            when_any_inner_helper<I>(context);
            ++context->processed;
            when_any_helper_struct<I+1, S>::apply(context, t);
        }
    };
    
    template<size_t S>
    struct __attribute__((__visibility__("hidden"))) when_any_helper_struct<S, S>
    {
        template<typename Context, typename... Futures>
        static void apply(Context* /*unused*/, std::tuple<Futures...>& /*unused*/)
        {
        }
    };
    
    template<size_t I, typename Context>
    void __attribute__((__visibility__("hidden"))) fill_result_helper(Context* /*unused*/)
    {
    }
    
    template<size_t I, typename Context, typename FirstFuture, typename... Futures>
    void __attribute__((__visibility__("hidden"))) fill_result_helper(Context* context, FirstFuture&& f, Futures&&... fs)
    {
        std::get<I>(context->result.sequence) = std::forward<FirstFuture>(f);
        fill_result_helper<I+1>(context, std::forward<Futures>(fs)...);
    }
    
    template<typename... Futures>
    auto when_any(Futures&&... futures) -> future<when_any_result<std::tuple<std::decay_t<Futures>...>>>
    {
        using result_inner_type = std::tuple<std::decay_t<Futures>...>;
        using future_inner_type = when_any_result<result_inner_type>;
        
        struct context_any
        {
            std::size_t total_futures = 0;
            std::size_t ready_futures = 0;
            std::atomic<std::size_t> processed {0};
            std::size_t failled = 0;
            future_inner_type result;
            std::vector<assoc_sub_state*> result_sub_state;
            promise<future_inner_type> p;
            bool ready = false;
            bool result_moved = false;
            std::mutex mutex;
            std::exception_ptr e = nullptr;
        };
        
        auto shared_context = new context_any;
        auto ret = shared_context->p.get_future();
        shared_context->total_futures = sizeof...(futures);
        shared_context->result_sub_state.reserve(shared_context->total_futures);
        
        fill_result_helper<0>(shared_context, std::forward<Futures>(futures)...);
        when_any_helper_struct<0, sizeof...(futures)>::apply(shared_context, shared_context->result.sequence);
        {
            std::lock_guard<std::mutex> lock(shared_context->mutex);
            if ((shared_context->ready || shared_context->failled == shared_context->total_futures) && !shared_context->result_moved)
            {
                shared_context->result_moved = true;
                if (shared_context->failled == shared_context->total_futures)
                {
                    shared_context->p.set_exception(shared_context->e);
                }
                else
                {
                    shared_context->p.set_value(std::move(shared_context->result));
                }
            }
            if (shared_context->ready_futures == shared_context->total_futures)
            {
                for(auto& it : shared_context->result_sub_state)
                {
                    it->release_shared();
                }
                delete shared_context;
            }
        }
        return ret;
    }
    
    // shared_future
    
    template<class T>
    class shared_future
    {
        assoc_state<T>* _state {nullptr};
        
        template<typename InputIt>
        friend auto when_all(InputIt first, InputIt last) -> future<std::vector<typename std::iterator_traits<InputIt>::value_type>>;
        template<std::size_t I, typename Context, typename Future>
        friend void __attribute__((__visibility__("hidden"))) when_inner_helper(Context* context, Future&& f);
        
        void then_error(cxx_function::unique_function<void(const std::exception_ptr&)>&& continuation);
        
    public:
        inline shared_future() noexcept : _state(nullptr)
        {
        }
        inline shared_future(const shared_future& rhs) noexcept : _state(rhs._state)
        {
            if (_state != nullptr)
            {
                _state->add_shared();
            }
        }
        inline shared_future(future<T>&& f) noexcept : _state(f._state)
        {
            f._state = nullptr;
        }
        inline shared_future(shared_future&& rhs) noexcept : _state(rhs._state)
        {
            rhs._state = nullptr;
        }
        ~shared_future();
        shared_future& operator=(const shared_future& rhs) noexcept;
        inline shared_future& operator=(shared_future&& rhs) noexcept
        {
            shared_future(std::move(rhs)).swap(*this);
            return *this;
        }
        
        inline const T& get() const
        {
            return _state->copy();
        }
        
        inline void swap(shared_future& rhs) noexcept
        {
            std::swap(_state, rhs._state);
        }
        
        inline bool valid() const noexcept
        {
            return _state != nullptr;
        }
        
        inline bool is_ready() const
        {
            if (_state != nullptr)
            {
                return _state->is_ready();
            }
            return false;
        }
        
        template<class F>
        future_then_t<T, F, shared_future<T>> then(F&& func);
        
        inline void wait() const
        {
            _state->wait();
        }
        template<class Rep, class Period>
        inline future_status wait_for(const std::chrono::duration<Rep, Period>& rel_time) const
        {
            return _state->wait_for(rel_time);
        }
        template<class Clock, class Duration>
        inline future_status wait_until(const std::chrono::time_point<Clock, Duration>& abs_time) const
        {
            return _state->wait_until(abs_time);
        }
    };
    
    template<class T>
    shared_future<T>::~shared_future()
    {
        if (_state)
        {
            _state->release_shared();
        }
    }
    
    template<class T>
    shared_future<T>& shared_future<T>::operator=(const shared_future& rhs) noexcept
    {
        if (rhs._state)
        {
            rhs._state->add_shared();
        }
        
        if (_state)
        {
            _state->release_shared();
        }
        
        _state = rhs._state;
        return *this;
    }
    
    template<class T>
    inline shared_future<T> future<T>::share() noexcept
    {
        return shared_future<T>(std::move(*this));
    }
    
    template<class T>
    void shared_future<T>::then_error(cxx_function::unique_function<void(const std::exception_ptr&)>&& continuation)
    {
        return _state->then_error(std::move(continuation));
    }
    
    template<class T>
    template<class F>
    future_then_t<T, F, shared_future<T>> shared_future<T>::then(F&& func)
    {
        return _state->template then<T, F>(std::move(*this), std::forward<F>(func));
    }
    
    // shared_future<T&>
    
    template<class T>
    class shared_future<T&>
    {
        assoc_state<T&>* _state;
        
        template<typename InputIt>
        friend auto when_all(InputIt first, InputIt last) -> future<std::vector<typename std::iterator_traits<InputIt>::value_type>>;
        template<std::size_t I, typename Context, typename Future>
        friend void __attribute__((__visibility__("hidden"))) when_inner_helper(Context* context, Future&& f);
        
        void then_error(cxx_function::unique_function<void(const std::exception_ptr&)>&& continuation);
        
    public:
        inline shared_future() noexcept : _state(nullptr)
        {
        }
        inline shared_future(const shared_future& rhs) : _state(rhs._state)
        {
            if (_state)
            {
                _state->add_shared();
            }
        }
        inline shared_future(future<T&>&& f) noexcept : _state(f._state)
        {
            f._state = nullptr;
        }
        inline shared_future(shared_future&& rhs) noexcept : _state(rhs._state)
        {
            rhs._state = nullptr;
        }
        ~shared_future();
        shared_future& operator=(const shared_future& rhs);
        inline shared_future& operator=(shared_future&& rhs) noexcept
        {
            shared_future(std::move(rhs)).swap(*this);
            return *this;
        }
        
        inline T& get() const
        {
            return _state->copy();
        }
        
        inline void swap(shared_future& rhs) noexcept
        {
            std::swap(_state, rhs._state);
        }
        
        inline bool valid() const noexcept
        {
            return _state != nullptr;
        }
        
        inline bool is_ready() const
        {
            if (_state)
            {
                return _state->is_ready();
            }
            return false;
        }
        
        template<class F>
        future_then_t<T, F, shared_future<T>> then(F&& func);
        
        inline void wait() const
        {
            _state->wait();
        }
        template<class Rep, class Period>
        inline future_status wait_for(const std::chrono::duration<Rep, Period>& rel_time) const
        {
            return _state->wait_for(rel_time);
        }
        template<class Clock, class Duration>
        inline future_status wait_until(const std::chrono::time_point<Clock, Duration>& abs_time) const
        {
            return _state->wait_until(abs_time);
        }
    };
    
    template<class T>
    shared_future<T&>::~shared_future()
    {
        if (_state)
        {
            _state->release_shared();
        }
    }
    
    template<class T>
    shared_future<T&>& shared_future<T&>::operator=(const shared_future& rhs)
    {
        if (rhs._state)
        {
            rhs._state->add_shared();
        }
        if (_state)
        {
            _state->release_shared();
        }
        _state = rhs._state;
        return *this;
    }
    
    template<class T>
    inline shared_future<T&> future<T&>::share() noexcept
    {
        return shared_future<T&>(std::move(*this));
    }
    
    template<class T>
    template<class F>
    future_then_t<T, F, shared_future<T>> shared_future<T&>::then(F&& func)
    {
        return _state->template then<T, F>(std::move(*this), std::forward<F>(func));
    }
    
    template<class T>
    void shared_future<T&>::then_error(cxx_function::unique_function<void(const std::exception_ptr&)>&& continuation)
    {
        return _state->then_error(std::move(continuation));
    }
    
    // shared_future<void>
    
    template<>
    class shared_future<void>
    {
        assoc_sub_state* _state {nullptr};
        
        template<typename InputIt>
        friend auto when_all(InputIt first, InputIt last) -> future<std::vector<typename std::iterator_traits<InputIt>::value_type>>;
        template<std::size_t I, typename Context, typename Future>
        friend void __attribute__((__visibility__("hidden"))) when_inner_helper(Context* context, Future&& f);
        
        void then_error(cxx_function::unique_function<void(const std::exception_ptr&)>&& continuation);
        
    public:
        inline shared_future() noexcept : _state(nullptr)
        {
        }
        inline shared_future(const shared_future& rhs) : _state(rhs._state)
        {
            if (_state != nullptr)
            {
                _state->add_shared();
            }
        }
        inline shared_future(future<void>&& f) noexcept : _state(f._state)
        {
            f._state = nullptr;
        }
        inline shared_future(shared_future&& rhs) noexcept : _state(rhs._state)
        {
            rhs._state = nullptr;
        }
        ~shared_future();
        shared_future& operator=(const shared_future& rhs);
        inline shared_future& operator=(shared_future&& rhs) noexcept
        {
            shared_future(std::move(rhs)).swap(*this);
            return *this;
        }
        
        inline void get() const
        {
            _state->copy();
        }
        
        inline void swap(shared_future& rhs) noexcept
        {
            std::swap(_state, rhs._state);
        }
        
        inline bool valid() const noexcept
        {
            return _state != nullptr;
        }
        
        inline bool is_ready() const
        {
            if (_state != nullptr)
            {
                return _state->is_ready();
            }
            return false;
        }
        
        template<class F>
        future_then_t<void, F, shared_future<void>> then(F&& func);
        
        inline void wait() const
        {
            _state->wait();
        }
        template<class Rep, class Period>
        inline future_status wait_for(const std::chrono::duration<Rep, Period>& rel_time) const
        {
            return _state->wait_for(rel_time);
        }
        template<class Clock, class Duration>
        inline future_status wait_until(const std::chrono::time_point<Clock, Duration>& abs_time) const
        {
            return _state->wait_until(abs_time);
        }
    };
    
    template<class F>
    future_then_t<void, F, shared_future<void>> shared_future<void>::then(F&& func)
    {
        return _state->template then<void, F>(std::move(*this), std::forward<F>(func));
    }
    
    inline shared_future<void> future<void>::share() noexcept
    {
        return shared_future<void>(std::move(*this));
    }
    
    template<class T>
    inline void swap(shared_future<T>& x, shared_future<T>& y) noexcept
    {
        x.swap(y);
    }
} // namespace ps

namespace std
{
    
    template<>
    struct std::is_error_code_enum<ps::future_errc> : public std::true_type
    {
    };
    
    template<class T, class Alloc>
    struct uses_allocator<ps::promise<T>, Alloc> : public std::true_type
    {
    };
    
    template<class Callable, class Alloc>
    struct uses_allocator<ps::packaged_task<Callable>, Alloc> : public true_type
    {
    };
    
} // namespace std

#endif // FUTURE_FUTURE_HPP

