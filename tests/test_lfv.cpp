#include "Testing.h"
#include "LazyFunctionalVector.h"
#include <vector>


void test_type_presentation() {
    std::vector<int> t1({1,19,4});
    auto vt1 = wrap(t1);
    decltype(vt1)::value_type x;
    VALUE( typeid(x) == typeid(int) ) EXPECTED( true );
    VALUE( typeid(x) == typeid(float) ) EXPECTED( false );
    auto ft1 = vt1.filter(F(_>0));
    decltype(vt1)::value_type y;
    VALUE( typeid(y) == typeid(int) ) EXPECTED( true );

    auto mt1 = vt1.map(F(_+2));
    decltype(mt1)::value_type z1;
    VALUE( typeid(z1) == typeid(int) ) EXPECTED( true );

    auto mt2 = vt1.map(F( _ * 0.1 ));
    decltype(mt2)::value_type z2;
    VALUE( typeid(z2) == typeid(int) ) EXPECTED( true );


}

void test_count() {
    std::vector<int> t1({1,19,4});
    auto vt1 = wrap(t1);
    VALUE( vt1.size() ) EXPECTED (3 );
    const size_t count_below_5 = vt1.count( F(_<5));
    VALUE( count_below_5 ) EXPECTED (2);
    const size_t count_below_50 = vt1.count( F(_<50));
    VALUE( count_below_50 ) EXPECTED (3);
    const size_t count_never = vt1.count( F(false));
    VALUE( count_never ) EXPECTED (0);
}

void test_filter() {

    std::vector<int> t1({1,19,4, 2, 5, -1, 5});
    auto vt1 = wrap(t1);
    auto ft1 = vt1.filter( F( _ > 2));
    VALUE( ft1.size() ) EXPECTED( 4 );
    auto ft2 = ft1.filter( F( _ >= 5));
    VALUE( ft2.size() ) EXPECTED( 3 );
    std::vector<int> r;
    ft2.push_back_to(r);
    VALUE( int(r.at(0)) ) EXPECTED( 19 );
    size_t count_of_5 = ft2.count( F(_ ==5 ));
    VALUE( count_of_5 ) EXPECTED( 2 );

}

void test_map() {
    std::vector<int> t1({1,19,4, 2, 5, -1, 5});
    auto vt1 = wrap(t1);
    auto mt1 = vt1.map( F(_+2) );
//    VALUE( mt1.element_at(0)) EXPECTED ( 33 );

}

int main() {
    const int failed = 
      + SUITE(test_type_presentation)
      + SUITE(test_count)
      + SUITE(test_filter)
      + SUITE(test_map);
    // + SUITE(test_searching)
    // + SUITE(test_map) 
    // + SUITE(test_filter);

    std::cout << ( failed == 0  ? "ALL OK" : "FAILURE" ) << std::endl;
    return failed;
}