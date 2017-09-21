//
// type_traits.hpp
// future
//
// Created by Mathieu Garaud on 26/08/2017.
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

#include <type_traits>
#include <functional>

namespace ps
{

    template<class T, bool>
    struct dependent_type : public T
    {
    };

    template<class T>
    inline typename std::decay_t<T> decay_copy(T&& t)
    {
        return std::forward<T>(t);
    }

    struct nat
    {
        nat() = delete;
        nat(const nat&) = delete;
        nat& operator=(const nat&) = delete;
        ~nat() = delete;
    };

    struct any
    {
        any(...);
    };

    template<class T>
    struct is_reference_wrapper_impl : public std::false_type
    {
    };
    template<class T>
    struct is_reference_wrapper_impl<std::reference_wrapper<T>> : public std::true_type
    {
    };
    template<class T>
    struct is_reference_wrapper : public is_reference_wrapper_impl<std::remove_cv_t<T>>
    {
    };

    // check_complete

    template<class ...T>
    struct check_complete;

    template<>
    struct check_complete<>
    {
    };

    template<class H, class T0, class ...T>
    struct check_complete<H, T0, T...> : private check_complete<H>, private check_complete<T0, T...>
    {
    };

    template<class H>
    struct check_complete<H, H> : private check_complete<H>
    {
    };

    template<class T>
    struct check_complete<T>
    {
        static_assert(sizeof(T) > 0, "Type must be complete.");
    };

    template<class T>
    struct check_complete<T&> : private check_complete<T>
    {
    };

    template<class T>
    struct check_complete<T&&> : private check_complete<T>
    {
    };

    template<class R, class ...Param>
    struct check_complete<R (*)(Param...)> : private check_complete<R>
    {
    };

    template<class ...Param>
    struct check_complete<void(*)(Param...)>
    {
    };

    template<class R, class ...Param>
    struct check_complete<R (Param...)> : private check_complete<R>
    {
    };

    template<class ...Param>
    struct check_complete<void(Param...)>
    {
    };

    template<class R, class Class, class ...Param>
    struct check_complete<R (Class::*)(Param...)> : private check_complete<Class>
    {
    };

    template<class R, class Class, class ...Param>
    struct check_complete<R (Class::*)(Param...) const> : private check_complete<Class>
    {
    };

    template<class R, class Class, class ...Param>
    struct check_complete<R (Class::*)(Param...) volatile> : private check_complete<Class>
    {
    };

    template<class R, class Class, class ...Param>
    struct check_complete<R (Class::*)(Param...) const volatile> : private check_complete<Class>
    {
    };

    template<class R, class Class, class ...Param>
    struct check_complete<R (Class::*)(Param...) &> : private check_complete<Class>
    {
    };

    template<class R, class Class, class ...Param>
    struct check_complete<R (Class::*)(Param...) const&> : private check_complete<Class>
    {
    };

    template<class R, class Class, class ...Param>
    struct check_complete<R (Class::*)(Param...) volatile&> : private check_complete<Class>
    {
    };

    template<class R, class Class, class ...Param>
    struct check_complete<R (Class::*)(Param...) const volatile&> : private check_complete<Class>
    {
    };

    template<class R, class Class, class ...Param>
    struct check_complete<R (Class::*)(Param...) &&> : private check_complete<Class>
    {
    };

    template<class R, class Class, class ...Param>
    struct check_complete<R (Class::*)(Param...) const&&> : private check_complete<Class>
    {
    };

    template<class R, class Class, class ...Param>
    struct check_complete<R (Class::*)(Param...) volatile&&> : private check_complete<Class>
    {
    };

    template<class R, class Class, class ...Param>
    struct check_complete<R (Class::*)(Param...) const volatile&&> : private check_complete<Class>
    {
    };

    template<class R, class Class>
    struct check_complete<R Class::*> : private check_complete<Class>
    {
    };

    // member_pointer_class_type

    template<class DecayedF>
    struct member_pointer_class_type
    {
    };

    template<class Ret, class ClassType>
    struct member_pointer_class_type<Ret ClassType::*>
    {
        using type = ClassType;
    };

    // enable_if_bullet_#

    template <class F, class A0,
    class DecayF = std::decay_t<F>,
    class DecayA0 = std::decay_t<A0>,
    class ClassT = typename member_pointer_class_type<DecayF>::type>
    using enable_if_bullet1 = std::enable_if_t<std::is_member_function_pointer_v<DecayF> && std::is_base_of_v<ClassT, DecayA0>>;

    template<class F, class A0,
    class DecayF = std::decay_t<F>,
    class DecayA0 = std::decay_t<A0>>
    using enable_if_bullet2 = std::enable_if_t<std::is_member_function_pointer_v<DecayF> && is_reference_wrapper<DecayA0>::value>;

    template<class F, class A0,
    class DecayF = std::decay_t<F>,
    class DecayA0 = std::decay_t<A0>,
    class ClassT = typename member_pointer_class_type<DecayF>::type>
    using enable_if_bullet3 = std::enable_if_t<std::is_member_function_pointer_v<DecayF> && !std::is_base_of_v<ClassT, DecayA0> && !is_reference_wrapper<DecayA0>::value>;

    template<class F, class A0,
    class DecayF = std::decay_t<F>,
    class DecayA0 = std::decay_t<A0>,
    class ClassT = typename member_pointer_class_type<DecayF>::type>
    using enable_if_bullet4 = std::enable_if_t<std::is_member_object_pointer_v<DecayF> && std::is_base_of_v<ClassT, DecayA0>>;

    template<class F, class A0,
    class DecayF = std::decay_t<F>,
    class DecayA0 = std::decay_t<A0>>
    using enable_if_bullet5 = std::enable_if_t<std::is_member_object_pointer_v<DecayF> && is_reference_wrapper<DecayA0>::value>;

    template<class F, class A0,
    class DecayF = std::decay_t<F>,
    class DecayA0 = std::decay_t<A0>,
    class ClassT = typename member_pointer_class_type<DecayF>::type>
    using enable_if_bullet6 = std::enable_if_t<std::is_member_object_pointer_v<DecayF> && !std::is_base_of_v<ClassT, DecayA0> && !is_reference_wrapper<DecayA0>::value>;

    // invoke

    #define INVOKE_RETURN(...) \
        noexcept(noexcept(__VA_ARGS__)) -> decltype(__VA_ARGS__) \
        { return __VA_ARGS__; }

    template <class ...Args>
    auto invoke(any, Args&& ...args) -> nat;

    template <class ..._Args>
    auto invoke_constexpr(any, _Args&& ...args) -> nat;

    template<class F, class A0, class ...Args, class = enable_if_bullet1<F, A0>>
    inline auto invoke(F&& f, A0&& a0, Args&& ...args)
    INVOKE_RETURN((std::forward<A0>(a0).*f)(std::forward<Args>(args)...))

    template<class F, class A0, class ...Args, class = enable_if_bullet1<F, A0>>
    inline constexpr auto invoke_constexpr(F&& f, A0&& a0, Args&& ...args)
    INVOKE_RETURN((std::forward<A0>(a0).*f)(std::forward<Args>(args)...))

    template<class F, class A0, class ...Args, class = enable_if_bullet2<F, A0>>
    inline auto invoke(F&& f, A0&& a0, Args&& ...args)
    INVOKE_RETURN((a0.get().*f)(std::forward<Args>(args)...))

    template<class F, class A0, class ...Args, class = enable_if_bullet2<F, A0>>
    inline constexpr auto invoke_constexpr(F&& f, A0&& a0, Args&& ...args)
    INVOKE_RETURN((a0.get().*f)(std::forward<Args>(args)...))

    template<class F, class A0, class ...Args, class = enable_if_bullet3<F, A0>>
    inline auto invoke(F&& f, A0&& a0, Args&& ...args)
    INVOKE_RETURN(((*std::forward<A0>(a0)).*f)(std::forward<Args>(args)...))

    template<class F, class A0, class ...Args, class = enable_if_bullet3<F, A0>>
    inline constexpr auto invoke_constexpr(F&& f, A0&& a0, Args&& ...args)
    INVOKE_RETURN(((*std::forward<A0>(a0)).*f)(std::forward<Args>(args)...))

    template<class F, class A0, class = enable_if_bullet4<F, A0>>
    inline auto invoke(F&& f, A0&& a0)
    INVOKE_RETURN(std::forward<A0>(a0).*f)

    template<class F, class A0, class = enable_if_bullet4<F, A0>>
    inline constexpr auto invoke_constexpr(F&& f, A0&& a0)
    INVOKE_RETURN(std::forward<A0>(a0).*f)

    template<class F, class A0, class = enable_if_bullet5<F, A0>>
    inline auto invoke(F&& f, A0&& a0)
    INVOKE_RETURN(a0.get().*f)

    template <class F, class A0, class = enable_if_bullet5<F, A0>>
    inline constexpr auto invoke_constexpr(F&& f, A0&& a0)
    INVOKE_RETURN(a0.get().*f)

    template<class F, class A0, class = enable_if_bullet6<F, A0>>
    inline auto invoke(F&& f, A0&& a0)
    INVOKE_RETURN((*std::forward<A0>(a0)).*f)

    template<class F, class A0, class = enable_if_bullet6<F, A0>>
    inline constexpr auto invoke_constexpr(F&& f, A0&& a0)
    INVOKE_RETURN((*std::forward<A0>(a0)).*f)

    template<class F, class ...Args>
    inline auto invoke(F&& f, Args&& ...args)
    INVOKE_RETURN(std::forward<F>(f)(std::forward<Args>(args)...))

    template<class F, class ...Args>
    inline constexpr auto invoke_constexpr(F&& f, Args&& ...args)
    INVOKE_RETURN(std::forward<F>(f)(std::forward<Args>(args)...))

    #undef INVOKE_RETURN

    // invokable

    template<class Ret, class F, class ...Args>
    struct invokable_r : private check_complete<F>
    {
        using Result = decltype(ps::invoke(std::declval<F>(), std::declval<Args>()...));

        using type = typename std::conditional_t<
            !std::is_same_v<Result, nat>,
            typename std::conditional_t<
                std::is_void_v<Ret>,
                std::true_type,
                std::is_convertible<Result, Ret>
            >,
            std::false_type
        >;
        static const bool value = type::value;
    };

    template<class F, class ...Args>
    using invokable = invokable_r<void, F, Args...>;

    template<class F, class ...Args>
    struct invoke_of : public std::enable_if<invokable<F, Args...>::value, typename invokable_r<void, F, Args...>::Result>
    {
    };

    template<class F, class ...Args>
    using invoke_of_t = typename invoke_of<F, Args...>::type;

}
