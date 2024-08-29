// Copyright 2024, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)
#ifndef LazyFunctionalVector_h
#define LazyFunctionalVector_h
#include <type_traits>
#include <set>
#include <deque>
#include <cmath>
#include <optional>

#include "futils.h"


/*
The goal of the code below is to create functional vector wrapper
with all expensive operations done in a lazy manner.
E.g. a -> filter 1 -> map 1 -> filter 2 -> map 2
take no CPU time except for construction of functions involved in filtering & mapping.

Implementation is inspired by Scala collections where the rich collections API is implemented
using solely foreach method provided by actual container. In contrast though, here the operations are lazy where possible.
The actual containers are wrapped std::vectors but expansion to other containers is straightforward as only one method needs to be provided.

The foreach should take function object that is applied to element of the collection.
The function should return boolean, when the false is returned the iteration is interrupted.
The foreach takes additional instructions that can be used to optimize it's operation.

*/
namespace lfv {

namespace details {
    enum skip_take_logic_t { skip_elements, take_elements };
    enum min_max_logic_t { min_elements, max_elements };
    enum preserve_t { temporary, store }; // instruct if the results of for-each will be refereed to afterwards
    struct foreach_instructions {
        preserve_t preserve = temporary;
    };

    struct as_needed {};

    template<typename T>
    struct one_element_stack_container {
        T* ptr = nullptr;
        char data[sizeof(T)];

        template<typename U>
        void insert(const U& el) {
            static_assert(std::is_same<T, U>::value, "one_element_container can't handle plymorphic data");
            if (ptr != nullptr) throw std::runtime_error("one_element_container already has content");
            ptr = new (reinterpret_cast<T*>(data)) T(el);
        }
        template<typename U>
        void replace(const U& el) {
            static_assert(std::is_same<T, U>::value, "one_element_container can't handle plymorphic data");
            ptr = new (reinterpret_cast<T*>(data)) T(el);
        }
        bool empty() const { return ptr == nullptr; }

        T& get() { return *ptr; }
    };

    template<typename T>
    struct one_element_stack_container<T*> {
        const T* ptr = nullptr;

        template<typename U>
        void insert(const U* el) {
            if (ptr != nullptr) throw std::runtime_error("one_element_container already has content");
            ptr = el;
        }
        template<typename U>
        void replace(const U* el) {
            ptr = el;
        }
        bool empty() const { return ptr == nullptr; }

        const T* get() { return ptr; }
    };
  }

#define AS_NEEDED //template<typename T=details::as_needed>



namespace details {
    template<typename T> struct has_fast_element_access_tag { static constexpr bool value = false; };
}
constexpr size_t all_elements = std::numeric_limits<size_t>::max();
constexpr size_t invalid_index = std::numeric_limits<size_t>::max();


template< typename Container, typename Filter> class FilteredView;
template< typename Container, typename Map> class MappedView;
template<typename Container> class DirectView;
template<typename T, template<typename, typename> typename Container = std::vector> class OwningView;
template<typename T, template<typename, typename> typename Container = std::vector> class RefView;

template<typename Container, details::skip_take_logic_t> class TakeSkipNView;
template<typename Container> class NView;
template<typename Container> class EnumeratedView;
template<typename Container> class ReverseView;
template<typename Container> class CachedView;
template<typename Container, typename Subroutine> class InspectView;
template<typename Container, typename Filter, details::skip_take_logic_t> class TakeSkipWhileView;
template<typename Container1, typename Container2, typename ExposedType> class ChainView;
template<typename Container1, typename Container2> class ZipView;
template<typename Container1, typename Container2> class CartesianView;
template<typename Container> class CombinationsView;
template<typename Container> class PermutationsView;
template<typename Container1, typename Comparator> class SortedView;
template<typename Container1, typename Comparator> class MMView;
template<typename T> class Range;


template<typename Container, typename Stored>
class FunctionalInterface {
public:
    using value_type = Stored;
    using const_value_type = typename std::conditional<std::is_pointer<Stored>::value,
        const typename std::remove_pointer<value_type>::type*,
        const value_type>::type;
    using const_reference_type = const value_type&;
    // optimal type to pass around
    // using argument_type = const value_type&;
    using argument_type = typename std::conditional<std::is_pointer<Stored>::value,
        const_value_type,
        const_reference_type>::type;

    // identity (noop)
    static constexpr auto id = [](const Stored& x) -> const Stored& { return x; };
    
    FunctionalInterface(const Container& cont) : m_actual_container(cont) {}
    virtual ~FunctionalInterface() { }

    // transform using function provided, this is lazy operation
    template<typename F>
    auto map(F f) const {
        return MappedView<Container, F>(m_actual_container, f);
    }

    // filter provided predicate, this is lazy operation
    template<typename F>
    auto filter(F f) const {
        return FilteredView<Container, F>(m_actual_container, f);
    }


    // invoke function on each element of the container, 
    // can be followed by other operations, this is not lazy operation, for lazy operation like this see inspect
    template<typename F>
    auto foreach(F f) const -> const FunctionalInterface<Container, Stored>& {
        m_actual_container.foreach_imp([f](argument_type el) { f(el); return true; });
        return *this;
    }

    template<typename S>
    auto inspect(S s) const -> InspectView<Container, S> {
        return InspectView<Container, S>(m_actual_container, s);
    }

    // count of elements, this is an eager operation
    AS_NEEDED
    size_t size() const {
        size_t c = 0;
        m_actual_container.foreach_imp([&c](argument_type) { c++; return true;});
        return c;
    }

    AS_NEEDED
    bool empty() const {
        size_t c = 0;
        m_actual_container.foreach_imp([&c](argument_type) { c++; return false;});
        return c == 0;
    }

    // count element satisfying predicate, this is an eager operation
    template<typename Predicate>
    size_t count(Predicate pred) const {
        size_t c = 0;
        m_actual_container.foreach_imp([&c, pred](argument_type el) {
            c += (pred(el) ? 1 : 0); return true; });
        return c;
    }

    // finds if an element is in the container
    // sometimes referetd to as any()
    // returns false for empty container
    template<typename Predicate>
    bool contains(Predicate f) const {
        bool found = false;
        m_actual_container.foreach_imp([f, &found](argument_type el) {
            if (f(el)) {
                found = true;
                return false;
            }
            else {
                return true;
            }
            });
        return found;
    }
    // as above, but require identity
    bool contains(const_reference_type x) const {
        return contains([x](const_reference_type el) { return el == x; });
    }

    // mirror of contains()
    // returns true if all elements satisfy predicate, 
    // returns false for empty container
    template<typename Predicate>
    bool all(Predicate f) const {
        bool found = true;
        bool has_elements = false;
        m_actual_container.foreach_imp([f, &found, &has_elements](const_reference_type el) {
            has_elements = true;
            if (not f(el)) { // offending element
                found = false;
                return false;
            }
            else {
                return true;
            }
            });
        return found and has_elements;
    }

    // sort according to the key extractor @arg f, this is lazy operation
    template<typename F = decltype(id)>
    auto sort(F f = id) const {
        static_assert(Container::is_finite, "Can't sort in an infinite container");
        static_assert(Container::is_permanent, "Can't sort container that generates element on the flight, stage() it before");
        return SortedView<Container, F>(m_actual_container, f);
    }

    // selectors, lazy
    AS_NEEDED
    auto take(size_t n, size_t stride = 1) const {
        return TakeSkipNView<Container, details::take_elements>(m_actual_container, n, stride);
    }
    AS_NEEDED
    auto skip(size_t n, size_t stride = 1) const {
        return TakeSkipNView<Container, details::skip_elements>(m_actual_container, n, stride);
    }

    // produces container with indexed data

    AS_NEEDED
    auto enumerate(size_t offset = 0) const {
        return EnumeratedView<Container>(m_actual_container, offset);
    }

    template<typename F>
    auto take_while(F f) const {
        return TakeSkipWhileView<Container, F, details::take_elements>(m_actual_container, f);
    }

    template<typename F>
    auto skip_while(F f) const {
        return TakeSkipWhileView<Container, F, details::skip_elements>(m_actual_container, f);
    }

    // creates saves the data to STL container (vector by default)
    template<typename C = std::vector<Stored>>
    C stage() const {
        static_assert(Container::is_finite, "Can't stage an infinite container");
        C c;
        insert_to(c);
        return c;
    }

    AS_NEEDED
    auto reverse() const {
        static_assert(Container::is_finite, "Can't reverse an infinite container");
        return ReverseView<Container>(m_actual_container);
    }

    AS_NEEDED
    auto toptr() const {
        static_assert(!std::is_pointer<Stored>::value, "The content is already a pointer");
        return map([](const Stored& value) { return &value; });
    }

    AS_NEEDED
    auto toref() const {
        static_assert(std::is_pointer<Stored>::value, "The content is not a pointer");
        return map([](argument_type el) { return *el; });
    }

    // max and min
    template<typename KeyExtractor = decltype(id)>
    auto max(KeyExtractor by = id) const {
        static_assert(Container::is_finite, "Can't find max in an infinite container");
        return MMView<Container, KeyExtractor>(m_actual_container, by, details::max_elements);
    }
    template<typename KeyExtractor = decltype(id)>
    auto min(KeyExtractor by = id) const {
        static_assert(Container::is_finite, "Can't find min in an infinite container");
        return MMView<Container, KeyExtractor>(m_actual_container, by, details::min_elements);
    }


    // concatenate two containers, in a lazy way
    template<typename ExposedType = value_type, typename Other>
    auto chain(const Other& c) const {
        static_assert(Container::is_finite, "Can't chain with an infinite container");

        return ChainView<Container, Other, ExposedType>(m_actual_container, c);
    }

    // combine pairwise
    // the implementation is suboptimal, that is, it involves inexed access to one of the containers, it is therefore better if one of them is staged
    template<typename Other>
    auto zip(const Other& c) const {
        static_assert(Container::is_finite or Other::is_finite, "Can't combine infinite containers");
        return ZipView<Container, Other>(m_actual_container, c);
    }

    // compare pairwise, if the containers have diffrent len,  the comparison is performed only until smaller of the sizes
    // the comparator obtains a pair from first element is from 1st container, and second from container c
    template<typename Other, typename Comp>
    auto is_same(const Other& c, Comp comparator) const {
        static_assert(Container::is_finite or Other::is_finite, "Can't compare infinite containers");

        bool same = true;
        zip(c).foreach([&same, &comparator](const auto& el) {
            if (not comparator(el)) {
                same = false;
                return false;
            }
            return true;
            });
        return same;
    }

    // compare for equality
    template<typename Other>
    auto is_same(const Other& c) const {
        return is_same(c, [](const auto& el) { return el.first == el.second; });
    }

    // produce container of elements grouped by n
    // the elements are packaged in vector<optional<optimal_reference>> (tha "optimal" can be either refernce_wrapper or value - depends on underlying container that is grouped)
    // jump parameter informs how if elements would be reused
    // e.g. assume container: 0,1,2,3 and size=2, jump=2 then such groups will be formed (0,1)(2,3)
    // if the jump=1 the groups (0,1)(1,2)(2,3) will be formed
    // @warning if the container size is not a multiple of n, the trailing elements are not exposed, e.g. sth.take(8).group(3) will process only first 6 elements
    AS_NEEDED
    auto group(size_t size = 2, size_t jump = std::numeric_limits<size_t>::max()) const {
        if (jump == std::numeric_limits<size_t>::max())
            return NView<Container>(m_actual_container, size, size);
        else
            return NView<Container>(m_actual_container, size, jump);
    }

    // forms container of all pairs from this and other container
    template<typename Other>
    auto cartesian(const Other& c) const {
        return CartesianView(m_actual_container, c);
    }

    // TODO
    auto combinations(size_t) {}
    auto permutations(size_t) {}

    // sum, allows to transform the object before summation (i.e. can act like map + sum), for empty container returns the default value
    template<typename F = decltype(id)>
    auto sum(F f = id) const {
        static_assert(Container::is_finite, "Can't sum an infinite container");
        typename std::remove_const<typename std::remove_reference<typename std::invoke_result<F, typename Container::argument_type>::type>::type>::type s = {};
        m_actual_container.foreach_imp([&s, f](argument_type el) {
            s = s + f(el);
            return true; });
        return s;
    }
    // accumulate, function us used as: total = f(total, element), also take starting element
    template<typename F, typename R>
    auto accumulate(F f, R initial = {}) const {
        static_assert(Container::is_finite, "Can't sum an infinite container");
        static_assert(std::is_same<R, typename std::invoke_result<F, R, typename Container::argument_type>::type>::value, "function return type different than initial value");
        R s = initial;
        m_actual_container.foreach_imp([&s, f](argument_type el) { s = f(s, el);  return true; });
        return s;
    }

    template<typename F = decltype(id)>
    StatInfo stat(F f = id) const {
        static_assert(std::is_arithmetic< typename std::remove_reference<typename std::invoke_result<F, Stored>::type>::type >::value, "The type stored in this container is not arithmetic, can't calculate standard statistical properties");
        StatInfo info;
        m_actual_container.foreach_imp([&info, f](argument_type el) {
            auto v = f(el);
            info.count++;
            info.sum += v;
            info.sum2 += std::pow(v, 2);
            return true;
            });
        return info;
    }


    // access by index (not good idea to overuse)
    AS_NEEDED
    auto element_at(size_t  n) const -> std::optional<const_value_type> {
        details::one_element_stack_container<value_type> temp;
        size_t i = 0;
        m_actual_container.foreach_imp([&temp, n, &i](argument_type el) {
            if (i == n) {
                temp.insert(el);
                return false;
            }
            i++;
            return true;
            });
        return temp.empty() ? std::optional<const_value_type>() : temp.get();
    }

    AS_NEEDED
    auto get() const -> std::optional<const_value_type> {
        return element_at(0);
    }


    // first element satisfying predicate
    template<typename Predicate>
    auto first_of(Predicate f) const -> std::optional<value_type> {
        details::one_element_stack_container<value_type> temp;
        m_actual_container.foreach_imp([&temp, f](argument_type el) {
            if (f(el)) {
                temp.insert(el);
                return false;
            }
            return true;
            });
        return temp.empty() ? std::optional<value_type>() : temp.get();
    }

    // returns an index of first element satisfying the predicate
    template<typename Predicate>
    std::optional<size_t> first_of_index(Predicate f) const {
        size_t i = 0;
        std::optional<size_t> found;
        m_actual_container.foreach_imp([&i, f, &found](argument_type el) {
            if (f(el)) {
                found = i;
                return false;
            }
            ++i;
            return true;
            });
        return found;
    }

    // TODO applies point free transforms on actual data
    template<typename T>
    auto apply_on(const T& data) {
        //        retrun DirectView(data).go();
    }

    // back to regular vector
    template<typename R>
    void push_back_to(R& result) const {
        static_assert(Container::is_finite, "Can't save an infinite container");
        m_actual_container.foreach_imp([&result](argument_type el) {
            result.push_back(el); return true;
            });
    }

    template<typename R>
    void insert_to(R& result) const {
        static_assert(Container::is_finite, "Can't save an infinite container");
        m_actual_container.foreach_imp([&result](argument_type el) {
            result.insert(result.end(), el); return true;
            });
    }

protected:
    const Container& m_actual_container;
};



// provides view of subset of element which satisfy the predicate
template< typename Container, typename Filter>
class FilteredView : public FunctionalInterface<FilteredView<Container, Filter>, typename Container::value_type> {
public:
    using interface = FunctionalInterface<FilteredView<Container, Filter>, typename Container::value_type>;
    using value_type = typename interface::value_type;
    static constexpr bool is_finite = Container::is_finite;

    FilteredView(const Container& c, Filter f)
        : interface(*this),
        m_foreach_imp_provider(c),
        m_filterOp(f) {};

    template<typename F>
    void foreach_imp(F f, details::foreach_instructions how = {}) const {
        m_foreach_imp_provider.foreach_imp([f, this](typename interface::argument_type el) {
            if (this->m_filterOp(el)) {
                const bool go = f(el);
                if (not go)
                    return false;
            }
            return true;
            }, how);
    }
    FilteredView() = delete;
private:
    const Container m_foreach_imp_provider;
    Filter m_filterOp;
};


template< typename Container, typename KeyExtractor>
class SortedView : public FunctionalInterface<SortedView<Container, KeyExtractor>, typename Container::value_type> {
public:
    using interface = FunctionalInterface<SortedView<Container, KeyExtractor>, typename Container::value_type>;
    using value_type = typename interface::value_type;
    static constexpr bool is_finite = true;

    SortedView(const Container& c, KeyExtractor f)
        : interface(*this),
        m_foreach_imp_provider(c),
        m_keyExtractor(f) {};

    template<typename F, typename Q = value_type>
    typename std::enable_if< std::is_pointer<Q>::value, void>::type foreach_imp(F f, details::foreach_instructions how = {}) const {
        std::vector< value_type > lookup;
        m_foreach_imp_provider.foreach_imp([&lookup](typename interface::argument_type el) { lookup.push_back(el); return true; }, how);

        auto sorter = [this](auto a, auto b) { return m_keyExtractor(a) < m_keyExtractor(b); };
        std::sort(std::begin(lookup), std::end(lookup), sorter);
        for (auto ptr : lookup) {
            const bool go = f(ptr);
            if (not go)
                break;
        }
    }

    template<typename F, typename Q = value_type>
    typename std::enable_if< !std::is_pointer<Q>::value, void>::type foreach_imp(F f, details::foreach_instructions how = {}) const {
        std::vector< std::reference_wrapper<const value_type>> lookup;
        m_foreach_imp_provider.foreach_imp([&lookup](typename interface::argument_type el) { lookup.push_back(std::cref(el)); return true; }, how);

        auto sorter = [this](auto a, auto b) { return m_keyExtractor(a.get()) < m_keyExtractor(b.get()); };
        std::sort(std::begin(lookup), std::end(lookup), sorter);
        for (auto ref : lookup) {
            const bool go = f(ref.get());
            if (not go)
                break;
        }
    }
    SortedView() = delete;
private:
    const Container m_foreach_imp_provider;
    KeyExtractor m_keyExtractor;
};


// min max view
template< typename Container, typename KeyExtractor>
class MMView : public FunctionalInterface<MMView<Container, KeyExtractor>, typename Container::value_type> {
public:
    using interface = FunctionalInterface<MMView<Container, KeyExtractor>, typename Container::value_type>;
    using value_type = typename interface::value_type;
    using reference_type = value_type&;
    using const_reference_type = const value_type&;
    using compared_type = typename std::remove_const<typename std::remove_reference<typename std::invoke_result<KeyExtractor, typename interface::argument_type>::type>::type>::type;
    static constexpr bool is_finite = Container::is_finite;

    MMView(const Container& c, KeyExtractor f, details::min_max_logic_t logic)
        : interface(*this),
        m_actual_container(c),
        m_keyExtractor(f),
        m_logic(logic) {};

    template<typename F>
    void foreach_imp(F f, details::foreach_instructions how = {}) const {
        details::one_element_stack_container< compared_type > extremeValue;
        details::one_element_stack_container< value_type > extremeElement;

        m_actual_container.foreach_imp([&extremeValue, &extremeElement, this](typename interface::argument_type el) {
            compared_type val = m_keyExtractor(el);
            if (extremeValue.empty()) {
                extremeValue.insert(val);
                extremeElement.insert(el);
            }
            if ((m_logic == details::max_elements and val >= extremeValue.get())
                or
                (m_logic == details::min_elements and val < extremeValue.get())) {
                extremeValue.replace(val);
                extremeElement.replace(el);
            }
            return true;
            }, how);
        if (not extremeElement.empty()) {
            f(extremeElement.get());
        }
    }
    MMView() = delete;
private:
    const Container m_actual_container;
    KeyExtractor m_keyExtractor;
    details::min_max_logic_t m_logic;
};


// provides the transforming view
template< typename Container, typename Mapping>
class MappedView : public FunctionalInterface<MappedView<Container, Mapping>, typename std::invoke_result<Mapping, typename Container::value_type>::type > {
public:
    using interface = FunctionalInterface<MappedView<Container, Mapping>, typename std::invoke_result<Mapping, typename Container::value_type>::type >;
    using value_type = typename interface::value_type;
    static constexpr bool is_finite = Container::is_finite;
    static constexpr bool is_permanent = false;



    MappedView(const Container& c, Mapping m)
        : interface(*this),
        m_foreach_imp_provider(c),
        m_mappingOp(m) {};

    template<typename F>
    void foreach_imp(F f, details::foreach_instructions how = {}) const {
        m_foreach_imp_provider.foreach_imp([f, this](const auto& el) -> bool {
            const bool go = f(m_mappingOp(el));
            if (not go)
                return false;
            return true;
            }, how);
    }
    MappedView() = delete;
private:
    const Container m_foreach_imp_provider;
    Mapping m_mappingOp;
};

namespace {
    template<typename Container>
    struct select_type_for_n_view {
        using type = typename std::conditional< Container::is_permanent,
            RefView<typename Container::value_type, std::deque>,
            OwningView<typename Container::value_type, std::deque> >::type;
    };
}
template<typename Container>
class NView : public FunctionalInterface<NView<Container>, typename select_type_for_n_view<Container>::type> {
public:
    using interface = FunctionalInterface<NView<Container>, typename select_type_for_n_view<Container>::type>;
    using value_type = typename interface::value_type;
    static constexpr bool is_finite = true;

    NView(const Container& c, size_t n, size_t jump)
        : interface(*this),
        m_foreach_imp_provider(c),
        m_group(n),
        m_jump(jump) { }

    template<typename F>
    void foreach_imp(F f, details::foreach_instructions how = {}) const {
        value_type group;
        m_foreach_imp_provider.foreach_imp([f, this, &group](const auto& el) {
            group.insert(el);
            if (group.size() == m_group) {
                const bool go = f(group);
                if (not go)
                    return false;
                if (m_group == m_jump)
                    group.clear();
                else
                    for (size_t p = 0; p < m_jump; p++) group._underlying().pop_front();
            }
            return true;
            }, how);
    }
private:
    const Container m_foreach_imp_provider;
    size_t m_group;
    size_t m_jump;
};

template<typename Container, details::skip_take_logic_t logic>
class TakeSkipNView : public FunctionalInterface<TakeSkipNView<Container, logic>, typename Container::value_type> {
public:
    using interface = FunctionalInterface<TakeSkipNView<Container, logic>, typename Container::value_type>;
    using value_type = typename interface::value_type;
    static constexpr bool is_finite = logic == details::take_elements or Container::is_finite;
    static constexpr bool is_permanent = Container::is_permanent;

    TakeSkipNView(const Container& c, size_t n, size_t stride = 1)
        : interface(*this),
        m_foreach_imp_provider(c),
        m_elementsToTake(n),
        m_stride(stride) {}

    template<typename F>
    void take_foreach_imp(F f, details::foreach_instructions how = {}) const {
        size_t n = 0;
        m_foreach_imp_provider.foreach_imp([f, &n, this](typename interface::argument_type el) {
            if (n >= m_elementsToTake)
                return false;
            if (n % m_stride == 0) {
                const bool go = f(el);
                if (not go)
                    return false;
            }
            n++;
            return true;
            }, how);
    }

    template<typename F>
    void skip_foreach_imp(F f, details::foreach_instructions how = {}) const {
        size_t n = 0;
        m_foreach_imp_provider.foreach_imp([f, &n, this](typename interface::argument_type el) {
            if (n >= m_elementsToTake) {
                if (n % m_stride == 0) {
                    const bool go = f(el);
                    if (not go)
                        return false;
                }
            }
            n++;
            return true;
            }, how);
    }


    template<typename F>
    void foreach_imp(F f, details::foreach_instructions how = {}) const {
        if (logic == details::take_elements) take_foreach_imp(f, how);
        else skip_foreach_imp(f, how);
    }

private:
    const Container m_foreach_imp_provider;
    size_t m_elementsToTake;
    size_t m_stride;
};


template<typename Container>
class ReverseView : public FunctionalInterface<ReverseView<Container>, typename Container::value_type> {
public:
    using interface = FunctionalInterface<ReverseView<Container>, typename Container::value_type>;
    using value_type = typename interface::value_type;
    static constexpr bool is_finite = true;
    static constexpr bool is_permanent = Container::is_permanent;
    static_assert(Container::is_finite, "Only finite containers can be reversed");

    ReverseView(const Container& c)
        : interface(*this),
        m_foreach_imp_provider(c) {}

    template<typename F, typename Q = value_type>
    typename std::enable_if< std::is_pointer<Q>::value, void>::type foreach_imp(F f, details::foreach_instructions how = {}) const {
        std::vector< value_type > lookup;
        m_foreach_imp_provider.foreach_imp([&lookup](const value_type& el) {
            lookup.push_back(el); return true; });
        for (auto it = lookup.rbegin(); it != lookup.rend(); ++it) {
            const bool go = f(*it);
            if (not go)
                break;
        }
    }

    template<typename F, typename Q = value_type>
    typename std::enable_if< !std::is_pointer<Q>::value, void>::type foreach_imp(F f, details::foreach_instructions how = {}) const {
        std::vector< std::reference_wrapper<const value_type>> lookup;
        m_foreach_imp_provider.foreach_imp([&lookup](const value_type& el) {
            lookup.push_back(std::cref(el)); return true; });
        for (auto it = lookup.rbegin(); it != lookup.rend(); ++it) {
            const bool go = f(it->get());
            if (not go)
                break;
        }
    }

private:
    const Container m_foreach_imp_provider;
};


template<typename Container>
struct select_type_for_enumerated_view {
    using value_type = typename std::conditional< Container::is_permanent,
        typename Container::const_reference_type,
        typename Container::value_type
    >::type;
    using type = std::pair<size_t, value_type>;
};
template<typename Container>
class EnumeratedView : public FunctionalInterface<EnumeratedView<Container>, typename select_type_for_enumerated_view<Container>::type > {
public:
    using interface = FunctionalInterface<EnumeratedView<Container>, typename select_type_for_enumerated_view<Container>::type >;
    using value_type = typename interface::value_type;
    static constexpr bool is_finite = Container::is_finite;
    static constexpr bool is_permanent = false;



    EnumeratedView(const Container& c, size_t offset)
        : interface(*this),
        m_foreach_imp_provider(c),
        m_offset(offset) {}


    template<typename F>
    void foreach_imp(F f, details::foreach_instructions how = {}) const {
        size_t index = m_offset;
        m_foreach_imp_provider.foreach_imp([f, &index](const auto& el) {
            const bool go = f(value_type(index, el));
            if (not go)
                return false;
            ++index;
            return true;
            }, how);
    }
private:
    const Container m_foreach_imp_provider;
    size_t m_offset;
};


template< typename Container, typename Filter, details::skip_take_logic_t logic>
class TakeSkipWhileView : public FunctionalInterface<TakeSkipWhileView<Container, Filter, logic>, typename Container::value_type> {
public:
    using interface = FunctionalInterface<TakeSkipWhileView<Container, Filter, logic>, typename Container::value_type>;
    using value_type = typename interface::value_type;
    static constexpr bool is_finite = logic == details::take_elements or Container::is_finite;


    TakeSkipWhileView(const Container& c, Filter f)
        : FunctionalInterface<TakeSkipWhileView<Container, Filter, logic>, value_type >(*this),
        m_foreach_imp_provider(c),
        m_filterOp(f) {};

    template<typename F>
    void take_foreach_imp(F f, details::foreach_instructions how = {}) const {
        m_foreach_imp_provider.foreach_imp([f, this](typename interface::argument_type el) {
            if (not m_filterOp(el))
                return false;
            const bool go = f(el);
            if (not go)
                return false;
            return true;
            }, how);
    }

    template<typename F>
    void skip_foreach_imp(F f, details::foreach_instructions how = {}) const {
        bool keep_taking = false;
        m_foreach_imp_provider.foreach_imp([f, &keep_taking, this](typename interface::argument_type el) {
            if (not keep_taking) {
                if (not m_filterOp(el)) {
                    keep_taking = true;
                }
            }
            if (keep_taking) {
                const bool go = f(el);
                if (not go)
                    return false;
            }
            return true;
            }, how);
    }

    template<typename F>
    void foreach_imp(F f, details::foreach_instructions how = {}) const {
        if (logic == details::take_elements) take_foreach_imp(f, how);
        else skip_foreach_imp(f, how);
    }


    TakeSkipWhileView() = delete;
private:
    const Container m_foreach_imp_provider;
    Filter m_filterOp;
};


template<typename Container1, typename Container2, typename ExposedType>
class ChainView : public FunctionalInterface<ChainView<Container1, Container2, ExposedType>, ExposedType> {
public:
    using interface = FunctionalInterface<ChainView<Container1, Container2, ExposedType>, ExposedType>;
    using value_type = typename interface::value_type;

    static_assert(std::is_base_of<value_type, typename Container1::value_type>::value
        || std::is_same<value_type, typename Container1::value_type>::value
        , "Objects in 1st container do not derive from requested type or is not the same");
    static_assert(std::is_base_of<value_type, typename Container2::value_type>::value
        || std::is_same<value_type, typename Container2::value_type>::value
        , "Objects in 1st container do not derive from requested type or is not the same");

    //    static_assert(std::is_same<typename Container1::value_type, typename Container2::value_type>::value, "chained containers have to provide same stored type");
    static constexpr bool is_finite = Container1::is_finite && Container2::is_finite;


    ChainView(const Container1& c1, const Container2& c2)
        : interface(*this),
        m_foreach_imp_provider1(c1),
        m_foreach_imp_provider2(c2) {}

    template<typename F>
    void foreach_imp(F f, details::foreach_instructions how = {}) const {
        bool need_to_access_second_container = true;
        m_foreach_imp_provider1.foreach_imp([f, &need_to_access_second_container](typename interface::argument_type el) {
            const bool go = f(el);
            if (not go) {
                need_to_access_second_container = false;
                return false;
            }
            return true;
            }, how);
        if (not need_to_access_second_container) return;
        m_foreach_imp_provider2.foreach_imp([f](typename interface::argument_type el) {
            const bool go = f(el);
            if (not go) {
                return false;
            }
            return true;
            }, how);
    }

private:
    const Container1 m_foreach_imp_provider1;
    const Container2 m_foreach_imp_provider2;

};

template<typename Container1, typename Container2>
class ZipView : public FunctionalInterface<ZipView<Container1, Container2>, typename std::pair<typename Container1::interface::const_value_type, typename Container2::interface::const_value_type> > {
public:
    using interface = FunctionalInterface<ZipView<Container1, Container2>, typename std::pair<typename Container1::interface::const_value_type, typename Container2::interface::const_value_type> >;
    using value_type = typename interface::value_type;
    static constexpr bool is_permanent = Container1::is_permanent && Container2::is_permanent;
    static constexpr bool is_finite = Container1::is_finite or Container2::is_finite;

    static_assert(details::has_fast_element_access_tag<Container1>::value or details::has_fast_element_access_tag<Container2>::value, "At least one container needs to to provide fast element access, consider calling stage() ");

    ZipView(const Container1& c1, const Container2& c2)
        : interface(*this),
        m_foreach_imp_provider1(c1),
        m_foreach_imp_provider2(c2) {}

    template<typename F>
    void foreach_imp(F f, details::foreach_instructions how = {}) const {
        size_t index = 0;
        if (details::has_fast_element_access_tag<Container2>::value) {
            m_foreach_imp_provider1.foreach_imp([f, &index, this](typename Container1::argument_type el1) {
                auto el2_option = m_foreach_imp_provider2.element_at(index);
                if (el2_option.has_value() == false) // reached end of container 2
                    return false;
                const bool go = f(std::pair<typename Container1::interface::argument_type, typename Container2::interface::argument_type>(el1, el2_option.value()));
                if (not go)
                    return false;
                index++;
                return true;
                }, how);
        }
        else {
            m_foreach_imp_provider2.foreach_imp([f, &index, this](typename Container2::argument_type el2) {
                auto el1_option = m_foreach_imp_provider1.element_at(index);
                if (el1_option.has_value() == false)
                    return false;
                const bool go = f(std::pair<typename Container1::interface::argument_type, typename Container2::interface::argument_type>(el1_option.value(), el2));
                if (not go)
                    return false;
                index++;
                return true;
                }, how);
        }
    }
private:
    const Container1 m_foreach_imp_provider1;
    const Container2 m_foreach_imp_provider2;
};


template<typename Container1, typename Container2>
class CartesianView : public FunctionalInterface<CartesianView<Container1, Container2>, typename std::pair<typename Container1::interface::const_value_type, typename Container2::interface::const_value_type> > {
public:
    using interface = FunctionalInterface<CartesianView<Container1, Container2>, typename std::pair<typename Container1::interface::const_value_type, typename Container2::interface::const_value_type> >;
    using value_type = typename interface::value_type;
    static constexpr bool is_permanent = false;
    static_assert(Container1::is_finite&& Container2::is_finite, "Cartesian product makes sense only for finite containers");
    static constexpr bool is_finite = true;

    CartesianView(const Container1& c1, const Container2& c2)
        : interface(*this),
        m_foreach_imp_provider1(c1),
        m_foreach_imp_provider2(c2) {}

    template<typename F>
    void foreach_imp(F f, details::foreach_instructions how = {}) const {
        bool can_go = true;
        m_foreach_imp_provider1.foreach_imp([f, this, how, &can_go](typename Container1::interface::argument_type el1) {
            m_foreach_imp_provider2.foreach_imp([f, el1, &can_go](typename Container2::interface::argument_type el2) {
                const bool go = f(std::pair<typename Container1::interface::argument_type, typename Container2::interface::argument_type>(el1, el2));
                if (not go) {
                    can_go = false;
                    return false;
                }
                return true;
                }, how);
            if (not can_go)
                return false;
            return true;
            }
        , how);
    }
private:
    const Container1 m_foreach_imp_provider1;
    const Container2 m_foreach_imp_provider2;
};



template<typename Container>
class DirectView final : public FunctionalInterface<DirectView<Container>, typename Container::value_type> {
public:
    using interface = FunctionalInterface<DirectView<Container>, typename Container::value_type>;
    using value_type = typename interface::value_type;
    using const_value_type = typename interface::const_value_type;
    static constexpr bool is_permanent = true;
    static constexpr bool is_finite = true;

    DirectView(const Container& m)
        : interface(*this), m_data(m) {}

    template<typename F>
    void foreach_imp(F f, details::foreach_instructions = {}) const {
        for (const auto& el : m_data.get()) {
            const bool go = f(el);
            if (not go)
                break;
        }
    }

    using optional_value = std::optional<const_value_type>;
    optional_value element_at(size_t  n) const {
        if (n < m_data.get().size())
            return m_data.get().at(n);
        return {};
    }
    size_t size() const {
        return m_data.get().size();
    }
    void update_container(const Container& m) {m_data = m;}
private:
    std::reference_wrapper<const Container> m_data;
};
// only for internal use (expensive and insecure to copy)
template<typename T, template<typename, typename> typename Container>
class OwningView final : public FunctionalInterface<OwningView<T, Container>, T> {
public:
    using interface = FunctionalInterface<OwningView<T, Container>, T>;
    using value_type = typename interface::value_type;
    using const_value_type = typename interface::const_value_type;
    using container = OwningView<T, Container>;
    static constexpr bool is_permanent = true;
    static constexpr bool is_finite = true;

    OwningView()
        : interface(*this) {}

    OwningView(const Container<T, std::allocator<T>>& data)
        : interface(*this),
        m_data(data) {}

    OwningView(const Container<T, std::allocator<T>>&& data)
        : interface(*this),
        m_data(data) {}

    template<typename C>
    explicit OwningView(const C& data)
        : interface(*this) {
        for (auto el : data)
            m_data.push_back(el);
    }

    template<typename O>
    const OwningView& operator=(const O& rhs) {
        m_data.clear();
        rhs.push_back_to(m_data);
        return *this;
    }

    void insert(typename interface::argument_type d) { m_data.push_back(d); }

    void clear() { m_data.clear(); }

    Container<T, std::allocator<T>>& _underlying() { return m_data; }

    template<typename F>
    void foreach_imp(F f, details::foreach_instructions = {}) const {
        for (const auto& el : m_data) {
            const bool go = f(el);
            if (not go)
                break;
        }
    }

    auto element_at(size_t  n) const -> std::optional<const_value_type> {
        if (n < m_data.size())
            return m_data.at(n);
        return {};
    }
    size_t size() const {
        return m_data.size();
    }
private:
    Container<T, std::allocator<T>> m_data;
};


template<typename Container, typename S>
class InspectView : public FunctionalInterface<InspectView<Container, S>, typename Container::value_type> {
public:
    using interface = FunctionalInterface<InspectView<Container, S>, typename Container::value_type>;
    using value_type = typename interface::value_type;
    static constexpr bool is_finite = Container::is_finite;
    static constexpr bool is_permanent = Container::is_permanent;


    InspectView(const Container& c, S subroutine)
        : FunctionalInterface<InspectView<Container, S>, typename Container::value_type>(*this),
        m_foreach_imp_provider(c),
        m_subroutine(subroutine) {}

    template<typename F>
    void foreach_imp(F f, details::foreach_instructions how = {}) const {
        m_foreach_imp_provider.foreach_imp([this, f](const auto& el) -> bool {
            m_subroutine(el);
            return f(el); },
            how);
    }
private:
    const Container m_foreach_imp_provider;
    S m_subroutine;
};


template<typename T, template<typename, typename> typename Container>
class RefView final : public FunctionalInterface<RefView<T, Container>, T> {
public:
    using interface = FunctionalInterface<RefView<T, Container>, T>;
    using value_type = typename interface::value_type;
    static constexpr bool is_permanent = true;
    static constexpr bool is_finite = true;

    RefView()
        : interface(*this) {}

    void insert(typename interface::argument_type d) { m_data.push_back(std::ref(d)); }
    void clear() { m_data.clear(); }

    template<typename F>
    void foreach_imp(F f, details::foreach_instructions = {}) const {
        for (const auto& el : m_data) {
            const bool go = f(el.get());
            if (not go)
                break;
        }
    }

    auto element_at(size_t  n) const -> std::optional<value_type> {
        if (n < m_data.size())
            return m_data.at(n);
        return {};
    }
    size_t size() const {
        return m_data.size();
    }

    auto& _underlying() { return m_data; }

private:
    Container<std::reference_wrapper<const T>, std::allocator<std::reference_wrapper<const T>>> m_data;
};


template<typename T>
class Series : public FunctionalInterface<Series<T>, T > {
public:
    using interface = FunctionalInterface<Series<T>, T >;
    using value_type = typename interface::value_type;
    static constexpr bool is_permanent = false;
    static constexpr bool is_finite = false;


    Series(std::function<T(const T&)> gen, const T& start, const T& stop = std::numeric_limits<T>::max())
        : interface(*this),
        m_generator(gen),
        m_start(start),
        m_stop(stop) {}

    template<typename F>
    void foreach_imp(F f, details::foreach_instructions = {}) const {
        T current = m_start;
        while (current < m_stop) {
            const bool go = f(current);
            if (not go)
                break;
            current = m_generator(current);
        }
    }

private:
    std::function<T(const T&)> m_generator;
    T m_start;
    T m_stop;
};


template<typename T>
class Range : public FunctionalInterface<Range<T>, T > {
public:
    using interface = FunctionalInterface<Range<T>, T >;
    using value_type = typename interface::value_type;
    using const_value_type = const value_type;
    static constexpr bool is_permanent = false;
    static constexpr bool is_finite = true;
    static_assert(std::is_arithmetic<value_type>::value, "Can't generate range of not arithmetic type");

    Range(const T& begin, const T& end, const T& step = 1)
        : interface(*this),
        m_begin(begin),
        m_end(end),
        m_step(step) {
        if (m_step == 0) throw std::runtime_error("the step can be zero");
        if (m_step > 0 and m_begin > m_end) throw std::runtime_error("limits and step will result in an infinite range");
        if (m_step < 0 and m_begin < m_end) throw std::runtime_error("limits and step will result in an infinite range");

    }

    template<typename F>
    void foreach_imp(F f, details::foreach_instructions = {}) const {
        T current = m_begin;
        while ((m_step > 0 and current < m_end) or (m_step < 0 and current > m_end)) {
            const bool go = f(current);
            if (not go)
                break;
            current = current + m_step;
            // std::cout << "X" << current << std::endl;
        }
    }

    auto element_at(size_t  n) const -> std::optional<const_value_type> {
        if (n < size())
            return m_begin + m_step * n;
        return {};
    }
    size_t size() const {
        return std::abs((m_end - m_begin)) / std::abs(m_step);
    }


private:
    T m_begin;
    T m_end;
    T m_step;
};


template<typename T>
class One : public FunctionalInterface<One<T>, T> {
public:
    using interface = FunctionalInterface<One<T>, T >;
    using value_type = typename interface::value_type;
    using const_value_type = const value_type;
    static constexpr bool is_permanent = true;
    static constexpr bool is_finite = true;
    One(const T& data)
        : interface(*this),
        m_data(data) {}

    template<typename F>
    void foreach_imp(F f, details::foreach_instructions = {}) const {
        f(m_data);
    }
    auto element_at(size_t  n) const -> std::optional<const_value_type> {
        if (n == 0)
            return m_data;
        return {};
    }
    size_t size() const { return 1; }


private:
    T m_data;
};

template<typename Data>
class ArrayView final : public FunctionalInterface<ArrayView<Data>, Data> {
public:
    using interface = FunctionalInterface<ArrayView<Data>, Data>;
    using value_type = typename interface::value_type;
    using const_value_type = typename interface::const_value_type;
    static constexpr bool is_permanent = true;
    static constexpr bool is_finite = true;

    ArrayView(const Data* arr, size_t sz)
        : interface(*this), m_data(arr), m_size(sz) {}

    template<typename F>
    void foreach_imp(F f, details::foreach_instructions = {}) const {
        for ( size_t index = 0; index < m_size; ++index){
            const bool go = f(m_data[index]);
            if (not go)
                break;
        }
    }

    using optional_value = std::optional<const_value_type>;
    optional_value element_at(size_t  n) const {
        if (n < m_size)
            return m_data[n];
        return {};
    }
    size_t size() const {
        return m_size;
    }
    void update_container(const Data* arr, const size_t sz) {m_data = arr; m_size = sz; }
private:
    const Data* m_data;
    size_t m_size;
};

// infinite series of doubles following the geometric series recipe
template<typename T, typename U>
Series<T> geometric_stream(T coeff, U ratio) {
    return Series<T>([ratio](T c) { return c * ratio; }, coeff);
}

// infinite series starting from initial value incremented by the increment
template<typename T>
Series<T> arithmetic_stream(T initial, T increment) {
    return Series<T>([increment](double c) { return c + increment; }, initial);
}

// infinite series if integers incremented by 1
template<typename T = size_t>
Series<T> iota_stream(T initial = 0) {
    return Series<T>([](T c) { return c + 1; }, initial);;
}

// random integers using rand from c stdlib
// @warning - not high quality randomization
// @warning - it is not reproducible
template<typename T = int>
Series<T> crandom_stream() {
    return Series<T>([](T) { return static_cast<T>(rand()); }, rand());
}

// completely user fined stream, eg. the provided functions takes care of all
// this is by construction an infinite stream unless the provided function returns max value for type T
template<typename T>
Series<T> free_stream( std::function<T(T)> f) {    
    return Series<T>(f, f({}));
}


// finite range from x = begin until x < end
template<typename T>
Range<T> range_stream(T begin, T end, T step = 1) {
    return Range<T>(begin, end, step);
}

// view the data in the container
template<typename T>
auto lazy_view(const T& cont) {
    return DirectView(cont);
}

//!< view the data in plain array
template<typename T>
auto lazy_view(const T* array, size_t size) {
    return ArrayView(array, size);
}
//!< see above
template<typename T>
auto lazy_view(const T* array, const T* arrayend) {
    return ArrayView(array, arrayend-array);
}

//!< view one data piece (contrary to the views the data is copied & owned by the container)
template<typename T>
auto one_own(const T& ele) {
    return One(ele);
}



namespace details {
    template<typename T> struct has_fast_element_access_tag<DirectView<T>> { static constexpr bool value = true; };
    template<typename T> struct has_fast_element_access_tag<OwningView<std::vector<T>>> { static constexpr bool value = true; };
    template<typename T> struct has_fast_element_access_tag<OwningView<T, std::vector>> { static constexpr bool value = true; };
    template<typename T> struct has_fast_element_access_tag<RefView<T, std::vector>> { static constexpr bool value = true; };
    template<typename T> struct has_fast_element_access_tag<RefView<T, std::deque>> { static constexpr bool value = true; };
    template<typename T> struct has_fast_element_access_tag<Range<T>> { static constexpr bool value = true; };
    template<typename T> struct has_fast_element_access_tag<ArrayView<T>> { static constexpr bool value = true; };

}



} // eof lfv namespace
#undef AS_NEEDED



/**
 * Helper class enabling procssing of the Access objects alike the lazy container
 * The tree is considered to be an infinite container (so some functions, that are anyways useless, are not available)
 * Example:
 * class MyData : public ROOTTreeAccess {...};
 * TreeView<MyData> data( treePtr );
 * data.filter(...).take(...).skip(...).foreach(...);
 * For instance to simply count the elements passing the criteria:
 * data.count( [](auto d){ return d... }); // predicate
 * Typical use is though to just do: data.foreach( ...lambda... );
 **/
template<typename AccessDerivative>
class AccessView : public lfv::FunctionalInterface<AccessView<AccessDerivative>, AccessDerivative> {
public:
	using interface = lfv::FunctionalInterface<AccessView<AccessDerivative>, AccessDerivative>;
	static constexpr bool is_permanent = false;
	static constexpr bool is_finite = false;

	AccessView(AccessDerivative& t)
		: interface(*this),
		m_access(t) {}

	template<typename F>
	void foreach_imp(F f, lfv::details::foreach_instructions = {}) const {
		for (; m_access.get(); ++m_access.get()) { // this is what the wrapped class needs to provide
			const bool go = f(m_access.get());
			if (not go) break;
		}
	}

private:
	std::reference_wrapper<AccessDerivative> m_access;

};
#endif