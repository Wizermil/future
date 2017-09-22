//
// thread.cpp
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

#include "thread.hpp"

#include "future.hpp"
#include <cstddef>
#include <ctime>
#include <limits>
#include <sys/errno.h>
#include <sys/sysctl.h>
#include <utility>
#include <vector>

namespace ps
{
    
    // hidden_allocator
    
    template<class T>
    class __attribute__((__visibility__("hidden"))) hidden_allocator
    {
    public:
        using value_type = T;
        
        inline T* allocate(std::size_t n)
        {
            return static_cast<T*>(::operator new(n * sizeof(T)));
        }
        inline void deallocate(T* p, std::size_t /*unused*/)
        {
            ::operator delete(static_cast<void*>(p));
        }
        
        inline std::size_t max_size() const
        {
            return size_t(~0) / sizeof(T);
        }
    };
    
    // thread_struct_imp
    
    class __attribute__((__visibility__("hidden"))) thread_struct_imp
    {
        using AsyncStates = std::vector<assoc_sub_state*, hidden_allocator<assoc_sub_state*>>;
        using Notify = std::vector<std::pair<std::condition_variable*, std::mutex*>, hidden_allocator<std::pair<std::condition_variable*, std::mutex*>>>;
        
        AsyncStates _async_states;
        Notify _notify;
    public:
        thread_struct_imp() = default;
        thread_struct_imp(const thread_struct_imp&) =delete;
        thread_struct_imp& operator=(const thread_struct_imp&) = delete;
        thread_struct_imp(thread_struct_imp&&) noexcept =delete;
        thread_struct_imp& operator=(thread_struct_imp&&) noexcept = delete;
        ~thread_struct_imp();
        
        void notify_all_at_thread_exit(std::condition_variable* cv, std::mutex* m);
        void make_ready_at_thread_exit(assoc_sub_state* s);
    };
    
    thread_struct_imp::~thread_struct_imp()
    {
        for (const auto& notify : _notify)
        {
            notify.second->unlock();
            notify.first->notify_all();
        }
        for (const auto& async_state : _async_states)
        {
            async_state->make_ready();
            async_state->release_shared();
        }
    }
    
    void thread_struct_imp::notify_all_at_thread_exit(std::condition_variable* cv, std::mutex* m)
    {
        _notify.emplace_back(std::pair<std::condition_variable*, std::mutex*>(cv, m));
    }
    
    void thread_struct_imp::make_ready_at_thread_exit(assoc_sub_state* s)
    {
        _async_states.push_back(s);
        s->add_shared();
    }
    
    // thread_struct
    
    thread_struct::thread_struct() : _p(new thread_struct_imp)
    {
    }
    
    thread_struct::~thread_struct()
    {
        delete _p;
    }
    
    void thread_struct::notify_all_at_thread_exit(std::condition_variable* cv, std::mutex* m)
    {
        _p->notify_all_at_thread_exit(cv, m);
    }
    
    void thread_struct::make_ready_at_thread_exit(assoc_sub_state* s)
    {
        _p->make_ready_at_thread_exit(s);
    }
    
    // thread_specific_ptr
    
    thread_specific_ptr<thread_struct>& thread_local_data()
    {
        static thread_specific_ptr<thread_struct> p;
        return p;
    }
    
    // thread
    
    thread::~thread()
    {
        if (_t != nullptr)
        {
            std::terminate();
        }
    }
    
    void thread::join()
    {
        int ec = EINVAL;
        if (_t != nullptr)
        {
            ec = pthread_join(_t, nullptr);
            if (ec == 0)
            {
                _t = nullptr;
            }
        }
        
        if (ec != 0)
        {
            throw_system_error(ec, "thread::join failed");
        }
    }
    
    void thread::detach()
    {
        int ec = EINVAL;
        if (_t != nullptr)
        {
            ec = pthread_detach(_t);
            if (ec == 0)
            {
                _t = nullptr;
            }
        }
        
        if (ec != 0)
        {
            throw_system_error(ec, "thread::detach failed");
        }
    }
    
    unsigned thread::hardware_concurrency() noexcept
    {
        unsigned n;
        int mib[2] = {CTL_HW, HW_NCPU};
        std::size_t s = sizeof(n);
        sysctl(mib, 2, &n, &s, nullptr, 0);
        return n;
    }
    
    namespace this_thread
    {
        void sleep_for(const std::chrono::nanoseconds& ns)
        {
            if (ns > std::chrono::nanoseconds::zero())
            {
                std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds>(ns);
                timespec ts{};
                using ts_sec = decltype(ts.tv_sec);
                constexpr ts_sec ts_sec_max = std::numeric_limits<ts_sec>::max();
                
                if (s.count() < ts_sec_max)
                {
                    ts.tv_sec = static_cast<ts_sec>(s.count());
                    ts.tv_nsec = static_cast<decltype(ts.tv_nsec)>((ns - s).count());
                }
                else
                {
                    ts.tv_sec = ts_sec_max;
                    ts.tv_nsec = 999999999; // (10^9 - 1)
                }
                
                while (nanosleep(&ts, &ts) == -1 && errno == EINTR)
                {
                }
            }
        }
    } // namespace this_thread
    
} // namespace ps
