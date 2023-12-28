// Copyright 2022, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)
#ifndef filling_h
#define filling_h

#include <TH1.h>
#include <TH2.h>
#if ROOT_VERSION_CODE >= ROOT_VERSION(6, 26, 6)
#include <TGraph.h>
#include <TGraph2D.h>
// #define ALLOW_FILLING_TGRAPH
#endif

#include "HIST.h"
#include "Weights.h"
#include <TEfficiency.h>
#include <TProfile.h>
#include <TProfile2D.h>

#include "EagerFunctionalVector.h"
#include "LazyFunctionalVector.h"

// operators to streamline ROOT histograms filling
// - from PODs
// - eager functional vectors
// - lazy functional vectors

// fill from PODs
// 0.7 >> hist;

#define POD_FILL(_type)                                                        \
  double operator>>(_type v, TH1 &h) {                                         \
    h.Fill(v);                                                                 \
    return v;                                                                  \
  }
POD_FILL(double)
POD_FILL(float)
POD_FILL(int)
POD_FILL(unsigned int)
POD_FILL(long unsigned int)
POD_FILL(long int)

const std::string &operator>>(const std::string &v, TH1 &h) {
  h.Fill(v.c_str(), 1.0);
  return v;
}
// optionals are treated as PODs but if they are empty the fill is skipped
template <typename T>
const std::optional<T> &operator>>(const std::optional<T> &v, TH1 &h) {
  if (v.has_value())
    v.value() >> h;
  return v;
}

// code to handle tuples of values or optionals (any mix)
namespace detail {
template <typename T> struct has_value_helper {
  static bool check(const T &) { return true; }
};
template <typename T> struct has_value_helper<std::optional<T>> {
  static bool check(const T &v) { return v.has_value(); }
};
template <typename T> bool has_value(const T &v) {
  return has_value_helper<T>::check(v);
}

template <typename T> struct get_value_helper {
  static T get(const T &v) { return v; }
};

template <typename T> struct get_value_helper<std::optional<T>> {
  static typename T::value_type &get(const T &v) { return v.value(); }
};

template <typename T> auto get_value(const T &v) {
  return get_value_helper<T>::get(v);
}

template <typename C, typename H> const C &from_2(const C &v, H &h) {
  static_assert(std::tuple_size<C>::value == 2,
                "wrong dimension of operands, must be 2");
  auto &[x, y] = v;
  if (detail::has_value(x) and detail::has_value(y)) {
    h.Fill(detail::get_value(x), detail::get_value(y));
  }
  return v;
}

template <typename C, typename H> const C &from_3(const C &v, H &h) {
  static_assert(std::tuple_size<C>::value == 3,
                "wrong dimension of operands, must be 3");
  auto &[x, y, z] = v;
  if (detail::has_value(x) and detail::has_value(y) and detail::has_value(z)) {
    h.Fill(detail::get_value(x), detail::get_value(y), detail::get_value(z));
  }
  return v;
}

template <typename C, typename H> const C &from_4(const C &v, H &h) {
  static_assert(std::tuple_size<C>::value == 4,
                "wrong dimension of operands, must be 4");
  auto &[x, y, z, t] = v;
  if (detail::has_value(x) and detail::has_value(y) and detail::has_value(z) and
      detail::has_value(t)) {
    h.Fill(detail::get_value(x), detail::get_value(y),
           detail::get_value(z), detail::get_value(t));
  }
  return v;
}

} // namespace detail
// Fill histograms that can be filled from a pair of values, TH1 with weight,
// TH2 no-weight, TProfile
template <typename T, typename U, typename H>
const std::pair<T, U> &operator>>(const std::pair<T, U> &v, H &h) {
  return detail::from_2(v, h);
}
template <typename T, typename U, typename H>
const std::tuple<T, U> &operator>>(const std::tuple<T, U> &v, H &h) {
  return detail::from_2(v, h);
}
template <typename T, typename H>
const std::array<T, 2> &operator>>(const std::array<T, 2> &v, H &h) {
  return detail::from_2(v, h);
}

#ifdef ALLOW_FILLING_TGRAPH
template <typename T, typename U>
const std::pair<T, U> &operator>>(const std::pair<T, U> &v, TGraph &g) {
  g.AddPoint(v.first, v.second);
  return v;
}

template <typename T, typename U, typename V>
const triple<T, U, V> &operator>>(const triple<T, U, V> &v, TGraph2D &g) {
  g.AddPoint(v.first, v.second, v.third);
  return v;
}

template <typename T, typename U, typename V>
const std::tuple<T, U, V> &operator>>(const std::tuple<T, U, V> &v,
                                      TGraph2D &g) {
  g.AddPoint(std::get<0>(v), std::get<1>(v), std::get<2>(v));
  return v;
}

#endif

template <typename T, typename U, typename V>
const triple<T, U, V> &operator>>(const triple<T, U, V> &v, TProfile2D &h) {
  detail::from_3(std::make_tuple(v.first, v.second, v.third), h);
  return v;
}

template <typename B, typename T>
const std::pair<B, T> &operator>>(const std::pair<bool, T> &v, TEfficiency &h) {
  static_assert(std::is_same_v<B, bool>,
                "The first element of the pair has to be boolean value");
  if (h.GetDimension() == 2)
    throw std::runtime_error(
        "pair<bool, value> insufficient to fill 2D TEfficiency, triple<bool, "
        "y_value, x_value> is needed");
  return detail::from_2(v, h);
}

template <typename T, typename U, typename V, typename H>
const triple<T, U, V> &operator>>(const triple<T, U, V> &v, H &h) {
  detail::from_3(std::make_tuple(v.first, v.second, v.third), h);
  return v;
}

template <typename T, typename U, typename V, typename H>
const std::tuple<T, U, V> &operator>>(const std::tuple<T, U, V> &v, H &h) {
  return detail::from_3(v, h);
}

template <typename T, typename H>
const std::array<T, 3> &operator>>(const std::array<T, 3> &v, H &h) {
  return detail::from_3(v, h);
}


template <typename T, typename U, typename V, typename Z>
const std::tuple<T, U, V, Z> &operator>>(const std::tuple<T, U, V, Z> &v,
                                         TH3 &h) {
  return detail::from_4(v, h);
}

template <typename T, typename H>
const std::array<T, 4> &operator>>(const std::array<T, 4> &v, H &h) {
  return detail::from_4(v, h);
}


template <typename B, typename T, typename U>
const triple<B, T, U> &operator>>(const triple<B, T, U> &v, TEfficiency &h) {
  std::make_tuple(v.first, v.second, v.third) >> h;
  return v;
}

template <typename B, typename U, typename V>
const std::tuple<B, U, V> &operator>>(const std::tuple<B, U, V> &v,
                                      TEfficiency &h) {
  static_assert(std::is_same_v<B, bool>,
                "The first element of the triple has to be boolean value");
  if (h.GetDimension() == 2) {
    detail::from_3(v, h);
  } else if (h.GetDimension() == 1) {
    // missing handing optional
    h.FillWeighted(std::get<0>(v), std::get<2>(v),
                   std::get<1>(v)); // ROOT is inconsistent here
  }
  return v;
}

template <typename U, typename V, typename Z>
const std::tuple<bool, U, V, Z> &operator>>(const std::tuple<bool, U, V, Z> &v,
                                            TEfficiency &h) {
  if (h.GetDimension() == 2) {
    h.FillWeighted(std::get<0>(v), std::get<3>(v), std::get<1>(v),
                   std::get<2>(v)); // ROOT is inconsistent here
  } else {
    throw std::runtime_error(
        "Can not fill (yet?) efficiencies other than 2D using tuple of 4");
  }
  return v;
}

// lazy vector
template <typename A, typename T>
const lfv::FunctionalInterface<A, T> &
operator>>(const lfv::FunctionalInterface<A, T> &v, TH1 &h) {
  v.foreach ([&h](const auto &el) { el >> h; });
  return v;
}

template <typename A, typename T>
const lfv::FunctionalInterface<A, T> &
operator>>(const lfv::FunctionalInterface<A, T> &v, TH2 &h) {
  v.foreach ([&h](const auto &el) { el >> h; });
  return v;
}

template <typename A, typename T>
const lfv::FunctionalInterface<A, T> &
operator>>(const lfv::FunctionalInterface<A, T> &v, TH3 &h) {
  v.foreach ([&h](const auto &el) { el >> h; });
  return v;
}

template <typename A, typename T>
const lfv::FunctionalInterface<A, T> &
operator>>(const lfv::FunctionalInterface<A, T> &v, TProfile &h) {
  v.foreach ([&h](const auto &el) { el >> h; });
  return v;
}

template <typename A, typename T>
const lfv::FunctionalInterface<A, T> &
operator>>(const lfv::FunctionalInterface<A, T> &v, TEfficiency &h) {
  v.foreach ([&h](const auto &el) { el >> h; });
  return v;
}

#ifdef ALLOW_FILLING_TGRAPH
template <typename A, typename T>
const lfv::FunctionalInterface<A, T> &
operator>>(const lfv::FunctionalInterface<A, T> &v, TGraph &h) {
  v.foreach ([&h](const auto &el) { el >> h; });
  return v;
}

template <typename A, typename T>
const lfv::FunctionalInterface<A, T> &
operator>>(const lfv::FunctionalInterface<A, T> &v, TGraph2D &h) {
  v.foreach ([&h](const auto &el) { el >> h; });
  return v;
}
#endif

template <typename A, typename T>
const lfv::FunctionalInterface<A, T> &
operator>>(const lfv::FunctionalInterface<A, T> &v, TProfile2D &h) {
  v.foreach ([&h](const auto &el) { el >> h; });
  return v;
}

// TODO code remaining fills or replaec by templates

#endif