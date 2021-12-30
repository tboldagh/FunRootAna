//LazyFunctionalVector
// lazy version of WrappedVector
// TODO
// provide more optimal foreach_imp with option of early return


#include <type_traits>

template< typename Container, typename Filter> class FilteredView;
template< typename Container, typename Map> class MappedView;
template<typename T> class DirectView;
template<typename T> class OwningView;
template<typename Container> class TakeNView;
template<typename Container> class EnumeratedView;
template<typename Container, typename Filter> class TakeWhileView;
template<typename Container1, typename Container2> class ChainView;
template<typename Container1, typename Container2> class ZipView;
template<typename Container1, typename Comparator> class SortedView;

template<typename Container1, typename Comparator> class MarginalView;


namespace lfv {
    constexpr size_t all_elements = std::numeric_limits<size_t>::max();
    constexpr size_t invalid_index = std::numeric_limits<size_t>::max();

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
};


template<typename Container, typename Stored>
class FunctionalInterface {
public:
    FunctionalInterface(const Container& cont) : m_actual_container(cont) {}

    // subclasses need to implement this method
    template<typename F>
    void foreach(F f) const {
        m_actual_container.foreach_imp([f](auto el) { f(el); return true; });
    }

    // count of elements, this eager operation
    virtual size_t size() const {
        size_t c = 0;
        m_actual_container.foreach_imp([&c](typename Container::const_reference_type) { c++; return true;});
        return c;
    }

    virtual bool empty() const {
        size_t c = 0;
        m_actual_container.foreach_imp([&c](typename Container::const_reference_type) { c++; return false;});
        return c == 0;
    }


    // count element satisfying predicate, this eager operation
    template<typename Predicate>
    size_t count(Predicate&& pred) const {
        size_t c = 0;
        m_actual_container.foreach_imp([&c, pred](typename Container::const_reference_type el) {
            c += (pred(el) ? 1 : 0); return true; });
        return c;
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
    template<typename F >
    auto sorted(F f) const {
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
        m_actual_container.foreach_imp([&c](auto el) { c.insert(el); return true; });
        return c;
    }


    // max and min
    template<typename KeyExtractor>
    auto max(KeyExtractor by, size_t howMany) const {
        return MarginalView<Container, KeyExtractor>(m_actual_container, by, max_elements);
    }
    template<typename KeyExtractor>
    auto min(KeyExtractor by, size_t howMany) const {
        return MarginalView<Container, KeyExtractor>(m_actual_container, by, min_elements);
    }


    // concatenate two containers, in a lazy way
    template<typename Other>
    auto chain(const Other& c) const {
        return ChainView<Container, Other>(m_actual_container, c);
    }

    // combine pairwise
    // TODO provide implementation
    template<typename Other>
    auto zip(const Other& c) const {
        static_assert(true, "zip implementation is missing");
        return ZipView<Container, Other>(m_actual_container, c);
    }

    // compare pairwise
    template<typename Other>
    auto compare(const Other& c) const {
        static_assert(true, "compare implementation is missing");
    }


    // sum
    Stored sum() const {
        Stored s = {};
        m_actual_container.foreach_imp([&s](auto el) { s = s + el; return true; });
        return s;
    }
    // accumulate, function us used as: total = f(total, element), also take starting element
    template<typename F>
    auto accumulate(F f, Stored unit = {}) const {
        Stored s = unit;
        m_actual_container.foreach_imp([&s, f](auto el) { s = f(s, el);  return true; });
        return s;
    }
    // stat


    // ROOT histograms fill

    // access
    virtual auto element_at(size_t  n) const -> Stored {
        Stored temp;
        size_t i = 0;
        m_actual_container.foreach_imp([&temp, n, &i](auto el) {
            if (i == n) {
                temp = el;
//                std::cout << " element at " << n << "/" << i << " -> " << el << std::endl;
                return false;
            }
            i++;
            return true;
            });
//        std::cout << " element at out " << n << " -> " << temp << std::endl;
        return temp;
    }

    // back to regular vector
    template<typename R>
    void push_back_to(R& result) const {
        m_actual_container.foreach_imp([&result](auto el) {
            result.push_back(el); return true;
            });
    }

    template<typename R>
    void insert_to(R& result) const {
        m_actual_container.foreach_imp([&result](auto el) {
            result.insert(el); return true;
            });
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

    FilteredView(const Container& c, Filter f)
        : FunctionalInterface<FilteredView<Container, Filter>, value_type >(*this),
        m_foreach_imp_provider(c),
        m_filterOp(f) {};

    template<typename F>
    void foreach_imp(F&& f) const {
        m_foreach_imp_provider.foreach_imp([f, this](auto el) {
            if (this->m_filterOp(el)) {
                const bool go = f(el);
                if (not go)
                    return false;
            }
            return true;
            });
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
    void foreach_imp(F&& f) const {
        std::vector< std::reference_wrapper<const value_type>> lookup;
        m_foreach_imp_provider.foreach_imp([&lookup](const_reference_type el) { lookup.push_back(std::cref(el)); return true; });
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
    void foreach_imp(F&& f) const {
        m_foreach_imp_provider.foreach_imp([f, this](auto el) {
            const bool go = f(m_mappingOp(el));
            if (not go)
                return false;
            return true;
            });
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
    void foreach_imp(F&& f) const {
        size_t n = 0;
        m_foreach_imp_provider.foreach_imp([f, &n, this](auto el) {
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
            });
    }
private:
    const Container& m_foreach_imp_provider;
    size_t m_elementsToTake;
    size_t m_stride;
    skip_take_logic_t m_logic;
};


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

    // virtual auto element_at(size_t  n) const -> value_type override final {
    //     static_assert(false, "elements access not possible for enumerated container");
    // }

    template<typename F>
    void foreach_imp(F&& f) const {
        size_t index = m_offset;
        m_foreach_imp_provider.foreach_imp([f, &index](auto el) {
            const bool go = f(lfv::indexed(index, el));
            //            std::cout << "EnumeratedView " << go << "\n";
            if (not go)
                return false;
            ++index;
            return true;
            });
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
    void foreach_imp(F&& f) const {
        bool take = m_logic == take_elements;
        bool need_deciding = true; // to save on  calling to f once decided
        m_foreach_imp_provider.foreach_imp([f, &take, &need_deciding, this](auto el) {
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
            });
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
    void foreach_imp(F&& f) const {
        bool need_to_access_second_container = true;
        m_foreach_imp_provider1.foreach_imp([f, &need_to_access_second_container](auto el) {
            const bool go = f(el);
            if (not go) {
                need_to_access_second_container = false;
                return false;
            }
            return true;
            });
        if (not need_to_access_second_container) return;
        m_foreach_imp_provider2.foreach_imp([f](auto el) {
            const bool go = f(el);
            if (not go) {
                return false;
            }
            return true;
            });
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
    void foreach_imp(F&& f) const {
        for (const auto& el : m_data) {
            const bool go = f(el);
            //            std::cout << "DirectView::foreach_imp " << el << " " << go << "\n";
            if (not go)
                break;
        }
    };

    virtual auto element_at(size_t  n) const -> value_type override final {
        return m_data.at(n);
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
    void insert(const_reference_type d) { m_data.push_back(d); }
    template<typename F>
    void foreach_imp(F&& f) const {
        for (const auto& el : m_data) {
            const bool go = f(el);
            if (not go)
                break;
        }
    };

    virtual auto element_at(size_t  n) const -> value_type override final {
        return m_data.at(n);
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
    void foreach_imp(F&& f) const {
        for (const auto& el : m_data) {
            const bool go = f(el);
            if (not go)
                break;
        }
    };

    virtual auto element_at(size_t  n) const -> value_type override final {
        return m_data.at(n);
    }
    virtual size_t size() const  override final {
        return m_data.size();
    }
private:
    std::vector<const std::reference_wrapper<T>> m_data;
};


// macro generating generic, single agument, returning lambda
// example use: filter( F(_ < 0)) - the _ is the lambda argument
#define F(CODE) [&](const auto &_) { return CODE; }
// subroutine (nothing is returned)
#define S(CODE) [&](const auto &_) { CODE; }

template<typename T>
auto wrap(const std::vector<T>& vec) {
    return DirectView(vec);
}

