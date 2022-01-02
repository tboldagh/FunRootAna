#include "Testing.h"
#include "TFile.h"
#include "TSystem.h"
#include "HIST.h"
#include "unistd.h"

#include "filling.h"

class HistogrammingClassTest : public HandyHists {
public:
    void test_create() {
        HIST1("h1", ";x;y", 10, 0, 2);
        HIST1V("h2", ";x;y", std::vector<double>({ 0, 0.5, 1, 3, 10, 20 }));
        HIST2("h3", ";x;y", 10, 0, 2, 3, -0.5, 2.5);
        EFF1("e1", ";x;eff", 100, 0, 10);
        EFF1V("e2", ";x;eff", std::vector<double>({ -1, 0, 1 }));
        PROF1("p1", ";x;y", 10, 0, 1);
        PROF1V("p2", ";x;y", std::vector<double>({ 0, 0.5, 1, 3, 10, 20 }));
        {
            HCONTEXT("scope1_");
            HIST1("s1", ";x;y", 2, 0, 2);
            {
                HCONTEXT("scope2_");
                HIST1("s1", ";x;y", 20, 0, 2);
            }
        }
    }
    void test_fill() {
        WeightRAI w(0.5);
        auto h1 = HIST1("fh1", ";x;y", 3, 0, 3);
        h1.fill(0.5);
        h1.fill(0.5);
        h1.fill(1.5, 1.5);
        h1.wfill(2.5); // weighted fill using external & global weight
        VALUE(h1->GetEntries()) EXPECTED (4);
        VALUE(h1->GetBinContent(1)) EXPECTED (2);
        VALUE(h1->GetBinContent(2)) EXPECTED (1.5);
        VALUE(h1->GetBinContent(3)) EXPECTED (0.5);

        auto h2 = HIST2("fh2", ";x;y", 3, 0, 3, 3, 0, 3);
        h2.fill(0.5, 0.5);
        h2.fill(0.5, 0.5);
        h2.fill(1.5, 1.5, 0.2);
        h2.wfill(2.5, 2.5); 
        VALUE(h2->GetEntries()) EXPECTED (4);
        VALUE(h2->GetBinContent(1, 1)) EXPECTED (2);
        VALUE(h2->GetBinContent(2, 2)) EXPECTED (0.2);
        VALUE(h2->GetBinContent(3, 3)) EXPECTED (0.5);
    }

    void test_eager_fill()  {
        auto v1 = wrap(std::vector<float>({-1, -0.2, 0.5, 0.2, 1.5, 0.7}));
        auto h1 = HIST1("eh1", ";x;y", 4, 0, 1);
        v1 >> h1;
        VALUE( h1->GetEntries()) EXPECTED (6);
        VALUE( h1->GetBinContent(0)) EXPECTED (2); // underflows
        VALUE( h1->GetBinContent(1)) EXPECTED (1); // 0 - 0.25

        // generate weight
        auto h2 = HIST1("eh2", ";x;y", 4, 0, 1);
        v1.map(F( std::make_pair(_, 1.5))) >> h2;
        VALUE( h2->GetEntries()) EXPECTED (6);
        VALUE( h2->GetBinContent(1)) EXPECTED (1.5); // 0 - 0.25
    }


    void test_lazy_fill()  {
        auto v1 = lazy(std::vector<float>({-1, -0.2, 0.5, 0.2, 1.5, 0.7}));
        auto h1 = HIST1("lh1", ";x;y", 4, 0, 1);
        v1 >> h1;
        VALUE( h1->GetEntries()) EXPECTED (6);
        VALUE( h1->GetBinContent(0)) EXPECTED (2); // underflows
        VALUE( h1->GetBinContent(1)) EXPECTED (1); // 0 - 0.25

        // generate weight
        auto h2 = HIST1("lh2", ";x;y", 4, 0, 1);
        v1.map(F( std::make_pair(_, 1.5))) >> h2;
        VALUE( h2->GetEntries()) EXPECTED (6);
        VALUE( h2->GetBinContent(1)) EXPECTED (1.5); // 0 - 0.25


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
    VALUE(f->Get("h2")) NOT_EXPECTED(nullptr);
    VALUE(s(f->Get("h1")->ClassName())) EXPECTED("TH1D");

    VALUE(f->Get("h3")) NOT_EXPECTED(nullptr);
    VALUE(s(f->Get("h3")->ClassName())) EXPECTED("TH2D");

    VALUE(f->Get("e1")) NOT_EXPECTED(nullptr);
    VALUE(s(f->Get("e1")->ClassName())) EXPECTED("TEfficiency");

    VALUE(f->Get("e2")) NOT_EXPECTED(nullptr);

    VALUE(f->Get("p1")) NOT_EXPECTED(nullptr);
    VALUE(s(f->Get("p1")->ClassName())) EXPECTED("TProfile");
    VALUE(f->Get("p2")) NOT_EXPECTED(nullptr);

    VALUE(f->Get("scope1_s1")) NOT_EXPECTED(nullptr);
    VALUE(f->Get("scope1_scope2_s1")) NOT_EXPECTED(nullptr);
    VALUE(((TH1D*)(f->Get("scope1_s1")))->GetNbinsX()) EXPECTED(2);
    VALUE(((TH1D*)(f->Get("scope1_scope2_s1")))->GetNbinsX()) EXPECTED(20);

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

void test_lazy_fill() {

}



int main() {
    const int failed = SUITE(test_create)
        + SUITE(test_fill)
        + SUITE(test_eager_fill)
        + SUITE(test_lazy_fill);

    std::cout << (failed == 0 ? "ALL OK" : "FAILURE") << std::endl;
    return failed;
}
