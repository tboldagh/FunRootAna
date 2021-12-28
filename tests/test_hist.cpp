#include "Testing.h"
#include "TFile.h"
#include "TSystem.h"
#include "HIST.h"
#include "unistd.h"

class Test : public HandyHists {
    public:
        void test_create() {
            HIST1("h1", ";x;y", 10, 0, 2);
            HIST1V("h2", ";x;y", std::vector<double>({0, 0.5, 1, 3, 10, 20}));
            HIST2("h3", ";x;y", 10, 0, 2, 3, -0.5, 2.5);
            EFF1("e1", ";x;eff", 100, 0, 10);
            EFF1V("e2", ";x;eff", std::vector<double>({-1, 0, 1}));
            PROF1("p1", ";x;y", 10, 0, 1);            
            PROF1V("p2", ";x;y", std::vector<double>({0, 0.5, 1, 3, 10, 20}));            
            {
                HCONTEXT("scope1_");
                HIST1("s1", ";x;y", 2, 0, 2);
                {
                    HCONTEXT("scope2_");
                    HIST1("s1", ";x;y", 20, 0, 2);
                }
            }
        }


};

void test_create() {
    Test t1;
    t1.test_create();
    std::string fname = std::to_string(getpid())+"_test.root";
    t1.save(fname);

    auto s = [](const char* n){ return std::string(n); };

    TFile* f = TFile::Open(fname.c_str(), "OLD");
    VALUE( f ) NOT_EXPECTED( nullptr );
    VALUE( f->Get("h1")  ) NOT_EXPECTED( nullptr );
    VALUE( f->Get("h2") ) NOT_EXPECTED( nullptr );
    VALUE( s(f->Get("h1")->ClassName()) ) EXPECTED( "TH1D" );

    VALUE( f->Get("h3") ) NOT_EXPECTED( nullptr );
    VALUE( s(f->Get("h3")->ClassName()) ) EXPECTED( "TH2D" );

    VALUE( f->Get("e1") ) NOT_EXPECTED( nullptr );
    VALUE( s(f->Get("e1")->ClassName()) ) EXPECTED( "TEfficiency" );

    VALUE( f->Get("e2") ) NOT_EXPECTED( nullptr );

    VALUE( f->Get("p1") ) NOT_EXPECTED( nullptr );
    VALUE( s(f->Get("p1")->ClassName()) ) EXPECTED( "TProfile" );
    VALUE( f->Get("p2") ) NOT_EXPECTED( nullptr );

    VALUE( f->Get("scope1_s1") ) NOT_EXPECTED( nullptr );
    VALUE( f->Get("scope1_scope2_s1") ) NOT_EXPECTED( nullptr );
    VALUE (((TH1D*)(f->Get("scope1_s1")))->GetNbinsX()) EXPECTED( 2 );
    VALUE (((TH1D*)(f->Get("scope1_scope2_s1")))->GetNbinsX()) EXPECTED( 20 );

    gSystem->Unlink(fname.c_str());
}


int main() {
    const int failed = 
      SUITE(test_create);

    std::cout << ( failed == 0  ? "ALL OK" : "FAILURE" ) << std::endl;
    return failed;
}
