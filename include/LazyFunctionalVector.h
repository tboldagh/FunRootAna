// lazy version of WrappedVector
//LazyFunctionalVector
#include <type_traits>

template< typename Container, typename Filter> class FilteredView;
template< typename Container, typename Map> class MappedView;
template<typename T> class DirectView;
template<typename T> class OwningView;

template<typename Container, typename Stored>
class FunctionalInterface {
public:
    FunctionalInterface(const Container& cont) : m_foreach_provider(cont) {}

    // subclasses need to implement this method
    // template<typename F>
    // void foreach( F f ) const;

    // count of elements, this eager operation
    size_t size() const {
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

    // creates intermediate container to speed following up operations
    
    auto stage() const -> OwningView<Stored> {
        OwningView<Stored> c;
        m_foreach_provider.foreach( [&c]( auto el){ c.insert(el); } );
        return c;
    }
    
    // max and min

    // sum

    // accumulate

    // stat

    // ROOT histograms fill
/*
    auto element_at(size_t  n) {
        value_type temp;
        size_t i = 0;
        m_foreach_provider.foreach( [&temp, n, &i] ( auto el) {  
                                       if ( i == n ) temp = el; i++; 
                                  });
        return temp;
    }
*/
    // back to regular vector
    template<typename R>
    void push_back_to( R& result ) const { 
        m_foreach_provider.foreach( [&result]( auto el ){ 
                                    result.push_back(el); 
                                  });
    }

    template<typename R>
    void insert_to( R& result ) const { 
        m_foreach_provider.foreach( [&result]( auto el ){ 
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
    typedef typename Container::value_type value_type;
    typedef value_type& reference_type;
    typedef const value_type& const_reference_type;

   // typedef FunctionalInterface<Container, value_type> container;

    FilteredView(const Container& c, Filter f)
        : FunctionalInterface<FilteredView<Container, Filter>, value_type >(*this),
        m_foreach_provider(c),
        m_filterOp(f) {};

    template<typename F>
    void foreach(F&& f) const {
        m_foreach_provider.foreach( [f, this](auto el) {  
                                    if ( this->m_filterOp(el) ) 
                                       f(el); 
                                   });
    };
    FilteredView() = delete;
private:
    Filter m_filterOp;
    const Container& m_foreach_provider;
};

// provides the transforming view
template< typename Container, typename Mapping>
class MappedView : public FunctionalInterface<MappedView<Container, Mapping>, typename std::invoke_result<Mapping, typename Container::value_type>::type > {
public:
    using processed_type = typename Container::value_type;
    using value_type = typename std::invoke_result<Mapping, processed_type>::type;
    using const_value_type = const value_type;
    using const_reference_type = const_value_type &;
    using container = FunctionalInterface<Container, value_type>;

    MappedView(const Container& c, Mapping m)
        : FunctionalInterface<MappedView<Container, Mapping>, value_type >(*this),
        m_foreach_provider(c),
        m_mappingOp(m) {};

    template<typename F>
    void foreach(F&& f) const {
        m_foreach_provider.foreach( [f, this](auto el) {  
                                        f(this->m_mappingOp(el));
                                    });
    }
    MappedView() = delete;
private:
    Mapping m_mappingOp;
    const Container& m_foreach_provider;
};



template<typename T>
class DirectView : public FunctionalInterface<DirectView<T>, T> {
public:
    using value_type = T;
    using const_value_type =  const value_type;
    using  const_reference_type = const_value_type&;
    using  container = DirectView<T>;

    DirectView(const std::vector<T>& m)
        : FunctionalInterface<container, T>(*this), m_data(m) {}

    template<typename F>
    void foreach(F&& f) const {
        for (const auto& el : m_data) {
            f(el);
        }
    };
private:
    const std::vector<T>& m_data;
};


template<typename T>
class OwningView : public FunctionalInterface<OwningView<T>, T> {
public:
    using value_type = T;
    using const_value_type =  const value_type;
    using const_reference_type = const_value_type&;
    using container = OwningView<T>;

    OwningView()
        : FunctionalInterface<container, T>(*this) {}
    void insert( const_reference_type d) { m_data.push_back(d); }
    template<typename F>
    void foreach(F&& f) const {
        for (const auto& el : m_data) {
            f(el);
        }
    };
private:
    const std::vector<T> m_data;
};



// macro generating generic, single agument, returning lambda
// example use: filter( F(_ < 0)) - the _ is the lambda argument
#define F(CODE) [&](const auto &_) { return CODE; }



#define wrap( vec ) DirectView(vec)

// TODO 
// provide more optimal foreach with option of early return
// reverse foreach maybe?