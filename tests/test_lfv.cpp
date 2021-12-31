#include "Testing.h"
#include "LazyFunctionalVector.h"
#include <vector>


void test_type_presentation() {
    std::vector<int> t1({1,19,4});
    auto vt1 = lazy(t1);
    decltype(vt1)::value_type x;
    VALUE( typeid(x) == typeid(int) ) EXPECTED( true );
    VALUE( typeid(x) == typeid(float) ) EXPECTED( false );
    auto ft1 = vt1.filter(F(_>0));
    decltype(ft1)::value_type y;
    VALUE( typeid(y) == typeid(int) ) EXPECTED( true );
    auto mt1 = vt1.map(F(_+2));
    decltype(mt1)::value_type z1;
    VALUE( typeid(z1) == typeid(int) ) EXPECTED( true );

    auto mt2 = vt1.map(F( _ * 0.1 ));
    decltype(mt2)::value_type z2;
    VALUE( typeid(z2) == typeid(double) ) EXPECTED( true );
}

void test_count() {
    std::vector<int> t1({1,19,4});
    auto vt1 = lazy(t1);
    VALUE( vt1.size() ) EXPECTED (3 );
    const size_t count_below_5 = vt1.count( F(_<5));
    VALUE( count_below_5 ) EXPECTED (2);
    const size_t count_below_50 = vt1.count( F(_<50));
    VALUE( count_below_50 ) EXPECTED (3);
    const size_t count_never = vt1.count( F(false));
    VALUE( count_never ) EXPECTED (0);

    VALUE( vt1.empty() ) EXPECTED ( false );
}

void test_filter() {
    std::vector<int> t1({1,19,4, 2, 5, -1, 5});
    auto vt1 = lazy(t1);
    auto ft1 = vt1.filter( F( _ > 2));
    VALUE( ft1.size() ) EXPECTED( 4 );
    auto ft2 = ft1.filter( F( _ >= 5));
    VALUE( ft2.size() ) EXPECTED( 3 );
    std::vector<int> r;
    ft2.push_back_to(r);
    VALUE( int(r.at(0)) ) EXPECTED( 19 );
    size_t count_of_5 = ft2.count( F(_ ==5 ));
    VALUE( count_of_5 ) EXPECTED( 2 );

    VALUE( ft1.empty() ) EXPECTED ( false );
    auto ft3 = ft2.filter( F(false)); // impossible filter
    VALUE( ft3.size() ) EXPECTED ( 0 );
    VALUE( ft3.empty() ) EXPECTED ( true );



}
void test_map() {
    std::vector<int> t1({1,19,4, 2, 5, -1, 5});
    auto vt1 = lazy(t1);
    {
        auto mt1 = vt1.map( F(_+2) );
        std::vector<int> r;
        mt1.push_back_to(r);
        VALUE( int(r[0])) EXPECTED ( 3 );
        VALUE( int(r[1])) EXPECTED ( 21 );
    }
    {
        auto mt1 = vt1.map( F(_*0.2) );
        std::vector<double> r;
        mt1.push_back_to(r);
        VALUE( double(r[0])) EXPECTED ( 0.2 );
        VALUE( double(r[2])) EXPECTED ( 0.8 );
    }

//    VALUE( mt1.element_at(0)) EXPECTED ( 33 );
}

void test_filter_map() {
    std::vector<int> t1({1,19,4, 2, 5, -1, 5});
    auto vt1 = lazy(t1);
    {
        auto mt1 = vt1.map( F(_+2) ).filter(F(_>4));
        VALUE(mt1.size()) EXPECTED(4);
        std::vector<int> r;
        mt1.push_back_to(r);
        VALUE( int(r[0])) EXPECTED ( 21 );
        VALUE( int(r[1])) EXPECTED ( 6 );
    }
    // filter after type change
    {
        auto mt1 = vt1.map( F(_*0.2) ).filter(F(_>=1.0));
        VALUE(mt1.size()) EXPECTED(3);
        std::vector<double> r;
        mt1.push_back_to(r);
        VALUE( double(r[0])) EXPECTED ( 3.8 );
        VALUE( double(r[1])) EXPECTED ( 1.0 );
    }
    // filter and then map
    {
        auto mt1 = vt1.filter(F(_>=2)).map( F(_*0.2) );
        VALUE(mt1.size()) EXPECTED(5);
        std::vector<double> r;
        mt1.push_back_to(r);
        VALUE( double(r[0])) EXPECTED ( 3.8 );
        VALUE( double(r[1])) EXPECTED ( 0.8 );
        VALUE( double(r[2])) EXPECTED ( 0.4 );
    }
}

void test_staging() {
    std::vector<int> t1({1,19,4, 2, 5, -1, 5});
    auto vt1 = lazy(t1);
    auto mt1 = vt1.map( F(_+2) ).filter(F(_<4)).filter(F(_==1)).map(F(_*_*0.1)).stage();
    // one element (initially -1) should survive
    VALUE( mt1.size() ) EXPECTED ( 1 );
    OwningView<double> r;
    VALUE( typeid(mt1) == typeid(r)) EXPECTED ( true );
    VALUE( mt1.element_at(0) ) EXPECTED ( 0.1 );
}


void test_take() {
    std::vector<int> t1({1,19,4, 2, 5, -1, 5});
    auto vt1 = lazy(t1);
    auto tt1 = vt1.take(3);
    VALUE(tt1.size()) EXPECTED( 3 );
    VALUE(tt1.element_at(0)) EXPECTED(1);
    VALUE(tt1.element_at(2)) EXPECTED(4);
    // take element with stride
    auto tt1s = vt1.take(lfv::all_elements, 2);
    VALUE(tt1s.size()) EXPECTED( 4 );
    VALUE (tt1s.element_at(1)) EXPECTED( 4);
    VALUE (tt1s.element_at(3)) EXPECTED( 5);


    auto tt2 = vt1.take_while(F(_>0));
    VALUE(tt2.size()) EXPECTED( 5 );
    VALUE(tt2.element_at(4)) EXPECTED(5);

    auto tt3 = vt1.skip(3);
    VALUE( tt3.size()) EXPECTED( 4 );
    VALUE( tt3.element_at(0)) EXPECTED (2);

    auto tt4 = vt1.skip_while(F(_>0));
    VALUE( tt4.size()) EXPECTED( 2 );
    VALUE( tt4.element_at(0)) EXPECTED (-1);

    // combine
    auto tt5 = vt1.take(5).skip(3);
    VALUE( tt5.size()) EXPECTED( 2 );
    VALUE( tt5.element_at(0)) EXPECTED (2);
    VALUE( tt5.element_at(1)) EXPECTED (5);

    auto tt6 = vt1.take_while(F(_!=5)).skip_while(F( _!= 4));
    VALUE( tt6.size()) EXPECTED( 2 );
    VALUE( tt6.element_at(0)) EXPECTED (4);
    VALUE( tt6.element_at(1)) EXPECTED (2);
}


void test_sum_and_accumulate() {
    std::vector<int> t1({1,19,4, 2, 5, -1, 5});
    auto vt1 = lazy(t1);
    VALUE( vt1.sum() ) EXPECTED (35);
    VALUE( vt1.take(3).sum()) EXPECTED (24);
    const int multiply_el = vt1.take(4).skip(2).accumulate( [](int t, int el){return t*el;} , 1);
    VALUE( multiply_el ) EXPECTED ( 8 );
}

void test_chain() {
    std::vector<int> t1({1,19,4, 2});
    std::vector<int> t2({5, -1, 5});
    auto vt1 = lazy(t1);
    auto vt2 = lazy(t2);
    auto jt =  vt1.chain(vt2);
    VALUE( jt.size() ) EXPECTED ( 7 );
    VALUE( jt.element_at(0)) EXPECTED ( 1 );
    VALUE( jt.element_at(4)) EXPECTED ( 5 );
    VALUE( jt.element_at(5)) EXPECTED ( -1 );

    auto ajt = vt1.skip(1).chain( vt2.filter(F(_<0)));
    VALUE(ajt.size()) EXPECTED( 4 );
    VALUE(ajt.element_at(0)) EXPECTED (19);
    VALUE( ajt.element_at(3)) EXPECTED(-1);
}


void test_sort() {
    std::vector<int> t1({1,19,4, 2, 5, -1, 5});
    auto vt1 = lazy(t1);
    auto st1 = vt1.sorted(F(_));
    VALUE(st1.size()) EXPECTED( t1.size());
    VALUE(st1.element_at(0)) EXPECTED( -1);
    VALUE(st1.element_at(1)) EXPECTED( 1);
    VALUE(st1.element_at(6)) EXPECTED( 19);

    auto rst1 = vt1.sorted(F(-_)); // reverse sort
    VALUE(rst1.size()) EXPECTED( t1.size());
    VALUE(rst1.element_at(0)) EXPECTED( 19);
}

void test_enumerate() {
    // test first the indexed struct helper
    lfv::indexed<int> a;
    VALUE( a.index()) EXPECTED( lfv::invalid_index);
    int i = 77;
    lfv::indexed<int> b(2, i);
    VALUE( b.index()) EXPECTED( 2 );
    VALUE( b.data()) EXPECTED( 77 );
    a = b;
    VALUE( a.index()) EXPECTED( b.index() );
    VALUE( a.data()) EXPECTED( b.data() );
    lfv::indexed<int> c(a);
    VALUE( c.index()) EXPECTED( b.index() );
    VALUE( c.data()) EXPECTED( b.data() );
    int j = 100;
    lfv::indexed<int> d(12, j);
    a = d;
    VALUE( a.index()) EXPECTED( 12 );
    VALUE( a.data()) EXPECTED( 100 );


    std::vector<int> t1({1,19,4, 2, 5, -1, 5});
    auto vt1 = lazy(t1);
    auto en1 = vt1.enumerate();
    en1.foreach( S( std::cout << _.index()  << ":" << _.data() << "\n" ));

    // }
    VALUE( en1.element_at(0).index()) EXPECTED(0);
    VALUE( en1.element_at(0).data()) EXPECTED(1);
    VALUE( en1.element_at(1).index()) EXPECTED(1);
    VALUE( en1.element_at(1).data()) EXPECTED(19);


    // auto sen1 = en1.sorted(F(_.data()));
    // VALUE( sen1.element_at(0).data()) EXPECTED(-1);
    // VALUE( sen1.element_at(0).index()) EXPECTED(6);


}



int main() {
    const int failed =
      + SUITE(test_type_presentation)
      + SUITE(test_count)
      + SUITE(test_filter)
      + SUITE(test_map)
      + SUITE(test_filter_map)
      + SUITE(test_staging)
      + SUITE(test_take)
      + SUITE(test_sum_and_accumulate)
      + SUITE(test_chain)
      + SUITE(test_sort)
      + SUITE(test_enumerate);

    std::cout << ( failed == 0  ? "ALL OK" : "FAILURE" ) << std::endl;
    return failed;
}