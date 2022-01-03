// Copyright 2022, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)
#ifndef Weights_H
#define Weights_H
#include "stdexcept"

struct FillWeight {
  static double m_w;
  static void set( double w) { m_w = w; }
  static double value() { return m_w; }
};
struct WeightRAI{
  double _old = {0};
  WeightRAI(double w) {
    _old = FillWeight::value();
    FillWeight::set( _old * w );    
  }
  ~WeightRAI() {
    FillWeight::set(_old);
  }
  [[nodiscard]] static WeightRAI weight(double w) {
   return WeightRAI(w);  
  }
};
#define WEIGHT(_value) auto WEIGHT = WeightRAI::weight(_value);

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