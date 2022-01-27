// Copyright 2022, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)
#ifndef LazyFunctionalVector_h
#define LazyFunctionalVector_h
#include <type_traits>
#include <set>
#include <deque>
#include <cmath>

#include "futils.h"
#include "EagerFunctionalVector.h"


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

namespace lfv_details {
    enum skip_take_logic_t { skip_elements, take_elements };
    enum min_max_logic_t { min_elements, max_elements };
    enum preserve_t { temporary, store }; // instruct if the results of for-each will be refereed to afterwards
    struct foreach_instructions {
        preserve_t preserve = temporary;
    };

    template<typename T>
    struct one_element_stack_container {
        T* ptr = nullptr;
        char data[sizeof(T)];

        template<typename U>
        void insert(const U& el) {
            static_assert(std::is_same<U, T>::value, "one_element_container cant handle plymorphic data");
            if (ptr != nullptr) throw std::runtime_error("one_element_container already has content");
            ptr = new (reinterpret_cast<T*>(data)) T(el);
        }
        template<typename U>
        void replace(const U& el) {
            static_assert(std::is_same<U, T>::value, "one_element_container cant handle plymorphic data");
            ptr = new (reinterpret_cast<T*>(data)) T(el);
        }
        bool empty() const { return ptr == nullptr; }

        T& get() { return *ptr; }
    };
}




template< typename Container, typename Filter> class FilteredView;
template< typename Container, typename Map> class MappedView;
template<typename T, template<typename, typename> typename Container = std::vector> class DirectView;
template<typename T, template<typename, typename> typename Container = std::vector> class OwningView;
template<typename T, template<typename, typename> typename Container = std::vector> class RefView;

template<typename Container, lfv_details::skip_take_logic_t> class TakeSkipNView;
template<typename Container> class NView;
template<typename Container> class EnumeratedView;
template<typename Container> class ReverseView;
template<typename Container> class CachedView;
template<typename Container, typename Subroutine> class InspectView;
template<typename Container, typename Filter, lfv_details::skip_take_logic_t> class TakeSkipWhileView;
template<typename Container1, typename Container2> class ChainView;
template<typename Container1, typename Container2> class ZipView;
template<typename Container1, typename Container2> class CartesianView;
template<typename Container> class CombinationsView;
template<typename Container> class PermutationsView;
template<typename Container1, typename Comparator> class SortedView;
template<typename Container1, typename Comparator> class MMView;
template<typename T> class Range;

namespace lfv_details{
    template<typename C> struct has_fast_element_access_tag { static constexpr bool value = false; };
    template<typename T> struct has_fast_element_access_tag<DirectView<T, std::vector>> { static constexpr bool value = true; };
    template<typename T> struct has_fast_element_access_tag<OwningView<T, std::vector>> { static constexpr bool value = true; };
    template<typename T> struct has_fast_element_access_tag<DirectView<T, std::deque>> { static constexpr bool value = true; };
    template<typename T> struct has_fast_element_access_tag<OwningView<T, std::deque>> { static constexpr bool value = true; };
    template<typename T> struct has_fast_element_access_tag<RefView<T, std::vector>> { static constexpr bool value = true; };
    template<typename T> struct has_fast_element_access_tag<RefView<T, std::deque>> { static constexpr bool value = true; };
    template<typename T> struct has_fast_element_access_tag<Range<T>> { static constexpr bool value = true; };
}

namespace lfv {
    constexpr size_t all_elements = std::numeric_limits<size_t>::max();
    constexpr size_t invalid_index = std::numeric_limits<size_t>::max();
}



template<typename Container, typename Stored>
class FunctionalInterface {
public:
    using value_type = Stored;
    using const_reference_type = const value_type&;
    static constexpr auto id = [](const Stored& x){ return x; };

    FunctionalInterface(const Container& cont) : m_actual_container(cont) {}
    ~FunctionalInterface() {}

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
        m_actual_container.foreach_imp([f](const_reference_type el) { f(el); return true; });
        return *this;
    }

    template<typename S>
    auto inspect(S s) const -> InspectView<Container, S> {
        return InspectView<Container, S>(m_actual_container, s);
    }


    // count of elements, this is an eager operation
    virtual size_t size() const {
        size_t c = 0;
        m_actual_container.foreach_imp([&c](const_reference_type) { c++; return true;});
        return c;
    }

    virtual bool empty() const {
        size_t c = 0;
        m_actual_container.foreach_imp([&c](const_reference_type) { c++; return false;});
        return c == 0;
    }


    // count element satisfying predicate, this is an eager operation
    template<typename Predicate>
    size_t count(Predicate pred) const {
        size_t c = 0;
        m_actual_container.foreach_imp([&c, pred](const_reference_type el) {
            c += (pred(el) ? 1 : 0); return true; });
        return c;
    }

    // finds if an element is in the container
    // sometimes referetd to as any()
    // returns false for empty container
    template<typename Predicate>
    bool contains(Predicate f) const {
        bool found = false;
        m_actual_container.foreach_imp([f, &found](const_reference_type el) {
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
    auto sorted(F f = id) const {
        static_assert(Container::is_finite, "Can't sort in an infinite container");
        static_assert(Container::is_permanent, "Can't sort container that generates element on the flight, stage() it before");
        return SortedView<Container, F>(m_actual_container, f);
    }

    // selectors, lazy
    auto take(size_t n, size_t stride = 1) const {
        return TakeSkipNView<Container, lfv_details::take_elements>(m_actual_container, n, stride);
    }

    auto skip(size_t n, size_t stride = 1) const {
        return TakeSkipNView<Container, lfv_details::skip_elements>(m_actual_container, n, stride);
    }

    // produces container with indexed data
    auto enumerate(size_t offset = 0) const {
        return EnumeratedView<Container>(m_actual_container, offset);
    }

    template<typename F>
    auto take_while(F f) const {
        return TakeSkipWhileView<Container, F, lfv_details::take_elements>(m_actual_container, f);
    }

    template<typename F>
    auto skip_while(F f) const {
        return TakeSkipWhileView<Container, F, lfv_details::skip_elements>(m_actual_container, f);
    }

    // creates intermediate container to speed following up operations, typically useful after sorting
    auto stage() const -> OwningView<Stored> {
        static_assert(Container::is_finite, "Can't stage an infinite container");
        OwningView<Stored> c;
        m_actual_container.foreach_imp([&c](const_reference_type el) { c.insert(el); return true; });
        return c;
    }

    // lazy stage, that is, the data will be staged when needed
    auto cache() const -> CachedView<Container> {
        static_assert(Container::is_finite, "Can't cache an infinite container");
        return CachedView<Container>(m_actual_container);
    }

    auto reverse() const {
        static_assert(Container::is_finite, "Can't reverse an infinite container");
        return ReverseView<Container>(m_actual_container);
    }

    // max and min
    template<typename KeyExtractor = decltype(id)>
    auto max(KeyExtractor by = id) const {
        static_assert(Container::is_finite, "Can't find max in an infinite container");
        return MMView<Container, KeyExtractor>(m_actual_container, by, lfv_details::max_elements);
    }
    template<typename KeyExtractor = decltype(id)>
    auto min(KeyExtractor by = id) const {
        static_assert(Container::is_finite, "Can't find min in an infinite container");
        return MMView<Container, KeyExtractor>(m_actual_container, by, lfv_details::min_elements);
    }


    // concatenate two containers, in a lazy way
    template<typename Other>
    auto chain(const Other& c) const {
        static_assert(Container::is_finite, "Can't chain with an infinite container");

        return ChainView<Container, Other>(m_actual_container, c);
    }

    // combine pairwise
    // the implementation is suboptimal, that is, it involves inexed access to one of the containers, it is therefore better if one of them is staged
    template<typename Other>
    auto zip(const Other& c) const {
        static_assert(Container::is_finite or Other::is_finite, "Can't compare infinite containers");
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

    // sum, allows to transform the object before summation (i.e. can act like map + sum)
    template<typename F = decltype(id)>
    auto sum(F f =  id) const {
        static_assert(Container::is_finite, "Can't sum an infinite container");
        typename std::remove_reference<typename std::invoke_result<F, typename Container::const_reference_type>::type>::type s = {};
        m_actual_container.foreach_imp([&s, f](const_reference_type el) { 
            s = s + f(el); 
            return true; });
        return s;
    }
    // accumulate, function us used as: total = f(total, element), also take starting element
    template<typename F, typename R>
    auto accumulate(F f, R initial = {}) const {
        static_assert(Container::is_finite, "Can't sum an infinite container");
        static_assert(std::is_same<R, typename std::invoke_result<F, R, typename Container::const_reference_type>::type>::value, "function return type different than initial value");
        R s = initial;
        m_actual_container.foreach_imp([&s, f](const_reference_type el) { s = f(s, el);  return true; });
        return s;
    }

    template<typename F = decltype(id)>
    StatInfo stat(F f = id) const {
        static_assert(std::is_arithmetic< typename std::remove_reference<typename std::invoke_result<F, Stored>::type>::type >::value, "The type stored in this container is not arithmetic, can't calculate standard statistical properties");
        StatInfo info;
        m_actual_container.foreach_imp([&info, f](const_reference_type el) {
            auto v = f(el);
            info.count++;
            info.sum += v;
            info.sum2 += std::pow(v, 2);
            return true;
            });
        return info;
    }


    // access by index (not good idea to overuse)
    virtual auto element_at(size_t  n) const -> std::optional<value_type> {
        lfv_details::one_element_stack_container<value_type> temp;
        size_t i = 0;
        m_actual_container.foreach_imp([&temp, n, &i](const_reference_type el) {
            if (i == n) {
                temp.insert(el);
                return false;
            }
            i++;
            return true;
            });
        return temp.empty() ? std::optional<value_type>() : temp.get();
    }

    virtual auto get() const -> std::optional<value_type> {
        return element_at(0);
    }


    // first element satisfying predicate
    template<typename Predicate>
    auto first_of(Predicate f) const -> std::optional<value_type> {
        lfv_details::one_element_stack_container<value_type> temp;
        m_actual_container.foreach_imp([&temp, f](const_reference_type el) {
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
        m_actual_container.foreach_imp([&i, f, &found](const_reference_type el) {
            if (f(el)) {
                found = i;
                return false;
            }
            ++i;
            return true;
            });
        return found;
    }

    // applies point free transforms on actual data
    template<typename T>
    auto apply(const std::vector<T>& data) {
        //        retrun DirectView(data).go();
    }

    // back to regular vector
    template<typename R>
    void push_back_to(R& result) const {
        static_assert(Container::is_finite, "Can't save an infinite container");
        m_actual_container.foreach_imp([&result](const_reference_type el) {
            result.push_back(el); return true;
            });
    }

    template<typename R>
    void insert_to(R& result) const {
        static_assert(Container::is_finite, "Can't save an infinite container");
        m_actual_container.foreach_imp([&result](const_reference_type el) {
            result.insert(el); return true;
            });
    }
    // to non-lazy implementation
    EagerFunctionalVector<value_type> as_eager() const {
        static_assert(Container::is_finite, "Can't save an infinite container");
        EagerFunctionalVector<Stored> tmp;
        m_actual_container.foreach([&tmp](const_reference_type el) { tmp.__push_back(el); return true; });
        return tmp;
    }

protected:
    const Container& m_actual_container;
};



// provides view of subset of element which satisfy the predicate
template< typename Container, typename Filter>
class FilteredView : public FunctionalInterface<FilteredView<Container, Filter>, typename Container::value_type> {
public:
    using value_type = typename Container::value_type;
    using reference_type = value_type&;
    using const_reference_type = const value_type&;
    static constexpr bool is_finite = Container::is_finite;

    FilteredView(const Container& c, Filter f)
        : FunctionalInterface<FilteredView<Container, Filter>, value_type >(*this),
        m_foreach_imp_provider(c),
        m_filterOp(f) {};

    template<typename F>
    void foreach_imp(F f, lfv_details::foreach_instructions how = {}) const {
        m_foreach_imp_provider.foreach_imp([f, this](const_reference_type el) {
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
    const Container& m_foreach_imp_provider;
    Filter m_filterOp;
};


template< typename Container, typename KeyExtractor>
class SortedView : public FunctionalInterface<SortedView<Container, KeyExtractor>, typename Container::value_type> {
public:
    using value_type = typename Container::value_type;
    using reference_type = value_type&;
    using const_reference_type = const value_type&;
    static constexpr bool is_finite = Container::is_finite;

    SortedView(const Container& c, KeyExtractor f)
        : FunctionalInterface<SortedView<Container, KeyExtractor>, value_type >(*this),
        m_foreach_imp_provider(c),
        m_keyExtractor(f) {};

    template<typename F>
    void foreach_imp(F f, lfv_details::foreach_instructions how = {}) const {
        std::vector< std::reference_wrapper<const value_type>> lookup;
        m_foreach_imp_provider.foreach_imp([&lookup](const_reference_type el) { lookup.push_back(std::cref(el)); return true; }, how);
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
    const Container& m_foreach_imp_provider;
    KeyExtractor m_keyExtractor;
};


// min max view
template< typename Container, typename KeyExtractor>
class MMView : public FunctionalInterface<MMView<Container, KeyExtractor>, typename Container::value_type> {
public:
    using value_type = typename Container::value_type;
    using reference_type = value_type&;
    using const_reference_type = const value_type&;
    using compared_type = typename std::remove_const<typename std::remove_reference<typename std::invoke_result<KeyExtractor, const_reference_type>::type>::type>::type;
    static constexpr bool is_finite = Container::is_finite;

    MMView(const Container& c, KeyExtractor f, lfv_details::min_max_logic_t logic)
        : FunctionalInterface<MMView<Container, KeyExtractor>, value_type >(*this),
        m_actual_container(c),
        m_keyExtractor(f),
        m_logic(logic) {};

    template<typename F>
    void foreach_imp(F f, lfv_details::foreach_instructions how = {}) const {
        if (m_actual_container.empty()) return;
        lfv_details::one_element_stack_container< compared_type > extremeValue;
        lfv_details::one_element_stack_container< std::reference_wrapper<const value_type> > extremeElement;

        m_actual_container.foreach_imp([&extremeValue, &extremeElement, this](const_reference_type el) {
            compared_type val = m_keyExtractor(el);
            if (extremeValue.empty()) {
                extremeValue.insert(val);
                extremeElement.insert(std::cref(el));
            }
            if ((m_logic == lfv_details::max_elements and val >= extremeValue.get())
                or
                (m_logic == lfv_details::min_elements and val < extremeValue.get())) {
                extremeValue.replace(val);
                extremeElement.replace(std::cref(el));
            }
            return true;
            }, how);
        if (not extremeElement.empty())
            f(extremeElement.get().get());
    }
    MMView() = delete;
private:
    const Container& m_actual_container;
    KeyExtractor m_keyExtractor;
    lfv_details::min_max_logic_t m_logic;
};


// provides the transforming view
template< typename Container, typename Mapping>
class MappedView : public FunctionalInterface<MappedView<Container, Mapping>, typename std::invoke_result<Mapping, typename Container::value_type>::type > {
public:
    using processed_type = typename Container::value_type;
    using value_type = typename std::invoke_result<Mapping, processed_type>::type;
    using const_value_type = const value_type;
    using const_reference_type = const_value_type&;
    using container = FunctionalInterface<Container, value_type>;
    static constexpr bool is_finite = Container::is_finite;
    static constexpr bool is_permanent = false;



    MappedView(const Container& c, Mapping m)
        : FunctionalInterface<MappedView<Container, Mapping>, value_type >(*this),
        m_foreach_imp_provider(c),
        m_mappingOp(m) {};

    template<typename F>
    void foreach_imp(F f, lfv_details::foreach_instructions how = {}) const {
        m_foreach_imp_provider.foreach_imp([f, this](const auto& el) -> bool {
            const bool go = f(m_mappingOp(el));
            if (not go)
                return false;
            return true;
            }, how);
    }
    MappedView() = delete;
private:
    const Container& m_foreach_imp_provider;
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
    using value_type = typename select_type_for_n_view<Container>::type;
    using const_value_type = const value_type;
    using const_reference_type = const_value_type&;
    static constexpr bool is_finite = Container::is_finite;

    NView(const Container& c, size_t n, size_t jump)
        : FunctionalInterface<NView<Container>, value_type>(*this),
        m_foreach_imp_provider(c),
        m_group(n),
        m_jump(jump) { }

    template<typename F>
    void foreach_imp(F f, lfv_details::foreach_instructions how = {}) const {
        value_type group;
        m_foreach_imp_provider.foreach_imp([f, this, &group](const auto& el) {
            group.insert(el);
            //                       std::cout << "NView " << group.size() << " " << el << std::endl;
            if (group.size() == m_group) {
                const bool go = f(group);
                if (not go)
                    return false;
                //                std::cout << m_group << " " << m_jump << " " << group.size() << std::endl;
                if (m_group == m_jump)
                    group.clear();
                else
                    for (size_t p = 0; p < m_jump; p++) group._underlying().pop_front();
            }
            return true;
            }, how);
    }
private:
    const Container& m_foreach_imp_provider;
    size_t m_group;
    size_t m_jump;
};

template<typename Container, lfv_details::skip_take_logic_t logic>
class TakeSkipNView : public FunctionalInterface<TakeSkipNView<Container, logic>, typename Container::value_type> {
public:
    using value_type = typename Container::value_type;
    using const_value_type = const value_type;
    using const_reference_type = const_value_type&;
    using container = FunctionalInterface<Container, value_type>;
    static constexpr bool is_finite = logic == lfv_details::take_elements or Container::is_finite;

    TakeSkipNView(const Container& c, size_t n, size_t stride = 1, lfv_details::skip_take_logic_t l = lfv_details::take_elements)
        : FunctionalInterface<TakeSkipNView<Container, logic>, typename Container::value_type>(*this),
        m_foreach_imp_provider(c),
        m_elementsToTake(n),
        m_stride(stride) {}

    template<typename F>
    void take_foreach_imp(F f, lfv_details::foreach_instructions how = {}) const {
        size_t n = 0;
        m_foreach_imp_provider.foreach_imp([f, &n, this](const_reference_type el) {
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
    void skip_foreach_imp(F f, lfv_details::foreach_instructions how = {}) const {
        size_t n = 0;
        m_foreach_imp_provider.foreach_imp([f, &n, this](const_reference_type el) {
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
    void foreach_imp(F f, lfv_details::foreach_instructions how = {}) const {
        if (logic == lfv_details::take_elements) take_foreach_imp(f, how);
        else skip_foreach_imp(f, how);
    }

private:
    const Container& m_foreach_imp_provider;
    size_t m_elementsToTake;
    size_t m_stride;
};


template<typename Container>
class ReverseView : public FunctionalInterface<ReverseView<Container>, typename Container::value_type> {
public:
    using value_type = typename Container::value_type;
    using const_value_type = const value_type;
    using const_reference_type = const_value_type&;
    using container = FunctionalInterface<Container, value_type>;
    static constexpr bool is_finite = true;
    static constexpr bool is_permanent = Container::is_permanent;
    static_assert(Container::is_finite, "Only finite containers can be reversed");

    ReverseView(const Container& c)
        : FunctionalInterface<ReverseView<Container>, typename Container::value_type>(*this),
        m_foreach_imp_provider(c) {}

    template<typename F>
    void foreach_imp(F f, lfv_details::foreach_instructions how = {}) const {
        std::vector< std::reference_wrapper<const value_type>> lookup;
        m_foreach_imp_provider.foreach_imp([&lookup](const_reference_type el) {
            // std::cout << "ReverseView inserting " <<  el << "\n";             
            lookup.push_back(std::cref(el)); return true; });

        // for ( auto el: lookup) {
        //     std::cout << "Reversed " <<  el << "\n";             
        // }
        for (auto it = lookup.rbegin(); it != lookup.rend(); ++it) {
            // std::cout << "Handing " << *it << "\n";
            const bool go = f(it->get());
            if (not go)
                break;
        }
    }

private:
    const Container& m_foreach_imp_provider;
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
    using value_type = typename select_type_for_enumerated_view<Container>::type;
    using const_value_type = const value_type;
    using const_reference_type = const_value_type&;
    using container = FunctionalInterface<Container, value_type>;
    static constexpr bool is_finite = Container::is_finite;
    static constexpr bool is_permanent = false;



    EnumeratedView(const Container& c, size_t offset)
        : FunctionalInterface<EnumeratedView<Container>, value_type>(*this),
        m_foreach_imp_provider(c),
        m_offset(offset) {}


    template<typename F>
    void foreach_imp(F f, lfv_details::foreach_instructions how = {}) const {
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
    const Container& m_foreach_imp_provider;
    size_t m_offset;
};


template< typename Container, typename Filter, lfv_details::skip_take_logic_t logic>
class TakeSkipWhileView : public FunctionalInterface<TakeSkipWhileView<Container, Filter, logic>, typename Container::value_type> {
public:
    using value_type = typename Container::value_type;
    using reference_type = value_type&;
    using const_reference_type = const value_type&;
    static constexpr bool is_finite = logic == lfv_details::take_elements or Container::is_finite;


    TakeSkipWhileView(const Container& c, Filter f)
        : FunctionalInterface<TakeSkipWhileView<Container, Filter, logic>, value_type >(*this),
        m_foreach_imp_provider(c),
        m_filterOp(f) {};

    template<typename F>
    void take_foreach_imp(F f, lfv_details::foreach_instructions how = {}) const {
        m_foreach_imp_provider.foreach_imp([f, this](const_reference_type el) {
            if (not m_filterOp(el))
                return false;
            const bool go = f(el);
            if (not go)
                return false;
            return true;
            }, how);
    }

    template<typename F>
    void skip_foreach_imp(F f, lfv_details::foreach_instructions how = {}) const {
        bool keep_taking = false;
        m_foreach_imp_provider.foreach_imp([f, &keep_taking, this](const_reference_type el) {
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
    void foreach_imp(F f, lfv_details::foreach_instructions how = {}) const {
        if (logic == lfv_details::take_elements) take_foreach_imp(f, how);
        else skip_foreach_imp(f, how);
    }


    TakeSkipWhileView() = delete;
private:
    const Container& m_foreach_imp_provider;
    Filter m_filterOp;
};


template<typename Container1, typename Container2>
class ChainView : public FunctionalInterface<ChainView<Container1, Container2>, typename Container1::value_type> {
public:
    using value_type = typename Container1::value_type;
    static_assert(std::is_same<typename Container1::value_type, typename Container2::value_type>::value, "chained containers have to provide same stored type");
    using const_value_type = const value_type;
    using const_reference_type = const_value_type&;
    static constexpr bool is_finite = Container1::is_finite && Container2::is_finite;


    ChainView(const Container1& c1, const Container2& c2)
        : FunctionalInterface<ChainView<Container1, Container2>, value_type>(*this),
        m_foreach_imp_provider1(c1),
        m_foreach_imp_provider2(c2) {}

    template<typename F>
    void foreach_imp(F f, lfv_details::foreach_instructions how = {}) const {
        bool need_to_access_second_container = true;
        m_foreach_imp_provider1.foreach_imp([f, &need_to_access_second_container](const_reference_type el) {
            const bool go = f(el);
            if (not go) {
                need_to_access_second_container = false;
                return false;
            }
            return true;
            }, how);
        if (not need_to_access_second_container) return;
        m_foreach_imp_provider2.foreach_imp([f](const_reference_type el) {
            const bool go = f(el);
            if (not go) {
                return false;
            }
            return true;
            }, how);
    }

private:
    const Container1& m_foreach_imp_provider1;
    const Container2& m_foreach_imp_provider2;

};

template<typename Container1, typename Container2>
class ZipView : public FunctionalInterface<ZipView<Container1, Container2>, typename std::pair<const typename Container1::value_type, const typename Container2::value_type> > {
public:
    using value_type = typename std::pair<const typename Container1::value_type, const typename Container2::value_type>;
    using const_value_type = const value_type;
    using const_reference_type = const_value_type&;
    static constexpr bool is_permanent = Container1::is_permanent && Container2::is_permanent;
    static constexpr bool is_finite = Container1::is_finite or Container2::is_finite;

    static_assert(lfv_details::has_fast_element_access_tag<Container1>::value or lfv_details::has_fast_element_access_tag<Container2>::value, "At least one container needs to to provide fast element access, consider calling stage() ");

    ZipView(const Container1& c1, const Container2& c2)
        : FunctionalInterface<ZipView<Container1, Container2>, value_type>(*this),
        m_foreach_imp_provider1(c1),
        m_foreach_imp_provider2(c2) {}

    template<typename F>
    void foreach_imp(F f, lfv_details::foreach_instructions how = {}) const {
        size_t index = 0;
        if (lfv_details::has_fast_element_access_tag<Container2>::value) {
            m_foreach_imp_provider1.foreach_imp([f, &index, this](typename Container1::const_reference_type el1) {
                auto el2_option = m_foreach_imp_provider2.element_at(index);
                if (el2_option.has_value() == false) // reached end cof container 2
                    return false;
                const bool go = f(std::pair<typename Container1::const_reference_type, typename Container2::const_reference_type>(el1, el2_option.value()));
                if (not go)
                    return false;
                index++;
                return true;
                }, how);
        }
        else {
            m_foreach_imp_provider2.foreach_imp([f, &index, this](typename Container2::const_reference_type el2) {
                auto el1_option = m_foreach_imp_provider1.element_at(index);
                if (el1_option.has_value() == false)
                    return false;
                const bool go = f(std::pair<typename Container1::const_reference_type, typename Container2::value_type>(el1_option.value(), el2));
                if (not go)
                    return false;
                index++;
                return true;
                }, how);
        }
    }
private:
    const Container1& m_foreach_imp_provider1;
    const Container2& m_foreach_imp_provider2;
};


template<typename Container1, typename Container2>
class CartesianView : public FunctionalInterface<CartesianView<Container1, Container2>, typename std::pair<const typename Container1::value_type&, const typename Container2::value_type&> > {
public:
    using value_type = typename std::pair<const typename Container1::value_type&, const typename Container2::value_type&>;
    using const_value_type = const value_type;
    using const_reference_type = const_value_type&;
    static constexpr bool is_permanent = Container1::is_permanent && Container2::is_permanent;
    static_assert(Container1::is_finite&& Container2::is_finite, "Cartesian product makes sense only for finite containers");
    static constexpr bool is_finite = true;

    CartesianView(const Container1& c1, const Container2& c2)
        : FunctionalInterface<CartesianView<Container1, Container2>, value_type>(*this),
        m_foreach_imp_provider1(c1),
        m_foreach_imp_provider2(c2) {}

    template<typename F>
    void foreach_imp(F f, lfv_details::foreach_instructions how = {}) const {
        bool can_go = true;
        m_foreach_imp_provider1.foreach_imp([f, this, how, &can_go](typename Container1::const_reference_type el1) {
            m_foreach_imp_provider2.foreach_imp([f, el1, &can_go](typename Container2::const_reference_type el2) {
                const bool go = f(std::pair<typename Container1::const_reference_type, typename Container2::const_reference_type>(el1, el2));
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
    const Container1& m_foreach_imp_provider1;
    const Container2& m_foreach_imp_provider2;
};



template<typename Stored, template<typename, typename> typename Container>
class DirectView final : public FunctionalInterface<DirectView<Stored, Container>, Stored> {
public:
    using value_type = Stored;
    using const_value_type = const value_type;
    using const_reference_type = const_value_type&;
    using container = DirectView<Stored>;
    static constexpr bool is_permanent = true;
    static constexpr bool is_finite = true;

    DirectView(const Container<value_type, std::allocator<value_type>>& m)
        : FunctionalInterface<container, value_type>(*this), m_data(m) {}

    template<typename F>
    void foreach_imp(F f, lfv_details::foreach_instructions how = {}) const {
        for (const auto& el : m_data) {
            const bool go = f(el);
            if (not go)
                break;
        }
    }

    virtual auto element_at(size_t  n) const -> std::optional<value_type> override final {
        if (n < m_data.size())
            return m_data.at(n);
        return {};
    }
    virtual size_t size() const  override final {
        return m_data.size();
    }

private:
    const Container<value_type, std::allocator<value_type>>& m_data;
};


template<typename T, template<typename, typename> typename Container>
class OwningView final : public FunctionalInterface<OwningView<T, Container>, T> {
public:
    using value_type = T;
    using const_value_type = const value_type;
    using const_reference_type = const_value_type&;
    using container = OwningView<T, Container>;
    static constexpr bool is_permanent = true;
    static constexpr bool is_finite = true;

    OwningView()
        : FunctionalInterface<container, value_type>(*this) {}

    OwningView(const Container<T, std::allocator<T>>& data)
        : FunctionalInterface<container, value_type>(*this),
        m_data(data) {}

    OwningView(const Container<T, std::allocator<T>>&& data)
        : FunctionalInterface<container, value_type>(*this),
        m_data(data) {}

    template<typename O>
    const OwningView& operator=( const O& rhs) {
        m_data.clear();
        rhs.push_back_to(m_data);
        return *this;
    }

    void insert(const_reference_type d) { m_data.push_back(d); }
    void clear() { m_data.clear(); }

    Container<T, std::allocator<T>>& _underlying() { return m_data; }

    template<typename F>
    void foreach_imp(F f, lfv_details::foreach_instructions how = {}) const {
        for (const auto& el : m_data) {
            const bool go = f(el);
            if (not go)
                break;
        }
    }

    virtual auto element_at(size_t  n) const -> std::optional<value_type> override final {
        if (n < m_data.size())
            return m_data.at(n);
        return {};
    }
    virtual size_t size() const  override final {
        return m_data.size();
    }
private:
    Container<T, std::allocator<T>> m_data;
};


template<typename Container>
class CachedView : public FunctionalInterface<CachedView<Container>, typename Container::value_type> {
public:
    using value_type = typename Container::value_type;
    using const_value_type = const value_type;
    using const_reference_type = const_value_type&;
    using container = FunctionalInterface<Container, value_type>;
    static constexpr bool is_finite = Container::is_finite;
    static constexpr bool is_permanent = true;


    CachedView(const Container& c)
        : FunctionalInterface<CachedView<Container>, typename Container::value_type>(*this),
        m_foreach_imp_provider(c) {}

    template<typename F>
    void foreach_imp(F f, lfv_details::foreach_instructions how = {}) const {
        if (not m_filled) {
            m_foreach_imp_provider.foreach_imp([this](const auto& el) {
                m_cache.insert(el);
                return true;
                });
        }
        m_filled = true;
        m_cache.foreach_imp(f, how);
    }
private:
    const Container& m_foreach_imp_provider;
    mutable bool m_filled = false;
    mutable OwningView<value_type> m_cache;
};


template<typename Container, typename S>
class InspectView : public FunctionalInterface<InspectView<Container, S>, typename Container::value_type> {
public:
    using value_type = typename Container::value_type;
    using const_value_type = const value_type;
    using const_reference_type = const_value_type&;
    using container = FunctionalInterface<Container, value_type>;
    static constexpr bool is_finite = Container::is_finite;
    static constexpr bool is_permanent = Container::is_permanent;


    InspectView(const Container& c, S subroutine)
        : FunctionalInterface<InspectView<Container, S>, typename Container::value_type>(*this),
        m_foreach_imp_provider(c),
        m_subroutine(subroutine) {}

    template<typename F>
    void foreach_imp(F f, lfv_details::foreach_instructions how = {}) const {
        m_foreach_imp_provider.foreach_imp([this, f](const auto& el) -> bool {
            m_subroutine(el);
            return f(el); },
            how);
    }
private:
    const Container& m_foreach_imp_provider;
    S m_subroutine;
};


template<typename T, template<typename, typename> typename Container>
class RefView final : public FunctionalInterface<RefView<T, Container>, T> {
public:
    using value_type = T;
    using const_value_type = const value_type;
    using const_reference_type = const_value_type&;
    using container = RefView<T>;
    static constexpr bool is_permanent = true;
    static constexpr bool is_finite = true;



    RefView()
        : FunctionalInterface<container, value_type>(*this) {}

    void insert(const_reference_type d) { m_data.push_back(d); }
    void clear() { m_data.clear(); }

    template<typename F>
    void foreach_imp(F f, lfv_details::foreach_instructions how = {}) const {
        for (const auto& el : m_data) {
            const bool go = f(el.value());
            if (not go)
                break;
        }
    }

    virtual auto element_at(size_t  n) const -> std::optional<value_type> override final {
        if (n < m_data.size())
            return m_data.at(n);
        return {};
    }
    virtual size_t size() const  override final {
        return m_data.size();
    }
private:
    Container<const std::reference_wrapper<T>, std::allocator<std::reference_wrapper<T>>> m_data;
};


template<typename T>
class Series : public FunctionalInterface<Series<T>, T > {
public:
    using value_type = T;
    using const_value_type = const value_type;
    using const_reference_type = const_value_type&;
    static constexpr bool is_permanent = false;
    static constexpr bool is_finite = false;


    Series(std::function<T(const T&)> gen, const T& start, const T& stop = std::numeric_limits<T>::max())
        : FunctionalInterface<Series<T>, T >(*this),
        m_generator(gen),
        m_start(start),
        m_stop(stop) {}

    template<typename F>
    void foreach_imp(F f, lfv_details::foreach_instructions how = {}) const {
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
    using value_type = T;
    using const_value_type = const value_type;
    using const_reference_type = const_value_type&;
    static constexpr bool is_permanent = false;
    static constexpr bool is_finite = true;
    static_assert(std::is_arithmetic<value_type>::value, "Can't generate range of not arithmetic type");

    Range(const T& begin, const T& end, const T& stride = 1)
        : FunctionalInterface<Range<T>, T >(*this),
        m_begin(begin),
        m_end(end),
        m_stride(stride) {
        if (m_stride == 0) throw std::runtime_error("the stride can be zero");
        if (m_stride > 0 and m_begin > m_end) throw std::runtime_error("limits and stride will result in an infinite range");
        if (m_stride < 0 and m_begin < m_end) throw std::runtime_error("limits and stride will result in an infinite range");

    }

    template<typename F>
    void foreach_imp(F f, lfv_details::foreach_instructions how = {}) const {
        T current = m_begin;
        while ((m_stride > 0 and current < m_end) or (current > m_end)) {
            const bool go = f(current);
            if (not go)
                break;
            current = current + m_stride;
            // std::cout << "X" << current << std::endl;
        }
    }

    virtual auto element_at(size_t  n) const -> std::optional<value_type> override final {
        if (n < size())
            return m_begin + m_stride * n;
        return {};
    }
    virtual size_t size() const  override final {
        return std::abs((m_end - m_begin)) / std::abs(m_stride);
    }


private:
    T m_begin;
    T m_end;
    T m_stride;
};

// infinite series of doubled where each next is the previous multiplied by ratio
template<typename T, typename U>
Series<T> geometric_stream(T coeff, U ratio) {
    return Series<T>([ratio](T c) { return c * ratio; }, coeff);
}

// infinite series if starting from initial value incremented by increment
template<typename T>
Series<T> arithmetic_stream(T initial, T increment) {
    return Series<T>([increment](double c) { return c + increment; }, initial);
}

// infinite series if integers incremented by 1
template<typename T=size_t>
Series<T> iota_stream(T initial = 0) {
    return Series<T>([](T c) { return c + 1; }, initial);;
}

// random integers using rand from c stdlib
// @warning - not high quality randomization
// @warning - it is not reproducible
template<typename T=int>
Series<T> crandom_stream() {
    return Series<T>([](T) { return static_cast<T>(rand()); }, rand());
}

// finite range from x = begin until x < end
template<typename T>
Range<T> range_stream(T begin, T end, T stride = 1) {
    return Range<T>(begin, end, stride);
}

template<typename T>
DirectView<T, std::vector> lazy_view(const std::vector<T>& vec) {
    return DirectView(vec);
}

template<typename T>
OwningView<T, std::vector> lazy_own(const std::vector<T>&& vec) {
    return OwningView(vec);
}

template<typename T>
OwningView<T, std::vector> lazy_own(const std::vector<T>& vec) {
    return OwningView(vec);
}


#endif