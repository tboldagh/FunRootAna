// Copyright 2022, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)
#ifndef futils_h
#define futils_h


// macro generating generic, single agument, returning lambda
// example use: filter( F(_ < 0)) - the _ is the lambda argument, the function is pure (i.e. sees no outer scope), if that is needed use C
#define F(CODE) [](const auto &_) { return CODE; }

// closure, like function but see outer scope by reference (can change external variables)
#define C(CODE) [&](const auto &_) { return CODE; }

// subroutine (nothing is returned)
#define S(CODE) [&](const auto &_) { CODE; }

template<typename T, typename U, typename V>
struct triple {
  T first;
  U second;
  V third;
};
template<typename T, typename U, typename V>
triple<T, U, V> make_triple(T t, U u, V v) { return { t,u,v }; }


#endif