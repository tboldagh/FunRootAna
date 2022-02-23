// Copyright 2022, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)
#ifndef Weights_H
#define Weights_H
#include "stdexcept"

struct Weight {
  static double m_w;
  static void set(double w) { m_w = w; }
  static double value() { return m_w; }
};

struct MultWeightRAI{
  double _old = {0};
  MultWeightRAI(double w) {
    _old = Weight::value();
    Weight::set( _old * w );    
  }
  ~MultWeightRAI() {
    Weight::set(_old);
  }
  [[nodiscard]] static MultWeightRAI weight(double w) {
   return MultWeightRAI(w);  
  }
};

struct AbsWeightRAI{
  double _old = {0};
  AbsWeightRAI(double w) {
    _old = Weight::value();
    Weight::set( w );    
  }
  ~AbsWeightRAI() {
    Weight::set(_old);
  }
  [[nodiscard]] static AbsWeightRAI weight(double w) {
   return AbsWeightRAI(w);  
  }
};

#define WEIGHT Weight::value()
#define UPDATE_MULT_WEIGHT(_value) auto _WEIGHT_MULT = MultWeightRAI::weight(_value);
#define UPDATE_ABS_WEIGHT(_value) auto _WEIGHT_ABS = AbsWeightRAI::weight(_value);


// helper to streamline selection from set of values according to a boolean condition
// e.g use: double value  = option(condA, 0.2)
//                         .option(condB, 0.3)
//                         .option(condC, 1.1)
//                         .option(not condB, 0.9).select();
// raises, if no condition is satisfied
template<typename T = double>
struct Selector {
    Selector<T> option( bool cond, T value) {
        if (not _selected and cond ) {
            _value = value;
            _selected = true;
        }
        return *this;
    }
    Selector<T> option( T value ) {
        if ( not _selected ) {
            _value = value;
            _selected = true;
        }
        return *this;
    }

    T select(){ if ( not _selected ) throw std::runtime_error("No option is selected"); return _value; }
    T _value = {};
    bool _selected = {false};
};

template<typename T = double>
Selector<T> option(bool cond, double value) {
    Selector<T> s;
    s.option(cond, value);
    return s;
}



#endif