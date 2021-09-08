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

#define HCONTEXT(__CTX__) HistContext __hist_context(__CTX__);


#endif
