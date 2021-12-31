#ifndef futils_h
#define futils_h


// macro generating generic, single agument, returning lambda
// example use: filter( F(_ < 0)) - the _ is the lambda argument
#define F(CODE) [&](const auto &_) { return CODE; }
// subroutine (nothing is returned)
#define S(CODE) [&](const auto &_) { CODE; }

#endif