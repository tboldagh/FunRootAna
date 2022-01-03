// Copyright 2022, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)

#include "Testing.h"
#include "EagerFunctionalVector.h"
#include <vector>
struct A {
    double x;
    int y;
    double getx() const { return x; }
};

void test_access() {
    std::vector<int> t1({1,19,4});
    auto vt1 = wrap(t1);
    VALUE( vt1[0] ) EXPECTED ( 1 );
    VALUE( vt1[1] ) EXPECTED ( 19 );
    VALUE( vt1[2] ) EXPECTED ( 4 );
    VALUE( vt1.first(1).element_at(0) ) EXPECTED ( 1 );
    VALUE( vt1.first(2).element_at(1) ) EXPECTED ( 19 );

    VALUE( vt1.last(1).element_at(0) ) EXPECTED ( 4 );
    VALUE( vt1.last(2).element_at(0) ) EXPECTED ( 19 );
    VALUE( vt1.last(2).element_at(1) ) EXPECTED ( 4 );

    VALUE( vt1.range(1,3).element_at(0) ) EXPECTED ( 19 );
    VALUE( vt1.range(1,3).size()) EXPECTED ( 2 );
}

void test_reduce() {
    std::vector<int> t1({1,19,4});
    auto vt1 = wrap(t1);
    VALUE( vt1.sum() ) EXPECTED (24);
    const int sum_el_below_5 = vt1.sum(F( _< 5 ? 1 : 0; ));
    VALUE( sum_el_below_5 ) EXPECTED ( 2 );

    const int ac = vt1.reduce(0, [](int tot, int y){return tot + y; });
    VALUE( ac ) EXPECTED ( 24 );
}

void test_searching() { 
    std::vector<int> t1({1,19,4, 2, 5, -1, 5});
    auto vt1 = wrap(t1);
    VALUE(vt1.max().element_at(0)) EXPECTED ( 19 );
    VALUE(vt1.min().element_at(0)) EXPECTED ( -1 );
    const size_t count_of_5 = vt1.count(F(_ == 5));
    VALUE(count_of_5) EXPECTED ( 2 );
    const size_t count_of_non_5 = vt1.count(F(_ != 5));
    VALUE(count_of_non_5) EXPECTED ( 5 );
    const size_t count_of_3 = vt1.count(F(_ == 3));
    VALUE(count_of_3) EXPECTED ( 0 );
}

void test_map() {
    std::vector<int> t1({1,19,4, 2, 5, -1, 5});
    auto vt1 = wrap(t1);
    auto mt1 = vt1.map( F(_+2) );
    VALUE( mt1.size()) EXPECTED( vt1.size() );
    for ( int i = 0; i < vt1.size(); i++ ) {
        VALUE ( mt1.element_at(i) ) EXPECTED ( vt1.element_at(i) +2);
    }

    auto mt2 = vt1.map( F(_ < 5) );
    VALUE ( mt2.element_at(0)) EXPECTED ( true );
    VALUE ( mt2.element_at(1)) EXPECTED ( false );
}

void test_filter() {
    std::vector<int> t1({1,19,4, 2, 5, -1, 5});
    auto vt1 = wrap(t1);
    auto ft1 = vt1.filter( F( _ > 2));
    VALUE( ft1.size() ) EXPECTED ( 4 );
    VALUE( ft1.element_at(0)) EXPECTED ( 19 );
    VALUE( ft1.element_at(3)) EXPECTED ( 5 );

}

int main() {
    const int failed = 
      SUITE(test_access) 
    + SUITE(test_reduce)
    + SUITE(test_searching)
    + SUITE(test_map) 
    + SUITE(test_filter);

    std::cout << ( failed == 0  ? "ALL OK" : "FAILURE" ) << std::endl;
    return failed;
}