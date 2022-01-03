// helper for comparisons, taken from ATLAS experiment at the LHC codebase
// same author
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace {
  template <typename T>
  bool cmp_eq( T a, T b ) {  return a == b; }
  template<>
  [[maybe_unused]] bool cmp_eq<float>( float a, float b ) { return std::abs(a - b) < 1.e-4; }
  template<>
  [[maybe_unused]] bool cmp_eq<double>( double a, double b ) { return std::abs(a - b) < 1.e-6; }
}


class test_failure : public std::runtime_error {
  public:
    test_failure() : std::runtime_error("test failed") {}
};
template <typename T>
class TestedValue {
public:
  using value_type = typename std::remove_reference<typename std::remove_cv<T>::type>::type;
  TestedValue(const value_type& v, std::string&& f, int l)
    : m_value(v),
    m_file(std::move(f)),
    m_line(l) {}
  void EXPECTED(const value_type& e) {
    if (not cmp_eq(e, m_value)) {
      std::cerr << m_file << ":" << m_line << ": error: Test failed, "
        << " obtained: " << m_value << " expected: " << e << "\n";
      throw test_failure();
    }
  }
  void NOT_EXPECTED(const value_type& e) {
    if (cmp_eq(e, m_value)) {
      std::cerr << m_file << ":" << m_line << ": error: Test failed, "
        << " obtained: " << m_value << " NOT expected: " << e << "\n";
      throw test_failure();
    }
  }
private:
  value_type m_value;
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
    } catch ( const test_failure& e) {       
      std::cout << "... " << name << " FAILED\n";
        return FAILED;
    }
    std::cout << "... " << name << " OK\n";
    return ALLOK;
}
#define SUITE( f )  _SUITE(f, #f)