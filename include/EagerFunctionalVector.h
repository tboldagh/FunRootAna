// Copyright 2022, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)



// TODO there is a lot to do here!
// in particular, the operations can made cheaper (by much) by moving data instead of copying

#ifndef EagerFunctionalVector_h
#define EagerFunctionalVector_h
#include <algorithm>
#include <stdexcept>
#include <memory>
#include <map>
#include <cmath>
#include <iostream>
#include <type_traits>
#include "futils.h"


struct StatInfo {
  double count = {};
  double sum = {};
  double mean() const { return sum / count; }
  double sum2 = {};
  double var() const { return sum2 / count - std::pow(mean(), 2); }
  double sigma() const { return std::sqrt(var()); }
};

template<typename T>
class EagerFunctionalVector
{
private:
  std::vector<T> container;
  void __push_back(const T& x) {
    container.push_back(x);
  }
  template<typename U> friend class LazyFunctionalVector;
public:
  using value_type = T;

  EagerFunctionalVector() {}

  EagerFunctionalVector(const std::vector<T>& vec)
    : container(vec.begin(), vec.end()) {}

  EagerFunctionalVector(const EagerFunctionalVector<T>& rhs)
    : container(rhs.container) {}

  EagerFunctionalVector(typename std::vector<T>::const_iterator begin, typename std::vector<T>::const_iterator end)
    : container(begin, end) {}

  void operator=(EagerFunctionalVector<T>&& rhs) {
    container = std::move(rhs.container);
  }

  template<typename ResultT, typename Op>
  ResultT reduce(const ResultT& initialValue, Op operation) const {
    ResultT total = initialValue;
    for (auto e : container) {
      total = operation(total, e);
    }
    return total;
  }


  // template<typename Op>
  // EagerFunctionalVector<Container, decltype(std::declval<Op>()(std::declval<T>()))> integ( Op op ) const {
  //   EagerFunctionalVector<Container, decltype(std::declval<Op>()(std::declval<T>())) > ret;
  //   ret.push_back( sum(op) );
  //   return ret;
  // }

  template<typename Op>
  //  decltype(std::declval<Op>()(std::declval<T>())) sum( Op operation ) const {
  //    decltype(std::declval<Op>()(std::declval<T>())) total = {};
  typename std::invoke_result<Op, const T&>::type sum(Op operation) const {
    typename std::invoke_result<Op, const T&>::type total = {};

    for (auto e : container) {
      total += operation(e);
    }
    return total;
  }

  template<typename Op, typename R>
  R accumulate(Op operation, R initial = {}) const {
    R total = initial;
    for (auto e : container) {
      total = operation(total, e);
    }
    return total;
  }


  T sum() const {
    T total = {};
    for (auto e : container) {
      total += e;
    }
    return total;
  }


  template<typename Op>
  StatInfo stat(Op f) const {
    StatInfo info;
    for (auto e : container) {
      decltype(std::declval<Op>()(std::declval<T>())) v = f(e);
      info.count++;
      info.sum += v;
      info.sum2 += std::pow(v, 2);
    }
    return info;
  }


  EagerFunctionalVector<T> chain(const EagerFunctionalVector<T>& rhs) const {
    EagerFunctionalVector<T> clone(container);
    clone.unwrap().insert(clone.end(), rhs.begin(), rhs.end());
    return clone;
  }


  template<typename Op>
  EagerFunctionalVector<T> filter(Op f) const {
    EagerFunctionalVector<T> ret;
    for (const auto& el : container)
      if (f(el))
        ret.container.push_back(el);
    return ret;
  }

  template<typename Op>
  size_t count(Op f) const {
    size_t n = 0;
    for (const auto& e : container)
      n += (f(e) ? 1 : 0);
    return n;
  }

  inline bool empty() const {
    return container.empty();
  }

  static const T& identity(const T& c) { return c; }

  template< typename KeyExtractor = decltype(identity)>
  EagerFunctionalVector<T> sorted(KeyExtractor key = identity) const {
    EagerFunctionalVector<T> clone(container);
    std::sort(clone.container.begin(), clone.container.end(), [&](const T& el1, const T& el2) { return key(el1) < key(el2); });
    return clone;
  }


  EagerFunctionalVector<T> reverse() const {
    EagerFunctionalVector<T> ret;
    auto it_rbegin = container.rbegin();
    while (it_rbegin != container.rend()) {
      ret.container.push_back(*it_rbegin++);
    }
    return ret;
  }

  EagerFunctionalVector<std::pair<size_t, T>> enumerate(size_t offset = 0) const {
    EagerFunctionalVector<std::pair<size_t, T>> ret;
    size_t counter = offset;
    for (auto& el : container) {
      ret.unwrap().emplace_back(std::pair<size_t, T>(counter, el));
      ++counter;
    }
    return ret;
  }


  template<typename Op>
  EagerFunctionalVector<decltype(std::declval<Op>()(std::declval<T>()))> map(Op f) const {
    EagerFunctionalVector<decltype(std::declval<Op>()(std::declval<T>())) > ret;
    for (const auto& el : container)
      ret.push_back(f(el));
    return ret;
  }

  template<typename Op>
  std::map<decltype(std::declval<Op>()(std::declval<T>())), std::vector<T>> groupBy(Op f) const {
    std::map<decltype(std::declval<Op>()(std::declval<T>())), std::vector<T>> ret;
    for (const auto& el : container)
      ret[f(el)].push_back(el);
    return ret;
  }

  template<typename Op>
  EagerFunctionalVector<typename decltype(std::declval<Op>()(std::declval<T>()))::value_type> flatMap(Op f) const {
    EagerFunctionalVector<typename decltype(std::declval<Op>()(std::declval<T>()))::value_type> ret;
    this->map(f).foreach([&ret](const T& item) { ret.insert(ret, item); });
    return ret;
  }

  template< typename Op>
  const EagerFunctionalVector<T>& foreach(Op function)  const {
    for (auto& el : container) function(el);
    return *this;
  }

  template< typename Op>
  bool contains(Op predicate) const {
    for (auto& el : container) if (predicate(el)) return true;
    return false;
  }

    bool contains(const T& x) const {
        return contains([x](const T& el) { return el == x; });
    }


  template< typename Op>
  std::optional<T> first_of(Op predicate) const {
    for (auto& el : container) if (predicate(el)) return std::optional<T>(el);
    return {};
  }

  template< typename Op>
  bool all(Op predicate) const {
    if (container.empty()) return false;
    for (auto& el : container) if (not predicate(el)) return false;
    return true;
  }


  std::vector<T>& unwrap() {
    return container;
  }

  const std::vector<T>& unwrap() const {
    return container;
  }


  void push_back_to(std::vector<T>& v) const {
    v.insert(v.end(), container.begin(), container.end());
  }

  void push_back(const T& el) {
    container.emplace_back(el);
  }
  void clear() {
    container.clear();
  }

  template<typename T1, typename T2>
  void insert(const T1& destCont, const T2& items) {
    container.insert(destCont.container.end(), items.begin(), items.end());
  }

  size_t size() const {
    return container.size();
  }

  EagerFunctionalVector<std::pair<T, T>> unique_pairs() const {
    EagerFunctionalVector<std::pair<T, T>> ret;
    for (auto iter1 = std::begin(container); iter1 != std::end(container); ++iter1) {
      for (auto iter2 = iter1 + 1; iter2 != std::end(container); ++iter2) {
        ret.push_back(std::make_pair(*iter1, *iter2));
      }
    }
    return ret;
  }

  EagerFunctionalVector<T> rotate(int shift) const {
    if (shift == 0)
      return container;

    EagerFunctionalVector<T> ret;
    for (auto iter = std::begin(container) + shift; iter != std::end(container); ++iter)
      ret.push_back(*iter);
    for (auto iter = std::begin(container); iter != std::begin(container) + shift; ++iter)
      ret.push_back(*iter);
    return ret;
  }

  EagerFunctionalVector<T> take(size_t n, size_t stride = 1) const {
    EagerFunctionalVector<T> ret;
    auto end = container.begin() + std::min(n, container.size());
    for (auto iter = container.begin(); iter != end; iter += stride) {
      ret.push_back(*iter);
    }
    return ret;

    size_t counter = 0;
    for (auto& el : container) {
      if (counter % stride == 0)
        ret.push_back(el);
      if (counter >= n)
        return ret;
      counter++;
    }
    return ret;
  }

  EagerFunctionalVector<T> skip(size_t n, size_t stride = 1) const {
    EagerFunctionalVector<T> ret;
    auto iter = container.begin() + std::min(n, container.size());
    for (; iter != container.end(); iter += stride) {
      ret.push_back(*iter);
    }
    return ret;
  }


  template<typename Op>
  EagerFunctionalVector<T> take_while(Op predicate) const {
    EagerFunctionalVector<T> ret;
    for (auto& el : container) {
      if (not predicate(el))
        return ret;
      ret.__push_back(el);
    }
    return ret;
  }

  template<typename Op>
  EagerFunctionalVector skip_while(Op predicate) const {
    EagerFunctionalVector ret;
    bool start_taking = false;
    for (auto& el : container) {
      if (not start_taking and predicate(el)) {
        start_taking = true;
      }
      if (start_taking)
        ret.__push_back(el);
    }
    return ret;
  }


  template< typename KeyExtractor>
  EagerFunctionalVector<T> max(KeyExtractor key)  const {
    auto iterator = std::max_element(std::begin(container), std::end(container), [&](const T& a, const T& b) { return key(a) < key(b); });
    return std::begin(container) == std::end(container) ? *this : EagerFunctionalVector<T>(iterator, iterator + 1);
  }

  EagerFunctionalVector<T> max() const {
    auto iterator = std::max_element(std::begin(container), std::end(container), [&](const T& a, const T& b) { return a < b; });
    return std::begin(container) == std::end(container) ? *this : EagerFunctionalVector<T>(iterator, iterator + 1);
  }

  template< typename KeyExtractor>
  const EagerFunctionalVector<T> min(KeyExtractor key)  const {
    auto iterator = std::min_element(std::begin(container), std::end(container), [&](const T& a, const T& b) { return key(a) < key(b); });
    return std::begin(container) == std::end(container) ? *this : EagerFunctionalVector<T>(iterator, iterator + 1);
  }

  const EagerFunctionalVector<T> min()  const {
    auto iterator = std::min_element(std::begin(container), std::end(container), [&](const T& a, const T& b) { return a < b; });
    return std::begin(container) == std::end(container) ? *this : EagerFunctionalVector<T>(iterator, iterator + 1);
  }

  const EagerFunctionalVector<T> range(size_t start, size_t stop) const {
    return EagerFunctionalVector<T>(container.begin() + start, container.begin() + stop);
  }

  std::optional<T> element_at(size_t index) const { if (index < size()) return container.at(index); else return  {}; }

  const EagerFunctionalVector<T> first(size_t n = 1)  const {
    auto iterator = container.end() < container.begin() + n ? container.end() : container.begin() + n;
    return EagerFunctionalVector<T>(container.begin(), iterator);
  }

  const EagerFunctionalVector<T> last(size_t n = 1)  const {
    auto iterator = container.end() < container.begin() + n ? container.begin() : container.begin() + (container.size() - n);
    return EagerFunctionalVector<T>(iterator, container.end());
  }


  const EagerFunctionalVector<T>& stage()  const { return *this; }
  const EagerFunctionalVector<T>& cache()  const { return *this; }

  const EagerFunctionalVector<T> join(const EagerFunctionalVector<T>& other)  const {
    EagerFunctionalVector<T> ret(container.begin(), container.end());
    ret.container.insert(ret.container.end(), other.begin(), other.end());
    return ret;
  }

  template<typename U>
  const EagerFunctionalVector<std::pair<T, U> > zip(const EagerFunctionalVector<U>& other) {
    EagerFunctionalVector<std::pair<T, U> > ret;

    auto thisIter = std::cbegin(container);
    auto otherIter = std::cbegin(other.unwrap());
    for (; thisIter != std::end(container) and otherIter != std::end(other.unwrap()); ++thisIter, ++otherIter) {
      ret.unwrap().insert(ret.unwrap().end(), std::make_pair(*thisIter, *otherIter));
    }
    return ret;
  }

  template<typename Other, typename Comp>
  bool is_same(const Other& other, Comp comparator) const {
    auto thisIter = std::cbegin(container);
    auto otherIter = std::cbegin(other.unwrap());
    for (; thisIter != std::end(container) and otherIter != std::end(other.unwrap()); ++thisIter, ++otherIter) {
      if (not comparator( std::make_pair(*thisIter, *otherIter))) {
        return false;
      }
    }
    return true;
  }

  template<typename Other>
  auto is_same(const Other& c) const {
    return is_same(c, [](const auto& el) { return el.first != el.second; });
  }


  auto begin() const {
    return std::cbegin(container);
  }

  auto end() const {
    return std::cend(container);
  }

  // TODO
  // OwningView<T> as_lazy() const {
  //   return OwningView(container);
  // }

};

template<typename T>
EagerFunctionalVector<T> wrap(const std::vector<T>& v)
{
  return EagerFunctionalVector<T>(v);
}


template<typename T>
auto wrap(const std::initializer_list<T>& il)
{
  return wrap(std::vector<T>(std::begin(il), std::end(il)));
}

#endif