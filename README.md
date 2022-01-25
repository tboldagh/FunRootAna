# FunRootAna
This project is about providing functional analysis style to CERN ROOT users.
See the introduction in [Git Pages](https://tboldagh.github.io/FunRootAna/)


# Code structure
The code has has this basic elements:
- streamlined ROOT tree access layer
- histograms handling 
- two functional container implementations, eager and lazy one - the later should be typically used
- smaller utilities, weight handling, configuration, diagnostics. 
The examples and unit tests are also included. The

# API documentation
## Functional helpers
### Macro F
Helper to generate pure generic lambda expression. Example
```c++
auto f = F( _*_); 
\\ is identical to
auto f = [](auto _) { return _*_; };
```
### Macro C
Like `F` except it is a closure (can access variables that ere out of scope by reference).
```c++
double x = 88;
auto f = C( _* x); 
\\ is identical to
auto f = [&](auto _) { return _*x; };
```

### Macros S
Like `C` but w/o returning any value (sometimes needed when no value can be returned).
```c++
double x = 0;
auto f = S( x+=_); 
\\ is identical to
auto f = [&](auto _) { x += _; };
```

### Macro PRINT
Prints to the `std::cout` the content (if it is printable). Example:
```c++
data.inspect(PRINT).filter(...).inspect(PRINT).map(...)...
```


### struct triple
An extension to the `std::pair`. Contains additional field, `third`.

## Functional container (LazyFunctioanlVector/EagerFunctionalVector)
The containers are named "vectors", but in fact they can wrap other stl containers too.


### wrap(cont)
Produces eager container.

### lazy_view(cont)/lazy_own(cont)/lazy_own(cont&&)
Produces lazy containers.
lazy_view does not cause a copy. The lazy_own involve copies. The lazy_own with move optimizes copy by moving instead.
```c++
std::vector<MyData> d = ...; // data available in scope
auto dview = lazy_view(d); // this best option in this case

auto fview = lazy_own( std::move(code returning a vector) ); // best option in this case

```


### map( transformer )
Creates elements after transforming them.
```c++
data.map( F(_*_) ) // produces squared elements
``` 

### filter( predicate )
Crates reduced set of elements
```c++
data.filter(F(_<0)).count();
```

### foreach( subroutine )
Takes a subroutine (function that returns nothing) and evaluates it on every element. Typically used for side-effects, example: printing.
```c++
data.foreach(S( std::cout << _ << " "));
```

### inspect( subroutine )
Like the foreach - but evaluates the subroutine in a lazy way.

### empty
Returns `true` if the container is empty.

### size
Returns number of elements in the container. In case of lazy container it may require traversing the container (because of intermediate filtering).

### count( predicate )
Returns number of elements satisfying the predicate. 
```c++
const size_t n_elements_above0 = data.count(F( _>0 ));
\\ slightly a more efficient version of
data.filter(F(_>0)).size();
```

### contains( predicate ) / contains(value)
Returns true if there is at least one element satisfying the predicate. 
```c++
const bool any_elements_above0 = data.contains(F( _>0 ));
\\ slightly a more efficient version of
not data.filter(F(_>0)).empty();
```

### all( predicate )
Returns true if all elements satisfy predicate.
_Notice:_ `false` is always returned for an empty container.
### sorted( keyextractor )/ sorted()
Produces container of elements that ar sorted ascending by a property extracted by provided function
```c++
// assume data is collection of doubles
data.sorted() // the doubles in ascending order
data.sorted( F( -_)) // sort in descending order
```
*Notice* In case of lazy container the sorting involves making a temporary lightweight copy of references. 
It may be good idea to use `cache/stage` after sorting.

### take(n, stride=1)
Takes n first elements from the container skipping them by `stride`.
```c++
//Say the data contains letters A, B, C, D, E, F, ...

data.take(6,2) // results in A C E
```
### skip(n, stride=1)
Similar to the `take`, but skips `n` first elements.


### skip_while/take_while (predicate)
Similar to `take/while` takes or skips first elements satisfying the predicate.

### stage/cache
In case of lazy container make an intermediate container.
More in the [Project page] (https://tboldagh.github.io/FunRootAna/)
*Notice* In case of eager container this is void operation. 


### reverse
Produces container of elements in reverse order to the original one.

### min/max( keyextractor == identity)
Returns a single (or empty) container with the element that is extreme according to the value returned by the `keyextractor` function.
```c++
// assumed data is: -7, -1, 2, 2, 6, -3
data.max() // is single element container with 6
data.min( F(std::abs(_))).min() // is single element with -1
```

### chain( container )
Produces a container that is the concatenation of the two operands.
```c++
data1.chain(data2) // just concatenation of the the two
data1.chain(data1) // repeated container (no actual copies are involved) 
data1.chain(data2).chain(data3).chain(data4) // concatenation of 4 containers
```

### zip( cont )
Produces pairs of elements from operands.

*Notice* The containers can be of different length. The pairs are generated until exhaustion of elements in the shorter one.

### is_same( other, comparator )/is_same(other)
Compares container element by element using `comparator` and returns true if all comparisons were positive. Internally uses zip, and the same limitation applies.
```c++
// assume data1 is A B C D E F
// and data 2 is alec, ben, charles
data1.is_same(data2, F(std::tolower(_.first) == _.second[0])) // return true because first letters are the same as the chars in the first sequence
```
Version w/o the comparator compares elements by their respective == operation.

### group(n, jump)
Groups elements in the container in sub-container of size n. 
```c++
// assume data is A B C D E F
data.group(3) // is A B C and then D  E  F  
data.group(3, 1) // is A B then B C, then C D ...
```

### cartesian(cont)
Forms container of each pair that can be formed from the two operands.
```c++
// assume data1 is A B C
// data2 1 2
data1.cartesian(data2) // is A1, A2, B1, B2, C1, C2
```

### sum(transform) / sum()
Sums element in the container. If the `transform` is provided is is identical to mapping and the summing.

### accumulates (transform, initial_value)
Standard reduction operation.
```c++
// assume the data is 1 2 3 4 5
data.reduce( []( auto total, auto el){ return total*el; }, 1) // is 1*2*3*4*5
```
*Notice* This is very versatile operation and in fact can be used to implement almost all (non-lazy) operations of the container.


### stat(transform)
Calculates statistical properties (count/mean/variance). The `transform` is like applying a `map` before collecting the stats.

### element_at(n) 
Returns an element at an index. This operation can involve traversing the container and should in general be avoided. Returned ins std::optional that needs to be checked for content. 
I.e. It is perfectly correct to ask for element beyond the size of the container. The result will be an empty optional.

### get
Identical to element_at(0). Handy with min/max.

### first_of( predicate )
Returns first element satisfying the predicate. May be none, thus returning optional.

### push_back_to/insert_to
Insert data to standard containers.

## Infinite containers (only lazy)
### geometric_stream( c, r )
Generates an infinite sequence of double precision values:
`c, c*r, c*r^2, c*r^3, ...,`

### arithmetic_stream( c, i)
Generates an infinite sequence of values:
`c, c+i, c+i+i, c+i+i+i ...` 
*Notice* The type of `c` and `i` can be any allowing for addition, e.g. strings, integers, double, complex etc.

### iota_stream( c )
Like arithmetic stream but with increment == 1.

### crandom_stream
Random integers drawn using standard C random function. 

### range_stream( begin, end, stride=1)
Finite size stream of numbers
`begin, begin +stride, begin + stride^2` ... until the value is not greater than the `end`.




# Histogramming API
The `HistHelper` class is to be inherited analysis code in order to profit from the functionality. 

The public API of the histogramming has:
## save(filename)
Function saving histograms in the file.

## HIST1/HIST2/EFF/PROFF
Create (one-time)/register respective histograms.

# Configuration
The `Conf` class offers two sources of configuration. The config file formatted as follows:
```
settingA=valueA
#settingB=valueB
...
```
or environment variables that are set before running the analysis program:
```
export settingA=valueA
```

In the code the configuration can be accessed by instantiating `Conf` object with the file name (or w/o for reading the environment). 

There are two methods of the Conf class:
### has(setting)
That returns true if there is a setting of a given name.


### get<T>(name, default)
Returns the setting value, or the default is the setting is absent. 

# Weighting

A global weight that can be manipulated in RAII style with helper functions:
`UPDATE_MULT_WEIGHT` and `UPDATE_ABS_WEIGHT`.

Example:
```c++
Weight::set(0.5);
WEIGHT;  // get the absolute weight

{
  UPDATE_ABS_WEIGHT(0.9);// update the weight locally 
}
// weight restored
```

# Selector
It is frequent that we need to select a value depending on several conditions. If we want to have that value to be const the code to do it somewhat awkward to write pile of ternary expressions. The `Selector` offers a more legible alternative.
```c++
const double value = option(x < 0, 0.7)
                    .option(  0 < x and x < 1, 0.9)
                    .option(1.2).select();

const std::string descr = option(x<0, "less than zero").option(x>0, "more than zero").select();
// in this case (missing last - option call w/o the boolean) if x is 0 the select will raise an exception
```


# TODO 
* HIST cleanup - remove unnecessary hashing entirely





