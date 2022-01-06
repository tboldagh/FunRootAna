// Copyright 2022, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)
#ifndef LazyFunctionalVector_h
#define LazyFunctionalVector_h
#include <type_traits>
#include <set>

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
The foreach takes additional instructions that can be used to optimize it's operation or affect the order of operations.


*/


template< typename Container, typename Filter> class FilteredView;
template< typename Container, typename Map> class MappedView;
template<typename T> class DirectView;
template<typename T> class OwningView;
template<typename Container> class TakeNView;
template<typename Container> class EnumeratedView;
template<typename Container> class ReverseView;
template<typename Container, typename Filter> class TakeWhileView;
template<typename Container1, typename Container2> class ChainView;
template<typename Container1, typename Container2> class ZipView;
template<typename Container1, typename Comparator> class SortedView;
template<typename Container1, typename Comparator> class MMView;

namespace {
    template<typename C> struct has_fast_element_access_tag { static constexpr bool value = false; };
    template<typename T> struct has_fast_element_access_tag<DirectView<T>> { static constexpr bool value = true; };
    template<typename T> struct has_fast_element_access_tag<OwningView<T>> { static constexpr bool value = true; };

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



namespace lfv {
    constexpr size_t all_elements = std::numeric_limits<size_t>::max();
    constexpr size_t invalid_index = std::numeric_limits<size_t>::max();

    // TODO avoid copying into this structure
    template<typename T>
    class indexed {
    public:
        indexed() = default;
        indexed(size_t i, const T& d) : m_index(i), m_data(d) {}
        size_t index() const { return m_index; }
        const T& data() const {
            if (m_index == invalid_index) throw std::runtime_error("Invalid index");
            return m_data;
        }
        const T* ptr() const { return &m_data; }
    private:
        size_t m_index = invalid_index;
        T m_data;
    };

    template<typename T>
    std::ostream& operator<<(std::ostream& out, const indexed<T>& i) {
        return out << i.index() << ":" << i.data() << "(" << i.ptr() << ")";
    }

};

namespace {
    enum skip_take_logic_t { skip_elements, take_elements };
    enum min_max_logic_t { min_elements, max_elements };
    enum preserve_t { temporary, store }; // instruct if the results of for-each will be refereed to afterwards
    enum order_t { forward, reverse, any }; // direction of the foreach
    struct foreach_instructions {
        order_t order = forward;
        preserve_t preserve = temporary;
    };
};


template<typename Container, typename Stored>
class FunctionalInterface {
public:
    using value_type = Stored;
    using const_reference_type = const value_type&;
    static std::function<const_reference_type(const_reference_type) > identity;

    FunctionalInterface(const Container& cont) : m_actual_container(cont) {}

    // invoke function on each element of the container
    template<typename F>
    void foreach(F f) const {
        m_actual_container.foreach_imp([f](const_reference_type el) { f(el); return true; });
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

    // sort according to the key extractor @arg f, this is lazy operation
    template<typename F = decltype(identity)>
    auto sorted(F f = identity) const {
        return SortedView<Container, F>(m_actual_container, f);
    }

    // selectors, lazy
    auto take(size_t n, size_t stride = 1) const {
        return TakeNView<Container>(m_actual_container, n, stride, take_elements);
    }

    auto skip(size_t n, size_t stride = 1) const {
        return TakeNView<Container>(m_actual_container, n, stride, skip_elements);
    }

    // produces container with indexed data
    auto enumerate(size_t offset = 0) const {
        return EnumeratedView<Container>(m_actual_container, offset);
    }

    template<typename F>
    auto take_while(F f) const {
        return TakeWhileView<Container, F>(m_actual_container, f, take_elements);
    }

    template<typename F>
    auto skip_while(F f) const {
        return TakeWhileView<Container, F>(m_actual_container, f, skip_elements);
    }

    // creates intermediate container to speed following up operations, typically useful after sorting
    auto stage() const -> OwningView<Stored> {
        OwningView<Stored> c;
        m_actual_container.foreach_imp([&c](const_reference_type el) { c.insert(el); return true; });
        return c;
    }
    auto reverse() const {
        return ReverseView<Container>(m_actual_container);
    }


    // max and min
    template<typename KeyExtractor = decltype(identity)>
    auto max(KeyExtractor by = identity) const {
        return MMView<Container, KeyExtractor>(m_actual_container, by, max_elements);
    }
    template<typename KeyExtractor = decltype(identity)>
    auto min(KeyExtractor by = identity) const {
        return MMView<Container, KeyExtractor>(m_actual_container, by, min_elements);
    }


    // concatenate two containers, in a lazy way
    template<typename Other>
    auto chain(const Other& c) const {
        return ChainView<Container, Other>(m_actual_container, c);
    }

    // combine pairwise
    // the implementation is suboptimal, that is, it involves inexed access to one of the containers, it is therefore better if one of them is staged
    template<typename Other>
    auto zip(const Other& c) const {
        static_assert(true, "zip implementation is missing");
        return ZipView<Container, Other>(m_actual_container, c);
    }

    // compare pairwise, if the containers have diffrent len,  the comparison is performed only until smaller of the sizes
    // the comparator obtains a pair from first element is from 1st container, and second from container c
    template<typename Other, typename Comp>
    auto is_same(const Other& c, Comp comparator ) const {
        bool same = true;
        zip(c).foreach([&same, &comparator](const auto& el) {
            if ( comparator(el) ) {
                same = false;
                return false;
            }
            return true;
            });
        return same;
    }

    // compare for equality
    template<typename Other>
    auto is_same(const Other& c ) const {
        return is_same(c, []( const auto& el ){ return el.first != el.second; } );
    }



    // sum
    template<typename F = decltype(identity)>
    Stored sum(F f = identity) const {
        Stored s = {};
        m_actual_container.foreach_imp([&s, f](const_reference_type el) { s = s + f(el); return true; });
        return s;
    }

    // accumulate, function us used as: total = f(total, element), also take starting element
    template<typename F, typename R>
    auto accumulate(F f, R initial = {}) const {
        static_assert(std::is_same<R, typename std::invoke_result<F, R, typename Container::const_reference_type>::type>::value, "function return type different than initial value");
        R s = initial;
        m_actual_container.foreach_imp([&s, f](const_reference_type el) { s = f(s, el);  return true; });
        return s;
    }

    // access by index
    // TODO, move to a reference, return std::optional
    virtual auto element_at(size_t  n) const -> std::optional<value_type> {
        // std::optional<value_type> temp = {};
//        std::vector<value_type> temp;
        one_element_stack_container<value_type> temp;
        size_t i = 0;
        m_actual_container.foreach_imp([&temp, n, &i](const_reference_type el) {
            if (i == n) {
                temp.insert(el);
                return false;
            }
            i++;
            return true;
            });
        if (temp.empty())
            return {};
        else
            return temp.get();
    }

    // first element satisfying predicate
    template<typename Predicate>
    auto first_of(Predicate f) const -> std::optional<value_type> {
        std::optional<value_type> temp = {};
        m_actual_container.foreach_imp([&temp, f](const_reference_type el) {
            if (f(el)) {
                temp = el;
                return false;
            }
            return true;
            });
        return temp;
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


    // back to regular vector
    template<typename R>
    void push_back_to(R& result) const {
        m_actual_container.foreach_imp([&result](const_reference_type el) {
            result.push_back(el); return true;
            });
    }

    template<typename R>
    void insert_to(R& result) const {
        m_actual_container.foreach_imp([&result](const_reference_type el) {
            result.insert(el); return true;
            });
    }
    // to non-lazy implementation
    EagerFunctionalVector<value_type> as_eager() const {
        EagerFunctionalVector<Stored> tmp;
        m_actual_container.foreach([&tmp](const_reference_type el) { tmp.__push_back(el); return true; });
        return tmp;
    }



protected:
    const Container& m_actual_container;
};
template<typename Container, typename Stored>
std::function<const Stored& (const Stored& x) > FunctionalInterface<Container, Stored>::identity = [](const Stored& x) { return x; };


// provides view of subset of element which satisfy the predicate
template< typename Container, typename Filter>
class FilteredView : public FunctionalInterface<FilteredView<Container, Filter>, typename Container::value_type> {
public:
    using value_type = typename Container::value_type;
    using reference_type = value_type&;
    using const_reference_type = const value_type&;

    FilteredView(const Container& c, Filter f)
        : FunctionalInterface<FilteredView<Container, Filter>, value_type >(*this),
        m_foreach_imp_provider(c),
        m_filterOp(f) {};

    template<typename F>
    void foreach_imp(F f, foreach_instructions how = {}) const {
        m_foreach_imp_provider.foreach_imp([f, this](const_reference_type el) {
            if (this->m_filterOp(el)) {
                const bool go = f(el);
                if (not go)
                    return false;
            }
            return true;
            }, how);
    };
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

    SortedView(const Container& c, KeyExtractor f)
        : FunctionalInterface<SortedView<Container, KeyExtractor>, value_type >(*this),
        m_foreach_imp_provider(c),
        m_keyExtractor(f) {};

    template<typename F>
    void foreach_imp(F f, foreach_instructions how = {}) const {
        std::vector< std::reference_wrapper<const value_type>> lookup;
        m_foreach_imp_provider.foreach_imp([&lookup](const_reference_type el) { lookup.push_back(std::cref(el)); return true; }, how);
        auto sorter = [this](auto a, auto b) { return m_keyExtractor(a) < m_keyExtractor(b); };
        std::sort(std::begin(lookup), std::end(lookup), sorter);
        for (auto ref : lookup) {
            const bool go = f(ref.get());
            if (not go)
                break;
        }
    };
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

    MMView(const Container& c, KeyExtractor f, min_max_logic_t logic)
        : FunctionalInterface<MMView<Container, KeyExtractor>, value_type >(*this),
        m_actual_container(c),
        m_keyExtractor(f),
        m_logic(logic) {};

    template<typename F>
    void foreach_imp(F f, foreach_instructions how = {}) const {
        if (m_actual_container.empty()) return;
        one_element_stack_container< compared_type > extremeValue;
        one_element_stack_container< std::reference_wrapper<const value_type> > extremeElement;

        m_actual_container.foreach_imp([&extremeValue, &extremeElement, this](const_reference_type el) {
            compared_type val = m_keyExtractor(el);
            if (extremeValue.empty()) {
                extremeValue.insert(val);
                extremeElement.insert(std::cref(el));
            }
            if ((m_logic == max_elements and val >= extremeValue.get())
                or
                (m_logic == min_elements and val < extremeValue.get())) {
                extremeValue.replace(val);
                extremeElement.replace(std::cref(el));
            }
            return true;
            }, how);
        if (not extremeElement.empty())
            f(extremeElement.get().get());
    };
    MMView() = delete;
private:
    const Container& m_actual_container;
    KeyExtractor m_keyExtractor;
    min_max_logic_t m_logic;
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

    MappedView(const Container& c, Mapping m)
        : FunctionalInterface<MappedView<Container, Mapping>, value_type >(*this),
        m_foreach_imp_provider(c),
        m_mappingOp(m) {};

    template<typename F>
    void foreach_imp(F f, foreach_instructions how = {}) const {
        m_foreach_imp_provider.foreach_imp([f, this](const auto& el) {
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

template<typename Container>
class TakeNView : public FunctionalInterface<TakeNView<Container>, typename Container::value_type> {
public:
    using value_type = typename Container::value_type;
    using const_value_type = const value_type;
    using const_reference_type = const_value_type&;
    using container = FunctionalInterface<Container, value_type>;

    TakeNView(const Container& c, size_t n, size_t stride = 1, skip_take_logic_t l = take_elements)
        : FunctionalInterface<TakeNView<Container>, typename Container::value_type>(*this),
        m_foreach_imp_provider(c),
        m_elementsToTake(n),
        m_stride(stride),
        m_logic(l) {}

    template<typename F>
    void foreach_imp(F f, foreach_instructions how = {}) const {
        size_t n = 0;
        m_foreach_imp_provider.foreach_imp([f, &n, this](const_reference_type el) {
            const bool needed = n % m_stride == 0
                and ((m_logic == take_elements and n < m_elementsToTake)
                    or (m_logic == skip_elements and n >= m_elementsToTake));
            if (needed) {
                const bool go = f(el);
                if (not go)
                    return false;
            }
            n++;
            return true;
            }, how);
    }
private:
    const Container& m_foreach_imp_provider;
    size_t m_elementsToTake;
    size_t m_stride;
    skip_take_logic_t m_logic;
};


template<typename Container>
class ReverseView : public FunctionalInterface<ReverseView<Container>, typename Container::value_type> {
public:
    using value_type = typename Container::value_type;
    using const_value_type = const value_type;
    using const_reference_type = const_value_type&;
    using container = FunctionalInterface<Container, value_type>;

    ReverseView(const Container& c)
        : FunctionalInterface<ReverseView<Container>, typename Container::value_type>(*this),
        m_foreach_imp_provider(c) {}

    template<typename F>
    void foreach_imp(F f, foreach_instructions how = {}) const {
        m_foreach_imp_provider.foreach_imp([f](const auto& el) {
            const bool go = f(el);
            if (not go)
                return false;
            return true;
            },
            { (how.order == forward ? reverse : forward), how.preserve }); // here we rotate the order
    }
private:
    const Container& m_foreach_imp_provider;
};


// TODO, this needs work!
template<typename Container>
class EnumeratedView : public FunctionalInterface<EnumeratedView<Container>, lfv::indexed<typename Container::value_type>> {
public:
    using value_type = typename lfv::indexed<typename Container::value_type>;
    using const_value_type = const value_type;
    using const_reference_type = const_value_type&;
    using container = FunctionalInterface<Container, value_type>;

    EnumeratedView(const Container& c, size_t offset)
        : FunctionalInterface<EnumeratedView<Container>, value_type>(*this),
        m_foreach_imp_provider(c),
        m_offset(offset) {}


    template<typename F>
    void foreach_imp(F f, foreach_instructions how = {}) const {
        size_t index = m_offset;
        m_foreach_imp_provider.foreach_imp([f, &index](const auto& el) {
            const bool go = f(lfv::indexed(index, el));
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


template< typename Container, typename Filter>
class TakeWhileView : public FunctionalInterface<TakeWhileView<Container, Filter>, typename Container::value_type> {
public:
    using value_type = typename Container::value_type;
    using reference_type = value_type&;
    using const_reference_type = const value_type&;

    TakeWhileView(const Container& c, Filter f, skip_take_logic_t l)
        : FunctionalInterface<TakeWhileView<Container, Filter>, value_type >(*this),
        m_foreach_imp_provider(c),
        m_filterOp(f),
        m_logic(l) {};

    template<typename F>
    void foreach_imp(F f, foreach_instructions how = {}) const {
        bool take = m_logic == take_elements;
        bool need_deciding = true; // to save on  calling to f once decided
        m_foreach_imp_provider.foreach_imp([f, &take, &need_deciding, this](const_reference_type el) {
            if (need_deciding) {
                if (m_logic == take_elements) {
                    if (not m_filterOp(el)) {
                        take = false;
                        need_deciding = false;
                    }
                }
                if (m_logic == skip_elements) {
                    if (not m_filterOp(el)) {
                        take = true;
                        need_deciding = false;
                    }
                }
            }
            if (take) {
                const bool go = f(el);
                if (not go)
                    return false;
            }
            return true;
            }, how);
    };
    TakeWhileView() = delete;
private:
    const Container& m_foreach_imp_provider;
    Filter m_filterOp;
    skip_take_logic_t m_logic;
};


template<typename Container1, typename Container2>
class ChainView : public FunctionalInterface<ChainView<Container1, Container2>, typename Container1::value_type> {
public:
    using value_type = typename Container1::value_type;
    static_assert(std::is_same<typename Container1::value_type, typename Container2::value_type>::value, "chained containers have to provide same stored type");
    using const_value_type = const value_type;
    using const_reference_type = const_value_type&;

    ChainView(const Container1& c1, const Container2& c2)
        : FunctionalInterface<ChainView<Container1, Container2>, value_type>(*this),
        m_foreach_imp_provider1(c1),
        m_foreach_imp_provider2(c2) {}

    template<typename F>
    void foreach_imp(F f, foreach_instructions how = {}) const {
        bool need_to_access_second_container = true;
        if (how.order == reverse) {
            m_foreach_imp_provider2.foreach_imp([f, &need_to_access_second_container](auto el) {
                const bool go = f(el);
                if (not go) {
                    need_to_access_second_container = false;
                    return false;
                }
                return true;
                }, how);
            if (not need_to_access_second_container) return;
            m_foreach_imp_provider1.foreach_imp([f](const_reference_type el) {
                const bool go = f(el);
                if (not go) {
                    return false;
                }
                return true;
                }, how);
        }
        else {
            m_foreach_imp_provider1.foreach_imp([f, &need_to_access_second_container](const_reference_type el) {
                const bool go = f(el);
                if (not go) {
                    need_to_access_second_container = false;
                    return false;
                }
                return true;
                }, how);
            if (not need_to_access_second_container) return;
            m_foreach_imp_provider2.foreach_imp([f](auto el) {
                const bool go = f(el);
                if (not go) {
                    return false;
                }
                return true;
                }, how);
        }
    }
private:
    const Container1& m_foreach_imp_provider1;
    const Container2& m_foreach_imp_provider2;

};

template<typename Container1, typename Container2>
class ZipView : public FunctionalInterface<ZipView<Container1, Container2>, typename std::pair<const typename Container1::value_type&, const typename Container2::value_type&> > {
public:
    using value_type = typename std::pair<const typename Container1::value_type&, const typename Container2::value_type&>;
    using const_value_type = const value_type;
    using const_reference_type = const_value_type&;
    static_assert(has_fast_element_access_tag<Container1>::value or has_fast_element_access_tag<Container2>::value, "At least one container needs to to provide fast element access, consider calling stage() ");
    ZipView(const Container1& c1, const Container2& c2)
        : FunctionalInterface<ZipView<Container1, Container2>, value_type>(*this),
        m_foreach_imp_provider1(c1),
        m_foreach_imp_provider2(c2) {}

    template<typename F>
    void foreach_imp(F f, foreach_instructions how = {}) const {
        size_t index = 0;
        if (has_fast_element_access_tag<Container2>::value) {
            m_foreach_imp_provider1.foreach_imp([f, &index, this](typename Container1::const_reference_type el1) {
                auto el2 = m_foreach_imp_provider2.element_at(index);
                if (el2.has_value() == false) // reached end cof container 2
                    return false;
                const bool go = f(std::pair<typename Container1::const_reference_type, typename Container2::const_reference_type>(el1, el2.value()));
                if (not go)
                    return false;
                index++;
                return true;
                }, how);
        }
        else {
            m_foreach_imp_provider2.foreach_imp([f, &index, this](typename Container2::const_reference_type el2) {
                auto el1 = m_foreach_imp_provider1.element_at(index);
                if (el1.has_value() == false) // reached end cof container 2
                    return false;
                const bool go = f(std::pair<typename Container1::const_reference_type, typename Container2::const_reference_type>(el1.value(), el2));
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


template<typename Stored>
class DirectView : public FunctionalInterface<DirectView<Stored>, Stored> {
public:
    using value_type = Stored;
    using const_value_type = const value_type;
    using const_reference_type = const_value_type&;
    using container = DirectView<Stored>;

    DirectView(const std::vector<value_type>& m)
        : FunctionalInterface<container, value_type>(*this), m_data(m) {}

    template<typename F>
    void foreach_imp(F f, foreach_instructions how = {}) const {
        if (how.order == reverse) {
            for (auto riter = std::rbegin(m_data), riterend = std::rend(m_data); riter != riterend; ++riter) {
                const bool go = f(*riter);
                if (not go)
                    break;
            }
        }
        else {
            for (const auto& el : m_data) {
                const bool go = f(el);
                if (not go)
                    break;
            }
        }
    };

    virtual auto element_at(size_t  n) const -> std::optional<value_type> override final {
        if (n < m_data.size())
            return m_data.at(n);
        return {};
    }
    virtual size_t size() const  override final {
        return m_data.size();
    }

private:
    const std::vector<value_type>& m_data;
};


template<typename T>
class OwningView : public FunctionalInterface<OwningView<T>, T> {
public:
    using value_type = T;
    using const_value_type = const value_type;
    using const_reference_type = const_value_type&;
    using container = OwningView<T>;

    OwningView()
        : FunctionalInterface<container, value_type>(*this) {}

    OwningView(const std::vector<T>& data)
        : FunctionalInterface<container, value_type>(*this),
        m_data(data) {}

    void insert(const_reference_type d) { m_data.push_back(d); }
    template<typename F>
    void foreach_imp(F f, foreach_instructions how = {}) const {
        if (how.order == reverse) {
            for (auto riter = std::rbegin(m_data), riterend = std::rend(m_data); riter != riterend; ++riter) {
                const bool go = f(*riter);
                if (not go)
                    break;
            }
        }
        else {
            for (const auto& el : m_data) {
                const bool go = f(el);
                if (not go)
                    break;
            }
        }
    };

    virtual auto element_at(size_t  n) const -> std::optional<value_type> override final {
        if (n < m_data.size())
            return m_data.at(n);
        return {};
    }
    virtual size_t size() const  override final {
        return m_data.size();
    }
private:
    std::vector<T> m_data;
};

template<typename T>
class RefView : public FunctionalInterface<RefView<T>, T> {
public:
    using value_type = T;
    using const_value_type = const value_type;
    using const_reference_type = const_value_type&;
    using container = RefView<T>;

    RefView()
        : FunctionalInterface<container, value_type>(*this) {}
    void insert(const_reference_type d) { m_data.push_back(d); }
    template<typename F>
    void foreach_imp(F f, foreach_instructions how = {}) const {
        if (how.order == reverse) {
            for (auto riter = std::rbegin(m_data), riterend = std::rend(m_data); riter != riterend; ++riter) {
                const bool go = f(*riter);
                if (not go)
                    break;
            }
        }
        else {
            for (const auto& el : m_data) {
                const bool go = f(el.value());
                if (not go)
                    break;
            }
        }
    };

    virtual auto element_at(size_t  n) const -> std::optional<value_type> override final {
        if (n < m_data.size())
            return m_data.at(n);
        return {};
    }
    virtual size_t size() const  override final {
        return m_data.size();
    }
private:
    std::vector<const std::reference_wrapper<T>> m_data;
};

template<typename T>
auto lazy(const std::vector<T>& vec) {
    return DirectView(vec);
}

#endif