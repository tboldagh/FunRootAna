#include "Testing.h"
#include "WrappedSequenceContainer.h"
#include <vector>
struct A {
    double x;
    int y;
    double getx() const { return x; }
};

void test_access() {
    std::vector<int> t1({1,19,4});
    auto wt1 = wrap(t1);
    VALUE( wt1[0] ) EXPECTED ( 1 );
    VALUE( wt1[1] ) EXPECTED ( 19 );
    VALUE( wt1[2] ) EXPECTED ( 4 );
    VALUE( wt1.first(1).element_at(0) ) EXPECTED ( 1 );
    VALUE( wt1.first(2).element_at(1) ) EXPECTED ( 19 );

    VALUE( wt1.last(1).element_at(0) ) EXPECTED ( 4 );
    VALUE( wt1.last(2).element_at(0) ) EXPECTED ( 19 );
    VALUE( wt1.last(2).element_at(1) ) EXPECTED ( 4 );
}

void test_accumulate() {
    std::vector<int> t1({1,19,4});
    auto wt1 = wrap(t1);
}


int main() {
    const int failed = 
      SUITE(test_access) 
    + SUITE(test_accumulate);
    std::cout << ( failed == 0  ? "ALLOK" : "FAILURE" ) << std::endl;
    return failed;
}