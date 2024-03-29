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

#include "ROOTTreeAccess.h"
#include "Conf.h"

#include "LazyFunctionalVector.h"
#include "filling.h"
using namespace lfv;


// access using collated branches
class PointRefVector : public CollatedBranchesContainer<PointRefVector, Point>{
    public:
        using local_fillbr=fillbr<std::vector<float>>;
        PointRefVector( TTree* t, size_t current_tree_element) 
        : CollatedBranchesContainer(0),
        refx(local_fillbr(t, "x", current_tree_element)),
        refy(local_fillbr(t, "y", current_tree_element)),
        refz(local_fillbr(t, "z", current_tree_element)) {
            updateSize(refx.size());
        }
        Point build(size_t el) const final override { return {refx.at(el), refy.at(el), refz.at(el)}; }
    private:
        local_fillbr refx;
        local_fillbr refy;
        local_fillbr refz;
};


class PointsTreeROOTTreeAccess : public ROOTTreeAccess {
    public:
        using ROOTTreeAccess::ROOTTreeAccess;
        std::vector<Point> getPoints() {
            std::vector<Point> result;

           fillbr<std::vector<float>> xBranch( tree(), "x" , current());
           fillbr<std::vector<float>> yBranch( tree(), "y" , current());
           fillbr<std::vector<float>> zBranch( tree(), "z" , current());
           result.reserve( xBranch.data->size() ); // to avoid reallocations
            for ( size_t i = 0; i < xBranch.size(); ++i) {
                result.emplace_back( Point( {xBranch.at(i), yBranch.at(i), zBranch.at(i)} ) );
            }

            return result;
       }
       // more memory efficient approach 
       auto getPointsRefs() {
        return PointRefVector( tree(), current() );
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
        using std::chrono::high_resolution_clock;
        using std::chrono::duration_cast;
        using std::chrono::duration;
        using std::chrono::milliseconds;
        auto t1 = high_resolution_clock::now();

        // a simple loop
        // for (PointsTreeROOTTreeAccess event(t); event; ++event) {

        // or via functional collection interface
        PointsTreeROOTTreeAccess acc(t);
        AccessView<PointsTreeROOTTreeAccess> events(acc); // the tree wrapped in an functional container

        events
        .take(2000) // take only first 1000 events
        .filter( [&](auto event){ return event.current() %2 == 1; }) // every second event (because why not)
        .foreach([&](auto event) {
           // analyse content of the data entry
            std::cerr << ".";
            const int category = event.template get<int>("category");
            category >> HIST1("categories_count", ";category;count of events", 5, -0.5, 4.5); // create & fill the histogram with a plain , (creation done on demand and only once)
            auto category_view = event.template branch_view<int>("category");
            category_view >> HIST1("categories_view_count", ";category;count of events", 5, -0.5, 4.5); // create & fill the histogram with a plain , (creation done on demand and only once)

            std::vector<float> xVector = event.template get<std::vector<float>>("x");
            const auto x_copy = lazy_view(xVector); // get vector of data & wrap it into a functional style container
            // or
            const auto x_view = event.template branch_view<std::vector<float>>("x"); // // obtain directly functional style container directly (avoids data copies)

            x_copy >> HIST1("x_copy", "x[mm]", 100, 1, 2); // fill the histogram with the x coordinates
            if (category == 4) {
                HCONTEXT("cat4_"); // modify the context (histograms name prefixed)
                x_copy >> HIST1("x_copy", "x[mm]", 100, 1, 2); // fill the histogram with the x coordinates
            }
            if (category == 3) {
                HCONTEXT("cat3/"); // modify the context (histograms in subdirectory)
                x_copy >> HIST1("x_copy", "x[mm]", 100, 1, 2); // fill the histogram with the x coordinates
            }

            x_view >> HIST1("x_view", "x[mm]", 100, 1, 2); // fill the histogram with the x coordinates

            std::make_pair(x_view.sum(), x_view.size()) >> HIST2("tot_vs_size", ";sum;size", 20, 0, 150, 20, 0, 150);

            // processing objects
            PointRefVector pointsRefVector = event.getPointsRefs();
            auto points = lazy_view(pointsRefVector);            
            // or
            // auto pointsVector = event.getPoints();
            // auto points = lazy_view(pointsVector);
            points.map( F(_.rho_xy()) ) >> HIST1("rho_xy", ";rho", 100, 0, 5); // to extract the a quantity of interest for filling we do the "map" operation
            points.map(F(_.z)) >> HIST1("z", ";zcoord", 100, 0, 10); // another example
            points.filter(F(_.z > 1.0) ).map( F(_.rho_xy())) >> HIST1("rho", ";rho_{z>1}", 100, 0, 10); // another example
            points.map( F( std::make_pair(_.r(), _.z))) >>  HIST2("xy/r_z", ";x;y", 30, 0, 3, 20, 0, 15); // and 2D hist

        }
        );
        auto t2 = high_resolution_clock::now();
        duration<double, std::milli> spent = t2 - t1;
        report("USED: " + std::to_string(spent.count()) +" milliseconds");
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

    Conf c;    
    c.saveAsMetadata("Output.root", {{"metadata1", "example1"}, 
                                    {"input", "TestTree.root"}});
    report("finished ...");
}