# FunRootAna

This is a basic framework allowing to do [ROOT](https://root.cern.ch/) analysis in a more functional way.
In comparison to [RDFrame](https://root.cern.ch/doc/master/group__tutorial__dataframe.html) it offers more functional feel for the data analysis.  In particular collections processing is inspired by Apache Spark and the histograms creation and filling is much simplified. 
As consequence, <span style="color:red">a single line</span> containing selection, data extraction & histogram definition is sufficient to obtain one unit of result (that is, one histogram).

The promise is: 

**With FunRootAna the mount of lines of analysis code per histogram is converging to 1.**

Let's see how.


# Teaser
The example illustrating the concept is based on analysis of the plain ROOT Tree. A more complex content can be also analysed (and is typically equally easy).
Imagine you have the Tree with the info about some objects decomposed into 3 `std::vectors<float>`.
Say these are `x,y,z` coordinates of some measurements. In the TTree the measurements are grouped in so called *events* (think cycles of measurements).  
There is also an integer quantity indicating the event is in a category `category` - there is 5 of them, indexed from 0-4.

So the stage is set.


You can start the analysis with some basic plots:

```c++
for (Access event(t); event; ++event) {
    auto category = event.get<int>("category");
    const auto x = wrap(event.get<std::vector<float>>("x")); // get vector of data & wrap it into functional style container

    category >> HIST1("categories_count", ";category;count of events", 5, -0.5, 4.5); // create & fill the histogram, (creation done on demand and only once, the >> operator is responsible for filling)
    x >> HIST1("x", ";x[mm]", 100, 0, 100); // fill the histogram with the x coordinates (all of them)
    x >> HIST1("x_wide", ";x[mm]", 100, 0, 1000); // fill another histogram with the x coordinate
    x.filter( F(category==0)) >> HIST1("x_cat_0", ";x[mm]", 100, 0, 100); // we can filter the data before filling
    x.filter( F(category==0 && std::fabs(_) < 5 )) >> HIST1("x_cat_0_near_range", ";x[mm]", 100, 0, 5); // the filtering can depend on the variable
    x.map( F(1./std::sqrt(_)) ) >> HIST1("sq_x", ";x[mm]^{-1/2}", 100, 0, 5); // and transform as needed

    // a super complicated plot, 2histogram of number of outliers vs, category
    std::make_pair( x.count( F( std::fabs(_)>10 ) ), category ) >> HIST2("number_of_x_outliers_above_10_vs_category", ";count;category", 10, 0, 10,  5, -0.5, 4.5)
    
    // and so on .. one line per histogram

}
```

As a result of running that code over the TTree, histograms are made and saved in the output file. See complete code in example_analysis.cpp in examples dir.



# Processing collection of objects  

Now think that we would like to treat the `x,y,z` as a whole, in the end they represent one object, a measurement in 3D space. 
(In ROOT system the objects can be stored in the TTree directly. If that is the case you can skip the part of discussing how to restore the object from individual elements. )
So let's make the measurements look like objects. For that we would need to create a structure to hold the three coordinates together (we can also use the class but we have no reason for it here).
```c++
struct Point{
    float x;
    float y;
    float z;
    double rho_xy() const { return std::hypot(x, y); }
    double r() const { return std::hypot( rho_xy(), z); }
};
```
Most convenient way to extract a vector of those from the tree is to subclass the `Access` class and add a method that reconstructs it.
```c++
  class MyDataAccess : public Access {
      std::vector<Point> getPoints();
  };
```
For how to do it best check the `Access.cpp` source and/or example_analysis (there are better and worse implementations - the difference begin memory handling).


How do we profit from the fact the fact that we have objects. Is it still single line per histogram? Let's see:
```c++
       for (PointsTreeAccess event(t); event; ++event) {
            // processing objects example
            auto points = lazy_own(std::move(event.getPoints()));
            points.map( F(_.rho_xy()) ) >> HIST1("rho_xy", ";rho", 100, 0, 5); // to extract the a quantity of interest for filling we do the "map" operation
            points.map(F(_.z)) >> HIST1("z", ";zcoord", 100, 0, 10); // another example
            points.filter(F(_.z > 1.0) ).map( F(_.rho_xy())) >> HIST1("rho", ";rho_{z>1}", 100, 0, 10); // another example
            points.map( F( std::make_pair(_.r(), _.z))) >>  HIST2("xy/r_z", ";x;y", 30, 0, 3, 20, 0, 15); // and 2D hist
        }
        // see complete code in examples/example_analysis.cpp
```

# The functional container
A concise set of transformations that get us from the data to the data summary (histogram in this case) is possible thanks to the Functional Vector wrappers.
It is really a `std::vector` with extra methods.
Among many functions it offers, three that are the most relevant:
* `map` - that takes function defining the mapping operation i.e. can be as simple as taking attribute of an object or as complicated as ... 
* `filter` - that produces another container with potentially reduced set of elements
* reduce - there is several methods in ths category, most commonly used in ROOT analysis are various "fill"
 operations that fill the histograms

To cut a bit of c++ clutter there is a macro `F` defined to streamline definition of in place functions (generic c++ lambdas taking one argument and returning a result).
```c++
// say we have container of complex numbers and we want filter those that have norm < 1
container.filter( [](const std::complex& c){ return std::norm(c) < 1; } ); // long form
container.filter( [](const auto& c){ return std::norm(c) < 1; } ); // shorter form
container.filter( std::norm(_1) < 1 ); // future c++, does not work yet with any compiler
container.filter( F( std::norm(_) < 1) ); // using the F macro, quite close to the future
```

Other, maybe less commonly used functions, but still good to know about are:
* `size` - that return size of the container
* `count` - which shortcuts `filter` followed by `size`
* `sort` - that takes "key extractor" function that would return a sortable characteristics of the data in container and that is used to sort by in ascending order
  
```c++
  //  e.g. to sort points by `x`
  const auto sorted_poits = points.sort(F(_.x));
```
* `sum` and `accumulate` - with obvious results (the later takes customizable function F, and there is a similar variant for the sum), there are also `reduceLeft` `reduceRight` for those who are accustomed to the functional lingo
* `stat` to obtain mean, count, and RMS in one pass
* `max` & `min`
* `first`, `last`, `element_at` or `[]` for single element access
* ... and couple more.

## Lazy vs eager evaluation
The transformations performed  can be evaluated immediately or left till actual histogram filling (or any other summary) is needed.  The EagerFunctionalVector features the first approach.
If the containers are large in your particular problem it may be expensive to make many such copies. 
Another strategy can be used in this case. That is *lazy* evaluation. In this approach objects crated by every transformation are in fact very lightweight (i.e. no data is copied). 
Instead, the *recipes* of how to transform the data when it will be needed are kept in these intermediate objects. 
This functionality is provided by several classes residing in LazyFunctionalVector _(still under some development)_.
You can switch between one or the other implementation quite conveniently by just changing the include file which you use and rename `wrap` into `lazy_own/lazy_view`.
In general the lazy vector beats the eager one in terms of performance _(an effort to optimise both implementation is ongoing)_.

If the container that is to be processed is guaranteed to reside in memory the lazy wrapper can be very lazy! That is it does not have to copy the data into own memory and should be constructed with `lazy_view`.  If the container may disappear from the memory before the operation on data are all done the initial copy has to be done. In this case `lazy_own` should be used for construction.


The depth of the transformations in the lazy container are unlimited. It may be thus reasonable to construct the transformation in steps:
```c++
auto step1 = lazy_view(data).map(...).filter(...).take(...); // no computation involved
auto step2a = step1.map(...).filter(...).skipWhile(...); // no computation involved
auto step2b = step1.filter(...).map(...).max(); // no computation involved

step2a.map(...) >> HIST1("a", ...); // computations of step1 and step2a involved
step2b.map(...) >> HIST1("b", ...)  // computations of step1 repeated!!! followed by step2b  
```
It may be optimal to actually cache the `step1` result in memory so the operations needed to obtain do not need to be repeated.
The method `stage` (or `cache`) produces such an intermediate copy that is operated on by further transformations. This way, the transformations before the call to `stage` are not repeated. See an example.
```c++
auto step1 = lazy_view(data).map(...).filter(...).take(...).stage(); // computations done and intermediate container created
auto step1 = lazy_view(data).map(...).filter(...).take(...).cache(); // computations done when first time needed, then reused

```

In fact you can mix the two approaches if needed calling `as_eager()`, and other way round: `lazy_view(v.unwrap())`, but that should never be necessary.

# ROOT TTree as a lazy collection

The entries stored in the `TTree` can be considered as a functional collection as well. All typical transformations can then be applied to it. An example is shown below. It is advised however (for performance reason) to process the TTree only once. That is, to end the operations with the `foreach` that obtains a function processing the data stored in the tree.
```c++
        FunctionalAccess<PointsTreeAccess> events(t); // the tree wrapped in an functional container

        events
        .take(2000) // take only first 1000 events
        .filter( [&](auto event){ return event.current() %2 == 1; }) // every second event (because why not)
        .foreach([&](auto event) {
            // analyse content of the data entry
            const int category = event.template get<int>("category"); // "template" keyword needed here 
            category >> HIST1("categories_count", ";category;count of events", 5, -0.5, 4.5); // create & fill the histogram with a plain , (creation done on demand and only once)
            // and so on
        });
```

# Histograms handling
To keep to the promise of "one line per histogram" the histogram can't be declared / booked / registered / filled / saved separately. Right!?
All of this has to happen in one statement. `HandyHists` class helps with that. There are only two functionalities that we need to know about.
* `save` method that saves the histograms in a ROOT file (name given as an argument). *The file must not exist.* **No, there is no option to overwrite this behavior.**
* `HIST*` macros that: declare/book/register histograms and return a `ROOT TH` object reference.
  The arguments taken by these macros are just passed over to constructors of respective histogram classes. 
  In macros ending with `V` the `std::vector<double>` is used to define bin limits.
In the header `filling.h` a bunch of `>>` operators are defined to facilitate easy insertions of the data from functional containers (but also PODs).
Defined are: `HIST1` `HIST1V` for 1D histograms, `PROF`, `PROFV` for profiles, `EFF` `EFFV` for efficiencies, `HIST2` for 2D histograms.

The histogram made in a given file & line has to have always the same name. If name chaining is needed the `HCONTEXT` call can be used before. 
```c++
{
    HCONTEXT("HighRes_");
    HIST1("x", ...); // here the histogram name realy becomes HighRes_x 
    HIST1(std::string("x"+std::to_string(i)), ...); // this will be runtime error
}
```
The `HCONTEXT` argument can depend on some variables (and so can change) if needed. This behavior allows certain optimization. In any way, it is usually good idea to add context in scopes so that the histograms in the output are somewhat organized. Contexts are nesting, that is:
```c++
{
  HCONTEXT("PointsAnalysis_");
  {
    HCONTEXT("BasicPositions_");
    points.map( F(_.x)) >> HIST1("x", ...);
  }
}
```
would produce the histogram `PointsAnalysis_BasicPositions_x` histogram in the output.
When the name contains `/` i.e. `A/x` histograms `x` end up in subdirectory `A`  in the output file.


# Additional functionalities
## Configuration of the analysis 
Ach, everyone needs it at some point. So there it is, a simle helper `Conf` class that allows to:
* read the config file of the form: `property=value` (# as first character are considered comments),
* read environmental variables if config file is an empty string (i.e. before running you do `export prperty=value`),
* allows to check if the property is available with method `has("minx")`,
* and get the property with `get` like this: `conf.get<float>("minx", 0.1)` which means: take the `minx` from the config, convert it to `float` before handing to me, but if missing use the value `0.1`.
The code above does not depend on this.
## Diagnostics 
A trivial function `report` can be used to produce a message.
More useful is the `assure` function that will check if the first argument evaluates to `true` and if not will complain & end the execution via exception. 
There are less commonly needed `missing` and `assure_about_equal`.

# Playing with the example
The `examples` subdir contains code `example_analysis.cpp` that can be played with the library.
Input file with the points can be generated by calling: `root generateTree.C`
Attached cmake file should be sufficient to compile this lib, generate the test file and so on.
