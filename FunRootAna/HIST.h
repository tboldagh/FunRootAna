// Copyright 2022, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)
#ifndef HIST_H
#define HIST_H
#include <map>
#include <list>
#include <iostream>
#include "TH1.h"
#include "TEfficiency.h"
#include "TH2.h"
#include "TProfile.h"

#include "Weights.h"
#include "assure.h"


struct HistContext {
  HistContext(const std::string_view str)  {
    m_prev = s_histContext;
    s_histContext += str;
  }
  ~HistContext() {
    s_histContext = m_prev;
  }
  static const char* name( const char* n ) {
    static std::string s(512, '\0');
    s = s_histContext + n ;   
    return s.c_str();
  }
  static const std::string_view current() {
    return s_histContext;
  }
  static void reserve() { s_histContext.reserve(512); }
  static std::string s_histContext;
  std::string m_prev;
};

#define HCONTEXT(__CTX__) HistContext __hist_context(__CTX__);


class HandyHists {
 public:
  HandyHists() { HistContext::reserve();  }
  template<typename H>
    H* reg( H* h ) {
    h->SetDirectory(0);
    return h;  
  }

  TH1D* h1reg( TH1D* h) {
    m_h1.push_back(reg ( h ));
    return h;      
  }

  TEfficiency* effreg( TEfficiency* h) {
    m_eff.push_back(reg ( h ));
    return h;      
  }

  TH2D* h2reg( TH2D* h) {
    m_h2.push_back(reg ( h ));    
    return h;      
  }

  TProfile* profreg( TProfile* h) {
    m_prof.push_back(reg ( h ));    
    return h;      
  }

  void save( const std::string& fname);
  
 protected:
  std::list<TH1D*> m_h1;
  std::list<TEfficiency*> m_eff;    
  std::list<TH2D*> m_h2;
  std::list<TProfile*> m_prof;

};


// Here be dragons, these macros can be used in methods of the class inheriting from HandyHists, 
// they are capable of exposing histograms to be filled or crate them it if need be
// Each histogram is context aware, that is it has a name that depends on the concatenated contexts (made by HCONTEXT macro)
// The histogram created in this particular line are cached locally so the lookup (if needed at all) is fast, 
// therefore this locally generated lambda
// Why macro? That is the whole point. We want lambdas generated for each HIST macro occurrence.

// The innerworkings are as follows:
// - static (thus specific to each generated lambda) cache is declared
// - coordinates hash is generated and checked (it contains the context info)
// - if histogram is missing in the cache, it is then booked, registered for saving ...
// All these macros are very similar (modulo types - and arity specific to the histogram type).
// @warning - ANYHIST registration is not supported
// TODO once in c++20 use [[likely]] hints


#define HIST1( __NAME,__TITLE,__XBINS,__XMIN,__XMAX ) \
  ([this]() -> TH1D& { \
    static std::vector< std::pair<std::string, TH1D*>> cache; \
    for ( auto & [c, h]: cache) \
      if ( c == HistContext::current() ) return *h; \
    cache.emplace_back( HistContext::current(), this->h1reg ( new TH1D( HistContext::name(__NAME), __TITLE,__XBINS,__XMIN,__XMAX))); \
    static std::string name = __NAME; \
    assure( name.empty() or name == __NAME, std::string("Histograms names consitency in ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    return *cache.back().second; \
  }())


#define HIST1V( __NAME,__TITLE,__VEC ) \
  ([this]() -> TH1& { \
    static std::vector< std::pair<std::string, TH1*>> cache; \
    for ( auto & [c, h]: cache) \
      if ( c == HistContext::current() ) return *h; \
    cache.emplace_back( HistContext::current(), this->h1reg ( new TH1D( HistContext::name(__NAME),__TITLE,__VEC.size()-1,__VEC.data())) ); \
    static std::string name; \
    assure( name.empty() or name == __NAME, std::string("Histograms names consitency in ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    name = __NAME; \
    return *cache.back().second; \
  }())


#define PROF1( __NAME,__TITLE,__XBINS,__XMIN,__XMAX ) \
  ([this]() -> TProfile& { \
    static std::vector< std::pair<std::string, TProfile*>> cache; \
    for ( auto & [c, h]: cache) \
      if ( c == HistContext::current() ) return *h; \
    cache.emplace_back( HistContext::current() , this->profreg ( new TProfile(HistContext::name(__NAME),__TITLE,__XBINS,__XMIN,__XMAX))); \
    static std::string name = __NAME; \
    assure( name.empty() or name == __NAME, std::string("Histograms names consitency in ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    return *cache.back().second; \
  }())

#define PROF1V( __NAME,__TITLE,__VEC ) \
  ([this]() -> TProfile& { \
    static std::vector< std::pair<std::string, TProfile*>> cache; \
    for ( auto & [c, h]: cache) \
      if ( c == HistContext::current() ) return *h; \
    cache.emplace_back( HistContext::current(),  this->profreg ( new TProfile(HistContext::name(__NAME),__TITLE,__VEC.size()-1,__VEC.data()))); \
    static std::string name = __NAME; \
    assure( name.empty() or name == __NAME, std::string("Histograms names consitency in ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    return *cache.back().second; \
  }())


#define EFF1( __NAME,__TITLE,__XBINS,__XMIN,__XMAX ) \
  ([&]() -> TEfficiency& { \
    static std::vector< std::pair<std::string, TEfficiency*>> cache; \
    for ( auto & [c, h]: cache) \
      if ( c == HistContext::current() ) return *h; \
    cache.emplace_back( HistContext::current(), this->effreg( new TEfficiency(HistContext::name(__NAME),__TITLE,__XBINS,__XMIN,__XMAX))); \
    static std::string name = __NAME; \
    assure( name.empty() or name == __NAME, std::string("Histograms names consitency in ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    return *cache.back().second; \
  }())

#define EFF2( __NAME,__TITLE,__XBINS,__XMIN,__XMAX,__YBINS,__YMIN,__YMAX ) \
  ([&]() -> TEfficiency& { \
    static std::vector< std::pair<std::string, TEfficiency*>> cache; \
    for ( auto & [c, h]: cache) \
      if ( c == HistContext::current() ) return *h; \
    cache.emplace_back( HistContext::current(), this->effreg( new TEfficiency(HistContext::name(__NAME),__TITLE,__XBINS,__XMIN,__XMAX,__YBINS,__YMIN,__YMAX))); \
    static std::string name = __NAME; \
    assure( name.empty() or name == __NAME, std::string("Histograms names consitency in ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    return *cache.back().second; \
  }())


#define EFF1V( __NAME,__TITLE,__VEC ) \
  ([&]() -> TEfficiency& { \
    static std::vector< std::pair<std::string, TEfficiency*>> cache; \
    for ( auto & [c, h]: cache) \
      if ( c == HistContext::current() ) return *h; \
    cache.emplace_back( HistContext::current(), this->effreg( new TEfficiency(HistContext::name(__NAME),__TITLE,__VEC.size()-1,__VEC.data()))); \
    static std::string name = __NAME; \
    assure( name.empty() or name == __NAME, std::string("Histograms names consitency in ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    return *cache.back().second; \
  }())


#define HIST2( __NAME,__TITLE,__XBINS,__XMIN,__XMAX,__YBINS,__YMIN,__YMAX ) \
  ([&]() -> TH2D& { \
    static std::vector< std::pair<std::string, TH2D*>> cache; \
    for ( auto & [c, h]: cache) \
      if ( c == HistContext::current() ) return *h; \
    cache.emplace_back( HistContext::current(), this->h2reg( new TH2D( HistContext::name(__NAME),__TITLE,__XBINS,__XMIN,__XMAX,__YBINS,__YMIN,__YMAX )) ); \
    static std::string name = __NAME; \
    assure( name.empty() or name == __NAME, std::string("Histograms names consitency in ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    return *cache.back().second; \
  }())


#endif
