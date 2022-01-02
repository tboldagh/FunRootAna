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



template<typename T>
void operator >> ( const EagerFunctionalVector<T>& v, WeightedHist<TH1> & h) {
    v.foreach( [&h]( const T& el){ h.fill(el); } );
}

template<typename T1, typename T2>
void operator >> ( const EagerFunctionalVector< std::pair<T1, T2> >& v, WeightedHist<TH1> & h) {
    v.foreach( [&h]( const std::pair<T1, T2>& el){ h.fill(el.first, el.second); } );
}

template<typename T1, typename T2>
void operator >> ( const EagerFunctionalVector<std::pair<T1, T2>>& v, WeightedHist<TH2> & h) {
    v.foreach( [&h]( const std::pair<T1, T2>& el){ h.fill(el.first, el.second); } );
}

template<typename T1, typename T2, typename T3>
void operator >> ( const EagerFunctionalVector<triple<T1, T2, T3>>& v, WeightedHist<TH2> & h) {
    v.foreach( [&h]( const triple<T1, T2, T3>& el){ h.fill(el.first, el.second, el.third); } );
}

template<typename T>
void operator >> ( const EagerFunctionalVector<T>& v, WeightedHist<TProfile> & h) {
    v.foreach( [&h]( const T& el){ h.fill(el.first, el.second); } );
}

template<typename T>
void operator >> ( const EagerFunctionalVector<T>& v, WeightedHist<TEfficiency> & h) {
    v.foreach( [&h]( const T& el){ h.fill(el.first, el.second); } );
}

template<typename A, typename T >
void operator >> ( const FunctionalInterface<A,T>& v, WeightedHist<TH1> & h) {
    v.foreach( [&h]( const T& el){ h.fill(el); } );
}

template<typename A, typename T1, typename T2>
void operator >> ( const FunctionalInterface<A,std::pair<T1, T2>>& v, WeightedHist<TH1> & h) {
    v.foreach( [&h]( const std::pair<T1, T2>& el){ h.fill(el.first, el.second); } );
}

// TODO code remaining fills





#endif