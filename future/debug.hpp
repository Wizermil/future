//
// debug.hpp
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

namespace ps
{

#ifdef DEBUG
    #define ASSERT(x, m) ((x) ? (void)0 : libcpp_debug_function(libcpp_debug_info(__FILE__, __LINE__, #x, m)))
#else
    #define ASSERT(x, m) (void)0
#endif

    struct libcpp_debug_info
    {
        inline constexpr libcpp_debug_info() : file(nullptr), line(-1), pred(nullptr), msg(nullptr)
        {
        }
        inline constexpr libcpp_debug_info(const char* f, int l, const char* p, const char* m) : file(f), line(l), pred(p), msg(m)
        {
        }

        const char* file;
        int line;
        const char* pred;
        const char* msg;
    };

    using libcpp_debug_function_type = std::add_pointer<void(const libcpp_debug_info&)>::type;

    [[noreturn]] void libcpp_abort_debug_function(const libcpp_debug_info&);

    extern libcpp_debug_function_type libcpp_debug_function;

}
