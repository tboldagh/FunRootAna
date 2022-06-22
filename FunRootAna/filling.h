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

template<typename T>
const std::optional<T>& operator >> (const std::optional<T>& v, TH1& h ) {
    if ( v.has_value() ) v.value() >> h;
    return v;
}

double operator >> ( double v, TH1 & h) {
    h.Fill(v);
    return v;
}

float operator >> ( float v, TH1 & h) {
    h.Fill(v);
    return v;
}

int operator >> ( int v, TH1 & h) {
    h.Fill(v);
    return v;
}

unsigned operator >> ( unsigned v, TH1 & h) {
    h.Fill(v);
    return v;
}

long unsigned int operator >> ( long unsigned int v, TH1 & h) {
    h.Fill(v);
    return v;
}
long int operator >> ( long int v, TH1 & h) {
    h.Fill(v);
    return v;
}


template<typename T, typename U>
const std::pair<T,U>& operator >> ( const std::pair<T,U>& v, TH2 & h) {
    h.Fill(v.first, v.second);
    return v;
}

template<typename T, typename U>
const std::pair<T,U>& operator >> ( const std::pair<T,U>& v, TProfile & h) {
    h.Fill(v.first, v.second);
    return v;
}

template<typename T>
const std::pair<bool,T>& operator >> ( const std::pair<bool,T>& v, TEfficiency & h) {
    if ( h.GetDimension() == 2) throw std::runtime_error("pair<bool, value> insufficient to fill 2D TEfficiency, triple<bool, y_value, x_value> is needed");
    h.Fill(v.first, v.second);
    return v;
}

// fill with the weight
template<typename T, typename U>
const std::pair<T,U>& operator >> ( const std::pair<T,U>& v, TH1 & h) {
    h.Fill(v.first, v.second);
    return v;
}

template<typename T, typename U, typename V>
const triple<T,U,V>& operator >> ( const triple<T,U,V>& v, TH2 & h) {
    h.Fill(v.first, v.second, v.third);
    return v;
}

template<typename T, typename U, typename V>
const std::tuple<T,U,V>& operator >> ( const std::tuple<T,U,V>& v, TH2 & h) {
    h.Fill(std::get<0>(v), std::get<1>(v), std::get<2>(v));
    return v;
}


template<typename T, typename U, typename V>
const triple<T,U,V>& operator >> ( const triple<T,U,V>& v, TH3 & h) {
    h.Fill(v.first, v.second, v.third);
    return v;
}

template<typename T, typename U, typename V>
const std::tuple<T,U,V>& operator >> ( const std::tuple<T,U,V>& v, TH3 & h) {
    h.Fill(std::get<0>(v), std::get<1>(v), std::get<2>(v));
    return v;
}

template<typename T, typename U, typename V, typename Z>
const std::tuple<T,U,V,Z>& operator >> ( const std::tuple<T,U,V,Z>& v, TH3 & h) {
    h.Fill(std::get<0>(v), std::get<1>(v), std::get<2>(v), std::get<3>(v));
    return v;
}


template<typename T, typename U, typename V>
const triple<T,U,V>& operator >> ( const triple<T,U,V>& v, TProfile & h) {
    h.Fill(v.first, v.second, v.third);
    return v;
}

template<typename T, typename U, typename V>
const std::tuple<T,U,V>& operator >> ( const std::tuple<T,U,V>& v, TProfile & h) {
    h.Fill(std::get<0>(v), std::get<1>(v), std::get<2>(v));
    return v;
}


template<typename T, typename U>
const triple<bool,T,U>& operator >> ( const triple<bool,T,U>& v, TEfficiency & h) {
    std::make_tuple(v.first, v.second, v.third) >> h;
    return v;
}

template<typename U, typename V>
const std::tuple<bool,U,V>& operator >> ( const std::tuple<bool,U,V>& v, TEfficiency & h) {
    if ( h.GetDimension() == 2 ) {
        h.Fill(std::get<0>(v), std::get<1>(v), std::get<2>(v));
    } else if ( h.GetDimension() == 1 ) {
        h.FillWeighted(std::get<0>(v), std::get<2>(v), std::get<1>(v)); // ROOT is inconsistent here
    }
    return v;
}


template<typename U, typename V, typename Z>
const std::tuple<bool,U,V,Z>& operator >> ( const std::tuple<bool,U,V,Z>& v, TEfficiency & h) {
    if ( h.GetDimension() == 2 ) {
        h.FillWeighted(std::get<0>(v), std::get<3>(v), std::get<1>(v), std::get<2>(v)); // ROOT is inconsistent here    
    } else {
        throw std::runtime_error("Can not fill (yet?) efficiencies other than 2D using tuple of 4");
    }
    return v;
}


// Eager vector fill API
template<typename T>
const EagerFunctionalVector<T>& operator >> ( const EagerFunctionalVector<T>& v, TH1 & h) {
    v.foreach( [&h]( const T& el){ el >> h; } );
    return v;
}
template<typename T>
const EagerFunctionalVector<T>& operator >> ( const EagerFunctionalVector<T>& v, TH2 & h) {
    v.foreach( [&h]( const T& el){ el >> h; } );
    return v;
}

template<typename T>
const EagerFunctionalVector<T>& operator >> ( const EagerFunctionalVector<T>& v, TH3 & h) {
    v.foreach( [&h]( const T& el){ el >> h; } );
    return v;
}

template<typename T>
const EagerFunctionalVector<T>& operator >> ( const EagerFunctionalVector<T>& v, TProfile & h) {
    v.foreach( [&h]( const T& el){ el >> h; } );
    return v;
}


template<typename T>
const EagerFunctionalVector<T>& operator >> ( const EagerFunctionalVector<T>& v, TEfficiency & h) {
    v.foreach( [&h]( const T& el){ el >> h; } );
    return v;
}

// lazy vector
template<typename A, typename T >
const lfv::FunctionalInterface<A,T>& operator >> ( const lfv::FunctionalInterface<A,T>& v, TH1 & h) {
    v.foreach( [&h]( const auto& el){ el >> h; } );
    return v;
}

template<typename A, typename T >
const lfv::FunctionalInterface<A,T>& operator >> ( const lfv::FunctionalInterface<A,T>& v, TH2 & h) {
    v.foreach( [&h]( const auto& el){ el >> h; } );
    return v;
}

template<typename A, typename T >
const lfv::FunctionalInterface<A,T>& operator >> ( const lfv::FunctionalInterface<A,T>& v, TH3 & h) {
    v.foreach( [&h]( const auto& el){ el >> h; } );
    return v;
}


template<typename A, typename T >
const lfv::FunctionalInterface<A,T>& operator >> ( const lfv::FunctionalInterface<A,T>& v, TProfile & h) {
    v.foreach( [&h]( const auto& el){ el >> h; } );
    return v;
}

template<typename A, typename T >
const lfv::FunctionalInterface<A,T>& operator >> ( const lfv::FunctionalInterface<A,T>& v, TEfficiency & h) {
    v.foreach( [&h]( const auto& el){ el >> h; } );
    return v;
}



// TODO code remaining fills or replaec by templates

#endif