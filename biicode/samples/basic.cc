#include "vitaut/cppformat/format.h"

class Date {
  int year_, month_, day_;
 public:
  Date(int year, int month, int day) : year_(year), month_(month), day_(day) {}

  friend std::ostream &operator<<(std::ostream &os, const Date &d) {
    return os << d.year_ << '-' << d.month_ << '-' << d.day_;
  }
};

int main() {
  std::string s = fmt::format("The date is {}", Date(2012, 12, 9));
  fmt::print("Hello, {}!", "world");  // uses Python-like format string syntax
  fmt::printf("\n%s", s); // uses printf format string syntax
}
