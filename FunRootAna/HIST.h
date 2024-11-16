// Copyright 2022, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)
#ifndef HIST_H
#define HIST_H
#include <map>
#include <list>
#include <iostream>
#include <string>
#include <string>

#include "TH1.h"
#include "TEfficiency.h"
#include "TH2.h"
#include "TH3.h"
#include "TProfile.h"
#include "TProfile2D.h"
#include "TGraph.h"
#include "TGraph2D.h"

#include "LazyFunctionalVector.h"
#include "Weights.h"
#include "assure.h"

using namespace std::string_literals;
struct HistContext {
  HistContext(const std::string_view str, const std::string_view file="", int line=0);

  ~HistContext() {
    if ( s_latest != nullptr )
      s_latest = s_latest->prev();
  }

  // produce full histogram name
  static std::string name( const std::string& n ) {
    return s_latest->current()+n;
  }
  static std::string current() {
    return  s_latest->text();
  }

  static size_t currentHash() {
    return  s_latest->m_hash;
  }


  static bool sameAsCurrent( size_t contextHash) {
    return contextHash == s_latest->m_hash;
  }

  inline std::string text() const {
    return  prev() != nullptr ? prev()->text()+std::string(m_text) : std::string(m_text);
  }
  inline const HistContext* prev() const {
    return m_prev;
  }
  private:
  const static HistContext* s_latest;
  static std::map<std::string, std::pair<std::string, int> > s_contexts;

  const HistContext* m_prev;

  std::string m_text;
  size_t m_hash;
};

// histograms context switch (can be ever used only in one place in the program)
#define HCONTEXT(__CTX__) HistContext __hist_context(__CTX__, __FILE__, __LINE__);

// reusable histograms context (can be reused)
#define REHCONTEXT(__CTX__) HistContext __hist_context(__CTX__);


class HandyHists {
 public:
  HandyHists() {
  }
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
  template<typename T>
  T* namedreg( T* h) {
    assure( not lfv::lazy_view(m_named)
            .contains( [h]( const TNamed* in){ return std::string(h->GetName()) == in->GetName();}  ),
              std::string("Cant have two TGraph of the same name ") + h->GetName(), true );

    m_named.push_back( h );
    return h;
  }

  // accessors
  void foreach_histogram(std::function<void(TH1*)> fun);
  void foreach_efficiency(std::function<void(TEfficiency*)> fun);
  void foreach_profile(std::function<void(TProfile*)> fun);

  void save( const std::string& fname);

 protected:
  std::list<TH1*> m_h;
  std::list<TEfficiency*> m_eff;
  std::list<TProfile*> m_prof;
  std::list<TNamed*> m_named;
};


// Here be dragons, these macros can be used in methods of the class inheriting from HandyHists,
// they are capable of exposing histograms to be filled or crate them it if need be
// Each histogram is context aware, that is it has a name that depends on the concatenated contexts (made by HCONTEXT macro)
// The histogram created in a particular line are cached locally so the lookup (if needed at all) is fast,
// therefore this functions are static and generated for every line of source code where the histogram is defined
// Why macro? it can conveniently catch line number.

// The innerworkings are as follows:
// - static (thus specific to each generated histogram type) cache is declared
// - identifying hash is generated and checked (it contains the context info)
// - if histogram is missing in the cache, it is then booked, registered for saving ...
// All these macros&templates are very similar (modulo types - and arity specific to the histogram type).



#define HIST1( __NAME,__TITLE,__XBINS,__XMIN,__XMAX ) \
  ([&]() -> TH1D& { \
    static std::vector< std::pair<size_t, TH1D*>> cache; \
    for ( auto & [contextHash, histogram]: cache) \
      if ( HistContext::sameAsCurrent(contextHash) ) return *histogram; \
    assure(__XMIN < __XMAX, "Bin limits ordered incorrectly in "s+__NAME, true); \
    cache.emplace_back( HistContext::currentHash(), this->hreg ( new TH1D( HistContext::name(__NAME).c_str(), __TITLE,__XBINS,__XMIN,__XMAX))); \
    static std::string name = __NAME; \
    assure( name.empty() or name == __NAME, std::string("Histograms defined in the same line can't be different, use HCONTEXT instead, issue in: ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    return *cache.back().second; \
  }())


#define HIST1V( __NAME,__TITLE,__VEC ) \
  ([&]() -> TH1& { \
    static std::vector< std::pair<size_t, TH1*>> cache; \
    for ( auto & [contextHash, histogram]: cache) \
      if ( HistContext::sameAsCurrent(contextHash) ) return *histogram; \
    cache.emplace_back( HistContext::currentHash(), this->hreg ( new TH1D( HistContext::name(__NAME).c_str(),__TITLE,__VEC.size()-1,__VEC.data())) ); \
    static std::string name; \
    assure( name.empty() or name == __NAME, std::string("Histograms defined in the same line can't be different, use HCONTEXT instead, issue in: ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    name = __NAME; \
    return *cache.back().second; \
  }())


#define PROF1( __NAME,__TITLE,__XBINS,__XMIN,__XMAX ) \
  ([&]() -> TProfile& { \
    static std::vector< std::pair<size_t, TProfile*>> cache; \
    for ( auto & [contextHash, histogram]: cache) \
      if ( HistContext::sameAsCurrent(contextHash) ) return *histogram; \
    assure(__XMIN < __XMAX, "Bin limits ordered incorrectly in "s+__NAME, true); \
    cache.emplace_back( HistContext::currentHash() , this->profreg ( new TProfile(HistContext::name(__NAME).c_str(),__TITLE,__XBINS,__XMIN,__XMAX))); \
    static std::string name = __NAME; \
    assure( name.empty() or name == __NAME, std::string("Histograms defined in the same line can't be different, use HCONTEXT instead, issue in: ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    return *cache.back().second; \
  }())

#define PROF2( __NAME,__TITLE,__XBINS,__XMIN,__XMAX,__YBINS,__YMIN,__YMAX ) \
  ([&]() -> TProfile2D& { \
    static std::vector< std::pair<size_t, TProfile2D*>> cache; \
    for ( auto & [contextHash, histogram]: cache) \
      if ( HistContext::sameAsCurrent(contextHash) ) return *histogram; \
    assure(__XMIN < __XMAX, "Bin limits ordered incorrectly in "s+__NAME, true); \
    assure(__YMIN < __YMAX, "Bin Y limits ordered incorrectly in "s+__NAME, true); \
    cache.emplace_back( HistContext::currentHash() , this->hreg ( new TProfile2D(HistContext::name(__NAME).c_str(),__TITLE,__XBINS,__XMIN,__XMAX,__YBINS,__YMIN,__YMAX))); \
    static std::string name = __NAME; \
    assure( name.empty() or name == __NAME, std::string("Profile2D defined in the same line can't be different, use HCONTEXT instead, issue in: ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    return *cache.back().second; \
  }())


#define PROF1V( __NAME,__TITLE,__VEC ) \
  ([&]() -> TProfile& { \
    static std::vector< std::pair<size_t, TProfile*>> cache; \
    for ( auto & [contextHash, histogram]: cache) \
      if ( HistContext::sameAsCurrent(contextHash) ) return *histogram; \
    cache.emplace_back( HistContext::currentHash(),  this->profreg ( new TProfile(HistContext::name(__NAME).c_str(),__TITLE,__VEC.size()-1,__VEC.data()))); \
    static std::string name = __NAME; \
    assure( name.empty() or name == __NAME, std::string("Histograms defined in the same line can't be different, use HCONTEXT instead, issue in: ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    return *cache.back().second; \
  }())

#define PROF2V( __NAME,__TITLE,__VECX,__VECY ) \
  ([&]() -> TH1& { \
    static std::vector< std::pair<size_t, TProfile2D*>> cache; \
    for ( auto & [contextHash, histogram]: cache) \
      if ( HistContext::sameAsCurrent(contextHash) ) return *histogram; \
    cache.emplace_back( HistContext::currentHash(), this->hreg ( new TProfile2D( HistContext::name(__NAME).c_str(),__TITLE,__VECX.size()-1,__VECX.data(),__VECY.size()-1,__VECY.data())) ); \
    static std::string name; \
    assure( name.empty() or name == __NAME, std::string("TProfile2D defined in the same line can't be different, use HCONTEXT instead, issue in: ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    name = __NAME; \
    return *cache.back().second; \
  }())


#define EFF1( __NAME,__TITLE,__XBINS,__XMIN,__XMAX ) \
  ([&]() -> TEfficiency& { \
    static std::vector< std::pair<size_t, TEfficiency*>> cache; \
    for ( auto & [contextHash, histogram]: cache) \
      if ( HistContext::sameAsCurrent(contextHash) ) return *histogram; \
    assure(__XMIN < __XMAX, "Bin limits ordered incorrectly in "s+__NAME, true); \
    cache.emplace_back( HistContext::currentHash(), this->effreg( new TEfficiency(HistContext::name(__NAME).c_str(),__TITLE,__XBINS,__XMIN,__XMAX))); \
    static std::string name = __NAME; \
    assure( name.empty() or name == __NAME, std::string("Histograms defined in the same line can't be different, use HCONTEXT instead, issue in: ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    return *cache.back().second; \
  }())

#define EFF2( __NAME,__TITLE,__XBINS,__XMIN,__XMAX,__YBINS,__YMIN,__YMAX ) \
  ([&]() -> TEfficiency& { \
    static std::vector< std::pair<size_t, TEfficiency*>> cache; \
    for ( auto & [contextHash, histogram]: cache) \
      if ( HistContext::sameAsCurrent(contextHash) ) return *histogram; \
    assure(__XMIN < __XMAX, "Bin X limits ordered incorrectly in "s+__NAME, false); \
    assure(__YMIN < __YMAX, "Bin Y limits ordered incorrectly in "s+__NAME, false); \
    cache.emplace_back( HistContext::currentHash(), this->effreg( new TEfficiency(HistContext::name(__NAME).c_str(),__TITLE,__XBINS,__XMIN,__XMAX,__YBINS,__YMIN,__YMAX))); \
    static std::string name = __NAME; \
    assure( name.empty() or name == __NAME, std::string("Histograms defined in the same line can't be different, use HCONTEXT instead, issue in: ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    return *cache.back().second; \
  }())


#define EFF1V( __NAME,__TITLE,__VEC ) \
  ([&]() -> TEfficiency& { \
    static std::vector< std::pair<size_t, TEfficiency*>> cache; \
    for ( auto & [contextHash, histogram]: cache) \
      if ( HistContext::sameAsCurrent(contextHash) ) return *histogram; \
    cache.emplace_back( HistContext::currentHash(), this->effreg( new TEfficiency(HistContext::name(__NAME).c_str(),__TITLE,__VEC.size()-1,__VEC.data()))); \
    static std::string name = __NAME; \
    assure( name.empty() or name == __NAME, std::string("Histograms defined in the same line can't be different, use HCONTEXT instead, issue in: ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    return *cache.back().second; \
  }())

#define EFF2V( __NAME,__TITLE,__VECX,__VECY ) \
  ([&]() -> TEfficiency& { \
    static std::vector< std::pair<size_t, TEfficiency*>> cache; \
    for ( auto & [contextHash, histogram]: cache) \
      if ( HistContext::sameAsCurrent(contextHash) ) return *histogram; \
    cache.emplace_back( HistContext::currentHash(), this->effreg( new TEfficiency(HistContext::name(__NAME).c_str(),__TITLE,__VECX.size()-1,__VECX.data(), __VECY.size()-1,__VECY.data()))); \
    static std::string name = __NAME; \
    assure( name.empty() or name == __NAME, std::string("Histograms defined in the same line can't be different, use HCONTEXT instead, issue in: ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    return *cache.back().second; \
  }())


#define HIST2( __NAME,__TITLE,__XBINS,__XMIN,__XMAX,__YBINS,__YMIN,__YMAX ) \
  ([&]() -> TH2D& { \
    static std::vector< std::pair<size_t, TH2D*>> cache; \
    for ( auto & [contextHash, histogram]: cache) \
      if ( HistContext::sameAsCurrent(contextHash) ) return *histogram; \
    assure(__XMIN < __XMAX, "Bin X limits ordered incorrectly in "s+__NAME, true); \
    assure(__YMIN < __YMAX, "Bin Y limits ordered incorrectly in "s+__NAME, true); \
    cache.emplace_back( HistContext::currentHash(), this->hreg( new TH2D( HistContext::name(__NAME).c_str(),__TITLE,__XBINS,__XMIN,__XMAX,__YBINS,__YMIN,__YMAX )) ); \
    static std::string name = __NAME; \
    assure( name.empty() or name == __NAME, std::string("Histograms defined in the same line can't be different, use HCONTEXT instead, issue in: ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    return *cache.back().second; \
  }())

#define HIST2V( __NAME,__TITLE,__VECX,__VECY ) \
  ([&]() -> TH1& { \
    static std::vector< std::pair<size_t, TH2*>> cache; \
    for ( auto & [contextHash, histogram]: cache) \
      if ( HistContext::sameAsCurrent(contextHash) ) return *histogram; \
    cache.emplace_back( HistContext::currentHash(), this->hreg ( new TH2D( HistContext::name(__NAME).c_str(),__TITLE,__VECX.size()-1,__VECX.data(),__VECY.size()-1,__VECY.data())) ); \
    static std::string name; \
    assure( name.empty() or name == __NAME, std::string("Histograms defined in the same line can't be different, use HCONTEXT instead, issue in: ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    name = __NAME; \
    return *cache.back().second; \
  }())


#define HIST3( __NAME,__TITLE,__XBINS,__XMIN,__XMAX,__YBINS,__YMIN,__YMAX,__ZBINS,__ZMIN,__ZMAX ) \
  ([&]() -> TH3D& { \
    static std::vector< std::pair<size_t, TH3D*>> cache; \
    for ( auto & [contextHash, histogram]: cache) \
      if ( HistContext::sameAsCurrent(contextHash) ) return *histogram; \
    assure(__XMIN < __XMAX, "Bin X limits ordered incorrectly in "s+__NAME, true); \
    assure(__YMIN < __YMAX, "Bin Y limits ordered incorrectly in "s+__NAME, true); \
    assure(__ZMIN < __ZMAX, "Bin Y limits ordered incorrectly in "s+__NAME, true); \
    cache.emplace_back( HistContext::currentHash(), this->hreg( new TH3D( HistContext::name(__NAME).c_str(),__TITLE,__XBINS,__XMIN,__XMAX,__YBINS,__YMIN,__YMAX,__ZBINS,__ZMIN,__ZMAX )) ); \
    static std::string name = __NAME; \
    assure( name.empty() or name == __NAME, std::string("Histograms defined in the same line can't be different, use HCONTEXT instead, issue in: ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    return *cache.back().second; \
  }())

#define HIST3V( __NAME,__TITLE,__VECX,__VECY,__VECZ ) \
  ([&]() -> TH3D& { \
    static std::vector< std::pair<size_t, TH3D*>> cache; \
    for ( auto & [contextHash, histogram]: cache) \
      if ( HistContext::sameAsCurrent(contextHash) ) return *histogram; \
    cache.emplace_back( HistContext::currentHash(), this->hreg ( new TH3D( HistContext::name(__NAME).c_str(),__TITLE,__VECX.size()-1,__VECX.data(),__VECY.size()-1,__VECY.data(),__VECZ.size()-1,__VECZ.data())) ); \
    static std::string name; \
    assure( name.empty() or name == __NAME, std::string("Histograms defined in the same line can't be different, use HCONTEXT instead, issue in: ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    name = __NAME; \
    return *cache.back().second; \
  }())

#define GRAPH( __NAME,__TITLE ) \
  ([&]() -> TGraph& { \
    static std::vector< std::pair<size_t, TGraph*>> cache; \
    for ( auto & [contextHash, histogram]: cache) \
      if ( HistContext::sameAsCurrent(contextHash) ) return *histogram; \
    TGraph *g = new TGraph(); \
    g->SetName(HistContext::name(__NAME).c_str()); \
    g->SetTitle(__TITLE); \
    cache.emplace_back( HistContext::currentHash(), this->namedreg ( g ) ); \
    static std::string name; \
    assure( name.empty() or name == __NAME, std::string("Histograms defined in the same line can't be different, use HCONTEXT instead, issue in: ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    name = __NAME; \
    return *cache.back().second; \
  }())

#define GRAPH2( __NAME,__TITLE ) \
  ([&]() -> TGraph2D& { \
    static std::vector< std::pair<size_t, TGraph2D*>> cache; \
    for ( auto & [contextHash, histogram]: cache) \
      if ( HistContext::sameAsCurrent(contextHash) ) return *histogram; \
    TGraph2D *g = new TGraph2D(); \
    g->SetName(HistContext::name(__NAME).c_str()); \
    g->SetTitle(__TITLE); \
    cache.emplace_back( HistContext::currentHash(), this->namedreg ( g ) ); \
    static std::string name; \
    assure( name.empty() or name == __NAME, std::string("Histograms defined in the same line can't be different, use HCONTEXT instead, issue in: ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    name = __NAME; \
    return *cache.back().second; \
  }())

#endif
