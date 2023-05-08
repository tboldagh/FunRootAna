// Copyright 2022, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)
#include <chrono>
#include <cstdlib>
#include <vector>
#include <iostream>
#include "LazyFunctionalVector.h"
#include "EagerFunctionalVector.h"
#include "HIST.h"


class HBenchmark : public HandyHists {
    public:
    void test_fixed_names() {
        HCONTEXT("top_");
        for ( int i = 0; i < 1000; i++) {
            HCONTEXT("group_");
            for ( int j = 0; j < 100 ; ++j ) {
                HCONTEXT("sub_");
                // report( "... HC "+  HistContext::current() );
                HIST1("a", "a", 10, 0, 10);
            }
        }
     }
    void test_changing_names() {
        HCONTEXT("top_");
        for ( int i = 0; i < 10; i++) {
            HCONTEXT("loop_"+std::to_string(i)+"_");
            for ( int j = 0; j < 1000 ; ++j ) {
                HCONTEXT("tail_");
                // report( "... HC "+  HistContext::current() );

                HIST1("a", "a", 10, 0, 10);
            }
        }
     }

};



// this is to compare (very roughly) performance if lazy and eager functional vectors
using namespace lfv;


void randfill(std::vector<double>& v) {
    for (auto& el : v)
        el = static_cast<double>(rand()) / static_cast<double>(RAND_MAX);
}

template<typename V>
double filter_map_sum(const V& v) {
    double result = v.filter(F(_ < 0.5)).map(F(_ + 1)).sum();
    return result;
}

double traditional( const std::vector<double>& v) {
    double sum = 0;
    for ( double el: v) {
        if ( el < 0.5 ) sum += el+1;
    }
    return sum;
}

void hcontext() {

}



template<typename FUN, typename ARG>
double measure(FUN f, ARG& a, size_t repeat) {
    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::duration;
    using std::chrono::milliseconds;
    auto t1 = high_resolution_clock::now();

    for (size_t rep = 0; rep < repeat; ++rep) {
        f(a);
    }
    auto t2 = high_resolution_clock::now();
    duration<double, std::milli> spent = t2 - t1;
    return spent.count();
}



int main() {

    std::vector<double> small(10); // 10 elements
    randfill(small);


    auto eager_small = wrap(small);
    auto lazy_small  = lazy_view(small);
    std::cout << "[ms] eager filter_map_sum on small vector " <<    measure( filter_map_sum<decltype(eager_small)>, eager_small, 100) << std::endl;
    std::cout << "[ms] lazy  filter_map_sum on small vector " <<    measure( filter_map_sum<decltype(lazy_small)>, lazy_small, 100) << std::endl;
    std::cout << "[ms] basic filter_map_sum on small vector " <<    measure( traditional, small, 100) << std::endl;


    std::vector<double> big(1000);
    randfill(big);
    auto eager_big = wrap(big);
    auto lazy_big  = lazy_view(big);
    std::cout << "[ms] eager filter_map_sum on big vector " <<    measure( filter_map_sum<decltype(eager_big)>, eager_big, 100) << std::endl;
    std::cout << "[ms] lazy  filter_map_sum on big vector " <<    measure( filter_map_sum<decltype(lazy_big)>, lazy_big, 100) << std::endl;
    std::cout << "[ms] basic filter_map_sum on big vector " <<    measure( traditional, big, 100) << std::endl;



    std::vector<double> large(100000);
    randfill(large);
    auto eager_large = wrap(large);
    auto lazy_large  = lazy_view(large);
    std::cout << "[ms] eager filter_map_sum on large vector " <<    measure( filter_map_sum<decltype(eager_large)>, eager_large, 100) << std::endl;
    std::cout << "[ms] lazy  filter_map_sum on large vector " <<    measure( filter_map_sum<decltype(lazy_large)>, lazy_large, 100) << std::endl;
    std::cout << "[ms] basic filter_map_sum on large vector " <<    measure( traditional, large, 100) << std::endl;


    HBenchmark h;
    auto test_fixed = [&h]( auto& ignore) { h.test_fixed_names(); };
    std::cout << "[ms] histogramming fixed context names " << measure( test_fixed, small, 500 ) << std::endl;
    auto test_changing = [&h]( auto& ignore) { h.test_changing_names(); };
    std::cout << "[ms] histogramming changing context names " << measure( test_changing, small, 1000 ) << std::endl;



}


