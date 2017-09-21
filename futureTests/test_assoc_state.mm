//
// test_assoc_state.mm
// futureTests
//
// Created by Mathieu Garaud on 29/08/2017.
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
#include <string>

@interface test_assoc_state : XCTestCase

@end

@implementation test_assoc_state

- (void)testNew {

    ps::assoc_state<int> a;
    a.set_value(42);
    XCTAssertEqual(a.copy(), 42);
    XCTAssertEqual(a.move(), 42);

    ps::assoc_state<int>* b = new ps::assoc_state<int>();
    b->set_value(42);
    XCTAssertEqual(b->copy(), 42);
    XCTAssertEqual(b->move(), 42);
    delete b;

    ps::assoc_state<std::string> c;
    c.set_value("toto");
    XCTAssertEqual(c.copy(), "toto");
    XCTAssertEqual(c.move(), "toto");

    ps::assoc_state<std::string>* d = new ps::assoc_state<std::string>();
    d->set_value("toto");
    XCTAssertEqual(d->copy(), "toto");
    XCTAssertEqual(d->move(), "toto");
    delete d;

    ps::assoc_state<std::string>* e = new ps::assoc_state<std::string>();
    e->set_value("totototototototototototototototototototototototototototototototo");
    XCTAssertEqual(e->copy(), "totototototototototototototototototototototototototototototototo");
    XCTAssertEqual(e->move(), "totototototototototototototototototototototototototototototototo");
    delete e;

    struct foo
    {
        int a1;
        std::string a2;
    };
    ps::assoc_state<foo> f;
    foo ff {42, "totototototototototototototototototototototototototototototototo"};
    f.set_value(std::move(ff));

    XCTAssertEqual(f.copy().a1, 42);
    XCTAssertEqual(f.copy().a2, "totototototototototototototototototototototototototototototototo");
    foo ffm = f.move();
    XCTAssertEqual(ffm.a1, 42);
    XCTAssertEqual(ffm.a2, "totototototototototototototototototototototototototototototototo");

}

@end
