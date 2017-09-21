//
// memory.hpp
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

#pragma once

#include <memory>
#include <type_traits>
#include <utility>
#include "tuple.hpp"
#include <tuple>
#include <cstddef>
#include "type_traits.hpp"

namespace ps
{

    // shared_count

    class shared_count
    {
    protected:
        long _shared_owners;
        virtual ~shared_count();

    private:
        virtual void on_zero_shared() noexcept = 0;

    public:
        inline explicit shared_count(long refs = 0) noexcept : _shared_owners(refs)
        {
        }
        shared_count(const shared_count&) = delete;
        shared_count& operator=(const shared_count&) = delete;

        inline void add_shared() noexcept
        {
            __atomic_add_fetch(&_shared_owners, 1, __ATOMIC_RELAXED);
        }

        inline bool release_shared() noexcept
        {
            if (__atomic_add_fetch(&_shared_owners, -1, __ATOMIC_ACQ_REL) == -1)
            {
                on_zero_shared();
                return true;
            }
            return false;
        }

        inline long use_count() const noexcept
        {
            return __atomic_load_n(&_shared_owners, __ATOMIC_RELAXED) + 1;
        }
    };

    struct release_shared_count
    {
        inline void operator()(shared_count* p)
        {
            p->release_shared();
        }
    };

    // has_rebind

    template<class T, class U>
    struct has_rebind
    {
    private:
        struct two
        {
            char lx;
            char lxx;
        };
        template<class X>
        static two test(...);
        template<class X>
        static char test(typename X::template rebind<U>* = 0);
    public:
        static const bool value = sizeof(test<T>(0)) == 1;
    };

    template<class T, class U, bool = has_rebind<T, U>::value>
    struct has_rebind_other
    {
    private:
        struct two
        {
            char lx;
            char lxx;
        };
        template<class X>
        static two test(...);
        template<class X>
        static char test(typename X::template rebind<U>::other* = 0);
    public:
        static const bool value = sizeof(test<T>(0)) == 1;
    };

    template<class T, class U>
    struct has_rebind_other<T, U, false>
    {
        static const bool value = false;
    };

    template<class T, class U, bool = has_rebind_other<T, U>::value>
    struct allocator_traits_rebind
    {
        using type = typename T::template rebind<U>::other;
    };

    // allocator_destructor

    template<class Alloc>
    class allocator_destructor
    {
        using alloc_traits = std::allocator_traits<Alloc>;
    public:
        using pointer = typename alloc_traits::pointer;
        using size_type = typename alloc_traits::size_type;
    private:
        Alloc& _alloc;
        size_type _s;
    public:
        inline allocator_destructor(Alloc& a, size_type s) noexcept : _alloc(a), _s(s)
        {
        }
        inline void operator()(pointer p) noexcept
        {
            alloc_traits::deallocate(_alloc, p, _s);
        }
    };

    // compressed_pair

    template<class T, std::size_t Idx, bool CanBeEmptyBase = std::is_empty<T>::value && !std::is_final<T>::value>
    struct compressed_pair_elem {
        using ParamT = T;
        using reference = T&;
        using const_reference = const T&;

        constexpr compressed_pair_elem() : _value()
        {
        }

        template<class U, class = typename std::enable_if<!std::is_same<compressed_pair_elem, typename std::decay<U>::type>::value>::type>
        constexpr explicit compressed_pair_elem(U&& u) : _value(std::forward<U>(u))
        {
        }

        template<class... Args, std::size_t... Indexes>
        inline constexpr compressed_pair_elem(std::piecewise_construct_t, std::tuple<Args...> args, tuple_indices<Indexes...>) : _value(std::forward<Args>(std::get<Indexes>(args))...)
        {
        }

        reference get() noexcept
        {
            return _value;
        }
        const_reference get() const noexcept
        {
            return _value;
        }

    private:
        T _value;
    };

    template<class T, std::size_t Idx>
    struct compressed_pair_elem<T, Idx, true> : private T
    {
        using ParamT = T;
        using reference = T&;
        using const_reference = const T&;
        using value_type = T;

        constexpr compressed_pair_elem() = default;

        template<class U, class = typename std::enable_if<!std::is_same<compressed_pair_elem, typename std::decay<U>::type>::value>::type>
        constexpr explicit compressed_pair_elem(U&& u) : value_type(std::forward<U>(u))
        {
        }

        template<class... Args, std::size_t... Indexes>
        inline constexpr compressed_pair_elem(std::piecewise_construct_t, std::tuple<Args...> args, tuple_indices<Indexes...>) : value_type(std::forward<Args>(std::get<Indexes>(args))...)
        {
        }

        reference get() noexcept
        {
            return *this;
        }
        const_reference get() const noexcept
        {
            return *this;
        }
    };

    struct second_tag
    {
    };

    template<class T1, class T2>
    class compressed_pair : private compressed_pair_elem<T1, 0>, private compressed_pair_elem<T2, 1>
    {
        using Base1 = compressed_pair_elem<T1, 0>;
        using Base2 = compressed_pair_elem<T2, 1>;

    public:
        template<bool Dummy = true, class = typename std::enable_if<dependent_type<std::is_default_constructible<T1>, Dummy>::value && dependent_type<std::is_default_constructible<T2>, Dummy>::value>::type>
        inline constexpr compressed_pair()
        {
        }

        template<class T, typename std::enable_if<!std::is_same<typename std::decay<T>::type, compressed_pair>::value, bool>::type = true>
        inline constexpr explicit compressed_pair(T&& t) : Base1(std::forward<T>(t)), Base2()
        {
        }

        template<class T>
        inline constexpr compressed_pair(second_tag, T&& t) : Base1(), Base2(std::forward<T>(t))
        {
        }

        template<class U1, class U2>
        inline constexpr compressed_pair(U1&& t1, U2&& t2) : Base1(std::forward<U1>(t1)), Base2(std::forward<U2>(t2))
        {
        }

        template<class... Args1, class... Args2>
        inline constexpr compressed_pair(std::piecewise_construct_t pc, std::tuple<Args1...> first_args, std::tuple<Args2...> second_args) : Base1(pc, std::move(first_args), typename make_tuple_indices<sizeof...(Args1)>::type()), Base2(pc, std::move(second_args), typename make_tuple_indices<sizeof...(Args2)>::type())
        {
        }

        inline typename Base1::reference first() noexcept
        {
            return static_cast<Base1&>(*this).get();
        }

        inline typename Base1::const_reference first() const noexcept
        {
            return static_cast<Base1 const&>(*this).get();
        }

        inline typename Base2::reference second() noexcept
        {
            return static_cast<Base2&>(*this).get();
        }

        inline typename Base2::const_reference second() const noexcept
        {
            return static_cast<Base2 const&>(*this).get();
        }

        inline void swap(compressed_pair& x) noexcept(std::is_nothrow_swappable<T1>::value && std::is_nothrow_swappable<T2>::value)
        {
            std::swap(first(), x.first());
            std::swap(second(), x.second());
        }
    };

    template<class T1, class T2>
    inline void swap(compressed_pair<T1, T2>& x, compressed_pair<T1, T2>& y) noexcept(std::is_nothrow_swappable<T1>::value && std::is_nothrow_swappable<T2>::value)
    {
        x.swap(y);
    }

}
