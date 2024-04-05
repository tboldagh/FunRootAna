// Copyright 2022, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)
#include <stdlib.h>
#include <iostream>
#include <cmath>

#include "assure.h"
void assure( bool cond, const std::string_view& message, bool silentIfok) {
  if ( cond == false ) {
    std::cout << ".. ERROR " << message << "\n";
    throw std::runtime_error("Failed to assure conditions is true, see error");
  } else {
    if ( ! silentIfok )
      std::cout << "... OK   " << message << "\n";
  }
}

void assure( bool cond, const std::string_view& messageOK, const std::string_view& messageFAIL) {
  if ( cond == false ) {
    std::cout << ".. ERROR " << messageFAIL << "\n";
    exit(-1);
  } else {
      std::cout << "... OK   " << messageOK << "\n";
  }
}

void report( const std::string_view& message) {
  std::cout << ".... INFO " << message << "\n";
}

[[deprecated]] void missing(const std::string_view& message ) {
  assure(false, message);
  throw std::runtime_error(std::string(message));
}


void assure_about_equal(const std::string_view& msg, double a, double b, double tolerance ){
  if ( std::isnan(a) or std::isnan(b) ) {
    std::cout<< msg << " either the fist value: " + std::to_string(a) + " or the second value: " + std::to_string(b) + " are NaN" << std::endl;
    throw std::runtime_error("assure_about_equal NaN");

  }
  if ( (std::abs(a - b) / std::abs(b) ) > tolerance ) {
    std::cout<< msg << " the fist value: " + std::to_string(a) + " differ from the second value: " + std::to_string(b) + " by mode than the tolerance." << std::endl;
    throw std::runtime_error("assure_about_equal check failed");
  }
}