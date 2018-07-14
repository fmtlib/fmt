// A folly::StringPiece stub.

#include <cstring>

namespace folly {
class StringPiece {
 public:
  explicit StringPiece(const char *s) : data_(s), size_(std::strlen(s)) {}

  const char* data() const { return "foo"; }
  size_t size() const { return 3; }

 private:
  const char *data_;
  size_t size_;
};
}
