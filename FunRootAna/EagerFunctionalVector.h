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
#include <optional>
#include "futils.h"



template<typename Stored>
class EagerFunctionalVector
{
private:
  std::vector<Stored> container;
  void __push_back(const Stored& x) {
    container.push_back(x);
  }
  template<typename U> friend class LazyFunctionalVector;
public:
  using value_type = Stored;
  static constexpr auto id = [](const Stored& x){ return x; };


  EagerFunctionalVector() {}

  EagerFunctionalVector(const std::vector<Stored>& vec)
    : container(vec.begin(), vec.end()) {}

  EagerFunctionalVector(const EagerFunctionalVector<Stored>& rhs)
    : container(rhs.container) {}

  EagerFunctionalVector(typename std::vector<Stored>::const_iterator begin, typename std::vector<Stored>::const_iterator end)
    : container(begin, end) {}

  void operator=(EagerFunctionalVector<Stored>&& rhs) {
    container = std::move(rhs.container);
  }

  template<typename Result, typename Op>
  Result reduce(const Result& initialValue, Op operation) const {
    Result total = initialValue;
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
  typename std::invoke_result<Op, const Stored&>::type sum(Op operation) const {
    typename std::invoke_result<Op, const Stored&>::type total = {};

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


  Stored sum() const {
    Stored total = {};
    for (auto e : container) {
      total += e;
    }
    return total;
  }

  
  template<typename F = decltype(id)>
  StatInfo stat(F f = id) const{
    static_assert(std::is_arithmetic< typename std::remove_reference<typename std::invoke_result<F, Stored>::type>::type >::value, "The type stored in this container is not arithmetic, can't calculate standard statistical properties");
    StatInfo info;
    for (const auto &v : container) {
      info.count++;
      info.sum += v;
      info.sum2 += std::pow(v, 2);
    }
    return info;
  }


  EagerFunctionalVector<Stored> chain(const EagerFunctionalVector<Stored>& rhs) const {
    EagerFunctionalVector<Stored> clone(container);
    clone.unwrap().insert(clone.end(), rhs.begin(), rhs.end());
    return clone;
  }


  template<typename Op>
  EagerFunctionalVector<Stored> filter(Op f) const {
    EagerFunctionalVector<Stored> ret;
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


  template< typename KeyExtractor = decltype(id)>
  EagerFunctionalVector<Stored> sort(KeyExtractor key = id) const {
    EagerFunctionalVector<Stored> clone(container);
    std::sort(clone.container.begin(), clone.container.end(), [&](const Stored& el1, const Stored& el2) { return key(el1) < key(el2); });
    return clone;
  }


  EagerFunctionalVector<Stored> reverse() const {
    EagerFunctionalVector<Stored> ret;    
    for ( auto it = container.rbegin(); it != container.rend(); ++it) {
      ret.unwrap().emplace_back( *it );
    }
    return ret;
  }

    auto toptr() const {
        static_assert( !std::is_pointer<Stored>::value, "The content is already a pointer");
        return map( []( const Stored& value){ return &value; });
    }


    auto toref() const { 
        static_assert( std::is_pointer<Stored>::value, "The content is not a pointer");
        return map( []( const auto & el) { return *el; });
    }


  EagerFunctionalVector<std::pair<size_t, Stored>> enumerate(size_t offset = 0) const {
    EagerFunctionalVector<std::pair<size_t, Stored>> ret;
    size_t counter = offset;
    for (auto& el : container) {
      ret.unwrap().emplace_back(std::pair<size_t, Stored>(counter, el));
      ++counter;
    }
    return ret;
  }


  template<typename Op>
  EagerFunctionalVector<decltype(std::declval<Op>()(std::declval<Stored>()))> map(Op f) const {
    EagerFunctionalVector<decltype(std::declval<Op>()(std::declval<Stored>())) > ret;
    for (const auto& el : container)
      ret.push_back(f(el));
    return ret;
  }

  template<typename Op>
  std::map<decltype(std::declval<Op>()(std::declval<Stored>())), std::vector<Stored>> groupBy(Op f) const {
    std::map<decltype(std::declval<Op>()(std::declval<Stored>())), std::vector<Stored>> ret;
    for (const auto& el : container)
      ret[f(el)].push_back(el);
    return ret;
  }

  template<typename Op>
  EagerFunctionalVector<typename decltype(std::declval<Op>()(std::declval<Stored>()))::value_type> flatMap(Op f) const {
    EagerFunctionalVector<typename decltype(std::declval<Op>()(std::declval<Stored>()))::value_type> ret;
    this->map(f).foreach([&ret](const Stored& item) { ret.insert(ret, item); });
    return ret;
  }

  template< typename Op>
  const EagerFunctionalVector<Stored>& foreach(Op function)  const {
    for (auto& el : container) function(el);
    return *this;
  }

  template< typename Op>
  bool contains(Op predicate) const {
    for (auto& el : container) if (predicate(el)) return true;
    return false;
  }

    bool contains(const Stored& x) const {
        return contains([x](const Stored& el) { return el == x; });
    }


  template< typename Op>
  std::optional<Stored> first_of(Op predicate) const {
    for (auto& el : container) if (predicate(el)) return std::optional<Stored>(el);
    return {};
  }

  template< typename Op>
  bool all(Op predicate) const {
    if (container.empty()) return false;
    for (auto& el : container) if (not predicate(el)) return false;
    return true;
  }


  std::vector<Stored>& unwrap() {
    return container;
  }

  const std::vector<Stored>& unwrap() const {
    return container;
  }


  void push_back_to(std::vector<Stored>& v) const {
    v.insert(v.end(), container.begin(), container.end());
  }

  void push_back(const Stored& el) {
    container.emplace_back(el);
  }
  void clear() {
    container.clear();
  }

  template<typename Stored1, typename Stored2>
  void insert(const Stored1& destCont, const Stored2& items) {
    container.insert(destCont.container.end(), items.begin(), items.end());
  }

  size_t size() const {
    return container.size();
  }

  EagerFunctionalVector<std::pair<Stored, Stored>> unique_pairs() const {
    EagerFunctionalVector<std::pair<Stored, Stored>> ret;
    for (auto iter1 = std::begin(container); iter1 != std::end(container); ++iter1) {
      for (auto iter2 = iter1 + 1; iter2 != std::end(container); ++iter2) {
        ret.push_back(std::make_pair(*iter1, *iter2));
      }
    }
    return ret;
  }

  EagerFunctionalVector<Stored> rotate(int shift) const {
    if (shift == 0)
      return container;

    EagerFunctionalVector<Stored> ret;
    for (auto iter = std::begin(container) + shift; iter != std::end(container); ++iter)
      ret.push_back(*iter);
    for (auto iter = std::begin(container); iter != std::begin(container) + shift; ++iter)
      ret.push_back(*iter);
    return ret;
  }

  EagerFunctionalVector<Stored> take(size_t n, size_t stride = 1) const {
    EagerFunctionalVector<Stored> ret;
    size_t counter = 0;
    for ( auto & el: container) {
      if ( counter >= n ) break;
      if ( counter % stride == 0 ) ret.unwrap().emplace_back(el);
      ++counter;
    }
    return ret;
  }

  EagerFunctionalVector<Stored> skip(size_t n, size_t stride = 1) const {
    EagerFunctionalVector<Stored> ret;
    size_t counter = 0;
    for ( auto &el : container ) {
      if ( counter >= n and counter % stride == 0 )
        ret.unwrap().emplace_back(el);
      ++counter;
    }
    return ret;
  }


  template<typename Op>
  EagerFunctionalVector<Stored> take_while(Op predicate) const {
    EagerFunctionalVector<Stored> ret;
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
      if (not start_taking and not predicate(el)) {
        start_taking = true;
      }
      if (start_taking)
        ret.unwrap().emplace_back(el);
    }
    return ret;
  }


  template< typename KeyExtractor>
  EagerFunctionalVector<Stored> max(KeyExtractor key)  const {
    auto iterator = std::max_element(std::begin(container), std::end(container), [&](const Stored& a, const Stored& b) { return key(a) < key(b); });
    return std::begin(container) == std::end(container) ? *this : EagerFunctionalVector<Stored>(iterator, iterator + 1);
  }

  EagerFunctionalVector<Stored> max() const {
    auto iterator = std::max_element(std::begin(container), std::end(container), [&](const Stored& a, const Stored& b) { return a < b; });
    return std::begin(container) == std::end(container) ? *this : EagerFunctionalVector<Stored>(iterator, iterator + 1);
  }

  template< typename KeyExtractor>
  const EagerFunctionalVector<Stored> min(KeyExtractor key)  const {
    auto iterator = std::min_element(std::begin(container), std::end(container), [&](const Stored& a, const Stored& b) { return key(a) < key(b); });
    return std::begin(container) == std::end(container) ? *this : EagerFunctionalVector<Stored>(iterator, iterator + 1);
  }

  const EagerFunctionalVector<Stored> min()  const {
    auto iterator = std::min_element(std::begin(container), std::end(container), [&](const Stored& a, const Stored& b) { return a < b; });
    return std::begin(container) == std::end(container) ? *this : EagerFunctionalVector<Stored>(iterator, iterator + 1);
  }

  const EagerFunctionalVector<Stored> range(size_t start, size_t stop) const {
    return EagerFunctionalVector<Stored>(container.begin() + start, container.begin() + stop);
  }

  std::optional<Stored> element_at(size_t index) const { if (index < size()) return container.at(index); else return  {}; }

  const EagerFunctionalVector<Stored> first(size_t n = 1)  const {
    auto iterator = container.end() < container.begin() + n ? container.end() : container.begin() + n;
    return EagerFunctionalVector<Stored>(container.begin(), iterator);
  }

  const EagerFunctionalVector<Stored> last(size_t n = 1)  const {
    auto iterator = container.end() < container.begin() + n ? container.begin() : container.begin() + (container.size() - n);
    return EagerFunctionalVector<Stored>(iterator, container.end());
  }


  const std::vector<Stored>& stage()  const { return container; }

  const EagerFunctionalVector<Stored> join(const EagerFunctionalVector<Stored>& other)  const {
    EagerFunctionalVector<Stored> ret(container.begin(), container.end());
    ret.container.insert(ret.container.end(), other.begin(), other.end());
    return ret;
  }

  template<typename Stored2>
  const EagerFunctionalVector<std::pair<Stored, Stored2> > zip(const EagerFunctionalVector<Stored2>& other) {
    EagerFunctionalVector<std::pair<Stored, Stored2> > ret;

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
    return is_same(c, [](const auto& el) { return el.first == el.second; });
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

template<typename Stored>
EagerFunctionalVector<Stored> wrap(const std::vector<Stored>& v)
{
  return EagerFunctionalVector<Stored>(v);
}


template<typename Stored>
auto wrap(const std::initializer_list<Stored>& il)
{
  return wrap(std::vector<Stored>(std::begin(il), std::end(il)));
}

#endif