// Copyright 2022, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)
#ifndef assure_h
#define assure_h
#include <string>

void assure( bool cond, const std::string_view& message, bool silentIfok=false);
void assure( bool cond, const std::string_view& messageOK, const std::string_view& messageFAIL);
void report( const std::string_view& message);
void missing( const std::string_view& message);
void assure_about_equal(const std::string_view& msg, double a, double b, double tolerance=0.01); // test if the values are equal within 1%, if not acts as an assure

#endif //assure_h
