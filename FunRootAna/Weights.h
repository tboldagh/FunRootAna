// Copyright 2024, Tomasz Bold
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

#endif