//
// thread.hpp
// future
//
// Created by Mathieu Garaud on 25/08/2017.
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

#ifndef FUTURE_THREAD_HPP
#define FUTURE_THREAD_HPP

#include "debug.hpp"
#include "system_error.hpp"
#include "tuple.hpp"
#include <chrono>
#include <condition_variable>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <ostream>
#include <pthread.h>
#include <tuple>
#include <type_traits>

namespace ps
{
    
    template<class T>
    class thread_specific_ptr;
    class thread_struct;
    class __attribute__((__visibility__("hidden"))) thread_struct_imp;
    class assoc_sub_state;
    
    thread_specific_ptr<thread_struct>& thread_local_data();
    
    class thread_struct
    {
        thread_struct_imp* _p;
    public:
        thread_struct();
        thread_struct(const thread_struct&) = delete;
        thread_struct& operator=(const thread_struct&) = delete;
        thread_struct(thread_struct&&) noexcept = delete;
        thread_struct& operator=(thread_struct&&) noexcept = delete;
        ~thread_struct();
        
        void notify_all_at_thread_exit(std::condition_variable* cv, std::mutex* m);
        void make_ready_at_thread_exit(assoc_sub_state* s);
    };
    
    template<class T>
    class thread_specific_ptr
    {
        pthread_key_t _key{0};
        
        // Only thread_local_data() may construct a thread_specific_ptr and only with T == thread_struct.
        static_assert((std::is_same<T, thread_struct>::value), "");
        thread_specific_ptr()
        {
            int ec = pthread_key_create(&_key, &thread_specific_ptr::at_thread_exit);
            if (ec != 0)
            {
                throw_system_error(ec, "thread_specific_ptr construction failed");
            }
        }
        
        friend thread_specific_ptr<thread_struct>& thread_local_data();
        
        static void at_thread_exit(void* p)
        {
            delete static_cast<pointer>(p);
        }
        
    public:
        using pointer = T*;
        
        // thread_specific_ptr is only created with a static storage duration so this destructor is only invoked during program termination.
        // Invoking pthread_key_delete(_key) may prevent other threads from deleting their thread local data. For this reason we leak the key.
        ~thread_specific_ptr() = default;
        thread_specific_ptr(const thread_specific_ptr&)  = delete;
        thread_specific_ptr& operator=(const thread_specific_ptr&) = delete;
        thread_specific_ptr(thread_specific_ptr&&) noexcept = delete;
        thread_specific_ptr& operator=(thread_specific_ptr&&) noexcept = delete;
        
        pointer get() const
        {
            return static_cast<T*>(pthread_getspecific(_key));
        }
        pointer operator*() const
        {
            return *get();
        }
        pointer operator->() const
        {
            return get();
        }
        void set_pointer(pointer p)
        {
            ASSERT(get() == nullptr, "Attempting to overwrite thread local data");
            pthread_setspecific(_key, p);
        }
    };
    
    class thread;
    class thread_id;

#ifdef __ANDROID__
#define PS_PTHREAD_T_NULL 0
#else
#define PS_PTHREAD_T_NULL nullptr
#endif

    
    namespace this_thread
    {
        inline thread_id get_id() noexcept;
    } // namespace this_thread
    
    class thread_id
    {
        // FIXME: pthread_t is a pointer on Darwin but a long on Linux.
        // NULL is the no-thread value on Darwin.  Someone needs to check
        // on other platforms.  We assume 0 works everywhere for now.
        pthread_t _id{PS_PTHREAD_T_NULL};
        
    public:
        inline thread_id() noexcept = default;
        
        friend inline bool operator==(thread_id x, thread_id y) noexcept
        {
            return pthread_equal(x._id, y._id) != 0;
        }
        friend inline bool operator!=(thread_id x, thread_id y) noexcept
        {
            return !(x == y);
        }
        friend inline bool operator<(thread_id x, thread_id y) noexcept
        {
            return (x._id < y._id);
        }
        
        friend inline bool operator<=(thread_id x, thread_id y) noexcept
        {
            return !(y < x);
        }
        friend inline bool operator>(thread_id x, thread_id y) noexcept
        {
            return (y < x);
        }
        friend inline bool operator>=(thread_id x, thread_id y) noexcept
        {
            return !(x < y);
        }
        template<class CharT, class Traits>
        friend inline std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, thread_id id)
        {
            return os << id._id;
        }
        
    private:
        inline thread_id(pthread_t id) : _id(id)
        {
        }
        
        friend thread_id this_thread::get_id() noexcept;
        friend class thread;
        friend struct std::hash<thread_id>;
    };
    
    namespace this_thread
    {
        inline thread_id get_id() noexcept
        {
            return pthread_self();
        }
    } // namespace this_thread
    
    class thread
    {
        pthread_t _t {PS_PTHREAD_T_NULL};
        
    public:
        using id = thread_id;
        using native_handle_type = pthread_t;
        
        inline thread() noexcept = default;
        
        template<class F, class ...Args, class = typename std::enable_if_t<!std::is_same<typename std::decay_t<F>, thread>::value>>
        explicit thread(F&& f, Args&&... args);
        ~thread();
        
        thread(const thread&) = delete;
        thread& operator=(const thread&) = delete;
        inline thread(thread&& t) noexcept : _t(t._t)
        {
            t._t = PS_PTHREAD_T_NULL;
        }
        inline thread& operator=(thread&& t) noexcept;
        
        inline void swap(thread& t) noexcept
        {
            std::swap(_t, t._t);
        }
        
        inline bool joinable() const noexcept
        {
            return !(_t == PS_PTHREAD_T_NULL);
        }
        void join();
        void detach();
        inline id get_id() const noexcept
        {
            return _t;
        }
        inline native_handle_type native_handle() noexcept
        {
            return _t;
        }
        
        static unsigned hardware_concurrency() noexcept;
    };
    
    template <class TS, class F, class ...Args, std::size_t ...Indices>
    inline void thread_execute(std::tuple<TS, F, Args...>& t, tuple_indices<Indices...> /*unused*/)
    {
        invoke(std::move(std::get<1>(t)), std::move(std::get<Indices>(t))...);
    }
    
    template<class F>
    void* thread_proxy(void* vp)
    {
        // _Fp = std::tuple< unique_ptr<__thread_struct>, Functor, Args...>
        std::unique_ptr<F> p(static_cast<F*>(vp));
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundefined-func-template"
        thread_local_data().set_pointer(std::get<0>(*p).release());
#pragma clang diagnostic pop
        using Index = typename make_tuple_indices<tuple_size<F>::value, 2>::type;
        thread_execute(*p, Index());
        return nullptr;
    }
    
    template<class F, class ...Args, class>
    thread::thread(F&& f, Args&&... args)
    {
        using TSPtr = std::unique_ptr<thread_struct> ;
        TSPtr tsp(new thread_struct);
        using G = std::tuple<TSPtr, typename std::decay_t<F>, typename std::decay_t<Args>...>;
        std::unique_ptr<G> p(new G(std::move(tsp), decay_copy(std::forward<F>(f)), decay_copy(std::forward<Args>(args))...));
        int ec = pthread_create(&_t, NULL, &thread_proxy<G>, p.get());
        if (ec == 0)
        {
            p.release();
        }
        else
        {
            throw_system_error(ec, "thread constructor failed");
        }
    }
    
    inline thread& thread::operator=(thread&& t) noexcept
    {
        if (_t != PS_PTHREAD_T_NULL)
        {
            std::terminate();
        }
        _t = t._t;
        t._t = PS_PTHREAD_T_NULL;
        return *this;
    }
    
    inline void swap(thread& x, thread& y) noexcept
    {
        x.swap(y);
    }
    
    namespace this_thread
    {
        void sleep_for(const std::chrono::nanoseconds& ns);
        
        template<class Rep, class Period>
        void sleep_for(const std::chrono::duration<Rep, Period>& d)
        {
            if (d > std::chrono::duration<Rep, Period>::zero())
            {
                constexpr std::chrono::duration<long double> Max = std::chrono::nanoseconds::max();
                std::chrono::nanoseconds ns{};
                if (d < Max)
                {
                    ns = std::chrono::duration_cast<std::chrono::nanoseconds>(d);
                    if (ns < d)
                    {
                        ++ns;
                    }
                }
                else
                {
                    ns = std::chrono::nanoseconds::max();
                }
                sleep_for(ns);
            }
        }
        
        template<class Clock, class Duration>
        void sleep_until(const std::chrono::time_point<Clock, Duration>& t)
        {
            std::mutex mut;
            std::condition_variable cv;
            std::unique_lock<std::mutex> lk(mut);
            while (Clock::now() < t)
            {
                cv.wait_until(lk, t);
            }
        }
        
        template<class Duration>
        inline void sleep_until(const std::chrono::time_point<std::chrono::steady_clock, Duration>& t)
        {
            sleep_for(t - std::chrono::steady_clock::now());
        }
        
        inline void yield() noexcept
        {
            sched_yield();
        }
    } // namespace this_thread
    
} // namespace ps

namespace std
{
    template<>
    struct std::hash<ps::thread_id>;
    
    template<>
    struct std::hash<ps::thread_id> : public std::unary_function<ps::thread_id, std::size_t>
    {
        inline size_t operator()(ps::thread_id v) const noexcept
        {
            return std::hash<pthread_t>()(v._id);
        }
    };
} // namespace std

#endif // FUTURE_THREAD_HPP
