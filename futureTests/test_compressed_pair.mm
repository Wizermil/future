//
// test_compressed_pair.mm
// futureTests
//
// Created by Mathieu Garaud on 19/09/2017.
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
#include <functional>

@interface test_compressed_pair : XCTestCase

@end

@implementation test_compressed_pair

- (void)testNew {
    ps::compressed_pair<int, int> c1(1, 2);
    XCTAssertEqual(c1.first(), 1);
    XCTAssertEqual(c1.second(), 2);

    const ps::compressed_pair<int, int> c2(1, 2);
    XCTAssertEqual(c2.first(), 1);
    XCTAssertEqual(c2.second(), 2);

    ps::compressed_pair<int, int> c3;
    c3.first() = 1;
    XCTAssertEqual(c3.first(), 1);
    c3.second() = 2;
    XCTAssertEqual(c3.second(), 2);

    const ps::compressed_pair<int, int> c4(1);
    XCTAssertEqual(c4.first(), 1);

    ps::compressed_pair<int, int> c5(ps::second_tag(), 2);
    XCTAssertEqual(c5.second(), 2);

    constexpr ps::compressed_pair<int, int> c6(3,4);
    XCTAssertEqual(c6.first(), 3);
    XCTAssertEqual(c6.second(), 4);

    auto c7 = new ps::compressed_pair<int, const int>(3,4);
    XCTAssertEqual(c7->first(), 3);
    XCTAssertEqual(c7->second(), 4);
    delete c7;

    ps::compressed_pair<int*, int> c8(nullptr, 4);
    XCTAssertEqual(c8.second(), 4);

    int i = 0;
    ps::compressed_pair<std::function<void(void)>, int> c9;
    c9.second() = 42;
    c9.first() = [&i]() {
        i = 42;
    };
    c9.first()();
    XCTAssertEqual(i, 42);
    XCTAssertEqual(c9.second(), 42);

    struct A
    {
        int testa()
        {
            return 1;
        }
    };
    struct B
    {
        int testb() const
        {
            return 2;
        }
    };
    ps::compressed_pair<A, const B> c10;
    XCTAssertEqual(c10.first().testa(), 1);
    XCTAssertEqual(c10.second().testb(), 2);
}

@end
