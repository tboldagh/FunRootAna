// Copyright 2022, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)
#include "TFile.h"
#include "HIST.h"


namespace {

std::pair<TDirectory*, std::string> dirForName(TFile* f, const std::string& name){
  if ( name.find('/') == std::string::npos ) {// no subdirectory designation, to be saved in the file main subdir
  return std::make_pair(f, name);
  }

  std::string dirName( name.substr(0, name.find_last_of('/')) );
  std::string oName( name.substr(name.find_last_of('/')+1) );

  //  report("Saving " + std::string(name) +" in " + dirName + " renaming as " + oName);
  TDirectory* d = f->GetDirectory(dirName.c_str());
  if ( d == nullptr )
    d = f->mkdir(dirName.c_str());
  return std::make_pair(d, oName);
}

template<typename T>
void _save(TFile* f, T* o) {
  const std::string name(o->GetName());
  auto [dest, newname] = dirForName(f, name);
  o->SetName(newname.c_str());
  o->SetDirectory(dest);
}
}

void HandyHists::save(const std::string& fname) {
  TDirectory* current = gDirectory;
  TFile* f = TFile::Open(fname.c_str(), "NEW");
  assure(f != nullptr, "Opening output file " + fname);
  for (auto hptr : m_h) {
    _save(f, hptr);
  }

  for (auto hptr : m_eff) {
    _save(f, hptr);
  }

  for (auto hptr : m_prof) {
    _save(f, hptr);
  }

  for (auto g : m_named) {
    auto [dest, newname] = dirForName(f, g->GetName());
    g->SetName(newname.c_str());
    dest->cd();
    g->Write();
    f->cd();
  }

  f->Write();
  f->Close();
  gDirectory = current;
}

void HandyHists::foreach_histogram(std::function<void(TH1*)> fun) {
  for ( auto h: m_h )
    fun(h);
}

void HandyHists::foreach_efficiency(std::function<void(TEfficiency*)> fun) {
  for ( auto h: m_eff )
    fun(h);
}

void HandyHists::foreach_profile(std::function<void(TProfile*)> fun) {
  for ( auto h: m_prof )
    fun(h);
}


const HistContext* HistContext::s_latest = nullptr;
static const HistContext topContext ("");