//
// test_shared_future.mm
// futureTests
//
// Created by Mathieu Garaud on 28/08/2017.
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
#include <string>
#include <functional>

@interface test_shared_future : XCTestCase

@end

@implementation test_shared_future

- (void)testReadyT {
    ps::promise<int> p1;
    ps::shared_future<int> f1 = p1.get_future().share();
    XCTAssertFalse(f1.is_ready());
    p1.set_value(42);
    XCTAssertTrue(f1.is_ready());
    
    ps::promise<int> p2;
    ps::shared_future<int> f2 = p2.get_future().share();
    XCTAssertFalse(f2.is_ready());
    p2.set_exception(std::make_exception_ptr(std::logic_error("logic_error2")));
    XCTAssertTrue(f2.is_ready());
}

- (void)testReadyVoid {
    ps::promise<void> p1;
    ps::shared_future<void> f1 = p1.get_future().share();
    XCTAssertFalse(f1.is_ready());
    p1.set_value();
    XCTAssertTrue(f1.is_ready());
    
    ps::promise<void> p2;
    ps::shared_future<void> f2 = p2.get_future().share();
    XCTAssertFalse(f2.is_ready());
    p2.set_exception(std::make_exception_ptr(std::logic_error("logic_error2")));
    XCTAssertTrue(f2.is_ready());
}

- (void)testReadyReference {
    ps::promise<int&> p1;
    ps::shared_future<int&> f1 = p1.get_future().share();
    XCTAssertFalse(f1.is_ready());
    int j = 42;
    p1.set_value(j);
    XCTAssertTrue(f1.is_ready());
    
    ps::promise<int&> p2;
    ps::shared_future<int&> f2 = p2.get_future().share();
    XCTAssertFalse(f2.is_ready());
    p2.set_exception(std::make_exception_ptr(std::logic_error("logic_error2")));
    XCTAssertTrue(f2.is_ready());
}

- (void)testThreadAtExitReadyT {
    using namespace std::chrono_literals;
    
    ps::promise<int> p1;
    ps::shared_future<int> f1 = p1.get_future().share();
    auto t1 = ps::thread([p = std::move(p1)]() mutable {
        p.set_value_at_thread_exit(42);
        ps::this_thread::sleep_for(5ms);
    });
    ps::this_thread::sleep_for(1ms);
    XCTAssertFalse(f1.is_ready());
    if (t1.joinable())
        t1.join();
    XCTAssertTrue(f1.is_ready());
    
    ps::promise<int> p2;
    ps::shared_future<int> f2 = p2.get_future().share();
    auto t2 = ps::thread([p = std::move(p2)]() mutable {
        p.set_exception_at_thread_exit(std::make_exception_ptr(std::logic_error("logic_error2")));
        ps::this_thread::sleep_for(5ms);
    });
    ps::this_thread::sleep_for(1ms);
    XCTAssertFalse(f2.is_ready());
    if (t2.joinable())
        t2.join();
    XCTAssertTrue(f2.is_ready());
}

- (void)testThreadAtExitReadyVoid {
    using namespace std::chrono_literals;
    
    ps::promise<void> p1;
    ps::shared_future<void> f1 = p1.get_future().share();
    auto t1 = ps::thread([p = std::move(p1)]() mutable {
        p.set_value_at_thread_exit();
        ps::this_thread::sleep_for(5ms);
    });
    ps::this_thread::sleep_for(1ms);
    XCTAssertFalse(f1.is_ready());
    if (t1.joinable())
        t1.join();
    XCTAssertTrue(f1.is_ready());
    
    ps::promise<void> p2;
    ps::shared_future<void> f2 = p2.get_future().share();
    auto t2 = ps::thread([p = std::move(p2)]() mutable {
        p.set_exception_at_thread_exit(std::make_exception_ptr(std::logic_error("logic_error2")));
        ps::this_thread::sleep_for(5ms);
    });
    ps::this_thread::sleep_for(1ms);
    XCTAssertFalse(f2.is_ready());
    if (t2.joinable())
        t2.join();
    XCTAssertTrue(f2.is_ready());
}

- (void)testThreadAtExitReadyReference {
    using namespace std::chrono_literals;
    
    int j = 42;
    ps::promise<int&> p1;
    auto f1 = p1.get_future().share();
    auto t1 = ps::thread([&j, p = std::move(p1)]() mutable {
        p.set_value_at_thread_exit(j);
        ps::this_thread::sleep_for(5ms);
    });
    ps::this_thread::sleep_for(1ms);
    XCTAssertFalse(f1.is_ready());
    if (t1.joinable())
        t1.join();
    XCTAssertTrue(f1.is_ready());
    
    ps::promise<int&> p2;
    auto f2 = p2.get_future().share();
    auto t2 = ps::thread([p = std::move(p2)]() mutable {
        p.set_exception_at_thread_exit(std::make_exception_ptr(std::logic_error("logic_error2")));
        ps::this_thread::sleep_for(5ms);
    });
    ps::this_thread::sleep_for(1ms);
    XCTAssertFalse(f2.is_ready());
    if (t2.joinable())
        t2.join();
    XCTAssertTrue(f2.is_ready());
}

- (void)testThenT {
    using namespace std::chrono_literals;
    
    ps::promise<int> p1;
    auto f1 = p1.get_future().share();
    auto t1 = ps::thread([p = std::move(p1)]() mutable {
        ps::this_thread::sleep_for(5ms);
        p.set_value(42);
    });
    int i = 0;
    auto res1 = f1.then([&i](auto f) {
        i = f.get();
        return 7+i;
    }).get();
    if (t1.joinable())
        t1.join();
    XCTAssertEqual(i, 42);
    XCTAssertEqual(res1, 7+42);
    
    i = 0;
    ps::promise<int> p2;
    auto f2 = p2.get_future().share();
    p2.set_value(42);
    auto res2 = f2.then([&i](auto f) {
        i = f.get();
        return 7+i;
    }).get();
    XCTAssertEqual(i, 42);
    XCTAssertEqual(res2, 7+42);
    
    std::exception_ptr e = nullptr;
    i = 0;
    ps::promise<int> p3;
    auto f3 = p3.get_future().share();
    p3.set_exception(std::make_exception_ptr(std::logic_error("logic_error3")));
    try {
        f3.then([&i](auto f) {
            i = f.get();
            return 7+i;
        }).get();
    } catch(...) {
        e = std::current_exception();
    }
    XCTAssertEqual(i, 0);
    XCTAssertNotEqual(e, nullptr);
    
    e = nullptr;
    i = 0;
    ps::promise<int> p4;
    auto f4 = p4.get_future().share();
    auto t4 = ps::thread([p = std::move(p4)]() mutable {
        ps::this_thread::sleep_for(5ms);
        p.set_exception(std::make_exception_ptr(std::logic_error("logic_error4")));
    });
    try {
        f4.then([&i](auto f) {
            i = f.get();
            return 7+i;
        }).get();
    } catch(...) {
        e = std::current_exception();
    }
    if (t4.joinable())
        t4.join();
    XCTAssertEqual(i, 0);
    XCTAssertNotEqual(e, nullptr);
    
    e = nullptr;
    i = 0;
    ps::promise<int> p5;
    auto f5 = p5.get_future().share();
    p5.set_value(42);
    try {
        f5.then([&i](auto f) {
            i = f.get();
            throw std::logic_error("logic_error5");
            return 7+i;
        }).get();
    } catch(...) {
        e = std::current_exception();
    }
    XCTAssertEqual(i, 42);
    XCTAssertNotEqual(e, nullptr);
    
    e = nullptr;
    i = 0;
    ps::promise<int> p6;
    auto f6 = p6.get_future().share();
    auto t6 = ps::thread([p = std::move(p6)]() mutable {
        ps::this_thread::sleep_for(5ms);
        p.set_value(42);
    });
    try {
        f6.then([&i](auto f) {
            i = f.get();
            throw std::logic_error("logic_error6");
            return 7+i;
        }).get();
    } catch(...) {
        e = std::current_exception();
    }
    if (t6.joinable())
        t6.join();
    XCTAssertEqual(i, 42);
    XCTAssertNotEqual(e, nullptr);
    
    e = nullptr;
    i = 0;
    ps::promise<int> p7;
    auto f7 = p7.get_future().share();
    auto t7 = ps::thread([p = std::move(p7)]() mutable {
        ps::this_thread::sleep_for(5ms);
        p.set_value(42);
    });
    auto ret7 = f7.then([&i](auto f) {
        i = f.get();
        return "7"+std::to_string(i);
    }).get();
    if (t7.joinable())
        t7.join();
    XCTAssertEqual(i, 42);
    XCTAssertEqual(ret7, "742");
    
    e = nullptr;
    i = 0;
    ps::promise<int> p8;
    auto f8 = p8.get_future().share();
    auto t8 = ps::thread([p = std::move(p8)]() mutable {
        ps::this_thread::sleep_for(10ms);
        p.set_value(42);
    });
    auto ret8 = f8.then([&i](auto f) {
        i = f.get();
        ps::promise<std::string> prom;
        auto fut = prom.get_future();
        auto t = ps::thread([i, p = std::move(prom)]() mutable {
            ps::this_thread::sleep_for(5ms);
            p.set_value("toto"+std::to_string(i));
        });
        t.detach();
        return fut;
    }).get();
    if (t8.joinable())
        t8.join();
    XCTAssertEqual(i, 42);
    XCTAssertEqual(ret8, "toto42");
    
    e = nullptr;
    i = 0;
    ps::promise<int> p9;
    auto f9 = p9.get_future().share();
    auto t9 = ps::thread([p = std::move(p9)]() mutable {
        ps::this_thread::sleep_for(10ms);
        p.set_value(42);
    });
    try {
        auto ret9 = f9.then([&i](auto f) {
            i = f.get();
            ps::promise<std::string> prom;
            auto fut = prom.get_future();
            auto t = ps::thread([p = std::move(prom)]() mutable {
                ps::this_thread::sleep_for(5ms);
                p.set_exception(std::make_exception_ptr(std::logic_error("logic_error9")));
            });
            t.detach();
            return fut;
        }).get();
    } catch(...) {
        e = std::current_exception();
    }
    if (t9.joinable())
        t9.join();
    XCTAssertEqual(i, 42);
    XCTAssertNotEqual(e, nullptr);
    
    e = nullptr;
    i = 0;
    ps::promise<int> p10;
    auto f10 = p10.get_future().share();
    auto t10 = ps::thread([p = std::move(p10)]() mutable {
        ps::this_thread::sleep_for(10ms);
        p.set_value(42);
    });
    try {
        auto ret10 = f10.then([&i](auto f) {
            i = f.get();
            ps::promise<std::string> prom;
            auto fut = prom.get_future();
            auto t = ps::thread([p = std::move(prom)]() mutable {
                ps::this_thread::sleep_for(5ms);
                try {
                    p.set_exception(std::make_exception_ptr(std::logic_error("logic_error10-1")));
                } catch (...) {
                }
            });
            t.detach();
            throw std::logic_error("logic_error10-2");
            return fut;
        }).get();
    } catch(...) {
        e = std::current_exception();
    }
    if (t10.joinable())
        t10.join();
    XCTAssertEqual(i, 42);
    XCTAssertNotEqual(e, nullptr);
}

- (void)testThreadAtExitThenT {
    using namespace std::chrono_literals;
    
    ps::promise<int> p1;
    auto f1 = p1.get_future().share();
    auto t1 = ps::thread([p = std::move(p1)]() mutable {
        p.set_value_at_thread_exit(42);
        ps::this_thread::sleep_for(5ms);
    });
    int i = 0;
    auto res1 = f1.then([&i](auto f) {
        i = f.get();
        return 7+i;
    }).get();
    if (t1.joinable())
        t1.join();
    XCTAssertEqual(i, 42);
    XCTAssertEqual(res1, 7+42);
    
    std::exception_ptr e = nullptr;
    i = 0;
    ps::promise<int> p4;
    auto f4 = p4.get_future().share();
    auto t4 = ps::thread([p = std::move(p4)]() mutable {
        p.set_exception_at_thread_exit(std::make_exception_ptr(std::logic_error("logic_error4")));
        ps::this_thread::sleep_for(5ms);
    });
    try {
        f4.then([&i](auto f) {
            i = f.get();
            return 7+i;
        }).get();
    } catch(...) {
        e = std::current_exception();
    }
    if (t4.joinable())
        t4.join();
    XCTAssertEqual(i, 0);
    XCTAssertNotEqual(e, nullptr);
    
    e = nullptr;
    i = 0;
    ps::promise<int> p6;
    auto f6 = p6.get_future().share();
    auto t6 = ps::thread([p = std::move(p6)]() mutable {
        p.set_value_at_thread_exit(42);
        ps::this_thread::sleep_for(5ms);
    });
    try {
        f6.then([&i](auto f) {
            i = f.get();
            throw std::logic_error("logic_error6");
            return 7+i;
        }).get();
    } catch(...) {
        e = std::current_exception();
    }
    if (t6.joinable())
        t6.join();
    XCTAssertEqual(i, 42);
    XCTAssertNotEqual(e, nullptr);
    
    e = nullptr;
    i = 0;
    ps::promise<int> p7;
    auto f7 = p7.get_future().share();
    auto t7 = ps::thread([p = std::move(p7)]() mutable {
        p.set_value_at_thread_exit(42);
        ps::this_thread::sleep_for(5ms);
    });
    auto ret7 = f7.then([&i](auto f) {
        i = f.get();
        return "7"+std::to_string(i);
    }).get();
    if (t7.joinable())
        t7.join();
    XCTAssertEqual(i, 42);
    XCTAssertEqual(ret7, "742");
    
    e = nullptr;
    i = 0;
    ps::promise<int> p8;
    auto f8 = p8.get_future().share();
    auto t8 = ps::thread([p = std::move(p8)]() mutable {
        p.set_value_at_thread_exit(42);
        ps::this_thread::sleep_for(10ms);
    });
    auto ret8 = f8.then([&i](auto f) {
        i = f.get();
        ps::promise<std::string> prom;
        auto fut = prom.get_future();
        auto t = ps::thread([i, p = std::move(prom)]() mutable {
            p.set_value_at_thread_exit("toto"+std::to_string(i));
            ps::this_thread::sleep_for(5ms);
        });
        t.detach();
        return fut;
    }).get();
    if (t8.joinable())
        t8.join();
    XCTAssertEqual(i, 42);
    XCTAssertEqual(ret8, "toto42");
    
    e = nullptr;
    i = 0;
    ps::promise<int> p9;
    auto f9 = p9.get_future().share();
    auto t9 = ps::thread([p = std::move(p9)]() mutable {
        p.set_value_at_thread_exit(42);
        ps::this_thread::sleep_for(10ms);
    });
    try {
        auto ret9 = f9.then([&i](auto f) {
            i = f.get();
            ps::promise<std::string> prom;
            auto fut = prom.get_future();
            auto t = ps::thread([p = std::move(prom)]() mutable {
                p.set_exception_at_thread_exit(std::make_exception_ptr(std::logic_error("logic_error9")));
                ps::this_thread::sleep_for(5ms);
            });
            t.detach();
            return fut;
        }).get();
    } catch(...) {
        e = std::current_exception();
    }
    if (t9.joinable())
        t9.join();
    XCTAssertEqual(i, 42);
    XCTAssertNotEqual(e, nullptr);
    
    e = nullptr;
    i = 0;
    ps::promise<int> p10;
    auto f10 = p10.get_future().share();
    auto t10 = ps::thread([p = std::move(p10)]() mutable {
        p.set_value_at_thread_exit(42);
        ps::this_thread::sleep_for(10ms);
    });
    try {
        auto ret10 = f10.then([&i](auto f) {
            i = f.get();
            ps::promise<std::string> prom;
            auto fut = prom.get_future();
            auto t = ps::thread([p = std::move(prom)]() mutable {
                try {
                    p.set_exception_at_thread_exit(std::make_exception_ptr(std::logic_error("logic_error10-1")));
                } catch (...) {
                }
                ps::this_thread::sleep_for(5ms);
            });
            t.detach();
            throw std::logic_error("logic_error10-2");
            return fut;
        }).get();
    } catch(...) {
        e = std::current_exception();
    }
    if (t10.joinable())
        t10.join();
    XCTAssertEqual(i, 42);
    XCTAssertNotEqual(e, nullptr);
}

- (void)testThenThenT {
    using namespace std::chrono_literals;
    
    ps::promise<int> p1;
    auto f1 = p1.get_future().share();
    auto t1 = ps::thread([p = std::move(p1)]() mutable {
        ps::this_thread::sleep_for(5ms);
        p.set_value(42);
    });
    int i = 0, j = 0;
    auto res1 = f1.then([&i](auto f) {
        i = f.get();
        return 6+i;
    }).then([&j](auto f) {
        j = f.get();
        return 7+j;
    }).get();
    if (t1.joinable())
        t1.join();
    XCTAssertEqual(i, 42);
    XCTAssertEqual(j, 6+42);
    XCTAssertEqual(res1, 7+6+42);
    
    i = 0;
    j =0;
    ps::promise<int> p2;
    auto f2 = p2.get_future().share();
    p2.set_value(42);
    auto res2 = f2.then([&i](auto f) {
        i = f.get();
        return 6+i;
    }).then([&j](auto f) {
        j = f.get();
        return 7+j;
    }).get();
    XCTAssertEqual(i, 42);
    XCTAssertEqual(j, 6+42);
    XCTAssertEqual(res2, 7+6+42);
    
    std::exception_ptr e = nullptr;
    i = 0;
    j =0;
    ps::promise<int> p5;
    auto f5 = p5.get_future().share();
    p5.set_value(42);
    try {
        f5.then([&i](auto f) {
            i = f.get();
            throw std::logic_error("logic_error5");
            return 6+i;
        }).then([&j](auto f) {
            j = f.get();
            return 7+j;
        }).get();
    } catch(...) {
        e = std::current_exception();
    }
    XCTAssertEqual(i, 42);
    XCTAssertEqual(j, 0);
    XCTAssertNotEqual(e, nullptr);
    
    e = nullptr;
    i = 0;
    j =0;
    ps::promise<int> p6;
    auto f6 = p6.get_future().share();
    auto t6 = ps::thread([p = std::move(p6)]() mutable {
        ps::this_thread::sleep_for(5ms);
        p.set_value(42);
    });
    try {
        f6.then([&i](auto f) {
            i = f.get();
            throw std::logic_error("logic_error6");
            return 6+i;
        }).then([&j](auto f) {
            j = f.get();
            return 7+j;
        }).get();
    } catch(...) {
        e = std::current_exception();
    }
    if (t6.joinable())
        t6.join();
    XCTAssertEqual(i, 42);
    XCTAssertEqual(j, 0);
    XCTAssertNotEqual(e, nullptr);
    
    e = nullptr;
    i = 0;
    j =0;
    ps::promise<int> p7;
    auto f7 = p7.get_future().share();
    p7.set_value(42);
    try {
        f7.then([&i](auto f) {
            i = f.get();
            return 6+i;
        }).then([&j](auto f) {
            j = f.get();
            throw std::logic_error("logic_error7");
            return 7+j;
        }).get();
    } catch(...) {
        e = std::current_exception();
    }
    XCTAssertEqual(i, 42);
    XCTAssertEqual(j, 6+42);
    XCTAssertNotEqual(e, nullptr);
    
    e = nullptr;
    i = 0;
    j =0;
    ps::promise<int> p8;
    auto f8 = p8.get_future().share();
    auto t8 = ps::thread([p = std::move(p8)]() mutable {
        ps::this_thread::sleep_for(5ms);
        p.set_value(42);
    });
    try {
        f8.then([&i](auto f) {
            i = f.get();
            return 6+i;
        }).then([&j](auto f) {
            j = f.get();
            throw std::logic_error("logic_error8");
            return 7+j;
        }).get();
    } catch(...) {
        e = std::current_exception();
    }
    if (t8.joinable())
        t8.join();
    XCTAssertEqual(i, 42);
    XCTAssertEqual(j, 6+42);
    XCTAssertNotEqual(e, nullptr);
    
    e = nullptr;
    i = 0;
    std::string k;
    ps::promise<int> p9;
    auto f9 = p9.get_future().share();
    auto t9 = ps::thread([p = std::move(p9)]() mutable {
        ps::this_thread::sleep_for(10ms);
        p.set_value(42);
    });
    auto ret9 = f9.then([&i](auto f) {
        i = f.get();
        ps::promise<std::string> prom;
        auto fut = prom.get_future();
        auto t = ps::thread([i, p = std::move(prom)]() mutable {
            ps::this_thread::sleep_for(5ms);
            p.set_value("toto"+std::to_string(i));
        });
        t.detach();
        return fut;
    }).then([&k](auto f) {
        k = f.get();
        return "7"+k;
    }).get();
    if (t9.joinable())
        t9.join();
    XCTAssertEqual(i, 42);
    XCTAssertEqual(k, "toto42");
    XCTAssertEqual(ret9, "7toto42");
    
    e = nullptr;
    i = 0;
    k = "";
    ps::promise<int> p10;
    auto f10 = p10.get_future().share();
    auto t10 = ps::thread([p = std::move(p10)]() mutable {
        ps::this_thread::sleep_for(10ms);
        p.set_value(42);
    });
    try {
        auto ret10 = f10.then([&i](auto f) {
            i = f.get();
            ps::promise<std::string> prom;
            auto fut = prom.get_future();
            auto t = ps::thread([i, p = std::move(prom)]() mutable {
                ps::this_thread::sleep_for(5ms);
                p.set_value("toto"+std::to_string(i));
            });
            t.detach();
            return fut;
        }).then([&k](auto f) {
            k = f.get();
            throw std::logic_error("logic_error8");
            return "7"+k;
        }).get();
    } catch (...) {
        e = std::current_exception();
    }
    
    if (t10.joinable())
        t10.join();
    XCTAssertEqual(i, 42);
    XCTAssertEqual(k, "toto42");
    XCTAssertNotEqual(e, nullptr);
}

- (void)testThenVoid {
    using namespace std::chrono_literals;
    
    ps::promise<void> p1;
    auto f1 = p1.get_future().share();
    auto t1 = ps::thread([p = std::move(p1)]() mutable {
        ps::this_thread::sleep_for(5ms);
        p.set_value();
    });
    int i = 0;
    f1.then([&i](auto f) {
        f.get();
        i = 7;
    }).get();
    if (t1.joinable())
        t1.join();
    XCTAssertEqual(i, 7);
    
    i = 0;
    ps::promise<int> p2;
    auto f2 = p2.get_future().share();
    p2.set_value(42);
    f2.then([&i](auto f) {
        i = f.get() + 7;
    }).get();
    XCTAssertEqual(i, 7+42);
    
    std::exception_ptr e = nullptr;
    i = 0;
    ps::promise<void> p3;
    auto f3 = p3.get_future().share();
    p3.set_exception(std::make_exception_ptr(std::logic_error("logic_error3")));
    try {
        f3.then([&i](auto f) {
            f.get();
            i = 7;
        }).get();
    } catch(...) {
        e = std::current_exception();
    }
    XCTAssertEqual(i, 0);
    XCTAssertNotEqual(e, nullptr);
    
    e = nullptr;
    i = 0;
    ps::promise<int> p4;
    auto f4 = p4.get_future().share();
    auto t4 = ps::thread([p = std::move(p4)]() mutable {
        ps::this_thread::sleep_for(5ms);
        p.set_exception(std::make_exception_ptr(std::logic_error("logic_error4")));
    });
    try {
        f4.then([&i](auto f) {
            i = 7 + f.get();
        }).get();
    } catch(...) {
        e = std::current_exception();
    }
    if (t4.joinable())
        t4.join();
    XCTAssertEqual(i, 0);
    XCTAssertNotEqual(e, nullptr);
    
    e = nullptr;
    i = 0;
    ps::promise<void> p5;
    auto f5 = p5.get_future().share();
    p5.set_value();
    try {
        f5.then([&i](auto f) {
            f.get();
            i = 7;
            throw std::logic_error("logic_error5");
        }).get();
    } catch(...) {
        e = std::current_exception();
    }
    XCTAssertEqual(i, 7);
    XCTAssertNotEqual(e, nullptr);
    
    e = nullptr;
    i = 0;
    ps::promise<int> p6;
    auto f6 = p6.get_future().share();
    auto t6 = ps::thread([p = std::move(p6)]() mutable {
        ps::this_thread::sleep_for(5ms);
        p.set_value(42);
    });
    try {
        f6.then([&i](auto f) {
            i = f.get();
            throw std::logic_error("logic_error6");
        }).get();
    } catch(...) {
        e = std::current_exception();
    }
    if (t6.joinable())
        t6.join();
    XCTAssertEqual(i, 42);
    XCTAssertNotEqual(e, nullptr);
    
    e = nullptr;
    i = 0;
    ps::promise<void> p7;
    auto f7 = p7.get_future().share();
    auto t7 = ps::thread([p = std::move(p7)]() mutable {
        ps::this_thread::sleep_for(5ms);
        p.set_value();
    });
    auto ret7 = f7.then([&i](auto f) {
        f.get();
        i = 42;
        return "7"+std::to_string(i);
    }).get();
    if (t7.joinable())
        t7.join();
    XCTAssertEqual(i, 42);
    XCTAssertEqual(ret7, "742");
    
    e = nullptr;
    i = 0;
    ps::promise<int> p8;
    auto f8 = p8.get_future().share();
    auto t8 = ps::thread([p = std::move(p8)]() mutable {
        ps::this_thread::sleep_for(10ms);
        p.set_value(42);
    });
    f8.then([&i](auto f) {
        i = f.get();
        ps::promise<void> prom;
        auto fut = prom.get_future();
        auto t = ps::thread([p = std::move(prom)]() mutable {
            ps::this_thread::sleep_for(5ms);
            p.set_value();
        });
        t.detach();
        return fut;
    }).get();
    if (t8.joinable())
        t8.join();
    XCTAssertEqual(i, 42);
    
    e = nullptr;
    i = 0;
    ps::promise<int> p9;
    auto f9 = p9.get_future().share();
    auto t9 = ps::thread([p = std::move(p9)]() mutable {
        ps::this_thread::sleep_for(10ms);
        p.set_value(42);
    });
    try {
        f9.then([&i](auto f) {
            i = f.get();
            ps::promise<void> prom;
            auto fut = prom.get_future();
            auto t = ps::thread([p = std::move(prom)]() mutable {
                ps::this_thread::sleep_for(5ms);
                p.set_exception(std::make_exception_ptr(std::logic_error("logic_error9")));
            });
            t.detach();
            return fut;
        }).get();
    } catch(...) {
        e = std::current_exception();
    }
    if (t9.joinable())
        t9.join();
    XCTAssertEqual(i, 42);
    XCTAssertNotEqual(e, nullptr);
    
    e = nullptr;
    i = 0;
    ps::promise<int> p10;
    auto f10 = p10.get_future().share();
    auto t10 = ps::thread([p = std::move(p10)]() mutable {
        ps::this_thread::sleep_for(10ms);
        p.set_value(42);
    });
    try {
        f10.then([&i](auto f) {
            i = f.get();
            ps::promise<void> prom;
            auto fut = prom.get_future();
            auto t = ps::thread([p = std::move(prom)]() mutable {
                ps::this_thread::sleep_for(5ms);
                try {
                    p.set_exception(std::make_exception_ptr(std::logic_error("logic_error10-1")));
                } catch (...) {
                }
            });
            t.detach();
            throw std::logic_error("logic_error10-2");
            return fut;
        }).get();
    } catch(...) {
        e = std::current_exception();
    }
    if (t10.joinable())
        t10.join();
    XCTAssertEqual(i, 42);
    XCTAssertNotEqual(e, nullptr);
}

- (void)testThenThenVoid {
    using namespace std::chrono_literals;
    
    ps::promise<void> p1;
    auto f1 = p1.get_future().share();
    auto t1 = ps::thread([p = std::move(p1)]() mutable {
        ps::this_thread::sleep_for(5ms);
        p.set_value();
    });
    int i = 0, j = 0;
    f1.then([&i](auto f) {
        i = 7;
        return 6+i;
    }).then([&j](auto f) {
        j = f.get();
    }).get();
    if (t1.joinable())
        t1.join();
    XCTAssertEqual(i, 7);
    XCTAssertEqual(j, 6+7);
    
    i = 0;
    j =0;
    ps::promise<void> p2;
    auto f2 = p2.get_future().share();
    p2.set_value();
    f2.then([&i](auto f) {
        i = 7;
        return 6+i;
    }).then([&j](auto f) {
        j = f.get();
    }).get();
    XCTAssertEqual(i, 7);
    XCTAssertEqual(j, 6+7);
    
    std::exception_ptr e = nullptr;
    i = 0;
    j =0;
    ps::promise<void> p5;
    auto f5 = p5.get_future().share();
    p5.set_value();
    try {
        f5.then([&i](auto f) {
            i = 7;
            throw std::logic_error("logic_error5");
            return 6+i;
        }).then([&j](auto f) {
            j = f.get();
        }).get();
    } catch(...) {
        e = std::current_exception();
    }
    XCTAssertEqual(i, 7);
    XCTAssertEqual(j, 0);
    XCTAssertNotEqual(e, nullptr);
    
    e = nullptr;
    i = 0;
    j =0;
    ps::promise<void> p6;
    auto f6 = p6.get_future().share();
    auto t6 = ps::thread([p = std::move(p6)]() mutable {
        ps::this_thread::sleep_for(5ms);
        p.set_value();
    });
    try {
        f6.then([&i](auto f) {
            i = 7;
            throw std::logic_error("logic_error6");
            return 6+i;
        }).then([&j](auto f) {
            j = f.get();
        }).get();
    } catch(...) {
        e = std::current_exception();
    }
    if (t6.joinable())
        t6.join();
    XCTAssertEqual(i, 7);
    XCTAssertEqual(j, 0);
    XCTAssertNotEqual(e, nullptr);
    
    e = nullptr;
    i = 0;
    j =0;
    ps::promise<void> p7;
    auto f7 = p7.get_future().share();
    p7.set_value();
    try {
        f7.then([&i](auto f) {
            i = 7;
            return 6+i;
        }).then([&j](auto f) {
            j = f.get();
            throw std::logic_error("logic_error7");
            return 7+j;
        }).get();
    } catch(...) {
        e = std::current_exception();
    }
    XCTAssertEqual(i, 7);
    XCTAssertEqual(j, 6+7);
    XCTAssertNotEqual(e, nullptr);
    
    e = nullptr;
    i = 0;
    j =0;
    ps::promise<void> p8;
    auto f8 = p8.get_future().share();
    auto t8 = ps::thread([p = std::move(p8)]() mutable {
        ps::this_thread::sleep_for(5ms);
        p.set_value();
    });
    try {
        f8.then([&i](auto f) {
            f.get();
            i = 7;
            return 6+i;
        }).then([&j](auto f) {
            j = f.get();
            throw std::logic_error("logic_error8");
        }).get();
    } catch(...) {
        e = std::current_exception();
    }
    if (t8.joinable())
        t8.join();
    XCTAssertEqual(i, 7);
    XCTAssertEqual(j, 6+7);
    XCTAssertNotEqual(e, nullptr);
    
    e = nullptr;
    i = 0;
    std::string k;
    ps::promise<void> p9;
    auto f9 = p9.get_future().share();
    auto t9 = ps::thread([p = std::move(p9)]() mutable {
        ps::this_thread::sleep_for(10ms);
        p.set_value();
    });
    f9.then([&i](auto f) {
        i = 7;
        f.get();
        ps::promise<std::string> prom;
        auto fut = prom.get_future();
        auto t = ps::thread([i, p = std::move(prom)]() mutable {
            ps::this_thread::sleep_for(5ms);
            p.set_value("toto"+std::to_string(i));
        });
        t.detach();
        return fut;
    }).then([&k](auto f) {
        k = f.get();
    }).get();
    if (t9.joinable())
        t9.join();
    XCTAssertEqual(i, 7);
    XCTAssertEqual(k, "toto7");
    
    e = nullptr;
    i = 0;
    k = "";
    ps::promise<void> p10;
    auto f10 = p10.get_future().share();
    auto t10 = ps::thread([p = std::move(p10)]() mutable {
        ps::this_thread::sleep_for(10ms);
        p.set_value();
    });
    try {
        f10.then([&i](auto f) {
            i = 7;
            f.get();
            ps::promise<std::string> prom;
            auto fut = prom.get_future();
            auto t = ps::thread([i, p = std::move(prom)]() mutable {
                ps::this_thread::sleep_for(5ms);
                p.set_value("toto"+std::to_string(i));
            });
            t.detach();
            return fut;
        }).then([&k](auto f) {
            k = f.get();
            throw std::logic_error("logic_error8");
        }).get();
    } catch (...) {
        e = std::current_exception();
    }
    
    if (t10.joinable())
        t10.join();
    XCTAssertEqual(i, 7);
    XCTAssertEqual(k, "toto7");
    XCTAssertNotEqual(e, nullptr);
}

- (void)testThenTReference {
    using namespace std::chrono_literals;
    
    ps::promise<int&> p1;
    auto f1 = p1.get_future().share();
    int j = 52;
    auto t1 = ps::thread([&j, p = std::move(p1)]() mutable {
        ps::this_thread::sleep_for(5ms);
        p.set_value(j);
    });
    int i = 0;
    auto res1 = f1.then([&i](auto f) {
        const int& k = f.get();
        i = k;
        return 7+i;
    }).get();
    if (t1.joinable())
        t1.join();
    XCTAssertEqual(i, 52);
    XCTAssertEqual(j, 52);
    XCTAssertEqual(res1, 7+52);
    
    i = 0;
    j = 52;
    ps::promise<int&> p2;
    auto f2 = p2.get_future().share();
    p2.set_value(j);
    auto res2 = f2.then([&i](auto f) {
        const int& k = f.get();
        i = k;
        return 7+i;
    }).get();
    XCTAssertEqual(i, 52);
    XCTAssertEqual(j, 52);
    XCTAssertEqual(res2, 7+52);
    
    std::exception_ptr e = nullptr;
    i = 0;
    j = 52;
    ps::promise<int&> p3;
    auto f3 = p3.get_future().share();
    p3.set_exception(std::make_exception_ptr(std::logic_error("logic_error3")));
    try {
        f3.then([&i](auto f) {
            const int& k = f.get();
            i = k;
            return 7+i;
        }).get();
    } catch(...) {
        e = std::current_exception();
    }
    XCTAssertEqual(i, 0);
    XCTAssertNotEqual(e, nullptr);
    
    e = nullptr;
    i = 0;
    j = 52;
    ps::promise<int&> p4;
    auto f4 = p4.get_future().share();
    auto t4 = ps::thread([p = std::move(p4)]() mutable {
        ps::this_thread::sleep_for(5ms);
        p.set_exception(std::make_exception_ptr(std::logic_error("logic_error4")));
    });
    try {
        f4.then([&i](auto f) {
            const int& k = f.get();
            i = k;
            return 7+i;
        }).get();
    } catch(...) {
        e = std::current_exception();
    }
    if (t4.joinable())
        t4.join();
    XCTAssertEqual(i, 0);
    XCTAssertNotEqual(e, nullptr);
    
    e = nullptr;
    i = 0;
    j = 52;
    ps::promise<int&> p5;
    auto f5 = p5.get_future().share();
    p5.set_value(j);
    try {
        f5.then([&i](auto f) {
            const int& k = f.get();
            i = k;
            throw std::logic_error("logic_error5");
            return 7+i;
        }).get();
    } catch(...) {
        e = std::current_exception();
    }
    XCTAssertEqual(i, 52);
    XCTAssertEqual(j, 52);
    XCTAssertNotEqual(e, nullptr);
    
    e = nullptr;
    i = 0;
    j = 52;
    ps::promise<int&> p6;
    auto f6 = p6.get_future().share();
    auto t6 = ps::thread([&j, p = std::move(p6)]() mutable {
        ps::this_thread::sleep_for(5ms);
        p.set_value(j);
    });
    try {
        f6.then([&i](auto f) {
            const int& k = f.get();
            i = k;
            throw std::logic_error("logic_error6");
            return 7+i;
        }).get();
    } catch(...) {
        e = std::current_exception();
    }
    if (t6.joinable())
        t6.join();
    XCTAssertEqual(i, 52);
    XCTAssertEqual(j, 52);
    XCTAssertNotEqual(e, nullptr);
    
    e = nullptr;
    i = 0;
    j = 52;
    ps::promise<int&> p7;
    auto f7 = p7.get_future().share();
    auto t7 = ps::thread([&j, p = std::move(p7)]() mutable {
        ps::this_thread::sleep_for(5ms);
        p.set_value(j);
    });
    auto ret7 = f7.then([&i](auto f) {
        const int& k = f.get();
        i = k;
        return "7"+std::to_string(i);
    }).get();
    if (t7.joinable())
        t7.join();
    XCTAssertEqual(i, 52);
    XCTAssertEqual(j, 52);
    XCTAssertEqual(ret7, "752");
    
    e = nullptr;
    i = 0;
    j = 52;
    ps::promise<int&> p8;
    auto f8 = p8.get_future().share();
    auto t8 = ps::thread([&j, p = std::move(p8)]() mutable {
        ps::this_thread::sleep_for(10ms);
        p.set_value(j);
    });
    auto ret8 = f8.then([&i](auto f) {
        const int& k = f.get();
        i = k;
        ps::promise<std::string> prom;
        auto fut = prom.get_future();
        auto t = ps::thread([&i, p = std::move(prom)]() mutable {
            ps::this_thread::sleep_for(5ms);
            i += 7;
            p.set_value("toto"+std::to_string(i));
        });
        t.detach();
        return fut;
    }).get();
    if (t8.joinable())
        t8.join();
    XCTAssertEqual(i, 52+7);
    XCTAssertEqual(j, 52);
    XCTAssertEqual(ret8, "toto59");
    
    e = nullptr;
    i = 0;
    j = 52;
    ps::promise<int&> p9;
    auto f9 = p9.get_future().share();
    auto t9 = ps::thread([&j, p = std::move(p9)]() mutable {
        ps::this_thread::sleep_for(10ms);
        p.set_value(j);
    });
    try {
        auto ret9 = f9.then([&i](auto f) {
            const int& k = f.get();
            i = k;
            ps::promise<std::string> prom;
            auto fut = prom.get_future();
            auto t = ps::thread([p = std::move(prom)]() mutable {
                ps::this_thread::sleep_for(5ms);
                p.set_exception(std::make_exception_ptr(std::logic_error("logic_error9")));
            });
            t.detach();
            return fut;
        }).get();
    } catch(...) {
        e = std::current_exception();
    }
    if (t9.joinable())
        t9.join();
    XCTAssertEqual(i, 52);
    XCTAssertEqual(j, 52);
    XCTAssertNotEqual(e, nullptr);
    
    e = nullptr;
    i = 0;
    j = 52;
    ps::promise<int&> p10;
    auto f10 = p10.get_future().share();
    auto t10 = ps::thread([&j, p = std::move(p10)]() mutable {
        ps::this_thread::sleep_for(10ms);
        p.set_value(j);
    });
    try {
        auto ret10 = f10.then([&i](auto f) {
            const int& k = f.get();
            i = k;
            ps::promise<std::string> prom;
            auto fut = prom.get_future();
            auto t = ps::thread([p = std::move(prom)]() mutable {
                ps::this_thread::sleep_for(5ms);
                try {
                    p.set_exception(std::make_exception_ptr(std::logic_error("logic_error10-1")));
                } catch (...) {
                }
            });
            t.detach();
            throw std::logic_error("logic_error10-2");
            return fut;
        }).get();
    } catch(...) {
        e = std::current_exception();
    }
    if (t10.joinable())
        t10.join();
    XCTAssertEqual(i, 52);
    XCTAssertEqual(j, 52);
    XCTAssertNotEqual(e, nullptr);
}

- (void)testWhenAllT {
    using namespace std::chrono_literals;
    
    std::vector<ps::shared_future<int>> vec1;
    vec1.reserve(2);
    vec1.emplace_back(ps::async([]() {
        ps::this_thread::sleep_for(10ms);
        return 4;
    }));
    vec1.emplace_back(ps::async([]() {
        ps::this_thread::sleep_for(5ms);
        return 2;
    }));
    auto fut1 = ps::when_all(vec1.begin(), vec1.end()).share();
    auto res1 = fut1.get();
    XCTAssertEqual(res1[0].get(), 4);
    XCTAssertEqual(res1[1].get(), 2);
    
    auto fut2 = ps::when_all(ps::async([]() {
        ps::this_thread::sleep_for(10ms);
        return 4;
    }).share(),
                             ps::async([]() {
        ps::this_thread::sleep_for(5ms);
        return "2";
    }).share(),
                             ps::async([]() {
        ps::this_thread::sleep_for(1ms);
        return 8.8f;
    }).share()).share();
    auto res2 = fut2.get();
    XCTAssertEqual(std::get<0>(res2).get(), 4);
    XCTAssertEqual(std::get<1>(res2).get(), std::string("2"));
    XCTAssertEqualWithAccuracy(std::get<2>(res2).get(), 8.8f, std::numeric_limits<float>::epsilon());
    
    std::exception_ptr e = nullptr;
    auto fut3 = ps::when_all(ps::async([]() {
        ps::this_thread::sleep_for(10ms);
        return 4;
    }).share(),
                             ps::async([]() {
        ps::this_thread::sleep_for(5ms);
        throw std::logic_error("logic_error3");
        return 2;
    }).share(),
                             ps::async([]() {
        ps::this_thread::sleep_for(1ms);
        return 8;
    }).share()).share();
    try {
        auto res3 = fut3.get();
    } catch(...) {
        e = std::current_exception();
    }
    XCTAssertNotEqual(e, nullptr);
    
    e = nullptr;
    auto fut4 = ps::when_all(ps::async([]() {
        ps::this_thread::sleep_for(10ms);
        return 4;
    }).share(),
                             ps::async([]() {
        ps::this_thread::sleep_for(5ms);
        return ps::async([]() {
            ps::this_thread::sleep_for(5ms);
            throw std::logic_error("logic_error4");
            return "2";
        });
    }).share(),
                             ps::async([]() {
        ps::this_thread::sleep_for(1ms);
        return 8.8f;
    }).share()).share();
    try {
        auto res4 = fut4.get();
    } catch(...) {
        e = std::current_exception();
    }
    XCTAssertNotEqual(e, nullptr);
    
    e = nullptr;
    std::vector<ps::shared_future<int>> vec5;
    vec5.reserve(2);
    vec5.emplace_back(ps::async([]() {
        ps::this_thread::sleep_for(10ms);
        return 4;
    }).share());
    vec5.emplace_back(ps::async([]() {
        ps::this_thread::sleep_for(5ms);
        return ps::async([]() {
            ps::this_thread::sleep_for(5ms);
            throw std::logic_error("logic_error4");
            return 2;
        });
    }).share());
    auto fut5 = ps::when_all(vec5.begin(), vec5.end()).share();
    try {
        auto res5 = fut5.get();
    } catch(...) {
        e = std::current_exception();
    }
    XCTAssertNotEqual(e, nullptr);
}

- (void)testWhenAllVoid {
    using namespace std::chrono_literals;
    
    int i = 0, j = 0;
    std::vector<ps::shared_future<void>> vec1;
    vec1.reserve(2);
    vec1.emplace_back(ps::async([&i]() {
        ps::this_thread::sleep_for(10ms);
        i = 4;
    }).share());
    vec1.emplace_back(ps::async([&j]() {
        ps::this_thread::sleep_for(5ms);
        j = 2;
    }).share());
    auto fut1 = ps::when_all(vec1.begin(), vec1.end()).share();
    auto res1 = fut1.get();
    XCTAssertEqual(i, 4);
    XCTAssertEqual(j, 2);
    
    i = 0;
    std::string k = "";
    float l = 0.f;
    auto fut2 = ps::when_all(ps::async([&i]() {
        ps::this_thread::sleep_for(10ms);
        i = 4;
    }).share(),
                             ps::async([&k]() {
        ps::this_thread::sleep_for(5ms);
        k = "2";
    }).share(),
                             ps::async([&l]() {
        ps::this_thread::sleep_for(1ms);
        l = 8.8f;
    }).share()).share();
    auto res2 = fut2.get();
    XCTAssertEqual(i, 4);
    XCTAssertEqual(k, std::string("2"));
    XCTAssertEqualWithAccuracy(l, 8.8f, std::numeric_limits<float>::epsilon());
    
    i = 0;
    j = 0;
    int m = 0;
    std::exception_ptr e = nullptr;
    auto fut3 = ps::when_all(ps::async([&i]() {
        ps::this_thread::sleep_for(10ms);
        i = 4;
    }).share(),
                             ps::async([]() {
        ps::this_thread::sleep_for(5ms);
        throw std::logic_error("logic_error3");
    }).share(),
                             ps::async([&m]() {
        ps::this_thread::sleep_for(1ms);
        m = 8;
    }).share()).share();
    try {
        auto res3 = fut3.get();
    } catch(...) {
        e = std::current_exception();
    }
    XCTAssertNotEqual(e, nullptr);
    XCTAssertEqual(i, 4);
    XCTAssertEqual(j, 0);
    XCTAssertEqual(m, 8);
    
    e = nullptr;
    i = 0;
    k = "";
    l = 0.f;
    auto fut4 = ps::when_all(ps::async([&i]() {
        ps::this_thread::sleep_for(10ms);
        i = 4;
    }).share(),
                             ps::async([]() {
        ps::this_thread::sleep_for(5ms);
        return ps::async([]() {
            ps::this_thread::sleep_for(5ms);
            throw std::logic_error("logic_error4");
        });
    }).share(),
                             ps::async([&l]() {
        ps::this_thread::sleep_for(1ms);
        l = 8.8f;
    }).share()).share();
    try {
        auto res4 = fut4.get();
    } catch(...) {
        e = std::current_exception();
    }
    XCTAssertNotEqual(e, nullptr);
    XCTAssertEqual(i, 4);
    XCTAssertEqual(k, std::string(""));
    XCTAssertEqualWithAccuracy(l, 8.8f, std::numeric_limits<float>::epsilon());
    
    e = nullptr;
    i = 0;
    j = 0;
    std::vector<ps::shared_future<void>> vec5;
    vec5.reserve(2);
    vec5.emplace_back(ps::async([&i]() {
        ps::this_thread::sleep_for(10ms);
        i = 4;
    }).share());
    vec5.emplace_back(ps::async([]() {
        ps::this_thread::sleep_for(5ms);
        return ps::async([]() {
            ps::this_thread::sleep_for(5ms);
            throw std::logic_error("logic_error4");
        });
    }).share());
    auto fut5 = ps::when_all(vec5.begin(), vec5.end()).share();
    try {
        auto res5 = fut5.get();
    } catch(...) {
        e = std::current_exception();
    }
    XCTAssertNotEqual(e, nullptr);
    XCTAssertEqual(i, 4);
    XCTAssertEqual(j, 0);
}

@end
