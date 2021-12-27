#include <cmath>
#include <iostream>
#include <stdexcept>
namespace {
  template <typename T>
  bool cmp_eq( T a, T b ) {  return a == b; }
  template<>
  bool cmp_eq<float>( float a, float b ) { return std::abs(a - b) < 1.e-4; }
  template<>
  bool cmp_eq<double>( double a, double b ) { return std::abs(a - b) < 1.e-6; }
}

// helper for comparisons, taken from ATLAS experiment at the LHC codebase
template <typename T>
class TestedValue {
public:
  TestedValue(T v, std::string&& f, int l)
    : m_value(v),
    m_file(std::move(f)),
    m_line(l) {}
  void EXPECTED(const T& e) {
    if (not cmp_eq(e, m_value)) {
      std::cerr << m_file << ":" << m_line << ": error: Test failed, "
        << "expected: " << e << " obtained: " << m_value << "\n";
      throw std::runtime_error("test failed");
    }
  }
  void NOT_EXPECTED(const T& e) {
    if (cmp_eq(e, m_value)) {
      std::cerr << m_file << ":" << m_line << ": error: Test failed, "
        << "NOT expected: " << e << " obtained: " << m_value << "\n";
      throw std::runtime_error("test failed");
    }
  }
private:
  T m_value;
  std::string m_file;
  int m_line;
};

#define VALUE( TESTED ) TestedValue<decltype(TESTED)>(TESTED, __FILE__, __LINE__).

constexpr int ALLOK = 0;
constexpr int FAILED = 1;

template<typename F>
int _SUITE( F f, const char* name ) {
    try { 
        f(); 
    } catch ( const std::exception& e) {       
      std::cout << "... " << name << " FAILED\n";
        return FAILED;
    }
    std::cout << "... " << name << " OK\n";
    return ALLOK;
}
#define SUITE( f )  _SUITE(f, #f)