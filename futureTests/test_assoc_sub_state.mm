//
// test_assoc_sub_state.mm
// futureTests
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

#import <XCTest/XCTest.h>
#import <future/future.h>
#include <exception>
#include <stdexcept>
#include <chrono>

@interface test_assoc_sub_state : XCTestCase

@end

@implementation test_assoc_sub_state

- (void)testNew {
    ps::assoc_sub_state a;
    
    XCTAssertFalse(a.has_value());
    XCTAssertFalse(a.has_future_attached());
    XCTAssertFalse(a.is_ready());
}

- (void)testReady {
    ps::assoc_sub_state a;
    a.make_ready();
    XCTAssertTrue(a.is_ready());
}

- (void)testFutureAttached {
    ps::assoc_sub_state a;
    a.set_future_attached();
    XCTAssertTrue(a.has_future_attached());
}

- (void)testSetter {
    ps::assoc_sub_state a;
    a.set_value();
    XCTAssertTrue(a.is_ready());
    XCTAssertThrows(a.set_value());
    XCTAssertThrows(a.set_value_at_thread_exit());
    XCTAssertThrows(a.set_exception(std::make_exception_ptr(std::logic_error("logic_error"))));
    XCTAssertThrows(a.set_exception_at_thread_exit(std::make_exception_ptr(std::logic_error("logic_error"))));
    
    ps::assoc_sub_state b;
    b.set_exception(std::make_exception_ptr(std::logic_error("logic_error")));
    XCTAssertTrue(b.is_ready());
    XCTAssertThrows(b.set_value());
    XCTAssertThrows(b.set_value_at_thread_exit());
    XCTAssertThrows(b.set_exception(std::make_exception_ptr(std::logic_error("logic_error"))));
    XCTAssertThrows(b.set_exception_at_thread_exit(std::make_exception_ptr(std::logic_error("logic_error"))));
}

- (void)testWait {
    using namespace std::chrono_literals;
    
    constexpr auto dur = 5ms;
    constexpr auto upper_dur = dur + 2ms;
    
    ps::assoc_sub_state a;
    auto now = std::chrono::high_resolution_clock::now();
    a.wait_for(dur);
    auto res = (std::chrono::high_resolution_clock::now() - now);
    XCTAssertGreaterThan(res, dur);
    XCTAssertLessThan(res, upper_dur);
    
    ps::assoc_sub_state b;
    now = std::chrono::high_resolution_clock::now();
    a.wait_until(now + dur);
    res = (std::chrono::high_resolution_clock::now() - now);
    XCTAssertGreaterThan(res, dur);
    XCTAssertLessThan(res, upper_dur);
    
    ps::assoc_sub_state c;
    auto tc = ps::thread([&c, &dur]() {
        ps::this_thread::sleep_for(dur);
        c.make_ready();
    });
    now = std::chrono::high_resolution_clock::now();
    c.wait();
    res = (std::chrono::high_resolution_clock::now() - now);
    XCTAssertGreaterThan(res, dur);
    XCTAssertLessThan(res, upper_dur);
    if (tc.joinable())
        tc.join();
    
    ps::assoc_sub_state d;
    auto td = ps::thread([&d, &dur]() {
        ps::this_thread::sleep_for(dur);
        d.set_value();
    });
    now = std::chrono::high_resolution_clock::now();
    d.wait();
    res = (std::chrono::high_resolution_clock::now() - now);
    XCTAssertGreaterThan(res, dur);
    XCTAssertLessThan(res, upper_dur);
    if (td.joinable())
        td.join();
    
    ps::assoc_sub_state e;
    auto te = ps::thread([&e, &dur]() {
        ps::this_thread::sleep_for(dur);
        e.set_exception(std::make_exception_ptr(std::logic_error("logic_error")));
    });
    now = std::chrono::high_resolution_clock::now();
    e.wait();
    res = (std::chrono::high_resolution_clock::now() - now);
    XCTAssertGreaterThan(res, dur);
    XCTAssertLessThan(res, upper_dur);
    if (te.joinable())
        te.join();
}

- (void)testExecute {
    ps::assoc_sub_state a;
    XCTAssertThrows(a.execute());
}

- (void)testSharedCount {
    ps::assoc_sub_state a;
    XCTAssertEqual(a.use_count(), 1);
    a.add_shared();
    XCTAssertEqual(a.use_count(), 2);
    a.release_shared();
    XCTAssertEqual(a.use_count(), 1);
    
    ps::assoc_sub_state* b = new ps::assoc_sub_state();
    b->release_shared();
}

- (void)testThen {
    std::exception_ptr i = nullptr;
    ps::assoc_sub_state a;
    a.make_ready();
    a.then([&i](std::exception_ptr e) {
        i = e;
    });
    XCTAssertEqual(i, nullptr);
    
    i = nullptr;
    ps::assoc_sub_state b;
    b.set_value();
    b.then([&i](std::exception_ptr e) {
        i = e;
    });
    XCTAssertEqual(i, nullptr);
    
    i = nullptr;
    ps::assoc_sub_state c;
    c.set_exception(std::make_exception_ptr(std::logic_error("logic_error")));
    c.then([&i](std::exception_ptr e) {
        i = e;
    });
    XCTAssertNotEqual(i, nullptr);
}

@end
