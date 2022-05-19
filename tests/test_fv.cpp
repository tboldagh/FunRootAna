// Copyright 2022, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)

#include "Testing.h"
#include "LazyFunctionalVector.h"
#include <vector>

using namespace lfv;
#ifdef TEST_LAZY
#define functional_vector lazy_view
#else
#define functional_vector wrap
#endif

void test_type_presentation() {
    std::vector<int> t1({ 1,19,4 });
    auto vt1 = functional_vector(t1);
    decltype(vt1)::value_type x;
    VALUE(typeid(x) == typeid(int)) EXPECTED(true);
    VALUE(typeid(x) == typeid(float)) EXPECTED(false);
    auto ft1 = vt1.filter(F(_ > 0));
    decltype(ft1)::value_type y;
    VALUE(typeid(y) == typeid(int)) EXPECTED(true);
    auto mt1 = vt1.map(F(_ + 2));
    decltype(mt1)::value_type z1;
    VALUE(typeid(z1) == typeid(int)) EXPECTED(true);

    auto mt2 = vt1.map(F(_ * 0.1));
    decltype(mt2)::value_type z2;
    VALUE(typeid(z2) == typeid(double)) EXPECTED(true);

    static_assert(details::has_fast_element_access_tag<int>::value == false);
    static_assert(details::has_fast_element_access_tag<DirectView<std::vector<double>>>::value == true);
}

void test_element_access() {
    std::vector<int> t1({ 1,19,4 });
    auto vt1 = functional_vector(t1);
    VALUE( vt1.element_at(0).value()) EXPECTED(1);
    VALUE( vt1.element_at(2).value()) EXPECTED(4);
    VALUE( vt1.element_at(3).has_value()) EXPECTED(false);
}

void test_count_and_find() {
    std::vector<int> t1({ 1,19,4 });
    auto vt1 = functional_vector(t1);
    VALUE(vt1.size()) EXPECTED(3);
    const size_t count_below_5 = vt1.count(F(_ < 5));
    VALUE(count_below_5) EXPECTED(2);
    const size_t count_below_50 = vt1.count(F(_ < 50));
    VALUE(count_below_50) EXPECTED(3);
    const size_t count_never = vt1.count(F(false));
    VALUE(count_never) EXPECTED(0);

    VALUE(vt1.empty()) EXPECTED(false);

    VALUE(vt1.contains(2)) EXPECTED(false);
    VALUE(vt1.contains(4)) EXPECTED(true);
    const bool contains_mod_3 = vt1.contains(F(_ % 3 == 0));
    VALUE(contains_mod_3) EXPECTED(false);
    const bool contains_mod_2 = vt1.contains(F(_ % 2 == 0));
    VALUE(contains_mod_2) EXPECTED(true);
    const auto first_above_10 = vt1.first_of(F(_ > 10));
    VALUE(first_above_10.value()) EXPECTED(19);
    const auto contains_above_100 = vt1.first_of(F(_ > 100));
    VALUE(contains_above_100.has_value()) EXPECTED(false);

    const bool gt_10 = vt1.all(F(_ > 10));
    VALUE(gt_10) EXPECTED(false);
    const bool gt_0 = vt1.all(F(_ > 0));
    VALUE(gt_0) EXPECTED(true);



}

void test_filter() {
    std::vector<int> t1({ 1,19,4, 2, 5, -1, 5 });
    auto vt1 = functional_vector(t1);
    auto ft1 = vt1.filter(F(_ > 2));
    VALUE(ft1.size()) EXPECTED(4);
    auto ft2 = ft1.filter(F(_ >= 5));
    VALUE(ft2.size()) EXPECTED(3);
    std::vector<int> r;
    ft2.push_back_to(r);
    VALUE(int(r.at(0))) EXPECTED(19);
    size_t count_of_5 = ft2.count(F(_ == 5));
    VALUE(count_of_5) EXPECTED(2);

    VALUE(ft1.empty()) EXPECTED(false);
    auto ft3 = ft2.filter(F(false)); // impossible filter
    VALUE(ft3.size()) EXPECTED(0);
    VALUE(ft3.empty()) EXPECTED(true);
}

void test_map() {
    std::vector<int> t1({ 1,19,4, 2, 5, -1, 5 });
    auto vt1 = functional_vector(t1);
    {
        auto mt1 = vt1.map(F(_ + 2));
        std::vector<int> r;
        mt1.push_back_to(r);
        VALUE(int(r[0])) EXPECTED(3);
        VALUE(int(r[1])) EXPECTED(21);
    }
    {
        auto mt1 = vt1.map(F(_ * 0.2));
        std::vector<double> r;
        mt1.push_back_to(r);
        VALUE(double(r[0])) EXPECTED(0.2);
        VALUE(double(r[2])) EXPECTED(0.8);
    }

    //    VALUE( mt1.element_at(0)) EXPECTED ( 33 );
}

void test_filter_map() {
    std::vector<int> t1({ 1,19,4, 2, 5, -1, 5 });
    auto vt1 = functional_vector(t1);
    {
        auto mt1 = vt1.map(F(_ + 2)).filter(F(_ > 4));
        VALUE(mt1.size()) EXPECTED(4);

        std::vector<int> r;
        mt1.push_back_to(r);
        VALUE(int(r[0])) EXPECTED(21);
        VALUE(int(r[1])) EXPECTED(6);
    }
    // filter after type change
    {
        auto mt1 = vt1.map(F(_ * 0.2)).filter(F(_ >= 1.0));
        VALUE(mt1.size()) EXPECTED(3);
        std::vector<double> r;
        mt1.push_back_to(r);
        VALUE(double(r[0])) EXPECTED(3.8);
        VALUE(double(r[1])) EXPECTED(1.0);
    }
    // filter and then map
    {
        auto mt1 = vt1.filter(F(_ >= 2)).map(F(_ * 0.2));
        VALUE(mt1.size()) EXPECTED(5);
        std::vector<double> r;
        mt1.push_back_to(r);
        VALUE(double(r[0])) EXPECTED(3.8);
        VALUE(double(r[1])) EXPECTED(0.8);
        VALUE(double(r[2])) EXPECTED(0.4);
    }
}

void test_staging() {
    std::vector<int> t1({ 1,19,4, 2, 5, -1, 5 });
    auto vt1 = functional_vector(t1);
    auto plainVec = vt1.map(F(_ + 2)).filter(F(_ < 4)).filter(F(_ == 1)).map(F(_ * _ * 0.1)).stage();
    auto mt1 = functional_vector(plainVec);
    // one element (initially -1) should survive
    VALUE(mt1.size()) EXPECTED(1);
    VALUE(mt1.element_at(0).value()) EXPECTED(0.1);
    int a = 1, b = 8, c = 7;
    std::vector< const int *> v = {&a, &b, &c};
    auto s = lazy_view(v);
    auto ssum = s.map( F(*_)).sum();
    VALUE( ssum ) EXPECTED (16);
    auto ssum2 = s.toref().sum();
    VALUE( ssum2 ) EXPECTED ( 16 );

    auto sv = s.element_at(1).value();
    VALUE( *sv ) EXPECTED ( 8 );
    sv = s.sort( F(*_)).element_at(1).value();
    VALUE( *sv ) EXPECTED ( 7 ); 
    sv = s.sort( F(*_)).reverse().element_at(0).value();
    VALUE( *sv ) EXPECTED ( 8 ); 



}


void test_take() {
    std::vector<int> t1({ 1,19,4, 2, 5, -1, 5 });
    auto vt1 = functional_vector(t1);
    auto tt1 = vt1.take(3);
    VALUE(tt1.size()) EXPECTED(3);
    VALUE(tt1.element_at(0).value()) EXPECTED(1);
    VALUE(tt1.element_at(2).value()) EXPECTED(4);
    // take element with stride
    auto tt1s = vt1.take(lfv::all_elements, 2);
    VALUE(tt1s.size()) EXPECTED(4);
    VALUE(tt1s.element_at(1).value()) EXPECTED(4);
    VALUE(tt1s.element_at(3).value()) EXPECTED(5);


    auto tt2 = vt1.take_while(F(_ > 0));
    VALUE(tt2.size()) EXPECTED(5);
    VALUE(tt2.element_at(4).value()) EXPECTED(5);

    auto tt3 = vt1.skip(3);
    VALUE(tt3.size()) EXPECTED(4);
    VALUE(tt3.element_at(0).value()) EXPECTED(2);

    auto tt4 = vt1.skip_while(F(_ > 0));
    VALUE(tt4.size()) EXPECTED(2);
    VALUE(tt4.element_at(0).value()) EXPECTED(-1);

    // combine
    auto tt5 = vt1.take(5).skip(3);
    VALUE(tt5.size()) EXPECTED(2);
    VALUE(tt5.element_at(0).value()) EXPECTED(2);
    VALUE(tt5.element_at(1).value()) EXPECTED(5);

    auto tt6 = vt1.take_while(F(_ != 5)).skip_while(F(_ != 4));
    VALUE(tt6.size()) EXPECTED(2);
    VALUE(tt6.element_at(0).value()) EXPECTED(4);
    VALUE(tt6.element_at(1).value()) EXPECTED(2);
}


void test_sum_and_accumulate() {
    std::vector<int> t1({ 1,19,4, 2, 5, -1, 5 });
    auto vt1 = functional_vector(t1);
    auto s = vt1.sum();
    VALUE( s ) EXPECTED(35);
    auto s3 = vt1.take(3).sum(F(_));
    VALUE(s3) EXPECTED(24);
    const int multiply_el = vt1.take(4).skip(2).accumulate([](int t, int el) {return t * el;}, 1);
    VALUE(multiply_el) EXPECTED(8);
}

void test_chain() {
    std::vector<int> t1({ 1,19,4, 2 });
    std::vector<int> t2({ 5, -1, 3 });
    auto vt1 = functional_vector(t1);
    auto vt2 = functional_vector(t2);
    auto jt = vt1.chain(vt2);
    VALUE(jt.size()) EXPECTED(7);
    std::vector<int> byhand({1,19,4, 2, 5, -1, 3});
    auto vb = functional_vector(byhand);
    VALUE( vb.is_same(jt)) EXPECTED(true);

    auto ajt = vt1.skip(1).chain(vt2.filter(F(_ < 0)));
    VALUE(ajt.size()) EXPECTED(4);
    VALUE(ajt.element_at(0).value()) EXPECTED(19);
    VALUE(ajt.element_at(3).value()) EXPECTED(-1);

    auto r = jt.reverse();
    VALUE(r.element_at(0).value()) EXPECTED(3);


}


void test_sort() {
    std::vector<int> t1({ 1,19,4, 2, 5, -1, 5 });
    auto vt1 = functional_vector(t1);
    auto st1 = vt1.sort(); // default use values themselves
    VALUE(st1.size()) EXPECTED(t1.size());
    VALUE(st1.element_at(0).value()) EXPECTED(-1);
    VALUE(st1.element_at(1).value()) EXPECTED(1);
    VALUE(st1.element_at(6).value()) EXPECTED(19);

    auto rst1 = vt1.sort(F(-std::abs(_))); // reverse sort
    VALUE(rst1.size()) EXPECTED(t1.size());
    VALUE(rst1.element_at(0).value()) EXPECTED(19);


}

void test_enumerate() {
    // test first the indexed struct helper

    std::vector<int> t1({ 1,19,4, 2, 5, -1, 5 });
    auto vt1 = functional_vector(t1);
    auto en1 = vt1.enumerate();
    std::cout << "..... ";
    en1.foreach(S(std::cout << _.first << ":" << _.second << " "));
    std::cout << "\n";

    VALUE(en1.element_at(0).value().first) EXPECTED(0);
    VALUE(en1.element_at(0).value().second) EXPECTED(1);
    VALUE(en1.element_at(1).value().first) EXPECTED(1);
    VALUE(en1.element_at(1).value().second) EXPECTED(19);

    auto index_greater_than_value = en1.first_of(F(static_cast<int>(_.first) > _.second));
    VALUE(index_greater_than_value.value().first) EXPECTED(3);
    VALUE(index_greater_than_value.value().second) EXPECTED(2);

    // this is failing as expected
    //    auto sen1 = en1.take(20).sort(F(_.second)).stage();
    // auto plainVec = en1.take(20).stage();
    // auto sen1 = lazy_view(plainVec).sort(F(_.second)); // it should be staged again
    // // TODO figure out why staging was needed after sort
    // VALUE(sen1.element_at(0).value().second) EXPECTED(-1);
    // VALUE(sen1.element_at(0).value().first) EXPECTED(5);
}

void test_reversal() {
    std::vector<int> t1({ 1,19,4, 2, 5, -1, 5 });
    auto vt1 = functional_vector(t1);
    auto r1 = vt1.reverse();
    VALUE( r1.size() ) EXPECTED( 7);
    VALUE(r1.element_at(0).value()) EXPECTED(5);
    VALUE(r1.element_at(1).value()) EXPECTED(-1);

    auto back = r1.reverse();
    VALUE( back.size() ) EXPECTED( vt1.size() );
    VALUE( back.is_same(vt1) ) EXPECTED(true);

    auto last_3 = r1.reverse().take(3).reverse();
    VALUE(last_3.size()) EXPECTED(3);
    VALUE(last_3.element_at(0).value()) EXPECTED(4);
    VALUE(last_3.element_at(1).value()) EXPECTED(19);
    VALUE(last_3.element_at(2).value()) EXPECTED(1);



    auto s1 = r1.sort();
    VALUE(s1.element_at(0).value()) EXPECTED(-1);

    // double reversescala
    auto r2 = r1.reverse();
    VALUE(r2.element_at(1).value()) EXPECTED(19);

    // TODO try sorting
    auto s2 = r2.sort();
    VALUE(s2.element_at(0).value()) EXPECTED(-1);
}

void test_min_max() {
    std::vector<int> t1({ 1, 19, 4, 2, 5, -1, 5 });
    auto vt1 = functional_vector(t1);


    auto max1 = vt1.max(F(_));
    VALUE(max1.contains(19)) EXPECTED(true);

    auto min1 = vt1.min();
    VALUE(min1.contains(-1)) EXPECTED(true);

    VALUE(vt1.take(5).skip(2).size()) EXPECTED(3);
    VALUE(vt1.take(5).skip(2).size()) EXPECTED(3);
    VALUE(vt1.take(5).skip(2).empty()) EXPECTED(false);

    auto min2 = vt1.take(5).skip(2).min();
    VALUE(min2.size()) EXPECTED (1);
    vt1.take(5).skip(2).foreach(S(std::cout << _ << " "));
    VALUE(min2.element_at(0).value()) EXPECTED(2);

}

void test_zip() {
    std::vector<int> t1({ 1, 19, 4, 2, 5, -1, 5 });
    auto vt1 = functional_vector(t1);

    std::vector<int> t2({ 0, -1, -2, -3, -4 });
    auto vt2 = functional_vector(t2).reverse();
    auto z = vt1.zip(vt2);
    std::cout << "..... ";
    z.foreach(S(std::cout << _.first << ":" << _.second << ", "));
    std::cout << "\n";

    VALUE(z.size()) EXPECTED(std::min(t1.size(), t2.size()));
    VALUE(z.element_at(0).value().first) EXPECTED(1);
    VALUE(z.element_at(0).value().second) EXPECTED(-4);

    const bool same = vt1.is_same(vt1);
    const bool diff = vt1.is_same(vt2);
    VALUE(same) EXPECTED(true);
    VALUE(diff) EXPECTED(false);
}
void test_redirect() {
#ifdef TEST_LAZY
    std::vector<int> t1({ 1, 19 });
    std::vector<int> t2({-1, 5 });
    auto v = DirectView(t1);
    VALUE(v.sum()) EXPECTED(20);
    v.update_container(t2);
    VALUE(v.sum()) EXPECTED(4);
#endif
}
void test_series() {
#ifdef TEST_LAZY
    auto s1 = geometric_stream(2.5, 2);
    auto s1_5 = s1.take(5);
    VALUE(s1_5.size()) EXPECTED(5);
    auto s1_10 = s1.take(10);
    VALUE(s1_10.size()) EXPECTED(10);

    VALUE(s1_5.element_at(0).value()) EXPECTED(2.5);
    VALUE(s1_5.element_at(1).value()) EXPECTED(5.0);
    auto staged = s1_10.stage();
    VALUE(s1_5.is_same(lazy_view(staged))) EXPECTED(true); // we compare only first 5 elements

    auto s2 = arithmetic_stream(2, 3);
    VALUE(s2.element_at(0).value()) EXPECTED(2);
    VALUE(s2.element_at(1).value()) EXPECTED(5);
    VALUE(s2.element_at(2).value()) EXPECTED(8);
#endif
    auto ra = range_stream(6, 12);
    ra.foreach(PRINT);
    auto inv = range_stream(6., 0., -1.5);
    inv.foreach(PRINT);

    ra.zip(inv).foreach(PRINT);

    VALUE(ra.size()) EXPECTED(6);
    VALUE(ra.element_at(0).value()) EXPECTED(6);
    VALUE(ra.element_at(5).value()) EXPECTED(11);
    VALUE(ra.element_at(6).has_value()) EXPECTED(false);

    auto rd = range_stream(0.1, 0.2, 0.01);
    VALUE(rd.size()) EXPECTED(10);
#ifdef TEST_LAZY
    // randoms
    auto r = crandom_stream();
    std::cout << "..... ";
    r.map(F(double(_) / RAND_MAX)).take_while(F(_ < 0.9)).foreach(S(std::cout << _ << " "));
    // std::cout << "\n";

    // calculate pi for fun
    const size_t npoints = 10000;
    auto points = crandom_stream().map(F(double(_) / RAND_MAX)).group(2).take(npoints);
    size_t points_in = points.map(F(_.map(F(_ * _)).sum())).filter(F(_ < 1.0)).size();
    std::cout << "..... pi " << 4.0 * static_cast<double>(points_in) / npoints;
#endif
}


void test_cartesian() {
    auto x = range_stream(2, 6);
    auto y = range_stream(-3, 0);

    auto z = x.cartesian(y);
    VALUE(z.size()) EXPECTED(12);
    z.foreach(S(std::cout << _.first << ":" << _.second << ", "));
    auto permanent = z.map(F(std::make_pair(_.first, _.second)));
    // TODO fix an issue with element access and stage ( issue is in use of pair<&,&> which is evaporative)
    VALUE(permanent.element_at(0).value().first) EXPECTED(2);
    VALUE(permanent.element_at(0).value().second) EXPECTED(-3);

}

void test_group() {
    auto x = range_stream(0, 4);
    const size_t s2 = x.group(2).size();
    VALUE(s2) EXPECTED(2);
    const size_t s2_by_1 = x.group(2, 1).size();
    VALUE(s2_by_1) EXPECTED(3);
    x.group(2, 1).foreach(S(std::cout << _.element_at(0).value() << ":" << _.element_at(1).value() << " "));
    x.group(2).map( F(_.sum()));
}

void test_stat() {
    std::vector<int> t1({ 1, 19, 4, 2, 5, -1, 5 });
    auto vt1 = functional_vector(t1); 

    auto s = vt1.stat();
    VALUE(s.sum) EXPECTED( vt1.sum() );
    VALUE(s.count) EXPECTED( vt1.size() );
    VALUE(s.mean()) EXPECTED( static_cast<double>( vt1.sum())/vt1.size() );
    double var = vt1.map( CLOSURE(_-s.mean())).map( F(_*_) ).sum()/vt1.size(); // calculate it by hand
    VALUE(s.var()) EXPECTED( var);
}

void test_to_ref_ptr() {
    std::vector<int> t1({ 1, 19, 4, 2, 5, -1, 5 });
    auto vt1 = functional_vector(t1);
    const int sum1 = vt1.sum();
    const int sum2 = vt1.toptr().sum( F(*_));
    VALUE( sum1 ) EXPECTED( sum2 );

    const int sum3 = vt1.toptr().toref().sum( F(_));
    VALUE( sum1 ) EXPECTED( sum3 );

}

// a tricky container
template<typename T>
struct MyVec{
    using value_type = T*;
    std::vector<T*> data;
    ~MyVec(){ for ( auto el: data) delete el; }
    auto begin() const { return data.begin(); }
    auto end() const { return data.end(); }
    size_t size() const { return data.size(); }
    const T* at(size_t i) const { return data[i]; }

};




void test_one_element_container() {
    auto one = one_own(7.15);

    auto filtered_ok = one.filter(F(_>3));
    VALUE(filtered_ok.size()) EXPECTED(1);
    VALUE(filtered_ok.get().value()) EXPECTED( 7.15);
    auto filtered_toempty = one.filter(F(_<3));
    VALUE(filtered_toempty.size()) EXPECTED(0);
}

void test_match() {
    // test how finding best element between two containers would work (also when no match is found)
    // auto ints = lazy_own({1,4,5,6});
    // auto floats = lazy_own({1.5,3.9,5.12});
    // auto distance = [&floats](int i) { 
    //     // returns value it i is closer to ant of the floats than 0.2
    //     return floats.filter( CLOSURE( std::abs(_-i) < 0.2) ).min( CLOSURE( std::abs(_-i)) ).get(); 
    // };
    // VALUE(distance(1).has_value()) EXPECTED(false);
    // TODO - this needs fixing - strange memory issues here
//    // r.size();
//     auto close_pairs = ints.map( CLOSURE( std::make_pair(_,distance(_))) );
//     VALUE( close_pairs.size() ) EXPECTED( 4 );
//     VALUE( close_pairs.element_at(0).value().first ) EXPECTED(1);
//     VALUE( close_pairs.element_at(1).value().first ) EXPECTED(4);
//     VALUE( close_pairs.element_at(0).value().second.size() ) EXPECTED(0);
//     VALUE( close_pairs.element_at(1).value().second.size() ) EXPECTED(0);

    // auto only_with_matches = close_pairs.filter( F(not _.second.empty()) ); // can filter missing
    // VALUE( only_with_matches.element_at(0).value().first ) EXPECTED( 4 );
    // VALUE( only_with_matches.element_at(1).value().first ) EXPECTED( 5 );

//    auto defaulted_matches = close_pairs.map( F( std::make_pair( _.first, _.second.get().value_or(-1)) ) ).stage();

}

void test_ptr_view() {
    std::vector<int*> v;
    v.push_back( new int(1));
    v.push_back( new int(7));
    v.push_back( new int(-3));
    auto lv = lazy_view(v);
    const int s = lv.map(F(*_)).sum();
    VALUE(s) EXPECTED(5);
    const int s1 = lv.filter(F(*_ > 0)).map(F(*_)).sum();
    VALUE(s1) EXPECTED(8);
    const int s2 = lv.toref().filter(F(_>0)).sum();
    VALUE(s2) EXPECTED(s1);

}

void test_array_view() {
#ifdef TEST_LAZY
    int data[5] = {1,7,8,2,-1};
    auto array_view = lazy_view(data, sizeof(data)/sizeof(int));
    VALUE( array_view.sum() ) EXPECTED ( 17 );
    VALUE( array_view.take(3).sum() ) EXPECTED ( 16 );

    auto byptr_view = lazy_view( data, data+4);
    VALUE( byptr_view.sum() ) EXPECTED (18);
    VALUE( byptr_view.skip(3).max().get().value() ) EXPECTED (2);
    VALUE( byptr_view.element_at(5).has_value()) EXPECTED(false);
#endif
}

int main() {
    const int failed =
        + SUITE(test_type_presentation)
        + SUITE(test_element_access)
        + SUITE(test_count_and_find)
        + SUITE(test_filter)
        + SUITE(test_map)
        + SUITE(test_filter_map)
        + SUITE(test_staging)
        + SUITE(test_take)
        + SUITE(test_sum_and_accumulate)
        + SUITE(test_chain)
        + SUITE(test_sort)
        + SUITE(test_enumerate)
        + SUITE(test_reversal)
        + SUITE(test_min_max)
        + SUITE(test_zip)
        + SUITE(test_redirect)
        + SUITE(test_series)
        + SUITE(test_cartesian)
        + SUITE(test_group)
        + SUITE(test_stat)
        + SUITE(test_to_ref_ptr)
        + SUITE(test_one_element_container)
        + SUITE(test_match)
        + SUITE(test_ptr_view)
        + SUITE(test_array_view);

    std::cout << (failed == 0 ? "ALL OK" : "FAILURE") << std::endl;
    return failed;
}