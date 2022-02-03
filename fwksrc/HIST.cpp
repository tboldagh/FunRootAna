// Copyright 2022, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)
#include "TFile.h"
#include "HIST.h"

#include <string_view>

namespace {
template<typename T>
void _save(TFile* f, T* o) {
  const std::string_view& name(o->GetName());
  if ( name.find('/') == std::string::npos ) {// no subdirectory designation, to be saved in the file main subdir
    o->SetDirectory(f);
    return;
  }

  std::string dirName( name.substr(0, name.find_last_of('/')) );
  std::string oName( name.substr(name.find_last_of('/')+1) );

  //  report("Saving " + std::string(name) +" in " + dirName + " renaming as " + oName);
  o->SetName(oName.c_str());
  TDirectory* d = f->GetDirectory(dirName.c_str());
  if ( d == nullptr )
    d = f->mkdir(dirName.c_str());
  o->SetDirectory(d);
}
}

void HandyHists::save(const std::string& fname) {
  TDirectory* current = gDirectory;
  TFile* f = TFile::Open(fname.c_str(), "NEW");
  assure(f != nullptr, "Opening output file " + fname);
  for (auto [hash, hptr] : m_h1) {
    _save(f, hptr);
  }

  for (auto [hash, hptr] : m_h2) {
    _save(f, hptr);
  }

  for (auto [hash, hptr] : m_eff) {
    _save(f, hptr);
  }

  for (auto [hash, hptr] : m_prof) {
    _save(f, hptr);
  }
  f->Write();
  f->Close();
  gDirectory = current;
}


std::string HistContext::s_histContext = "";
uint32_t HistContext::s_histContextNum = chash(HistContext::s_histContext.c_str());

