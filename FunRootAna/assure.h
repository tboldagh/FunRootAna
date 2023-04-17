// Copyright 2022, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)
#ifndef assure_h
#define assure_h
#include <string>

void assure( bool cond, const std::string& message, bool silentIfok=false);
void assure( bool cond, const std::string& messageOK, const std::string& messageFAIL);
void report( const std::string& message);
void missing( const std::string& message);
void assure_about_equal(const std::string& msg, double a, double b, double tolerance=0.01); // test if the values are equal within 1%, if not acts as an assure

#endif //assure_h
