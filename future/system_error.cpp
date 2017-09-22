//
// system_error.cpp
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

#include "system_error.hpp"

#include "debug.hpp"
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace ps
{
    namespace
    {
        constexpr std::size_t strerror_buff_size = 1024;
        
        std::string do_strerror_r(int ev);
        __attribute__((unused)) const char* handle_strerror_r_return(const char* strerror_return, char* buffer)
        {
            return strerror_return;
        }
        
        const char* handle_strerror_r_return(int strerror_return, const char* buffer)
        {
            if (strerror_return == 0)
            {
                return buffer;
            }
            
            int new_errno = strerror_return == -1 ? errno : strerror_return;
            if (new_errno == EINVAL)
            {
                return "";
            }
            
            ASSERT(new_errno == ERANGE, "unexpected error from ::strerror_r");
            std::abort();
        }
        
        std::string do_strerror_r(int ev)
        {
            char buffer[strerror_buff_size];
            const int old_errno = errno;
            const char* error_message = handle_strerror_r_return(::strerror_r(ev, buffer, strerror_buff_size), buffer);
            
            if (*error_message == '\0')
            {
                std::snprintf(buffer, strerror_buff_size, "Unknown error %d", ev);
                error_message = buffer;
            }
            errno = old_errno;
            return std::string(error_message);
        }
    } // namespace
    
    void throw_system_error(int ev, const char* what_arg)
    {
        throw std::system_error(std::error_code(ev, std::system_category()), what_arg);
    }
    
    std::string do_message::message(int ev) const
    {
        return do_strerror_r(ev);
    }
    
} // namespace ps
