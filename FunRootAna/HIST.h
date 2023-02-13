// Copyright 2022, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)
#pragma once
#include <map>
#include <list>
#include <iostream>
#include <string>
#include "TH1.h"
#include "TEfficiency.h"
#include "TH2.h"
#include "TH3.h"
#include "TProfile.h"
#include "LazyFunctionalVector.h"
#include "Weights.h"
#include "assure.h"

using namespace std::string_literals;
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
  template<typename H>
  H* hreg( H* h) {
    assure( not lfv::lazy_view(m_h)
            .contains( [h]( const TH1* in){ return std::string(h->GetName()) == in->GetName();}  ), 
                "Cant have two TH1 of the same name "s + h->GetName(), true );

    m_h.push_back(reg ( h ));
    return h;      
  }

  TEfficiency* effreg( TEfficiency* h) {
    assure( not lfv::lazy_view(m_eff)
            .contains( [h]( const TEfficiency* in){ return std::string(h->GetName()) == in->GetName();}  ), 
              std::string("Cant have two TEfficiency of the same name ") + h->GetName(), true );

    m_eff.push_back(reg ( h ));
    return h;      
  }

  TProfile* profreg( TProfile* h) {
    assure( not lfv::lazy_view(m_prof)
            .contains( [h]( const TProfile* in){ return std::string(h->GetName()) == in->GetName();}  ), 
              std::string("Cant have two TProfile of the same name ") + h->GetName(), true );

    m_prof.push_back(reg ( h ));    
    return h;      
  }

  void save( const std::string& fname);
  
 protected:
  std::list<TH1*> m_h;
  std::list<TEfficiency*> m_eff;    
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
    assure(__XMIN < __XMAX, "Bin limits ordered incorrectly in "s+__NAME, true); \
    cache.emplace_back( HistContext::current(), this->hreg ( new TH1D( HistContext::name(__NAME), __TITLE,__XBINS,__XMIN,__XMAX))); \
    static std::string name = __NAME; \
    assure( name.empty() or name == __NAME, std::string("Histograms defined in the same line can't be different, use HCONTEXT instead, issue in: ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    return *cache.back().second; \
  }())


#define HIST1V( __NAME,__TITLE,__VEC ) \
  ([this]() -> TH1& { \
    static std::vector< std::pair<std::string, TH1*>> cache; \
    for ( auto & [c, h]: cache) \
      if ( c == HistContext::current() ) return *h; \
    cache.emplace_back( HistContext::current(), this->hreg ( new TH1D( HistContext::name(__NAME),__TITLE,__VEC.size()-1,__VEC.data())) ); \
    static std::string name; \
    assure( name.empty() or name == __NAME, std::string("Histograms defined in the same line can't be different, use HCONTEXT instead, issue in: ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    name = __NAME; \
    return *cache.back().second; \
  }())


#define PROF1( __NAME,__TITLE,__XBINS,__XMIN,__XMAX ) \
  ([this]() -> TProfile& { \
    static std::vector< std::pair<std::string, TProfile*>> cache; \
    for ( auto & [c, h]: cache) \
      if ( c == HistContext::current() ) return *h; \
    assure(__XMIN < __XMAX, "Bin limits ordered incorrectly in "s+__NAME, true); \
    cache.emplace_back( HistContext::current() , this->profreg ( new TProfile(HistContext::name(__NAME),__TITLE,__XBINS,__XMIN,__XMAX))); \
    static std::string name = __NAME; \
    assure( name.empty() or name == __NAME, std::string("Histograms defined in the same line can't be different, use HCONTEXT instead, issue in: ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    return *cache.back().second; \
  }())

#define PROF1V( __NAME,__TITLE,__VEC ) \
  ([this]() -> TProfile& { \
    static std::vector< std::pair<std::string, TProfile*>> cache; \
    for ( auto & [c, h]: cache) \
      if ( c == HistContext::current() ) return *h; \
    cache.emplace_back( HistContext::current(),  this->profreg ( new TProfile(HistContext::name(__NAME),__TITLE,__VEC.size()-1,__VEC.data()))); \
    static std::string name = __NAME; \
    assure( name.empty() or name == __NAME, std::string("Histograms defined in the same line can't be different, use HCONTEXT instead, issue in: ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    return *cache.back().second; \
  }())


#define EFF1( __NAME,__TITLE,__XBINS,__XMIN,__XMAX ) \
  ([&]() -> TEfficiency& { \
    static std::vector< std::pair<std::string, TEfficiency*>> cache; \
    for ( auto & [c, h]: cache) \
      if ( c == HistContext::current() ) return *h; \
    assure(__XMIN < __XMAX, "Bin limits ordered incorrectly in "s+__NAME, false); \
    cache.emplace_back( HistContext::current(), this->effreg( new TEfficiency(HistContext::name(__NAME),__TITLE,__XBINS,__XMIN,__XMAX))); \
    static std::string name = __NAME; \
    assure( name.empty() or name == __NAME, std::string("Histograms defined in the same line can't be different, use HCONTEXT instead, issue in: ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    return *cache.back().second; \
  }())

#define EFF2( __NAME,__TITLE,__XBINS,__XMIN,__XMAX,__YBINS,__YMIN,__YMAX ) \
  ([&]() -> TEfficiency& { \
    static std::vector< std::pair<std::string, TEfficiency*>> cache; \
    for ( auto & [c, h]: cache) \
      if ( c == HistContext::current() ) return *h; \
    assure(__XMIN < __XMAX, "Bin X limits ordered incorrectly in "s+__NAME, true); \
    assure(__YMIN < __YMAX, "Bin Y limits ordered incorrectly in "s+__NAME, true); \
    cache.emplace_back( HistContext::current(), this->effreg( new TEfficiency(HistContext::name(__NAME),__TITLE,__XBINS,__XMIN,__XMAX,__YBINS,__YMIN,__YMAX))); \
    static std::string name = __NAME; \
    assure( name.empty() or name == __NAME, std::string("Histograms defined in the same line can't be different, use HCONTEXT instead, issue in: ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    return *cache.back().second; \
  }())


#define EFF1V( __NAME,__TITLE,__VEC ) \
  ([&]() -> TEfficiency& { \
    static std::vector< std::pair<std::string, TEfficiency*>> cache; \
    for ( auto & [c, h]: cache) \
      if ( c == HistContext::current() ) return *h; \
    cache.emplace_back( HistContext::current(), this->effreg( new TEfficiency(HistContext::name(__NAME),__TITLE,__VEC.size()-1,__VEC.data()))); \
    static std::string name = __NAME; \
    assure( name.empty() or name == __NAME, std::string("Histograms defined in the same line can't be different, use HCONTEXT instead, issue in: ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    return *cache.back().second; \
  }())

#define EFF2V( __NAME,__TITLE,__VECX,__VECY ) \
  ([&]() -> TEfficiency& { \
    static std::vector< std::pair<std::string, TEfficiency*>> cache; \
    for ( auto & [c, h]: cache) \
      if ( c == HistContext::current() ) return *h; \
    cache.emplace_back( HistContext::current(), this->effreg( new TEfficiency(HistContext::name(__NAME),__TITLE,__VECX.size()-1,__VECX.data(), __VECY.size()-1,__VECY.data()))); \
    static std::string name = __NAME; \
    assure( name.empty() or name == __NAME, std::string("Histograms defined in the same line can't be different, use HCONTEXT instead, issue in: ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    return *cache.back().second; \
  }())


#define HIST2( __NAME,__TITLE,__XBINS,__XMIN,__XMAX,__YBINS,__YMIN,__YMAX ) \
  ([&]() -> TH2D& { \
    static std::vector< std::pair<std::string, TH2D*>> cache; \
    for ( auto & [c, h]: cache) \
      if ( c == HistContext::current() ) return *h; \
    assure(__XMIN < __XMAX, "Bin X limits ordered incorrectly in "s+__NAME, true); \
    assure(__YMIN < __YMAX, "Bin Y limits ordered incorrectly in "s+__NAME, true); \
    cache.emplace_back( HistContext::current(), this->hreg( new TH2D( HistContext::name(__NAME),__TITLE,__XBINS,__XMIN,__XMAX,__YBINS,__YMIN,__YMAX )) ); \
    static std::string name = __NAME; \
    assure( name.empty() or name == __NAME, std::string("Histograms defined in the same line can't be different, use HCONTEXT instead, issue in: ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    return *cache.back().second; \
  }())

#define HIST2V( __NAME,__TITLE,__VECX,__VECY ) \
  ([this]() -> TH1& { \
    static std::vector< std::pair<std::string, TH2*>> cache; \
    for ( auto & [c, h]: cache) \
      if ( c == HistContext::current() ) return *h; \
    cache.emplace_back( HistContext::current(), this->hreg ( new TH2D( HistContext::name(__NAME),__TITLE,__VECX.size()-1,__VECX.data(),__VECY.size()-1,__VECY.data())) ); \
    static std::string name; \
    assure( name.empty() or name == __NAME, std::string("Histograms defined in the same line can't be different, use HCONTEXT instead, issue in: ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    name = __NAME; \
    return *cache.back().second; \
  }())


#define HIST3( __NAME,__TITLE,__XBINS,__XMIN,__XMAX,__YBINS,__YMIN,__YMAX,__ZBINS,__ZMIN,__ZMAX ) \
  ([&]() -> TH3D& { \
    static std::vector< std::pair<std::string, TH3D*>> cache; \
    for ( auto & [c, h]: cache) \
      if ( c == HistContext::current() ) return *h; \
    assure(__XMIN < __XMAX, "Bin X limits ordered incorrectly in "s+__NAME, true); \
    assure(__YMIN < __YMAX, "Bin Y limits ordered incorrectly in "s+__NAME, true); \
    assure(__ZMIN < __ZMAX, "Bin Y limits ordered incorrectly in "s+__NAME, true); \
    cache.emplace_back( HistContext::current(), this->hreg( new TH3D( HistContext::name(__NAME),__TITLE,__XBINS,__XMIN,__XMAX,__YBINS,__YMIN,__YMAX,__ZBINS,__ZMIN,__ZMAX )) ); \
    static std::string name = __NAME; \
    assure( name.empty() or name == __NAME, std::string("Histograms defined in the same line can't be different, use HCONTEXT instead, issue in: ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    return *cache.back().second; \
  }())

#define HIST3V( __NAME,__TITLE,__VECX,__VECY,__VECZ ) \
  ([this]() -> TH3D& { \
    static std::vector< std::pair<std::string, TH3D*>> cache; \
    for ( auto & [c, h]: cache) \
      if ( c == HistContext::current() ) return *h; \
    cache.emplace_back( HistContext::current(), this->hreg ( new TH3D( HistContext::name(__NAME),__TITLE,__VECX.size()-1,__VECX.data(),__VECY.size()-1,__VECY.data(),__VECZ.size()-1,__VECZ.data())) ); \
    static std::string name; \
    assure( name.empty() or name == __NAME, std::string("Histograms defined in the same line can't be different, use HCONTEXT instead, issue in: ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    name = __NAME; \
    return *cache.back().second; \
  }())

