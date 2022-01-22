# FunRootAna

This is a basic framework allowing to do [ROOT](https://root.cern.ch/) analysis in a more functional way.
In comparison to RDFrame it offers more functional feel for the data analysis (in particular histograms filling) and is overall a bit more holistic. 
As consequence, <span style="color:red">a single line</span> containing selection, data extraction & histogram definition is sufficient to obtain one unit of result (one histogram).

The promise is: 

**Amount of lines of analysis code per histogram is converging to 1.**

Let's see how.


# Teaser
The example illustrating the concept is based on analysis of the plain ROOT Tree. A more complex content can be also analysed (and is typically equally easy).
Imagine you have the Tree with the info about some objects decomposed into 3 std::vectors.
Say these are `x,y,z` coordinates of the point. They are all vector of `float`. 
There is also an integer quantity indicating the the event is in a category `category` - there is 5 of them, indexed from 0-4.

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



# Analysis objects  

Now think that we would like to treat the `x,y,z` as a whole, in the end they represent one object, a point in 3D space. We need to do this with plain Ntuples but if the TTree contains object already then the functionality comes for free.
So let's make the points look like objects. For that we would need to create a structure to hold the three coordinates together (we can also use the class but we have no reason for it here).
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
For how to do it best check the `Access.cpp` source and/or example_analysis (there are better and worse implementations).


How do profit from the fact the fact that we have objects. Is it still single line per histogram? Let's see:
```c++
       for (PointsTreeAccess event(t); event; ++event) {
            const int category = event.get<int>("category");
            category >> HIST1("categories_count", ";category;count of events", 5, -0.5, 4.5); // create & fill the histogram with a plain , (creation done on demand and only once)

            const auto x = lazy_own(std::move(event.get<std::vector<float>>("x"))); // get vector of data & wrap it into a functional style container
            x >> HIST1("x", "x[mm]", 100, 1, 2); // fill the histogram with the x coordinates
            x.map( F(_ - 1) ) >> HIST1("x_shifted", "transformed x;|x-1|", 20, 0, 1); // transform & fill the histogram

            std::make_pair(x.sum(), x.size()) >> HIST2("tot_vs_size", ";sum;size", 20, 0, 150, 20, 0, 150);

            // processing objects
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
It is really a`std::vector` with extra methods.
Among many functions it offers, this three are the most relevant:
* `map` - that takes function defining the mapping operation i.e. can be as simple as taking attribute of an object or as complicated as ... 
* `filter` - that produces another container with potentially reduced set of elements
* reduce - there is several methods in ths category, most commonly used in ROOT analysis are various `fill`
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
* `sorted` - that takes "key extractor" function that would return a sortable characteristics of the data in container and that is used to sort by in ascending order
  
```c++
  //  e.g. to sort points by `x`
  const auto sorted_poits = points.sorted(F(_.x));
```
* `sum` and `accumulate` - with obvious results (the later takes customizable function F, and there is a similar variant for the sum), there are also `reduceLeft` `reduceRight` for those who are accustomed to the functional lingo
* `stat` to obtain mean, count, and RMS in one pass
* `max` & `min`
* `first`, `last`, `element_at` or `[]` for single element access
* ... and couple more.

## Lazy vs eager evaluation
The transformations performed  can be evaluated immediately or left till actual histogram filling is needed.  The EagerFunctionalVector features the first approach.
If the containers are large in your particular problem it may be expensive to make many such copies. 
Another strategy can be used in this case. That is *lazy* evaluation. In this approach objects crated by every transformation are in fact very lightweight (i.e. no data is copied). 
Instead, the *recipes* of how to transform the data when it will be needed are kept in these intermediate objects. 
This functionality is provided by several classes residing in LazyFunctionalVector _(still under some development)_.
You can switch between one or the other implementation quite conveniently by just changing the include file which you use and rename `wrap` into `lazy_own/lazy_view`.
In general the lazy vector beats the eager one in terms of performance _(an effort to optimise both implementation is ongoing)_.

If the container that is to be processed is guaranteed to reside in memory the lazy wrapper can be very lazy! That is it does not have to copy the data into own memory and should be constructed with `lazy_view`.  If the container may disappear from the memory before the wrapper is done the initial copy has to be done. In this case `lazy_own` should be used for construction.

In the lazy vector has one special methods  that can positively impact the performance sometimes. The method `stage` (or `cache`) produces an intermediate copy that is operated on by further transformations. This way, the transformations before the call to `stage` are not repeated. See an example.
```c++
auto prefiltered = container.filter(F(_.x < 2)).filter(F(_.y >5 )).filter(F(_.r() < 10)).stage(); // skipping stage() would result in filtering operations to be repeated when calculating x
auto x = prefiltered.map(F(_.x)).sort(F(_.x)).element_at(0); // here, the filtering from the line above does not happen, however the "prefilterd" is still lazy evaluating container
```

In fact you can mix the two approaches if needed calling `as_eager()`, and other way round: `lazy_view(v.unwrap())`, but tha should never be necessary.


# Histograms handling
To keep to the promise of "one line per histogram" the histogram can't be declared / booked / registered / filled / saved separately. Right!?
All of this has to happen in one statement. `HandyHists` class helps with that. While it has some functions there are rely only two functionalities that we need to know about.
* `save` method that saves the histograms in a ROOT file. *The file must not exist.* **No, there is no option to overwrite this behavior.**
* `HIST*` macros that: declare/book/register histograms and return a `WeightedHist` object. This object is really like pointer to `TH*` plus some extra functionality related to weighting.
  The arguments taken by these macros are just passed over to constructors (in simple cases). 
  The macros ending with `*V` the `std::vector<double>` is used to define bin limits.
These histograms interplay nicely with `fill` operations of the functional container described above.
Defined are: `HIST1` `HIST1V` for 1D histograms, `PROF`, `PROFV` for profiles, `EFF` `EFFV` for efficiencies, `HIST2` for 2D histograms.

The histogram made in a given file & line has to have always the same name. If name chaining is needed the `HCONTEXT` call can be used. 
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
    HIST1("x", ...);
  }
}
```
would produce the histogram `PointsAnalysis_BasicPositions_x` histogram in the output.
When the name contains `/` i.e. `A/x` histograms `x` end up in subdirectory `A`  in the output file.


# Additional functionalities
## Configuration of the analysis 
Ach, everyone needs it at some point. So there it is, a simle helper `Conf` class that allows to:
* read the config file of the form: `property=value` (# as first character are considered comments)
* read environmental variables if config file is an empty string (i.e. before running you do `export prperty=value`)
* allows to check if the property is available with `has`
* and get the property with `get` like this: `conf.get<float>("minx", 0.1)` which means: take the `minx` from the config, convert it to `float` before handing to me, but if missing use the value `0.1`.
The code above does not depend on this.
## Diagnostics 
Everyone needs to print something sometimes. 
Trivial `report` can be used for it. More useful is the `assure` function that will check if the first argument evaluates to `true` and if not will complain & end the execution via exception. 
There are less commonly needed `missing` and `assure_about_equal`.

# Playing with the example
Attached makefile compiles code in examples subdir. That one contains example_analysis.cpp that can be changed to see how things work.
Input file with the points can be generated using generateTree.C
Attached cmake file should be sufficient to compile this lib, generate the test file and so on.
# TODO 
* HIST cleanup - remove unnecessary hashing entirely





