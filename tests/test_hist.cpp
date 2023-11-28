// Copyright 2022, Tomasz Bold
// https://github.com/tboldagh/FunRootAna
// Distributed under the MIT License
// (See accompanying file LICENSE file)

#include "Testing.h"
#include "TFile.h"
#include "TSystem.h"
#include "HIST.h"
#include "unistd.h"

#include "filling.h"

using namespace lfv;
class HistogrammingClassTest : public HandyHists {
public:
    void test_create() {
        HIST1("h1", ";x;y", 10, 0, 2);
        HIST1V("h1v", ";x;y", std::vector<double>({ 0, 0.5, 1, 3, 10, 20 }));

        HIST2("h2", ";x;y", 10, 0, 2, 3, -0.5, 2.5);
        HIST2V("h2v", ";x;y;z", std::vector<double>({ 0, 0.5, 1, 3, 10, 20 }), std::vector<double>({ -100, 0, 200, 1000 }));

        HIST3("h3", ";x;y", 10, 0, 2, 3, -0.5, 2.5, 12, 0, 1.2);
        HIST3V("h3v", ";x;y;z", std::vector<double>({ 0, 0.5, 1, 3, 10, 20 }), std::vector<double>({ -100, 0, 200, 1000 }), std::vector<double>({ 0.1, 0.2, 0.3, 0.4, 1.0 }));

        EFF1("e1", ";x;eff", 100, 0, 10).Fill(true, 5);

        EFF1V("e1v", ";x;eff", std::vector<double>({ -1, 0, 1, 5, 50 })).Fill(true, 7);
        EFF2V("e2v", ";x;y;eff", std::vector<double>({ -1, 0, 1 }), std::vector<double>({ 0, 5, 15, 50, 500, 5000 })).Fill(true, 0.5, 70);

        PROF1("p1", ";x;y", 10, 0, 1);
        PROF1V("p2", ";x;y", std::vector<double>({ 0, 0.5, 1, 3, 10, 20 }));
        PROF2("p3", ";x;y;z", 10, 0, 10, 20, -2, 2);
        PROF2V("p4", ";x;y;z", std::vector<double>({ 0, 0.5, 1, 3, 10, 20 }), std::vector<double>({ -100, 0, 200, 1000 }));

        {
            HCONTEXT("scope1_");
            HIST1("s1", ";x;y", 2, 0, 2);
            {
                HCONTEXT("scope2_");
                HIST1("s1", ";x;y", 20, 0, 2);
            }
        }

        GRAPH("data1", ";x;y");
        {
            HCONTEXT("dir/");
            GRAPH("data2", ";x;y");
        }
    }
    void test_fill() {
        auto h1 = HIST1("fh1", ";x;y", 3, 0, 3);
        h1.Fill(0.5);
        h1.Fill(0.5);
        h1.Fill(1.5, 1.5);
        h1.Fill(2.5, 0.5);
        // fills using operators
        0.5 >> h1;
        1 >> h1;
        // weighted fills using operators
        std::make_pair(0.2, 7) >> h1;

        VALUE(h1.GetEntries()) EXPECTED (7);
        VALUE(h1.GetBinContent(1)) EXPECTED (10);
        VALUE(h1.GetBinContent(2)) EXPECTED (2.5);
        VALUE(h1.GetBinContent(3)) EXPECTED (0.5);

        auto h2 = HIST2("fh2", ";x;y", 3, 0, 3, 3, 0, 3);
        h2.Fill(0.5, 0.5);
        h2.Fill(0.5, 0.5);
        h2.Fill(1.5, 1.5, 0.2);
        make_triple(2.5, 2.5, 0.5) >> h2;
        // fills using operators
        std::make_pair(0.5, 1.5) >> h2;
        make_triple(2.5, 1.5, 2.7) >> h2;
        std::make_tuple(2.5, 1.5, 0.1) >> h2;

        VALUE(h2.GetEntries()) EXPECTED (7);
        VALUE(h2.GetBinContent(1, 1)) EXPECTED (2);
        VALUE(h2.GetBinContent(1, 2)) EXPECTED (1);
        VALUE(h2.GetBinContent(2, 2)) EXPECTED (0.2);
        VALUE(h2.GetBinContent(3, 3)) EXPECTED (0.5);
        VALUE(h2.GetBinContent(3, 2)) EXPECTED (2.8); // sum of weights 2.7 +0.1


        auto h3 = HIST3("fh3", ";x;y;z", 3, 0, 3, 3, 0, 3, 4, -1, 1);
        h3.Fill(0.5, 0.5, 0.51);
        make_triple(1.5, 0.1, -0.9) >> h3;
        std::make_tuple(1.5, 0.1, -0.9) >> h3;
        std::make_tuple(1.5, 0.1, -0.9, 3.0) >> h3; // weighted fill

        VALUE( h3.GetEntries() ) EXPECTED ( 4 );
        VALUE( h3.GetBinContent(1, 1, 4)) EXPECTED (1);
        VALUE( h3.GetBinContent(2, 1, 1)) EXPECTED (5); //


        auto ef = EFF1("eff", "", 2, 0, 2);
        std::make_pair(true, 0.3) >> ef;
        std::make_pair(false, 0.5) >> ef;
        std::make_pair(false, 1.5) >> ef;
        make_triple(true, 0.3, 1.5) >> ef; // filling with the weight
        std::make_tuple(true, 1.3, 0.5) >> ef; // filling with the weight

        VALUE(ef.GetPassedHistogram()->GetBinContent(1)) EXPECTED (2.5);
        VALUE(ef.GetTotalHistogram()->GetBinContent(1)) EXPECTED (3.5);
        VALUE(ef.GetPassedHistogram()->GetBinContent(2)) EXPECTED (0.5);
        VALUE(ef.GetTotalHistogram()->GetBinContent(2)) EXPECTED (1.5);


        auto ef2 = EFF2("eff1", "", 2, 0, 2, 2, 0, 10);
        make_triple(true, 0.3, 5.5) >> ef2;
        make_triple(false, 1.3, 5.5) >> ef2;
        VALUE(ef2.GetPassedHistogram()->GetBinContent(1,1)) EXPECTED (0);
        VALUE(ef2.GetTotalHistogram()->GetBinContent(1,1)) EXPECTED (0);
        VALUE(ef2.GetPassedHistogram()->GetBinContent(1,2)) EXPECTED (1);
        VALUE(ef2.GetTotalHistogram()->GetBinContent(1,2)) EXPECTED (1);
        VALUE(ef2.GetPassedHistogram()->GetBinContent(2,2)) EXPECTED (0);
        VALUE(ef2.GetTotalHistogram()->GetBinContent(2,2)) EXPECTED (1);

        auto p = PROF1("prof", "", 2, 0, 2);
        std::make_pair(0.2, 0.3) >> p;
        make_triple(1.1, 0.3, 1.5) >> p;
    }
    template<typename V>
    void test_vec_fill( const V& vec) {
        auto h1 = HIST1("eh1", ";x;y", 4, 0, 1);
        vec >> h1
            >> HIST1("eh1fine", ";x;y", 40, 0, 1);
        VALUE( h1.GetEntries()) EXPECTED (6);
        VALUE( h1.GetBinContent(0)) EXPECTED (2); // underflows
        VALUE( h1.GetBinContent(1)) EXPECTED (1); // 0 - 0.25

        // generate weight
        auto h2 = HIST1("eh2", ";x;y", 4, 0, 1);
        vec.map(F( std::make_pair(_, 1.5))) >> h2;
        VALUE( h2.GetEntries()) EXPECTED (6);
        VALUE( h2.GetBinContent(1)) EXPECTED (1.5); // 0 - 0.25

        auto h3 = HIST2("eh3", ";x;y", 4, 0, 1, 4, 0, 1);
        vec.map(F( std::make_pair(_, 1.5))) >> h3;
        vec.map(F( make_triple(_, 1.5, 0.5))) >> h3;

        auto ef = EFF1("eff", "", 2, 0, 2);
        vec.map(F( std::make_pair(_>1, 1.5))) >> ef;
        vec.map(F( make_triple(_>1, 1.5, 0.5))) >> ef;

        auto p = PROF1("prof", "", 2, 0, 2);
        vec.map(F( std::make_pair(_, 1.5))) >> p;
        vec.map(F( make_triple(_, 1.5, 0.5))) >> p;
    }

    void test_eager_fill()  {
        auto v1 = wrap(std::vector<float>({-1, -0.2, 0.5, 0.2, 1.5, 0.7}));
        test_vec_fill(v1);

    }


    void test_lazy_fill()  {
        std::vector<float> data({-1, -0.2, 0.5, 0.2, 1.5, 0.7});
        auto v1 = lazy_view(data);
        test_vec_fill(v1);
    }

    void test_option_fill() {
        auto o = HIST1("o", ";x;y", 10, 0, 2);
        std::optional<double> (3) >> o;
        std::optional<double> (6) >> o;
        std::optional<double> () >> o 
                                 >> HIST1("o2", ";x;y", 100, 0, 2)
                                 >> HIST1("o3", ";x;y", 100, 0, 0.2); // multiple fills
        VALUE( o.GetEntries()) EXPECTED(2);
    }    

    void test_joins_fill() {
        std::vector<float> data1({0.1, 0.2, 0.1});
        std::vector<float> data2({3, 2, 1, 4});
        auto v1 = lazy_view(data1);
        auto v2 = lazy_view(data2);

        auto fromcartesian = HIST1("fromcartesian", "", 5, 0, 5);
        v1.cartesian(v2).map( F(_.first+_.second) ) >> fromcartesian;
        VALUE(fromcartesian.GetEntries()) EXPECTED( v1.size()*v2.size());

        auto fromzip = HIST1("fromzip", "", 5, 0, 5);
        v1.zip(v2).map( F(_.first + _.second)) >> fromzip;
        VALUE( (unsigned)fromzip.GetEntries() ) EXPECTED( 3 ); // shortest of content in data1 & data2

        auto fromchain = HIST1("fromchain", "", 5, 0, 5);
        v1.chain(v2) >> fromchain;
        VALUE( (unsigned)fromchain.GetEntries() ) EXPECTED( v1.size() + v2.size() );

        auto fromgroup = HIST1("fromgroup", "", 5, 0, 5);
        v1.group(2,1).map( F(_.sum()) )  >> fromgroup;
        VALUE( (unsigned)fromgroup.GetEntries() ) EXPECTED( v1.size() - 1 );
    }

    void test_graphs() {
        std::vector<std::pair<float, double>> data1({{0.1, 0.2},{0.2, 0.1}});
        auto g1 = GRAPH("data1", ";x;y");
        lazy_view(data1) >> g1;
        VALUE(g1.GetN()) EXPECTED (2);

        std::vector<std::tuple<float, double, double>> data2({{0.1, 0.2, 8},{0.2, 0.1, 0.7}});
        auto g2 = GRAPH2("data2", ";x;y");
        lazy_view(data2) >> g2;
        VALUE(g2.GetN()) EXPECTED (2);
    }
};

void test_create() {
    HistogrammingClassTest t1;
    t1.test_create();
    std::string fname = std::to_string(getpid()) + "_test.root";
    t1.save(fname);

    auto s = [](const char* n) { return std::string(n); };

    TFile* f = TFile::Open(fname.c_str(), "OLD");
    VALUE(f) NOT_EXPECTED(nullptr);
    VALUE(f->Get("h1")) NOT_EXPECTED(nullptr);
    VALUE(s(f->Get("h1")->ClassName())) EXPECTED("TH1D");
    VALUE(f->Get("h1v")) NOT_EXPECTED(nullptr);
    VALUE(s(f->Get("h1v")->ClassName())) EXPECTED("TH1D");
    VALUE(((TH1D*)f->Get("h1v"))->GetXaxis()->GetNbins()) EXPECTED(5);


    VALUE(f->Get("h2")) NOT_EXPECTED(nullptr);
    VALUE(s(f->Get("h2")->ClassName())) EXPECTED("TH2D");

    VALUE(f->Get("h2v")) NOT_EXPECTED(nullptr);
    VALUE(s(f->Get("h2v")->ClassName())) EXPECTED("TH2D");
    VALUE(((TH2D*)f->Get("h2v"))->GetXaxis()->GetNbins()) EXPECTED(5);
    VALUE(((TH2D*)f->Get("h2v"))->GetYaxis()->GetNbins()) EXPECTED(3);


    VALUE(f->Get("h3")) NOT_EXPECTED(nullptr);
    VALUE(s(f->Get("h3")->ClassName())) EXPECTED("TH3D");

    VALUE(f->Get("h3v")) NOT_EXPECTED(nullptr);
    VALUE(s(f->Get("h3v")->ClassName())) EXPECTED("TH3D");
    VALUE(((TH2D*)f->Get("h3v"))->GetXaxis()->GetNbins()) EXPECTED(5);
    VALUE(((TH2D*)f->Get("h3v"))->GetYaxis()->GetNbins()) EXPECTED(3);
    VALUE(((TH2D*)f->Get("h3v"))->GetZaxis()->GetNbins()) EXPECTED(4);


    VALUE(f->Get("e1")) NOT_EXPECTED(nullptr);
    VALUE(s(f->Get("e1")->ClassName())) EXPECTED("TEfficiency");

    VALUE(f->Get("e1v")) NOT_EXPECTED(nullptr);
    VALUE(s(f->Get("e1v")->ClassName())) EXPECTED("TEfficiency");
    VALUE(((TEfficiency*)f->Get("e1v"))->GetTotalHistogram()->GetXaxis()->GetNbins()) EXPECTED(4);

    VALUE(f->Get("e2v")) NOT_EXPECTED(nullptr);
    VALUE(s(f->Get("e2v")->ClassName())) EXPECTED("TEfficiency");
    VALUE(((TEfficiency*)f->Get("e2v"))->GetTotalHistogram()->GetXaxis()->GetNbins()) EXPECTED(2);
    VALUE(((TEfficiency*)f->Get("e2v"))->GetTotalHistogram()->GetYaxis()->GetNbins()) EXPECTED(5);

    VALUE(f->Get("p1")) NOT_EXPECTED(nullptr);
    VALUE(s(f->Get("p1")->ClassName())) EXPECTED("TProfile");
    VALUE(f->Get("p2")) NOT_EXPECTED(nullptr);

    VALUE(s(f->Get("p3")->ClassName())) EXPECTED("TProfile2D");
    VALUE(f->Get("p4")) NOT_EXPECTED(nullptr);

    VALUE(f->Get("scope1_s1")) NOT_EXPECTED(nullptr);
    VALUE(f->Get("scope1_scope2_s1")) NOT_EXPECTED(nullptr);
    VALUE(((TH1D*)(f->Get("scope1_s1")))->GetNbinsX()) EXPECTED(2);
    VALUE(((TH1D*)(f->Get("scope1_scope2_s1")))->GetNbinsX()) EXPECTED(20);

    VALUE(f->Get("data1")) NOT_EXPECTED(nullptr);
    VALUE(f->Get("data1")->ClassName()) NOT_EXPECTED("TGraph");
    VALUE(f->Get("dir/data2")) NOT_EXPECTED(nullptr);
    VALUE(f->Get("dir/data2")->ClassName()) NOT_EXPECTED("TGraph");

    gSystem->Unlink(fname.c_str());
}



void test_fill() {
     HistogrammingClassTest t1;
     t1.test_fill();
}

void test_eager_fill() {
     HistogrammingClassTest t1;
     t1.test_eager_fill();
}

void test_option_fill() {
     HistogrammingClassTest t1;
     t1.test_option_fill();
}


void test_joins_fill() {
     HistogrammingClassTest t1;
     t1.test_joins_fill();
}

void test_graph() {
    HistogrammingClassTest t1;
    t1.test_graphs();
}


int main() {
    const int failed = SUITE(test_create)
        + SUITE(test_fill)
        + SUITE(test_eager_fill)
        + SUITE(test_option_fill)
        + SUITE(test_joins_fill)
        + SUITE(test_graph);

    std::cout << (failed == 0 ? "ALL OK" : "FAILURE") << std::endl;
    return failed;
}
