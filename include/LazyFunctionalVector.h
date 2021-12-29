// lazy version of WrappedVector
//LazyFunctionalVector
#include <type_traits>

template< typename Container, typename Filter> class FilteredView;
template< typename Container, typename Map> class MappedView;
template<typename T> class DirectView;
template<typename T> class OwningView;
template<typename Container> class TakeNView;
template<typename Container, typename Filter> class TakeWhileView;
template<typename Container1, typename Container2> class ChainView;
template<typename Container1, typename Container2> class ZipView;
template<typename Container1, typename Comparator> class SortedView;
template<typename Container1, typename Comparator> class MarginalView;

namespace flv {
    constexpr size_t all_elements = std::numeric_limits<size_t>::max();
};

namespace {
    enum skip_take_logic_t { skip_elements, take_elements };
    enum min_max_logic_t { min_elements, max_elements };
};


template<typename Container, typename Stored>
class FunctionalInterface {
public:
    FunctionalInterface(const Container& cont) : m_foreach_provider(cont) {}

    // subclasses need to implement this method
    // template<typename F>
    // void foreach( F f ) const;

    // count of elements, this eager operation
    virtual size_t size() const {
        size_t c = 0;
        m_foreach_provider.foreach([&c](typename Container::const_reference_type) { c++; });
        return c;
    }
    // count element satisfying predicate, this eager operation
    template<typename Predicate>
    size_t count(Predicate&& pred) const {
        size_t c = 0;
        m_foreach_provider.foreach([&c, pred](typename Container::const_reference_type datum) {
            c += (pred(datum) ? 1 : 0); });
        return c;
    }

    // transform using function provided, this is lazy operation
    template<typename F>
    auto map(F f) const {
        return MappedView<Container, F>(m_foreach_provider, f);
    }

    // filter provided predicate, this is lazy operation
    template<typename F>
    auto filter(F f) const {
        return FilteredView<Container, F>(m_foreach_provider, f);
    }

    // sort according to the key extractor @arg f, this is lazy operation
    template<typename F >
    auto sorted(F f) const {
        return SortedView<Container, F>(m_foreach_provider, f);
    }


    // selectors, lazy
    auto take(size_t n, size_t stride = 1) const {
        return TakeNView<Container>(m_foreach_provider, n, stride, take_elements);
    }

    auto skip(size_t n, size_t stride = 1) const {
        return TakeNView<Container>(m_foreach_provider, n, stride, skip_elements);
    }


    template<typename F>
    auto take_while(F f) const {
        return TakeWhileView<Container, F>(m_foreach_provider, f, take_elements);
    }

    template<typename F>
    auto skip_while(F f) const {
        return TakeWhileView<Container, F>(m_foreach_provider, f, skip_elements);
    }

    // creates intermediate container to speed following up operations, typically useful after sorting
    auto stage() const -> OwningView<Stored> {
        OwningView<Stored> c;
        m_foreach_provider.foreach([&c](auto el) { c.insert(el); });
        return c;
    }


    // max and min
    template<typename KeyExtractor>
    auto max( KeyExtractor by, size_t howMany) const { 
        return MarginalView<Container, KeyExtractor>( m_foreach_provider, by, max_elements);
    }
    template<typename KeyExtractor>
    auto min( KeyExtractor by, size_t howMany) const { 
        return MarginalView<Container, KeyExtractor>( m_foreach_provider, by, min_elements);
    }


    // concatenate two containers, in a lazy way
    template<typename Other>
    auto chain( const Other& c) const { 
        return ChainView<Container, Other>(m_foreach_provider, c);
    }

    // combine pairwise
    // TODO provide implementation
    template<typename Other>
    auto zip( const Other& c) const { 
        static_assert(true, "zip implementation is missing");
        return ZipView<Container, Other>(m_foreach_provider, c);
    }

    // compare pairwise
    template<typename Other>
    auto compare( const Other& c) const { 

    }


    // sum
    Stored sum() const {
        Stored s = {};
        m_foreach_provider.foreach([&s](auto el){ s = s + el; });
        return s;
    }
    // accumulate, function us used as: total = f(total, element), also take starting element
    template<typename F>
    auto accumulate(F f, Stored unit = {} ) const { 
        Stored s = unit;
        m_foreach_provider.foreach([&s, f](auto el){ s = f(s, el);  });
        return s;
    }
    // stat
    

    // ROOT histograms fill

    // access
    virtual auto element_at(size_t  n) const -> Stored {
        Stored temp;
        size_t i = 0;
        m_foreach_provider.foreach([&temp, n, &i](auto el) {
            if (i == n) temp = el; i++;
            });
        return temp;
    }

    // back to regular vector
    template<typename R>
    void push_back_to(R& result) const {
        m_foreach_provider.foreach([&result](auto el) {
            result.push_back(el);
            });
    }

    template<typename R>
    void insert_to(R& result) const {
        m_foreach_provider.foreach([&result](auto el) {
            result.insert(el);
            });
    }

protected:
    const Container& m_foreach_provider;

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
        m_foreach_provider(c),
        m_filterOp(f) {};

    template<typename F>
    void foreach(F&& f) const {
        m_foreach_provider.foreach([f, this](auto el) {
            if (this->m_filterOp(el))
                f(el);
            });
    };
    FilteredView() = delete;
private:
    const Container& m_foreach_provider;
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
        m_foreach_provider(c),
        m_keyExtractor(f) {};

    template<typename F>
    void foreach(F&& f) const {
        std::vector< std::reference_wrapper<const value_type>> lookup;
        m_foreach_provider.foreach([&lookup](const_reference_type el){ lookup.push_back(std::cref(el)); });
        auto sorter = [this]( auto a, auto b ) { return m_keyExtractor(a) < m_keyExtractor(b); };
        std::sort( std::begin(lookup), std::end(lookup), sorter);
        for ( auto ref : lookup ) {
            f(ref.get());
        }
    };
    SortedView() = delete;
private:
    const Container& m_foreach_provider;
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
        m_foreach_provider(c),
        m_mappingOp(m) {};

    template<typename F>
    void foreach(F&& f) const {
        m_foreach_provider.foreach([f, this](auto el) {
            f(m_mappingOp(el));
            });
    }
    MappedView() = delete;
private:
    const Container& m_foreach_provider;
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
        m_foreach_provider(c),
        m_elementsToTake(n),
        m_stride(stride),
        m_logic(l) {}

    template<typename F>
    void foreach(F&& f) const {
        size_t n = 0;
        m_foreach_provider.foreach([f, &n, this](auto el) {
            const bool needed = n % m_stride == 0
                and ((m_logic == take_elements and n < m_elementsToTake)
                    or (m_logic == skip_elements and n >= m_elementsToTake));
            if (needed)
                f(el);
            n++;
            });
    }
private:
    const Container& m_foreach_provider;
    size_t m_elementsToTake;
    size_t m_stride;
    skip_take_logic_t m_logic;
};


template< typename Container, typename Filter>
class TakeWhileView : public FunctionalInterface<TakeWhileView<Container, Filter>, typename Container::value_type> {
public:
    using value_type = typename Container::value_type;
    using reference_type = value_type&;
    using const_reference_type = const value_type&;

    TakeWhileView(const Container& c, Filter f, skip_take_logic_t l)
        : FunctionalInterface<TakeWhileView<Container, Filter>, value_type >(*this),
        m_foreach_provider(c),
        m_filterOp(f),
        m_logic(l) {};

    template<typename F>
    void foreach(F&& f) const {
        bool take = m_logic == take_elements;
        bool need_deciding = true; // to save on  calling to f once decided
        m_foreach_provider.foreach([f, &take, &need_deciding, this](auto el) {
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
            if (take)
                f(el);
            });
    };
    TakeWhileView() = delete;
private:
    const Container& m_foreach_provider;
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

    ChainView( const Container1& c1, const Container2& c2)
    : FunctionalInterface<ChainView<Container1, Container2>, value_type>(*this),
    m_foreach_provider1(c1),
    m_foreach_provider2(c2) {}
    template<typename F>
    void foreach(F&& f) const {
        m_foreach_provider1.foreach(f);
        m_foreach_provider2.foreach(f);
    }
private:
    const Container1& m_foreach_provider1;
    const Container2& m_foreach_provider2;

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
    void foreach(F&& f) const {
        for (const auto& el : m_data) {
            f(el);
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
    void foreach(F&& f) const {
        for (const auto& el : m_data) {
            f(el);
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
    void foreach(F&& f) const {
        for (const auto& el : m_data) {
            f(el);
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




// TODO
// provide more optimal foreach with option of early return
// TakeView ?



// macro generating generic, single agument, returning lambda
// example use: filter( F(_ < 0)) - the _ is the lambda argument
#define F(CODE) [&](const auto &_) { return CODE; }
template<typename T>
auto wrap(const std::vector<T>& vec) {
    return DirectView(vec);
}

