// Copyright 2024, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)
#include <vector>
#include <set>
#include <list>
#include "Testing.h"
#include "LazyFunctionalVector.h"


using namespace lfv;
struct TestObject {
    int x;
    double y;
    std::string z;
};

void test_basic_transfromations() {
    std::vector<TestObject> vec = { TestObject({0, 0.2, "object 1"}), TestObject({11, 0.2, "object 2"}), TestObject({22, 0.5, "object 3"}), TestObject({33, 0.5, "object 4"}) };
    auto fto = lazy_view(vec);

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
    auto fto = lazy_view(vec);

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
        VALUE(s) EXPECTED(22 * 11 * 2);
    }
}

class Interface {
public:
    virtual int getX() const = 0;
    virtual int getY() const = 0;
    virtual ~Interface() {}
};

class ImplA : public Interface {
public:
    ImplA(int x, int y) {
        arr[0] = x;
        arr[1] = y;
    }
    int getX() const override { return arr[0]; }
    int getY() const override { return arr[1]; }
private:
    int arr[2];
};


class ImplB : public Interface {
public:
    ImplB(double a, double b) : x{ a }, y{ b } {}
    int getX() const override { return x; }
    int getY() const override { return y; }
private:
    double x, y;
};

void test_heterogenous_chaining() {
    std::vector<ImplA> vecA = { ImplA(1,2), ImplA(4,8), ImplA(5,25) };
    std::vector<ImplB> vecB = { ImplB(-1,1), ImplB(-4,4), ImplB(-5,5) };

    auto a = lazy_view(vecA);
    auto b = lazy_view(vecB);
    auto chained = a.chain<Interface>(b);
    const auto x0 = chained.map( F(_.getX())).element_at(0).value();
    VALUE( x0 ) EXPECTED( 1 );
    const auto x3 = chained.map( F(_.getX())).element_at(3).value();
    VALUE( x3 ) EXPECTED( -1 );
    const auto y3 = chained.map( F(_.getY())).element_at(3).value();
    VALUE( y3 ) EXPECTED( 1 );
    const auto sumx = chained.map(F(_.getX())).sum();
    VALUE( sumx ) EXPECTED( 0 );
    const auto sumx_fitst4 = chained.take(4).map(F(_.getX())).sum();
    VALUE( sumx_fitst4 ) EXPECTED( 9 );
}



void test_pointers_collection() {
    std::vector<const TestObject*> vec = { new TestObject(), new TestObject() };
    auto v = lazy_view(vec).skip(1).take(1).stage();
    auto aslist = lazy_view(vec).stage<std::list<const TestObject*>>(); 
    auto asset = lazy_view(vec).stage<std::set<const TestObject*>>(); 
    VALUE(v.size()) EXPECTED (1);
    lazy_view(vec).foreach(S(delete _));
}


int main() {
    const int failed =
        + SUITE(test_basic_transfromations)
        + SUITE(test_advanced)
        + SUITE(test_heterogenous_chaining)
        + SUITE(test_pointers_collection);
    std::cout << (failed == 0 ? "ALL OK" : "FAILURE") << std::endl;
    return failed;
}