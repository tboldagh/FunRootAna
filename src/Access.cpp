// Copyright 2022, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)
#include "Access.h"

#include <string>
#include <limits>
#include <type_traits>
#include "TTree.h"
#include "TLeaf.h"
#include "Math/Vector4D.h"
#include "TLorentzVector.h"
#include "assure.h"



std::vector<TLorentz> Access::tlget( const std::string& brprefix ) {
  std::vector<TLorentz> result;
  fillbr<std::vector<float>>ptBr ( m_tree, brprefix + "_pt" , m_current);
  fillbr<std::vector<float>>etaBr( m_tree, brprefix + "_eta", m_current);
  fillbr<std::vector<float>>phiBr( m_tree, brprefix + "_phi", m_current);
  result.reserve(ptBr.data->size());
  for ( size_t i= 0, end = ptBr.data->size(); i != end; ++i )  {
    TLorentz tl;
    tl.SetPtEtaPhiM( ptBr.at(i), etaBr.at(i), phiBr.at(i), 0.0);
      result.push_back( tl );
  }
  return result;
}

