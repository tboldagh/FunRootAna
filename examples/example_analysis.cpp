// Copyright 2022, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)

/*
    This is an example analysis code.
    In real life this code should be split into several files (one "responsibility" in each).
 */

#include <cmath>
struct Point{
    float x;
    float y;
    float z;
    double rho_xy() const { return std::hypot(x, y); }
    double r() const { return std::hypot( rho_xy(), z); }
};

#include <vector>
#include <utility>

#include "Access.h"
#include "LazyFunctionalVector.h"
#include "filling.h"
class PointsTreeAccess : public Access {
    public:
        using Access::Access;
       std::vector<Point> getPoints() {
            std::vector<Point> result;

           /* simple but not efficient, involves copying
           const auto x = get<std::vector<float>>("x");
           const auto y = get<std::vector<float>>("y");
           const auto z = get<std::vector<float>>("z");
           result.reserve(x.size()); // to avoid reallocations
            for ( size_t i = 0; i < x.size(); ++i) {
                result.emplace_back(Point({x[i], y[i], z[i]}));
            }
           */

           /* more efficient apprach */
           fillbr<std::vector<float>> xBranch( tree(), "x" , current());
           fillbr<std::vector<float>> yBranch( tree(), "y" , current());
           fillbr<std::vector<float>> zBranch( tree(), "z" , current());
           result.reserve( xBranch.data->size() ); // to avoid reallocations
            for ( size_t i = 0; i < xBranch.size(); ++i) {
                result.emplace_back( Point( {xBranch.at(i), yBranch.at(i), zBranch.at(i)} ) );
            }

            return result;
       }
};




#include "TTree.h"
#include "TFile.h"


#include "HIST.h"
#include "LazyFunctionalVector.h"


class MyAnalysis : public HandyHists {
public:
    void configure() {} // maybe you want to configure some aspects of the analysis
    void run() {
        auto f = TFile::Open("TestTree.root", "OLD");
        assure(f != nullptr, "Input file");
        TTree* t = (TTree*)f->Get("points");
        assure(t != nullptr, "Tree" );

        // a simple loop
        //for (PointsTreeAccess event(t); event; ++event) 

        // or via functional collection interface
        FunctionalAccess<PointsTreeAccess> events(t); // the tree wrapped in an functional container

        events
        .take(2000) // take only first 1000 events
        .filter( [&](auto event){ return event.current() %2 == 1; }) // every second event (beause why not)
        .foreach([&](auto event) {
            // analyse content of the data entry
            const int category = event.template get<int>("category");
            category >> HIST1("categories_count", ";category;count of events", 5, -0.5, 4.5); // create & fill the histogram with a plain , (creation done on demand and only once)

            const auto x = lazy_own(std::move(event.template get<std::vector<float>>("x"))); // get vector of data & wrap it into a functional style container
            x >> HIST1("x", "x[mm]", 100, 1, 2); // fill the histogram with the x coordinates
            x.map( F(_ - 1) ) >> HIST1("x_shifted", "transformed x;|x-1|", 20, 0, 1); // transform & fill the histogram

            std::make_pair(x.sum(), x.size()) >> HIST2("tot_vs_size", ";sum;size", 20, 0, 150, 20, 0, 150);

            // processing objects
            auto points = lazy_own(std::move(event.getPoints()));
            points.map( F(_.rho_xy()) ) >> HIST1("rho_xy", ";rho", 100, 0, 5); // to extract the a quantity of interest for filling we do the "map" operation
            points.map(F(_.z)) >> HIST1("z", ";zcoord", 100, 0, 10); // another example
            points.filter(F(_.z > 1.0) ).map( F(_.rho_xy())) >> HIST1("rho", ";rho_{z>1}", 100, 0, 10); // another example
            points.map( F( std::make_pair(_.r(), _.z))) >>  HIST2("xy/r_z", ";x;y", 30, 0, 3, 20, 0, 15); // and 2D hist
        });
        f->Close();
    }
};



int main() { 
    report(".. start");
    MyAnalysis a;
    a.configure();
    report(".. configured, crunching the data");
    a.run();
    report(".. done, saving outut ..");
    a.save("Output.root");
    report("finished ...");
}