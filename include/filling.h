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


// operators to streamline ROOT histograms filling
// - from PODs
// - eager functional vectors
// - lazy functional vectors

// fill from PODs 
// 0.7 >> hist;


void operator >> ( double v, TH1 & h) {
    h.Fill(v);
}

template<typename T, typename U>
void operator >> ( const std::pair<T,U>& v, TH2 & h) {
    h.Fill(v.first, v.second);
}

template<typename T, typename U>
void operator >> ( const std::pair<T,U>& v, TProfile & h) {
    h.Fill(v.first, v.second);
}

template<typename T>
void operator >> ( const std::pair<bool,T>& v, TEfficiency & h) {
    h.Fill(v.first, v.second);
}

// fill with the weight
template<typename T, typename U>
void operator >> ( const std::pair<T,U>& v, TH1 & h) {
    h.Fill(v.first, v.second);
}

template<typename T, typename U, typename V>
void operator >> ( const triple<T,U,V>& v, TH2 & h) {
    h.Fill(v.first, v.second, v.third);
}

template<typename T, typename U, typename V>
void operator >> ( const triple<T,U,V>& v, TProfile & h) {
    h.Fill(v.first, v.second, v.third);
}
template<typename T, typename U>
void operator >> ( const triple<bool,T,U>& v, TEfficiency & h) {
    h.Fill(v.first, v.second, v.third);
}


// Eager vector fill API
template<typename T>
void operator >> ( const EagerFunctionalVector<T>& v, TH1 & h) {
    v.foreach( [&h]( const T& el){ el >> h; } );
}
template<typename T>
void operator >> ( const EagerFunctionalVector<T>& v, TH2 & h) {
    v.foreach( [&h]( const T& el){ el >> h; } );
}

template<typename T>
void operator >> ( const EagerFunctionalVector<T>& v, TProfile & h) {
    v.foreach( [&h]( const T& el){ el >> h; } );
}


template<typename T>
void operator >> ( const EagerFunctionalVector<T>& v, TEfficiency & h) {
    v.foreach( [&h]( const T& el){ el >> h; } );
}

// lazy vector
template<typename A, typename T >
void operator >> ( const FunctionalInterface<A,T>& v, TH1 & h) {
    v.foreach( [&h]( const auto& el){ el >> h; } );
}

template<typename A, typename T >
void operator >> ( const FunctionalInterface<A,T>& v, TH2 & h) {
    v.foreach( [&h]( const auto& el){ el >> h; } );
}

template<typename A, typename T >
void operator >> ( const FunctionalInterface<A,T>& v, TProfile & h) {
    v.foreach( [&h]( const auto& el){ el >> h; } );
}

template<typename A, typename T >
void operator >> ( const FunctionalInterface<A,T>& v, TEfficiency & h) {
    v.foreach( [&h]( const auto& el){ el >> h; } );
}



// TODO code remaining fills

#endif