//
// tuple.hpp
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

#include <cstddef>
#include <tuple>
#include <type_traits>

namespace ps
{

    template<std::size_t...>
    struct tuple_indices
    {
    };

    template<class IdxType, IdxType... Values>
    struct integer_sequence
    {
        template<template<class OIdxType, OIdxType...> class ToIndexSeq, class ToIndexType>
        using convert = ToIndexSeq<ToIndexType, Values...>;

        template<std::size_t S>
        using to_tuple_indices = tuple_indices<(Values + S)...>;
    };

    template<std::size_t E, std::size_t S>
    using make_indices_imp = typename __make_integer_seq<integer_sequence, std::size_t, E - S>::template to_tuple_indices<S>;

    template<std::size_t E, std::size_t S = 0>
    struct make_tuple_indices
    {
        static_assert(S <= E, "make_tuple_indices input error");
        using type = make_indices_imp<E, S>;
    };

    template<class T>
    class tuple_size;

    template<class T, class...>
    using enable_if_tuple_size_imp = T;

    template<class T>
    class tuple_size<enable_if_tuple_size_imp<const T, typename std::enable_if_t<!std::is_volatile_v<T>>, std::integral_constant<std::size_t, sizeof(tuple_size<T>)>>> : public std::integral_constant<std::size_t, tuple_size<T>::value>
    {
    };

    template<class T>
    class tuple_size<enable_if_tuple_size_imp<volatile T, typename std::enable_if_t<!std::is_const_v<T>>, std::integral_constant<std::size_t, sizeof(tuple_size<T>)>>> : public std::integral_constant<std::size_t, tuple_size<T>::value>
    {
    };

    template<class T>
    class tuple_size<enable_if_tuple_size_imp<const volatile T, std::integral_constant<std::size_t, sizeof(tuple_size<T>)>>> : public std::integral_constant<std::size_t, tuple_size<T>::value>
    {
    };

    template<class ...T>
    class tuple_size<std::tuple<T...>> : public std::integral_constant<std::size_t, sizeof...(T)>
    {
    };

} // namespace ps
