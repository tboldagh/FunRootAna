// Copyright 2022, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)

#include "Testing.h"
#include "LazyFunctionalVector.h"
#include <vector>


#ifdef TEST_LAZY
#define functional_vector lazy_view
#else
#define functional_vector wrap
#endif



struct TestObject {
    int x;
    double y;
    std::string z;
};


void test_basic_transfromations() {
    std::vector<TestObject> vec = { TestObject({0, 0.2, "object 1"}), TestObject({11, 0.2, "object 2"}), TestObject({22, 0.5, "object 3"}), TestObject({33, 0.5, "object 4"}) };
    auto fto = functional_vector(vec);

    VALUE(fto.size()) EXPECTED(4);
    VALUE(fto.empty()) EXPECTED(false);

    VALUE(fto.element_at(0).value().x) EXPECTED(0);
    VALUE(fto.element_at(3).value().x) EXPECTED(33);
    const size_t s1 = fto.filter(F(_.x > 0)).map(F(_.x)).size();
    VALUE(s1) EXPECTED(3);
    const int sum = fto.filter(F(_.x > 0)).map(F(_.x)).sum();
    VALUE(sum) EXPECTED(66);
    const int m = fto.filter(F(_.x > 0)).accumulate([](double tot, const TestObject& el) { return tot * el.y; }, 1.0);
    VALUE(m) EXPECTED(0.2 * 0.5 * 0.5);

}

void test_advanced() {
    std::vector<TestObject> vec = { TestObject({0, 0.2, "object 1"}), TestObject({11, 0.2, "object 2"}), TestObject({22, 0.5, "object 3"}), TestObject({33, 0.5, "object 4"}) };
    auto fto = functional_vector(vec);

    auto dsum = fto.chain(fto).chain(fto).filter(F(_.x < 20)).map(F(_.x)).sum();
    VALUE(dsum) EXPECTED(33); // 3 x 11 + 3 x 0
    {
        auto s = fto.chain(fto).chain(fto).skip(5).element_at(0).value().x;
        VALUE(s) EXPECTED(11); // second element from the second sub-collection
    }
    {
        auto s = fto.chain(fto).chain(fto).take(5).reverse().first_of(F(_.x > 10)).value().x;
        VALUE(s) EXPECTED(33); // have coll with x: 0 33 22 11 0
    }
    {
        auto s = fto.zip(fto).map(F(_.first.x * _.second.x)).sum(); // 0x0 11x11 22x22 33x33
        auto ssquares = fto.map(F(_.x)).map(F(_ * _)).sum();
        VALUE(s) EXPECTED(ssquares);
    }
    {
        auto s = fto.zip(fto.reverse()).map(F(_.first.x * _.second.x)).sum(); // 0x33 11x22 22x11 33x00
        VALUE(s) EXPECTED( 22*11*2 );
    }
}


int main() {
    const int failed =
        +SUITE(test_basic_transfromations)
        + SUITE(test_advanced);
    std::cout << (failed == 0 ? "ALL OK" : "FAILURE") << std::endl;
    return failed;
}