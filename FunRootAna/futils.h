// Copyright 2022, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)
#ifndef futils_h
#define futils_h
#include <functional>
#include <iostream>

// macro generating generic, single agument, returning lambda
// example use: filter( F(_ < 0)) - the _ is the lambda argument, the function is pure (i.e. sees no outer scope), if that is needed use C
// when integrated in larger projects it may clash with other symbols 
#define F(CODE) []([[maybe_unused]] const auto &_) { return CODE; }


// closure, like function but see outer scope by reference (e.g. can change external variables)
// example:
// double x = 0;
//  container.map(CLOSURE(... some code using x ...))
#define CLOSURE(CODE) [&]([[maybe_unused]] const auto &_) { return CODE; }

// like the closure but allows to deliver variables to the scope selectively
// example:
// double x,y = 0;
//  container.map([&x]SCLOSURE(... some code using x ... ... y is not accessible ...))
#define SCLOSURE(CODE) ([[maybe_unused]] const auto &_) { return CODE; }

// subroutine (nothing is returned)
#define S(CODE) [&]([[maybe_unused]] const auto &_) { CODE; }

template<typename T, typename U, typename V>
struct triple {
  T first;
  U second;
  V third;
};
template<typename T, typename U, typename V>
triple<T, U, V> make_triple(T t, U u, V v) { return { t,u,v }; }



template<typename T, typename U>
std::ostream& operator<<(std::ostream& o, const std::pair<T,U>& v ) {return o << v.first << ":" << v.second<< " "; }

template<typename T, typename U, typename V>
std::ostream& operator<<(std::ostream& o, const triple<T,U, V>& v ) {return o << v.first << ":" << v.second<< ":" << v.third << " "; }

#define PRINT S(std::cout << _ << " ";)


struct StatInfo {
  double count = {};
  double sum = {};
  double mean() const { return sum / count; }
  double sum2 = {};
  double var() const { return sum2 / count - std::pow(mean(), 2); }
  double sigma() const { return std::sqrt(var()); }
};

// helper macros to streamline coding simple transformation that use pure functions
// example data._filter(_.x > 0).map(_.y).stat() that is select points with x>0, take y and calculate statistics of it

#define _filter(CODE) filter([]([[maybe_unused]] const auto &_) { return CODE; })
#define _map(CODE) map([]([[maybe_unused]] const auto &_) { return CODE; })
#define _count(CODE) count([]([[maybe_unused]] const auto &_) { return CODE; })
#define _contains(CODE) contains([]([[maybe_unused]] const auto &_) { return CODE; })
#define _all(CODE) all([]([[maybe_unused]] const auto &_) { return CODE; })
#define _sort(CODE) sort([]([[maybe_unused]] const auto &_) { return CODE; })
#define _take_while(CODE) take_while([]([[maybe_unused]] const auto &_) { return CODE; })
#define _max(CODE) max([]([[maybe_unused]] const auto &_) { return CODE; })
#define _min(CODE) min([]([[maybe_unused]] const auto &_) { return CODE; })




#endif