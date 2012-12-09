/*
 Small, safe and fast printf-like formatting library for C++
 Author: Victor Zverovich
 */

#ifndef FORMAT_H_
#define FORMAT_H_

#include <cstddef>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>

namespace format {

class FormatError : public std::runtime_error {
 public:
  FormatError(const std::string &message) : std::runtime_error(message) {}
};

class ArgFormatter;

template <typename Callback>
class ArgFormatterWithCallback;

// A sprintf-like formatter that automatically allocates enough storage to
// fit all the output.
class Formatter {
 private:
  std::vector<char> buffer_;  // Output buffer.

  enum Type {
    // Numeric types should go first.
    INT, UINT, LONG, ULONG, DOUBLE, LONG_DOUBLE,
    LAST_NUMERIC_TYPE = LONG_DOUBLE,
    CHAR, STRING, WSTRING, POINTER, CUSTOM
  };

  typedef void (Formatter::*FormatFunc)(const void *arg, int width);

  // An argument.
  struct Arg {
    Type type;
    union {
      int int_value;
      unsigned uint_value;
      double double_value;
      long long_value;
      unsigned long ulong_value;
      long double long_double_value;
      struct {
        union {
          const char *string_value;
          const wchar_t *wstring_value;
          const void *pointer_value;
        };
        std::size_t size;
      };
      struct {
        const void *custom_value;
        FormatFunc format;
      };
    };

    explicit Arg(int value) : type(INT), int_value(value) {}
    explicit Arg(unsigned value) : type(UINT), uint_value(value) {}
    explicit Arg(long value) : type(LONG), long_value(value) {}
    explicit Arg(unsigned long value) : type(ULONG), ulong_value(value) {}
    explicit Arg(double value) : type(DOUBLE), double_value(value) {}
    explicit Arg(long double value)
    : type(LONG_DOUBLE), long_double_value(value) {}
    explicit Arg(char value) : type(CHAR), int_value(value) {}
    explicit Arg(const char *value, std::size_t size = 0)
    : type(STRING), string_value(value), size(size) {}
    explicit Arg(const wchar_t *value) : type(WSTRING), wstring_value(value) {}
    explicit Arg(const void *value) : type(POINTER), pointer_value(value) {}
    explicit Arg(const void *value, FormatFunc f)
    : type(CUSTOM), custom_value(value), format(f) {}
  };

  std::vector<Arg> args_;

  const char *format_;  // Format string.

  friend class ArgFormatter;

  void Add(const Arg &arg) {
    if (args_.empty())
      args_.reserve(10);
    args_.push_back(arg);
  }

  // Formats an argument of a built-in type, such as "int" or "double".
  template <typename T>
  void FormatBuiltinArg(
      const char *format, const T &arg, int width, int precision);

  // Formats an argument of a custom type, such as a user-defined class.
  template <typename T>
  void FormatCustomArg(const void *arg, int width);

  void Format();

 public:
  Formatter() : format_(0) {}

  ArgFormatter operator()(const char *format);

  template <typename Callback>
  ArgFormatterWithCallback<Callback> FormatWithCallback(const char *format);

  const char *c_str() const { return &buffer_[0]; }
  const char *data() const { return &buffer_[0]; }
  std::size_t size() const { return buffer_.size(); }

  void Swap(Formatter &f) {
    buffer_.swap(f.buffer_);
    args_.swap(f.args_);
  }
};

class ArgFormatter {
 private:
  friend class Formatter;

 protected:
  mutable Formatter *formatter_;

  ArgFormatter(const ArgFormatter& other) : formatter_(other.formatter_) {
    other.formatter_ = 0;
  }

  ArgFormatter& operator=(const ArgFormatter& other) {
    formatter_ = other.formatter_;
    other.formatter_ = 0;
    return *this;
  }

  Formatter *FinishFormatting() const {
    Formatter *f = formatter_;
    if (f) {
      formatter_ = 0;
      f->Format();
    }
    return f;
  }

 public:
  explicit ArgFormatter(Formatter &f) : formatter_(&f) {}
  ~ArgFormatter();

  friend const char *c_str(const ArgFormatter &af) {
    return af.FinishFormatting()->c_str();
  }

  friend std::string str(const ArgFormatter &af) {
    return af.FinishFormatting()->c_str();
  }

  ArgFormatter &operator<<(int value) {
    formatter_->Add(Formatter::Arg(value));
    return *this;
  }

  ArgFormatter &operator<<(unsigned value) {
    formatter_->Add(Formatter::Arg(value));
    return *this;
  }

  ArgFormatter &operator<<(long value) {
    formatter_->Add(Formatter::Arg(value));
    return *this;
  }

  ArgFormatter &operator<<(unsigned long value) {
    formatter_->Add(Formatter::Arg(value));
    return *this;
  }

  ArgFormatter &operator<<(double value) {
    formatter_->Add(Formatter::Arg(value));
    return *this;
  }

  ArgFormatter &operator<<(long double value) {
    formatter_->Add(Formatter::Arg(value));
    return *this;
  }

  ArgFormatter &operator<<(char value) {
    formatter_->Add(Formatter::Arg(value));
    return *this;
  }

  ArgFormatter &operator<<(const char *value) {
    formatter_->Add(Formatter::Arg(value));
    return *this;
  }

  ArgFormatter &operator<<(const wchar_t *value) {
    formatter_->Add(Formatter::Arg(value));
    return *this;
  }

  ArgFormatter &operator<<(const std::string &value) {
    formatter_->Add(Formatter::Arg(value.c_str(), value.size()));
    return *this;
  }

  ArgFormatter &operator<<(const void *value) {
    formatter_->Add(Formatter::Arg(value));
    return *this;
  }

  // This method is not implemented intentionally to disallow output of
  // arbitrary pointers. If you want to output a pointer cast it to void*.
  template <typename T>
  ArgFormatter &operator<<(const T *value);

  template <typename T>
  ArgFormatter &operator<<(T *value) {
    const T *const_value = value;
    return *this << const_value;
  }

  // If T is a pointer type, say "U*", AddPtrConst<T>::Value will be
  // "const U*". This additional const ensures that operator<<(const void *)
  // and not this method is called both for "const void*" and "void*".
  template <typename T>
  ArgFormatter &operator<<(const T &value) {
    formatter_->Add(Formatter::Arg(&value, &Formatter::FormatCustomArg<T>));
    return *this;
  }
};

template <typename Callback>
class ArgFormatterWithCallback : public ArgFormatter {
 public:
  explicit ArgFormatterWithCallback(Formatter &f) : ArgFormatter(f) {}

  ~ArgFormatterWithCallback() {
    if (!formatter_) return;
    Callback callback;
    callback(*formatter_);
  }
};

template <typename T>
void Formatter::FormatCustomArg(const void *arg, int width) {
  const T &value = *static_cast<const T*>(arg);
  std::ostringstream os;
  os << value;
  std::string str(os.str());
  if (width < 0) {
    // Extra char is reserved for terminating '\0'.
    buffer_.reserve(buffer_.size() + str.size() + 1);
    buffer_.insert(buffer_.end(), str.begin(), str.end());
    return;
  }
  FormatBuiltinArg("%-*s", str.c_str(), width, -1);
}

inline ArgFormatter Formatter::operator()(const char *format) {
  format_ = format;
  return ArgFormatter(*this);
}

template <typename Callback>
ArgFormatterWithCallback<Callback>
Formatter::FormatWithCallback(const char *format) {
  format_ = format;
  return ArgFormatterWithCallback<Callback>(*this);
}

class FullFormat : public ArgFormatter {
 private:
  mutable Formatter formatter_;

  // Do not implement.
  FullFormat& operator=(const FullFormat&);

 public:
  explicit FullFormat(const char *format) : ArgFormatter(formatter_) {
    ArgFormatter::operator=(formatter_(format));
  }

  FullFormat(const FullFormat& other) : ArgFormatter(other) {
    formatter_.Swap(other.formatter_);
  }

  ~FullFormat() {
    FinishFormatting();
  }
};

inline FullFormat Format(const char *format) { return FullFormat(format); }

class Print : public ArgFormatter {
 private:
  Formatter formatter_;

  // Do not implement.
  Print(const Print&);
  Print& operator=(const Print&);

 public:
  explicit Print(const char *format) : ArgFormatter(formatter_) {
    ArgFormatter::operator=(formatter_(format));
  }

  ~Print() {
    FinishFormatting();
    std::fwrite(formatter_.data(), 1, formatter_.size(), stdout);
  }
};
}

namespace fmt = format;

#endif  // FORMAT_H_
