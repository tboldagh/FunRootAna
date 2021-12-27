#pragma once
#include <algorithm>
#include <stdexcept>
#include <memory>
#include <map>
#include <iostream>
#include <type_traits>
#include <TH1.h>
#include <TH2.h>
#include <TEfficiency.h>
#include <TProfile.h>
#include "Weights.h"
#include "HIST.h"

struct StatInfo {
  double count={};
  double sum={};
  double mean() const { return sum/count;  }
  double sum2={};
  double var() const { return sum2/count - std::pow( mean(), 2); }
  double sigma() const { return std::sqrt( var() ); }
};

template<typename T, typename U, typename V>
struct triple{
  T first;
  U second;
  V third;
};
template<typename T, typename U, typename V>
triple<T,U,V> make_triple(T t, U u, V v){ return {t,u,v}; }

template<template<typename T, typename Allocator> class Container, typename T, typename Allocator=std::allocator<T>>
class WrappedSequenceContainer
{
private:
  Container<T, Allocator> container;
public:
  WrappedSequenceContainer(){}

  WrappedSequenceContainer(const Container<T, Allocator> &vec)
    : container( vec.begin(), vec.end() ) {}

  WrappedSequenceContainer(const WrappedSequenceContainer<Container, T, std::allocator<T>>& rhs)
    : container(rhs.container) {}

  WrappedSequenceContainer(typename Container<T, Allocator>::const_iterator begin, typename Container<T, Allocator>::const_iterator end )
    : container(begin, end) {}

  void operator=(WrappedSequenceContainer<Container, T, std::allocator<T>>&& rhs) {
    container = std::move(rhs.container);
  }

  template<typename ResultT, typename Op>
  ResultT reduce( const ResultT& initialValue, Op operation ) const {
    ResultT total = initialValue;
    for ( auto e: container ) {
      total = operation( total, e );
    }
    return total;
  }

  template<typename Op>
  WrappedSequenceContainer<Container, decltype(std::declval<Op>()(std::declval<T>()))> integ( Op op ) const {
    WrappedSequenceContainer<Container, decltype(std::declval<Op>()(std::declval<T>())) > ret;
    ret.push_back( sum(op) );
    return ret;
  }

  template<typename Op>
//  decltype(std::declval<Op>()(std::declval<T>())) sum( Op operation ) const {
//    decltype(std::declval<Op>()(std::declval<T>())) total = {};
  typename std::invoke_result<Op, const T&>::type sum( Op operation ) const {
  typename std::invoke_result<Op, const T&>::type total = {};

    for ( auto e: container ) {
      total += operation( e );
    }
    return total;
  }


  T sum() const {
    T total = {};
    for ( auto e: container ) {
      total += e;
    }
    return total;
  }


  template<typename Op>
  StatInfo stat( Op f ) const {
    StatInfo info;
    for ( auto e: container ) {
      decltype(std::declval<Op>()(std::declval<T>())) v = f( e );
      info.count++;
      info.sum += v;
      info.sum2 += std::pow(v,2);
    }
    return info;
  }





  template<typename Op>
  WrappedSequenceContainer<Container, T, Allocator> filter(Op f) const {
    WrappedSequenceContainer<Container, T, Allocator> ret;
    for (const auto &el : container)
      if(f(el))
      	ret.container.push_back(el);
    return ret;
  }

  template<typename Op>
  size_t count(Op f) const {
    size_t n = 0;
    for ( const auto& e : container )
      n +=  (f(e) ? 1 : 0);
    return n;
  }

  inline bool empty() const {
    return container.empty();
  }

  template< typename KeyExtractor>
  WrappedSequenceContainer<Container, T, Allocator> sorted( KeyExtractor key) const {
    WrappedSequenceContainer<Container, T, Allocator> clone( container );
    std::sort( clone.container.begin(), clone.container.end(), [&]( const T& el1, const T& el2){ return key(el1) < key(el2); } );
    return clone;
  }


  WrappedSequenceContainer<Container, T, Allocator> reverse() const {
    WrappedSequenceContainer<Container, T, Allocator> ret;
    auto it_rbegin = container.rbegin();
    while(it_rbegin != container.rend()){
      ret.container.push_back(*it_rbegin++);
    }
    return ret;
  }

  template<typename Op>
  WrappedSequenceContainer<Container, decltype(std::declval<Op>()(std::declval<T>()))> map(Op f) const {
    WrappedSequenceContainer<Container, decltype(std::declval<Op>()(std::declval<T>())) > ret;
    for(const auto &el : container)
      ret.push_back(f(el));
    return ret;
  }

  template<typename Op>
  std::map<decltype(std::declval<Op>()(std::declval<T>())), Container<T, Allocator>> groupBy(Op f) const {
    std::map<decltype(std::declval<Op>()(std::declval<T>())), Container<T, Allocator>> ret;
    for(const auto &el : container)
      ret[f(el)].push_back(el);
    return ret;
  }

  template<typename Op>
  WrappedSequenceContainer<Container, typename decltype(std::declval<Op>()(std::declval<T>()))::value_type, Allocator> flatMap(Op f) const {
    WrappedSequenceContainer<Container, typename decltype(std::declval<Op>()(std::declval<T>()))::value_type, Allocator> ret;
    this->map(f).foreach([&ret](const T &item){ ret.insert(ret, item); } );
    return ret;
  }

  template< typename Op>
  const WrappedSequenceContainer<Container, T, Allocator>& foreach( Op function )  const {
    for ( auto el: container ) function ( el );
    return *this;
  }


  Container<T, Allocator>& unwrap() {
    return container;
  }

  const Container<T, Allocator>& unwrap() const {
    return container;
  }

  void push_back(const T &el) {
    container.emplace_back(el);
  }
  void clear() { 
    container.clear();
  }



  template<typename T1, typename T2>
  void insert(const T1 &destCont, const T2 &items) {
    container.insert(destCont.container.end(), items.begin(), items.end());
  }

  size_t size() const {
    return container.size();
  }

  WrappedSequenceContainer<Container, std::pair<T,T>, Allocator> unique_pairs() const {
    WrappedSequenceContainer<Container, std::pair<T,T>, Allocator> ret;
    for ( auto iter1 = std::begin(container); iter1 != std::end(container); ++ iter1 ) {
      for ( auto iter2 = iter1+1; iter2 != std::end(container); ++iter2 ) {
	ret.push_back( std::make_pair(*iter1, *iter2) );
      }
    }
    return ret;
  }

  WrappedSequenceContainer<Container, T, Allocator> rotate( int shift ) const {
    if ( shift == 0 )
      return container;

    WrappedSequenceContainer<Container, T, Allocator> ret;
    for ( auto iter = std::begin(container) + shift; iter != std::end(container); ++iter )
      ret.push_back( *iter );
    for ( auto iter = std::begin(container); iter != std::begin(container) + shift; ++iter )
      ret.push_back( *iter );
    return ret;
  }

  template< typename KeyExtractor>
  WrappedSequenceContainer<Container, T, Allocator> max( KeyExtractor key )  const {
    auto iterator = std::max_element( std::begin(container), std::end(container), [&]( const T& a, const T& b) { return key(a) < key(b); } );
    return std::begin(container) == std::end(container) ? *this : WrappedSequenceContainer<Container, T, Allocator> ( iterator, iterator + 1 );
  }

  WrappedSequenceContainer<Container, T, Allocator> max( ) const {
    auto iterator = std::max_element( std::begin(container), std::end(container), [&]( const T& a, const T& b) { return a < b; } );
    return std::begin(container) == std::end(container) ? *this : WrappedSequenceContainer<Container, T, Allocator> ( iterator, iterator + 1 );
  }

  template< typename KeyExtractor>
  const WrappedSequenceContainer<Container, T, Allocator> min( KeyExtractor key )  const {
    auto iterator = std::min_element( std::begin(container), std::end(container), [&]( const T& a, const T& b) { return key(a) < key(b); } );
    return std::begin(container) == std::end(container) ? *this : WrappedSequenceContainer<Container, T, Allocator> ( iterator, iterator+1 );
  }

  const WrappedSequenceContainer<Container, T, Allocator> min()  const {
    auto iterator = std::min_element( std::begin(container), std::end(container), [&]( const T& a, const T& b) { return a < b; } );
    return std::begin(container) == std::end(container) ? *this : WrappedSequenceContainer<Container, T, Allocator> ( iterator, iterator+1 );
  }

  const WrappedSequenceContainer<Container, T, Allocator> range( size_t start, size_t stop ) const {
    return WrappedSequenceContainer<Container, T, Allocator>( container.begin()+start, container.begin ()+stop );
  }

  typename std::conditional<std::is_pod<T>::value, T, const T&>::type element_at( size_t index ) const { return container.at(index); }
  typename std::conditional<std::is_pod<T>::value, T, const T&>::type  operator[]( size_t index ) const { return container.at(index); }

  const WrappedSequenceContainer<Container, T, Allocator> first( size_t n = 1 )  const {
    auto iterator = container.end() < container.begin() + n ? container.end() : container.begin() + n;
    return WrappedSequenceContainer<Container, T, Allocator> ( container.begin(), iterator );
  }

  const WrappedSequenceContainer<Container, T, Allocator> last( size_t n = 1 )  const {    
    auto iterator = container.end() < container.begin() + n ? container.begin() : container.begin() + (container.size() - n);
    return WrappedSequenceContainer<Container, T, Allocator> ( iterator, container.end() );
  }


  const WrappedSequenceContainer<Container, T, Allocator> join( const WrappedSequenceContainer<Container, T, Allocator>& other  )  const {
    WrappedSequenceContainer<Container, T, Allocator> ret ( container.begin(), container.end() );
    ret.container.insert( ret.container.end(), other.begin(), other.end() );
    return ret;
  }

  template<typename U>
const WrappedSequenceContainer<Container, std::tuple<T,U> > zip( const WrappedSequenceContainer<Container, U, Allocator>& other ) {
    WrappedSequenceContainer<Container, std::tuple<T,U> > ret;
    
    auto thisIter = std::cbegin( container );
    auto otherIter = std::cbegin( other.unWrap() );
    for ( ; thisIter != std::end( container ) and otherIter != std::end( other.unWrap() ); ++thisIter, ++otherIter ) {
      ret.unWrap().insert( ret.unWrap().end(), std::make_tuple( *thisIter, *otherIter) );    
    }
    return ret;
  }

  auto begin() const {
    return std::cbegin( container );
  }

  auto end() const {
    return std::cend( container );
  }


  const WrappedSequenceContainer<Container, T, Allocator> fill( WeightedHist<TH1> h ) const {
    for ( auto e: container )
      h.fill(e);
    return *this;
  }

  const WrappedSequenceContainer<Container, T, Allocator> gwfill( WeightedHist<TH1> h ) const {
    for ( auto e: container )
      h.wfill(e);
    return *this;
  }

  const WrappedSequenceContainer<Container, T, Allocator> wfill( WeightedHist<TH1> h ) const {
    for ( auto e: container )
      h.fill(e.first, e.second);
    return *this;
  }


  const WrappedSequenceContainer<Container, T, Allocator> fill( WeightedHist<TH2> h ) const {
    for ( auto e: container )
      h.fill( e.first, e.second );
    return *this;
  }

  const WrappedSequenceContainer<Container, T, Allocator> wfill( WeightedHist<TH2> h ) const {
    for ( auto e: container )
      h.fill( e.first, e.second, e.third );
    return *this;
  }

  const WrappedSequenceContainer<Container, T, Allocator> wfill( WeightedHist<TProfile> h ) const {
    for ( auto e: container )
      h.fill( e.first, e.second, e.third );
    return *this;
  }



  const WrappedSequenceContainer<Container, T, Allocator> gwfill( WeightedHist<TH2> h ) const {
    for ( auto e: container )
      h.wfill( e.first, e.second );
    return *this;
  }


  const WrappedSequenceContainer<Container, T, Allocator> fill( WeightedHist<TEfficiency> h ) const {
    for ( auto e: container )
      h.fill( e.first, e.second  );
    return *this;
  }
};




template<template<typename T, typename Allocator> class Container, typename T>
WrappedSequenceContainer<Container, T, std::allocator<T>> wrap(const Container<T, std::allocator<T>> &v)
{
  return WrappedSequenceContainer<Container, T, std::allocator<T>>(v);
}


template<typename T>
auto wrap(const std::initializer_list<T>& il)
{
  return wrap( std::vector<T>(std::begin(il), std::end(il)) );
}


// macro generating generic, single agument, returning lambda
// example use: filter( F(_ < 0)) - the _ is the lambda argument
#define F(CODE) [&](const auto &_) { return CODE; }
