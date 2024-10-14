# FunRootAna

This is a basic framework allowing to do [ROOT](https://root.cern.ch/) analysis in a more functional way.
In comparison to [RDFrame](https://root.cern.ch/doc/master/group__tutorial__dataframe.html) it offers more functional feel for the data analysis.  In particular collections processing is inspired by Apache Spark and the histograms creation and filling is much simplified. 
As consequence, <span style="color:red">a single line</span> containing selection, data extraction & histogram definition is sufficient to obtain one unit of result (that is, one histogram).

The promise is: 

**With FunRootAna the number of lines of code per histogram is converging to 1.**

Let's see how.


# Teaser
The example illustrating the concept is based on analysis of the plain ROOT Tree. A more complex content can be also analysed (and is typically equally easy).
Imagine you have the ROOT Tree with the info about some objects decomposed into 3 `std::vectors<float>`.
Say these are `x,y,z` coordinates of some measurements. In the TTree the measurements are grouped in so called *events* (think cycles of measurements).  
There is also an integer quantity indicating the event is in a category `category` - there is 5 of them, indexed from 0-4.

So the stage is set.


You can start the analysis with some basic plots:

```c++
for (ROOTTreeAccess event(t); event; ++event) {
    auto category = event.get<int>("category");
    category >> HIST1("categories_count", ";category;count of events", 5, -0.5, 4.5); // create & fill the histogram, (creation done on demand and only once, the >> operator is responsible for filling)
    const auto xVec = event.get<std::vector<float>>("x"); // get vector of data & wrap it into functional style container
    auto x = lazy_view(xVec);
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

As a result of running that code over the TTree, histograms are made. See complete code in example_analysis.cpp in examples dir. It includes also oneliner to save the histograms in an output file.



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
  class MyDataAccess : public ROOTTreeAccess {
      std::vector<Point> getPoints();
  };
```
For how to do it best check the `ROOTTreeAccess.cpp` source and/or example_analysis (there are better and worse implementations - the difference begin memory handling).


How do we profit from the fact the fact that we have objects. Is it still single line per histogram? Let's see:
```c++
       for (PointsTreeAccess event(t); event; ++event) {
            // processing objects example
            auto pointsVec = event.getPoints();
            auto points = lazy_view(pointsVec);
            points.map( F(_.rho_xy()) ) >> HIST1("rho_xy", ";rho", 100, 0, 5); // to extract the a quantity of interest for filling we do the "map" operation
            points.map(F(_.z)) >> HIST1("z", ";zcoord", 100, 0, 10); // another example
            points.filter(F(_.z > 1.0) ).map( F(_.rho_xy())) >> HIST1("rho", ";rho_{z>1}", 100, 0, 10); // another example
            points.map( F( std::make_pair(_.r(), _.z))) >>  HIST2("xy/r_z", ";x;y", 30, 0, 3, 20, 0, 15); // and 2D hist
        }
        // see complete code in examples/example_analysis.cpp
```

Sometimes making an intermediate `vector<T>` may be expensive. Because the vector is large or because the T is large. 
An alternative could be to construct the `T` as we iterate over the data. 
Technically this is achieved by keeping the content in separate containers and by proving a helper that looks like a `collection of T` 
whereas underneath the objects are created on demand. The file `examples/example_analysis.cpp` contains an example of implementing this approach.

# CSV files
Similarly to ROOT trees tha data from CSV files can be read. 
Let's say the data ia available in a file: `data.csv` with the following content corresponding to `x,y,z,label` quantities.
```
2.5,9.87,87.2,ready
2.5,1.87,0.34,to check
... and so on
```
It can be loaded similarly to the ROOT tree:
```c++
auto stream = std::ifstream("data.csv");
using namesapce CSV;
CSVAccess acc( Record(Item("x"), Skip(), Item("z"), Skip()), stream); // only ever care about x and z
for ( ; acc; ++acc ) {
  acc.get<int>("x").value() >> HIST1(...); // the get method returns std::optional (to gently handle issues in decoding or missing fields)  
}
```
When defining the `Record` a delimiter different than the `,` can be specified like this: `Record(';', Item("x"), ... )`.
If the delimiter changes from field to field this construct can be used: `Record(Item("x", ';'), Item("y", ';'), Item("label"))`. 
That would work for CSV file formatted like this: 
```
1,2;hello
3,4;there
...
```

The fields in the record can be accessed with an index with two syntaxes: 
`acc.get<int>(0)` for the first element. 
If the fields location is fixed a compile time index can be provided in ths way:
`acc.get<0, int>()`. It is the fastest way.
The conversion happens only at the access so in case data filtering is involved it can be spared. 
It also means that one data field can be accessed as few different data types. E.g. `acc.get<std::string>("x")` or `acc.get<unsigned>("x")` or `acc.get<double>("x")` are all valid uses.

# The functional container
A concise set of transformations that get us from the data to the data summary (histogram in this case) is possible thanks to the Functional Vector wrappers.
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
* `sum` - with obvious results (the first take key extractor function defaulting to identity), 
* `accumulate` (typically called `reduce`) - that takes customizable function `f`, which is evaluated for each element with this meaning `total = f(total, element)` and start value,
* `stat` - to obtain mean, count, and RMS in one pass over elements,
* `max` & `min` - to obtain extreme values (takes key extractor),
* `first`, `last`, `element_at` or `[]` for single element access,
* ... and couple more.

It is really like a `std::vector` with these extra methods. However these extra methods allow to write a very clear and expressive code. 
For example, assume the task to find a maximum value in the two containers or, if both are empty obtain a default. The imperative code would look somewhat like this:
```c++
// data1 is a vector containing doubles
double max = std::numeric_limits<double>::min();
for ( auto el: data1) {
  if ( el > max ) {
    max = el;
  }
}
for ( auto el: data2) {
  if ( el > max ) {
    max = el;
  }
}

if ( max == std::numeric_limits<double>::min()) {
  max = 100;
}
// or with STL
double max = 100.0;
auto max_el1 = std::max_element(std::begin(data1), std::end(data1));
if ( max_el1 != std::end(data1))
  max = *max_el1;
auto max_el2 = std::max_element(std::begin(data2), std::end(data2));
if ( max_el1 != std::end(data2) && *max_el2 > *max_el1)
  max = *max_el;


// or with the functional container
const double max = data1.chain(data2).max().get().value_or(100.0);
```
The difference in the clarity of the intent is just striking. And so is the difference of the amount of code to write. Note also that, since there is no need to update the state of the `max` it can be made constant.

## Lazy vs eager evaluation
The transformations performed  can be evaluated immediately or left till actual histogram filling (or any other summary) is needed.  The EagerFunctionalVector features the first approach.
If the containers are large in your particular problem it may be expensive to make many such copies. 
Another strategy can be used in this case. That is *lazy* evaluation. In this approach objects crated by every transformation are in fact very lightweight (i.e. no data is copied). 
Instead, the *recipes* of how to transform the data when it will be needed are kept in these intermediate objects. 
This functionality is provided by several classes residing in LazyFunctionalVector _(still under some development)_.
You can switch between one or the other implementation quite conveniently by just changing the include file which you use and rename `wrap` into `lazy_view`. An important difference though is that the lazy containers do not manage memory, that is they avoid copying the data and thus are significantly faster. But you need to be aware about the data lifetime. If in doubt, you can always `stage` the data and then view it again.
In general the lazy vector beats the eager one in terms of performance.


The depth of the transformations in the lazy container are unlimited. It may be thus reasonable to construct the transformation in steps:
```c++
auto step1 = lazy_view(data).map(...).filter(...).take(...); // no computation involved
auto step2a = step1.map(...).filter(...).skipWhile(...); // no computation involved
auto step2b = step1.filter(...).map(...).max(); // no computation involved

step2a.map(...) >> HIST1("a", ...); // computations of step1 and step2a involved
step2b.map(...) >> HIST1("b", ...)  // computations of step1 repeated!!! followed by step2b  
```
It may be optimal to actually cache the `step1` result in memory so the operations needed to obtain do not need to be repeated.
The method `stage` produces copy that could then be operated on by further transformations. This way, the transformations before the call to `stage` are not repeated. See an example:
```c++
auto step1 = lazy_view(data).map(...).filter(...).take(...).stage(); // computations done and intermediate container created
auto step2 = lazy_view(step1).map(...).filter(...).take(...).stage(); // computations done when first time needed, then reused
```
The result of stage is just a `std::vector`. If other containers are better suited, i.e. `std::list`  the call would look lie this: `...).stage<std::list>();`. This way you can generate sets or maps, whatever fits best.
# TTree as a lazy collection

The entries stored in the `TTree` of `CSV` can be considered as a functional collection as well. All typical transformations can then be applied to it. An example is shown below. It is advised however (for performance reason) to process the TTree only once. That is, to end the operations with the `foreach` that obtains a function processing the data stored in the tree.
Also the data available in tree branches is available using the same, functional, interface.
```c++
        ROOTTreeAccess acc(t);
        AccessView events(acc); // the tree wrapped in an functional container

        // or for CSV 
        // CSVAccess acc(Record(...), stream);
        // AccessView events(acc);

        events
        .take(2000) // take only first 1000 events
        .filter( [&](auto event){ return event.current() %2 == 1; }) // every second event (because why not)
        .foreach([&](auto event) {
            // analyse content of the data entry

            auto category_view = event.template branch_view<int>("category"); // obtain single datum warped as functional container
            category_view.map(F(_-2)) >> HIST1("categories_view_count", ";category;count of events", 5, -0.5, 4.5); 
            

            const auto x_view = event.template branch_view<std::vector<float>>("x"); // obtain directly functional style container directly (avoids data copies)
            x_view.filter(F(_>0.2)).map(F(_*_)) >> HIST1("sq", ";squares>0.2", 10,0, 1);

            // and so on
        });
```

# Histograms handling
To keep to the promise of "one line per histogram" the histogram can't be declared / booked / registered / filled / saved separately. Right!?
All of this has to happen in one statement. `HandyHists` class helps with that. There are only tree functionalities that we need to know about.
* `save` method that saves the histograms in a ROOT file (name given as an argument). *The file must not exist.* **No, there is no option to overwrite this behavior.**
* macros that: declare/book/register histograms and return a `ROOT TH` object reference.
  The arguments taken by these macros are just passed over to constructors of respective histogram classes. 
  In macros ending with `V` the `std::vector<double>` is used to define bin limits.
  Defined are: `HIST1` and `HIST1V` for 1D histograms, `PROF1` and `PROF1V` for profiles, `EFF1` and `EFF1V`, `EFF2` and `EFF2V` (taking two `vector<double>` argument) for efficiencies, `HIST2` and `HIST2V` for 2D histograms, `HIST3` and `HIST3V` for 3D,
* and the context control. The `TGraph` and `TGraph2D` can be created with `GRAPH` and `GRAPH2` macros. As the two are basically collection of points in 2D and 3D spaces respectively only name snd title are needed to contruct them.

## Operator `>>` for histograms filling
In the header `filling.h` a bunch of `>>` operators are defined to facilitate easy insertions of the data from functional containers and PODs into histograms.
Histograms can be filled with their respective API however for syntactical clarity a set of operators `>>` is provided. Examples above illustrate how it can be used. In summary the value on the left side of `>>` can be either, single value, container of values or the  `std::optional`. For single value only one filling operation occur, if the functional container precedes the `>>` all the data in the container will be entered in the histogram. When the `std::optional` is used, the fill operation occurs only when there is a value in it. For 2/3-dimensional histograms or weighted histograms the values on the left can be std::pairs/std::tuples or std::array. E.g. to insert to a 2D histogram the pair of values is needed and it can be provided as `std::make_pair(a,b) >> hist;` or `std::make_tuple(a,b) >> hist;` or `std::array<2, float>({a, b}) >> hist;`. The same applies to filling from containers, i.e. each element needs to be mapped to a pair-like type. The same rule for `std::optional` applies as in case of 1D histograms. The result of fill operation is the input of it. That can be used to code filling of several, differently binned histograms. The only requirement is that histograms are defined in separate lines.
```c++ 
data.map(F(_.energy))  >> HIST1("energy", ";E[GeV]", 100, 0, 20)
                       >> HIST1("energy_zoom", ";E[GeV]", 100, 0, 2);

```



### Filling with weights
To fill 1D histogram either the scalars or pairs of values can be used. In the later case the second value plays the role of weight. Similarly, the 2D histograms and profile plot are filled with pairs or triples. In the later case the last value in the triple is the weight. The same logic applies to 3D histograms. When filling efficiency plot the first value of the pair should be boolean. When the triple of values is used the last one is the weight. As mentioned above, the values in the containers are taken one by one and entered in the histograms. For example:
```c++
  // v1 is a vector of object with getters x(), y(), weight()
  v1.map(F(make_pair(_.x(), _.y()))) >> HIST2(...); // has the same meaning as
  for ( auto el: v1 ) {
    HIST2(...).Fill(el.x(), el.y());
  }
  v1.map(F(make_tuple(_.x(), _.y(), _.weight()))) >> HIST2(...); // has the same meaning as
  for ( auto el: v1 ) {
    HIST2(...).Fill(el.x(), el.y(), el.weigh());
  }
```
In the above example the `HIST2` can be replaced by `PROF1`. If the `x()` would be a boolean access the `EFF1` filling would be implemented identically. 

## Context of the histogram
We often need to make similar histograms for different selection of data. For that the FunRootAna provides context switching helper, `HCONTEXT` taking the `std::string` argument. With this names of histograms in the  scope where the context is defined obtain a common prefix in their name. 
The `HCONTEXT` should be used instead of tweaking histogram name. (This behavior allows certain optimization.) However, above all the `HCONTEXT` allows to structure output logically.

An example below illustrates the use of the context:
```c++
{
    HCONTEXT("HighRes_");
    HIST1("x", ...); // here the histogram name in the output file becomes HighRes_x 
    HIST1(std::string("x"+std::to_string(i)), ...); // this will be runtime error
}
```
The `HCONTEXT` argument can depend on some variables (and so can change) if needed.   
Contexts are nesting, that is:
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
When the name of the context ends with `/` i.e. `A/` histograms `x` defined in this context will be saved in subdirectory `A`  in the output file.


# Additional functionalities
## Configuration of the analysis 
Ach, everyone needs it at some point. So there it is, a simle helper `Conf` class that allows to:
* read the config file of the form: `property=value` (# as first character are considered comments),
* read environmental variables if config file is an empty string (i.e. before running the code you would do `export property=value`),
* allows to check if the property is available with method `has("minx")`,
* and get the property with `get` like this: `conf.get<float>("minx", 0.1)` which means: take the `minx` from the config, convert it to `float` before handing to me, but if missing use the value `0.1`.
* Store the config in the output file in a form of metadata tree, i.e. key-value pairs (both are just strings). Additional information of this form can be saved conveniently in this way:
```c++
config.saveAsMetadata("file.root");
// or with an additional info 
std::map<std::string, std::string> extraInfo;
extraInfo["inputFile"] = "....";
extraInfo["jobStartTime"] = "....";
extraInfo["jobTime"] = "....";

config.saveAsMetadata("file.root", extraInfo);
```

## Diagnostics 
A trivial function `report` can be used to produce a message.
More useful is the `assure` function that will check if the first argument evaluates to `true` and if not will complain & end the execution via exception. 
There are less commonly needed `missing` and `assure_about_equal`.

# Playing with the example
The `examples` subdir contains code `example_analysis.cpp` that can be played with the library.
Input file with the points can be generated by calling: `root generateTree.C`
Attached cmake file should be sufficient to compile this lib, generate the test file and so on. The file contains also an example of how the FunRootAna can be integrated with the ATLAS analysis code.
