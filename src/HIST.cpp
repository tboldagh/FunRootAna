// Copyright 2022, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)
#include "TFile.h"
#include "HIST.h"

  HistContext::HistContext(const std::string_view str, const std::string_view file, int line)
    : m_prev(s_latest),
      m_text(str) {
      s_latest = this;
      m_hash = std::hash<std::string_view>{}(m_text)  ^ ( m_prev  ? m_prev->m_hash : 0l);
      auto there = s_contexts.find(std::string(str));
      if ( there != s_contexts.end() ) {
        if ( there->second.first != file )
          assure(false, std::string("Same context ") + std::string(str) + " used in different files " + there->second.first + " " + std::string(file));

        if(there->second.second != line)
           assure(false, std::string("Same context ") + std::string(str) + " used in different lines " + std::to_string(there->second.second) + " " + std::to_string(line), true);
      } else {
        s_contexts[std::string(str)] =  std::make_pair(std::string(file), line);
      }
  }


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
std::map<std::string, std::pair<std::string, int> > HistContext::s_contexts;
static const HistContext topContext ("");