# FunRootAna

This is basic framework allowing to do [ROOT](https://root.cern.ch/) analysis in a more functional way.
In comparison to RDFrame it offers more functional feel for the data analysis (in particular histograms filling) and is overall a bit more holistic. 
As consequence, typical a single line containing selection, data extraction & histogram definition is sufficient to obtain one unit of result.

The promise is: 

**Amount of lines of analysis code per histogram is converging to 1.**

Let's see how.


# Teaser
Example is based on analysis of the plain ROOT Tree, but it is not at all restricted to such use-case.
Imagine you have the Tree with the info about some objects decomposed into 3 std::vectors.
Say these are `x,y,z` coordinates of the point. They are all vector of `float`. 
There is also an integer quantity indicating the the event is in a category `category` - there is 5 of them, indexed from 0-4.

The stage is set.


The analysis you can start with some basic plots:

```c++
for (Access event(t); event; ++event) {
    auto category = event.get<int>("category");
    const auto x = wrap(event.get<std::vector<float>>("x")); // get vector of data & wrap it into functional style container

    HIST1("categories_count", ";category;count of events", 5, -0.5, 4.5).fill( category ); // create & fill the histogram, (creation done on demand and only once)
    x.fill(HIST1("x", ";x[mm]", 100, 0, 100)); // fill the histogram with the x coordinate
    x.fill(HIST1("x_wide", ";x[mm]", 100, 0, 1000)); // fill another histogram with the x coordinate


    HIST2("number_of_x_outliers_above_10_vs_category", ";count;category", 10, 0, 10,  5, -0.5, 4.5).fill( x.count( F( std::fabs(_)>10 ) ), category ); 
    // a super complicated plot, 2histogram of number of outliers vs, category
    // and so on .. one line per histogram
}
```


As a result of running that code* over the TTree, 4 histograms are made and saved in the output file. See complete code in examples dir.



# Grouping related info

Now think that we would like to treat the `x,y,z` as a whole, in the end they represent one object, a point in 3D space. 
For that we would need to create a structure to hold it all together (you can also use the class but we have no reason here for it here).
```c++
struct Point{
    float x;
    float y;
    float z;
    // you can equip it with some handy methods as well
};
```
Most convenient way to extract a vector of those from the tree is to subclass the `Access` class and add a method that reconstructs it.
```c++
  class MyDataAccess : public Access {
      std::vector<Point> getPoints();
  };
```
For an example check the `Access.cpp` source and/or examples (there are better and worse implementations).


Now, how do profit from the fact the fact that we have objects.
```c++
for (PointsTreeAccess event(t); event; ++event) {
    auto points = wrap(event.getPoints());
    // let's profit from the fact that we have an object    
    points.map( F(_.rho_xy()) ).fill(HIST1("rho_xy", ";rho", 100, 0, 100)); // to extract the a quantity of interest for filling we do the "map" operation
    points.filter( F(_.r() < 10 && _.z>5) )
          .map( F( std::make_pair(_.x, _.y)) )
          .fill(HIST2("xy/points_close_to_center_high_z", ";x;y", 20, 0, 20, 20, 0, 20));
    points.filter( F(_.r() < 10 && _.z<5) )
          .map( F( std::make_pair(_.x, _.y)) )
          .fill(HIST2("xy/points_close_to_center_low_z", ";x;y", 20, 0, 20, 20, 0, 20));
    // ... again one line per histogram
}
```

# The functional container
A concise set of transformations that get us from the data to the data summary (histogram in this case) is possible thanks to the WrappedSequenceContainer.
It is really a conveniently wrapped `std::vector` (see TODO if want to make it better). 
Among many functions it offers this three are the most important:
* `map` - that takes function defining the mapping operation i.e. can be as simple as taking attribute of an object or as complicated as ... 
* `filter` - that produces another container with potentially reduced set of elements
* reduce - there is several methods in ths category, most commonly used in root analysis are various `fill`
 operations that fill the histograms

To cut a bit of c++ clutter there is a macro `F` defined to streamline definition of in place functions (generic c++ lambdas taking one argument and returning one result).
```c++
// say we have container of complex numbers and we want filter those that have norm < 1
container.filter( [](const std::complex& c){ return std::norm(c) < 1; } ); // long form
container.filter( [](const auto& c){ return std::norm(c) < 1; } ); // shorter form
container.filter( F( std::norm(_) < 1) ); // using the F helper macro
container.filter( std::norm(_1) < 1 ); // future c++, does not work yet with any compiler
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
* ... and cople more.

# Histograms handling
To keep to the promise of "one line per histogram" the histogram can't be declared / booked / registered / filled / saved separately. Right!?
All of this has to happen in one statement. `HandyHists` class helps with that. While it has some functions there are rely only two functionalities that we need to know about.
* `save` method that saves the histograms in a ROOT file. *The file must not exist.* **No, there is no option to overwrite this behavior.**
* `HIST*` macros that: declare/book/register histograms and return a `WeghtedHist` object. This object is really like pointers to `TH*` plus some extra functionality related to weighting.
  The arguments taken by these macros are just passed over to constructors (in simple cases). For macros ending with `*V` the `std::vector<double>` is used to define bin limits.
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
The `HCONTEXT` argument can depend on some variables (and so can change) if needed. This behaviour allows certain optimization.

When the name contains `/` i.e. `A/x` histograms `x` end up in subdirectory `A`  in the output file.


# Configuration of the analysis
Ach, everyone needs it at some point. So there it is, a simle helper `Conf` class that allows to:
* read the config file of the form: `property=value` (# as first character are considered comments)
* read environmental variables if config file is an empty string (i.e. before running you do `export prperty=value`)
* allows to check if the property is available with `has`
* and get the property with `get` like this: `conf.get<float>("minx", 0.1)` which means: take the `minx` from the config, convert it to `float` before handing to me, but if missing use the value `0.1`.

# Diagnostics 
Everyone needs to print something sometimes. 
Trivial `report` can be used for it. More useful is the `assure` function that will check if the first argument evaluates to `true` and if not will complain & end the execution via exception. 
There are less commonly needed `missing` and `assure_about_equal`.


# Playing with the example
Attached makefile compiles code in examples subdir. That one contains example_analysis.cpp tha can be changed to see how things work.
Input file with the points can be generated using generateTree.C
Attached makefile should be sufficient to compile this lib.
# TODO 
* unit tests
* Move to better implementations of functional container (std::ranges/std::views) or [FTL](https://github.com/ftlorg/ftl).
* cmake build
* HIIST macros cleanup





