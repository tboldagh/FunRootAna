/*
    This is an example analysis code.
    In real life this code shoudl be split into several files (one "responsibility" in each).
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
#include "EagerFunctionalVector.h"
#include "filling.h"
class PointsTreeAccess : public Access{
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
           fillbr<std::vector<float>> xBr ( m_tree, "x" , m_current);
           fillbr<std::vector<float>> yBr ( m_tree, "y" , m_current);
           fillbr<std::vector<float>> zBr ( m_tree, "z" , m_current);
           result.reserve( xBr.data->size() ); // to avoid reallocations
            for ( size_t i = 0; i < xBr.size(); ++i) {
                result.emplace_back(Point({xBr.at(i), yBr.at(i), zBr.at(i)}));
            }

            return result;
       }
};

#include "TTree.h"
#include "TFile.h"


#include "HIST.h"
#include "EagerFunctionalVector.h"

class MyAnalysis : public HandyHists {
public:
    void configure() {} // maybe you want to configure some aspects of the analysis
    void run() {
        auto f = TFile::Open("TestTree.root", "OLD");
        assure(f != nullptr, "Input file");
        TTree* t = (TTree*)f->Get("points");
        assure(t != nullptr, "Tree" );

        for (PointsTreeAccess event(t); event; ++event) {
            auto category = event.get<int>("category");
            HIST1("categories_count", ";category;count of events", 5, -0.5, 4.5).fill(category); // create & fill the histogram, (creation done on demand and only once)
            const auto x = wrap(event.get<std::vector<float>>("x")); // get vector of data & wrap it into a functional style container
            x >> HIST1("x", "x[mm]", 100, 0, 100); // fill the histogram with the x coordinate
            x >> HIST1("x_wide", "x[mm]", 100, 0, 1000); // fill another histogram with the x coordinate

            HIST2("number_of_x_outliers_above_10_vs_category", ";count;category", 10, 0, 10,  5, -0.5, 4.5).fill(x.filter( F( std::fabs(_) ) ).filter( F(_ > 10) ).size(), category); // a super complicated plot, 2D histogram of number of outlayers vs, category

            auto points = wrap(event.getPoints());
            // let's profit from the fact that we have an object
            points.map( F(_.rho_xy()) ) >> HIST1("rho_xy", ";rho", 100, 0, 100); // to extract the a quantity of interest for filling we do the "map" operation
            points.filter( F(_.r() < 10 && _.z>5) ).map( F( std::make_pair(_.x, _.y)) ) >> HIST2("xy/points_close_to_center_high_z", ";x;y", 20, 0, 20, 20, 0, 20);
            points.filter( F(_.r() < 10 && _.z<5) ).map( F( std::make_pair(_.x, _.y)) ) >> HIST2("xy/points_close_to_center_low_z", ";x;y", 20, 0, 20, 20, 0, 20);
        }
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