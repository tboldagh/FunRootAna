// Copyright 2022, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)
#ifndef filling_h
#define filling_h

#include <TH1.h>
#include <TH2.h>
#include <TEfficiency.h>
#include <TProfile.h>
#include "Weights.h"
#include "HIST.h"

#include "EagerFunctionalVector.h"
#include "LazyFunctionalVector.h"


// fill from PODs 
// 0.7 >> hist;
void operator >> ( double v, WeightedHist<TH1> & h) {
    h.fill(v);
}

template<typename T, typename U>
void operator >> ( const std::pair<T,U>& v, WeightedHist<TH2> & h) {
    h.fill(v.first, v.second);
}

template<typename T, typename U>
void operator >> ( const std::pair<T,U>& v, WeightedHist<TProfile> & h) {
    h.fill(v.first, v.second);
}

template<typename T>
void operator >> ( const std::pair<bool,T>& v, WeightedHist<TEfficiency> & h) {
    h.fill(v.first, v.second);
}

// fill with the weight
template<typename T, typename U>
void operator >> ( const std::pair<T,U>& v, WeightedHist<TH1> & h) {
    h.fill(v.first, v.second);
}

template<typename T, typename U, typename V>
void operator >> ( const triple<T,U,V>& v, WeightedHist<TH2> & h) {
    h.fill(v.first, v.second, v.third);
}

template<typename T, typename U, typename V>
void operator >> ( const triple<T,U,V>& v, WeightedHist<TProfile> & h) {
    h.fill(v.first, v.second, v.third);
}
template<typename T, typename U>
void operator >> ( const triple<bool,T,U>& v, WeightedHist<TEfficiency> & h) {
    h.fill(v.first, v.second, v.third);
}


// Eager vector fill API
template<typename T>
void operator >> ( const EagerFunctionalVector<T>& v, WeightedHist<TH1> & h) {
    v.foreach( [&h]( const T& el){ el >> h; } );
}
template<typename T>
void operator >> ( const EagerFunctionalVector<T>& v, WeightedHist<TH2> & h) {
    v.foreach( [&h]( const T& el){ el >> h; } );
}

template<typename T>
void operator >> ( const EagerFunctionalVector<T>& v, WeightedHist<TProfile> & h) {
    v.foreach( [&h]( const T& el){ el >> h; } );
}


template<typename T>
void operator >> ( const EagerFunctionalVector<T>& v, WeightedHist<TEfficiency> & h) {
    v.foreach( [&h]( const T& el){ el >> h; } );
}

// lazy vector
template<typename A, typename T >
void operator >> ( const FunctionalInterface<A,T>& v, WeightedHist<TH1> & h) {
    v.foreach( [&h]( const auto& el){ el >> h; } );
}

template<typename A, typename T >
void operator >> ( const FunctionalInterface<A,T>& v, WeightedHist<TH2> & h) {
    v.foreach( [&h]( const auto& el){ el >> h; } );
}

template<typename A, typename T >
void operator >> ( const FunctionalInterface<A,T>& v, WeightedHist<TProfile> & h) {
    v.foreach( [&h]( const auto& el){ el >> h; } );
}

template<typename A, typename T >
void operator >> ( const FunctionalInterface<A,T>& v, WeightedHist<TEfficiency> & h) {
    v.foreach( [&h]( const auto& el){ el >> h; } );
}



// TODO code remaining fills

#endif