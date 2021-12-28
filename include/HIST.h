#ifndef HIST_H
#define HIST_H
#include <map>
#include <iostream>
#include "TH1.h"
#include "TEfficiency.h"
#include "TH2.h"
#include "TProfile.h"

#include "Weights.h"
#include "assure.h"

constexpr uint32_t chash(const char* str) {
  uint32_t seed = 0xd2d84a61;
  for ( const char* c = str; *c != '\0'; ++c )
    seed = (seed >> 5) + *c + (seed << 7);
  return seed;
}

typedef uint64_t hcoord;

template<typename HISTCLASS>
struct WeightedHist {

  HISTCLASS *_h;
  WeightedHist(HISTCLASS* h) : _h(h) {}

  void wfill(double v) { _h->Fill( v, FillWeight::value()); }
  void wfill(double v1, double v2) { _h->Fill( v1, v2, FillWeight::value()); }
  void wfill(double v1, double v2, double v3 ) { _h->Fill( v1, v2, v3, FillWeight::value()); }
  void fill(double v) { _h->Fill( v ); }
  void fill(double v1, double v2) { _h->Fill( v1, v2); }
  void fill(double v1, double v2, double v3 ) { _h->Fill( v1, v2, v3); }

  HISTCLASS* operator->() { return _h; }
  const HISTCLASS* operator->() const { return _h; }

};

struct HistContext {
  HistContext(const std::string& str)  {
    m_prev = s_histContext;
    s_histContext += str;
    s_histContextNum = chash(s_histContext.c_str());
  }
  ~HistContext() {
    s_histContext = m_prev;
    s_histContextNum = chash(s_histContext.c_str());
  }
  static const char* name( const char* n ) {
    static std::string s{};
    s = s_histContext + n ;   
    return s.c_str();
  }
  static const std::string& current() {
    return s_histContext;
  }
  static hcoord context_key( uint32_t fileline ) {
    return (static_cast<uint64_t>(fileline) << 32) ^ HistContext::s_histContextNum;
  }

  static std::string s_histContext;
  static uint32_t s_histContextNum;
  std::string m_prev;
};

class HandyHists {
 public:
  template<typename H>
    H* reg( H* h ) {
    h->SetDirectory(0);
    //assure(true, std::string("Registered histogram: ") +h->ClassName() + " " + h->GetName() );
    return h;  
  }

  TH1* h1reg( TH1* h, hcoord place ) {
    //    assure(true, std::string("Registered histogram: ") +h->ClassName() + " " + h->GetName() + " defined in file::line " +std::to_string(place)  );
    //assure( true, std::string("Registering ") + h->GetName() + " on hash " + std::to_string(place) + " " + std::to_string(reinterpret_cast<uint64_t>(m_h1[place])) + " " + HistContext::current());
//    assure( m_h1[place] == nullptr, std::string("Already registered histogram ")+ /*m_h1[place]->GetName() +*/ "under the hash " +std::to_string(place) + " new " +h->GetName());
    m_h1[place] = reg ( h );
    return h;      
  }

  TEfficiency* effreg( TEfficiency* h, hcoord place ) {
  //    assure( m_eff[place] != nullptr, std::string("Already registered histogram "));// + m_eff[place]->GetName() + "under the hash " +std::to_string(place));
    m_eff[place] = reg ( h );
    return h;      
  }

  TH2* h2reg( TH2* h, hcoord place ) {
//    assure( m_h2[place] != nullptr, std::string("Already registered histogram "));// + m_h2[place]->GetName() + "under the hash " +std::to_string(place));
    m_h2[place] = reg ( h );    
    return h;      
  }

  TProfile* profreg( TProfile* h, hcoord place ) {
//    assure( m_h2[place] != nullptr, std::string("Already registered histogram "));// + m_h2[place]->GetName() + "under the hash " +std::to_string(place));
    m_prof[place] = reg ( h );    
    return h;      
  }

  void save( const std::string& fname);
  
 protected:
  std::map<hcoord, TH1*> m_h1;
  std::map<hcoord, TEfficiency*> m_eff;    
  std::map<hcoord, TH2*> m_h2;
  std::map<hcoord, TProfile*> m_prof;

private:

};

constexpr uint32_t filecoord( const char* file_name, unsigned line) { return chash(file_name) + line ; }
#define COORD(F, L) HistContext::context_key(filecoord(F, L))

#define USE_INLINE_HISTOGRAMS_CACHE

#ifdef USE_INLINE_HISTOGRAMS_CACHE

template<typename H>
void is_naming_consistent( const std::vector<std::tuple<hcoord, std::string, WeightedHist<H>>>& cache, const char* file, int line ) {
  if ( cache.empty() ) return;
  const std::string first_name = std::get<1>(cache.at(0));
  for ( auto & [hashedkey, name, wh]: cache)
    assure( first_name == name, std::string("Histograms booked in ") + file + " and " + std::to_string(line) + " change name from " + first_name + " to " + name + " this is not allowed, please use HCONTEXT switching instead", true);
}

// Here be dragons, these macros can be used in methods of the class inherting from HandyHists, 
// they are capable of exposing histogram (WeightedHist) to be filled and crate it if needed
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

#define HIST1( __NAME,__TITLE,__XBINS,__XMIN,__XMAX ) \
  ([&]() { \
    static std::vector< std::pair<hcoord, WeightedHist<TH1>>> cache; \
    hcoord current = COORD(__FILE__, __LINE__); \
    for ( auto & [c, h]: cache) \
      if ( c == current ) return h; \
    WeightedHist<TH1> newh = this->h1reg ( new TH1D( HistContext::name(__NAME), __TITLE,__XBINS,__XMIN,__XMAX), current); \
    cache.emplace_back( current, newh); \
    static std::string name; \
    assure( name.empty() or name == __NAME, std::string("Histograms names consitency in ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    name = __NAME; \
    return newh; \
  }())

#define HIST1V( __NAME,__TITLE,__VEC ) \
  ([&]() { \
    static std::vector< std::pair<hcoord, WeightedHist<TH1>>> cache; \
    hcoord current = COORD(__FILE__, __LINE__); \
    for ( auto & [c, h]: cache) \
      if ( c == current ) return h; \
    WeightedHist<TH1> newh = this->h1reg ( new TH1D( HistContext::name(__NAME),__TITLE,__VEC.size()-1,__VEC.data()), current); \
    cache.emplace_back( current, newh); \
    static std::string name; \
    assure( name.empty() or name == __NAME, std::string("Histograms names consitency in ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    name = __NAME; \
    return newh; \
  }())


#define PROF1( __NAME,__TITLE,__XBINS,__XMIN,__XMAX ) \
  ([&]() { \
    static std::vector< std::pair<hcoord, WeightedHist<TProfile>>> cache; \
    hcoord current = COORD(__FILE__, __LINE__); \
    for ( auto & [c, h]: cache) \
      if ( c == current ) return h; \
    WeightedHist<TProfile> newh = this->profreg ( new TProfile(HistContext::name(__NAME),__TITLE,__XBINS,__XMIN,__XMAX), current); \
    cache.emplace_back( current, newh); \
    static std::string name; \
    assure( name.empty() or name == __NAME, std::string("Histograms names consitency in ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    name = __NAME; \
    return newh; \
  }())

#define PROF1V( __NAME,__TITLE,__VEC ) \
  ([&]() { \
    static std::vector< std::pair<hcoord, WeightedHist<TProfile>>> cache; \
    hcoord current = COORD(__FILE__, __LINE__); \
    for ( auto & [c, h]: cache) \
      if ( c == current ) return h; \
    WeightedHist<TProfile> newh = this->profreg ( new TProfile(HistContext::name(__NAME),__TITLE,__VEC.size()-1,__VEC.data()), current); \
    cache.emplace_back( current, newh); \
    static std::string name; \
    assure( name.empty() or name == __NAME, std::string("Histograms names consitency in ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    name = __NAME; \
    return newh; \
  }())


#define EFF1( __NAME,__TITLE,__XBINS,__XMIN,__XMAX ) \
  ([&]() { \
    static std::vector< std::pair<hcoord, WeightedHist<TEfficiency>>> cache; \
    hcoord current = COORD(__FILE__, __LINE__); \
    for ( auto & [c, h]: cache) \
      if ( c == current ) return h; \
    WeightedHist<TEfficiency> newh = this->effreg( new TEfficiency(HistContext::name(__NAME),__TITLE,__XBINS,__XMIN,__XMAX), current); \
    cache.emplace_back( current, newh); \
    static std::string name; \
    assure( name.empty() or name == __NAME, std::string("Histograms names consitency in ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    name = __NAME; \
    return newh; \
  }())


#define EFF1V( __NAME,__TITLE,__VEC ) \
  ([&]() { \
    static std::vector< std::pair<hcoord, WeightedHist<TEfficiency>>> cache; \
    hcoord current = COORD(__FILE__, __LINE__); \
    for ( auto & [c, h]: cache) \
      if ( c == current ) return h; \
    WeightedHist<TEfficiency> newh = this->effreg( new TEfficiency(HistContext::name(__NAME),__TITLE,__VEC.size()-1,__VEC.data()), current); \
    cache.emplace_back( current, newh); \
    static std::string name; \
    assure( name.empty() or name == __NAME, std::string("Histograms names consitency in ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    name = __NAME; \
    return newh; \
  }())


#define HIST2( __NAME,__TITLE,__XBINS,__XMIN,__XMAX,__YBINS,__YMIN,__YMAX ) \
  ([&]() { \
    static std::vector< std::pair<hcoord, WeightedHist<TH2>>> cache; \
    hcoord current = COORD(__FILE__, __LINE__); \
    for ( auto & [c, h]: cache) \
      if ( c == current ) return h; \
    WeightedHist<TH2> newh = this->h2reg( new TH2D( HistContext::name(__NAME),__TITLE,__XBINS,__XMIN,__XMAX,__YBINS,__YMIN,__YMAX ), current); \
    cache.emplace_back( current, newh ); \
    static std::string name; \
    assure( name.empty() or name == __NAME, std::string("Histograms names consitency in ") + __FILE__ + ":" + std::to_string(__LINE__), true); \
    name = __NAME; \
    return newh; \
  }())


#else
// simpler form of histograms management:
// histograms are maintained in the HistHelper maps, the lookup may thus be slower
#define HIST1( __NAME,__TITLE,__XBINS,__XMIN,__XMAX ) \
  WeightedHist( m_h1[ COORD(__FILE__, __LINE__) ]\
  ? m_h1[ COORD(__FILE__, __LINE__) ] \
  : h1reg( new TH1D( HistContext::name(__NAME),__TITLE,__XBINS,__XMIN,__XMAX), COORD(__FILE__, __LINE__) ) )

#define HIST1V( __NAME,__TITLE,__VEC ) \
  WeightedHist( m_h1[ COORD(__FILE__, __LINE__) ] \
  ? m_h1[ COORD(__FILE__, __LINE__) ] \
  : h1reg( new TH1D( HistContext::name(__NAME),__TITLE,__VEC.size()-1,__VEC.data()), COORD(__FILE__, __LINE__) ) )

#define PROF1( __NAME,__TITLE,__XBINS,__XMIN,__XMAX ) \
  WeightedHist( m_prof[COORD(__FILE__, __LINE__)] \
  ? m_prof[COORD(__FILE__, __LINE__)] \
  : profreg( new TProfile(HistContext::name(__NAME),__TITLE,__XBINS,__XMIN,__XMAX), COORD(__FILE__, __LINE__) ) )

#define PROF1V( __NAME,__TITLE,__VEC ) \
  WeightedHist( m_prof[COORD(__FILE__, __LINE__)] \
  ? m_prof[COORD(__FILE__, __LINE__)] \
  : profreg( new TProfile(HistContext::name(__NAME),__TITLE,__VEC.size()-1,__VEC.data()), COORD(__FILE__, __LINE__) ) )


#define EFF1( __NAME,__TITLE,__XBINS,__XMIN,__XMAX ) \
  WeightedHist( m_eff[COORD(__FILE__, __LINE__)] \
  ? m_eff[COORD(__FILE__, __LINE__)] \
  : effreg( new TEfficiency(HistContext::name(__NAME),__TITLE,__XBINS,__XMIN,__XMAX), COORD(__FILE__, __LINE__) ) )

#define EFF1V( __NAME,__TITLE,__VEC ) \
  WeightedHist( m_eff[COORD(__FILE__, __LINE__)] \
  ? m_eff[COORD(__FILE__, __LINE__)]  \
  : effreg( new TEfficiency(HistContext::name(__NAME),__TITLE,__VEC.size()-1,__VEC.data()), COORD(__FILE__, __LINE__) ) )

#define HIST2( __NAME,__TITLE,__XBINS,__XMIN,__XMAX,__YBINS,__YMIN,__YMAX ) \
  WeightedHist( m_h2[COORD(__FILE__, __LINE__)] \
  ? m_h2[COORD(__FILE__, __LINE__)] \
  : h2reg( new TH2D( HistContext::name(__NAME),__TITLE,__XBINS,__XMIN,__XMAX,__YBINS,__YMIN,__YMAX ), COORD(__FILE__, __LINE__) ) )


#define ANYHIST ( __new__ ) \
  WeightedHist( m_h1[COORD(__FILE__, __LINE__)] \
  ? m_h1[COORD(__FILE__, __LINE__)] \
  : hreg(  __new__ , COORD(__FILE__, __LINE__) ) ) \
  m_h1[COORD(__FILE__, __LINE__)].SetName( HistContext::name(m_h1[COORD(__FILE__, __LINE__)].GetName()) )
#endif

#define HCONTEXT(__CTX__) HistContext __hist_context(__CTX__);

#endif
