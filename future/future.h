//
// future.h
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

#ifndef FUTURE_FUTURE_H
#define FUTURE_FUTURE_H

#ifdef __cplusplus
    #define FUTURE_EXTERN extern "C"
#else
    #define FUTURE_EXTERN
#endif

//! Project version number for future.
FUTURE_EXTERN double futureVersionNumber;

//! Project version string for future.
FUTURE_EXTERN const unsigned char futureVersionString[];

// In this header, you should import all the public headers of your framework using statements like #import <future/PublicHeader.h>
#include <future/debug.hpp>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <future/cxx_function.hpp>
#pragma clang diagnostic pop
#include <future/future.hpp>
#include <future/memory.hpp>
#include <future/system_error.hpp>
#include <future/thread.hpp>
#include <future/tuple.hpp>
#include <future/type_traits.hpp>

#endif // FUTURE_FUTURE_H
