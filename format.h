/*
 Small, safe and fast string formatting library for C++
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

// A buffer with the first SIZE elements stored in the object itself.
template <typename T, std::size_t SIZE>
class Buffer {
 private:
  std::size_t size_;
  std::size_t capacity_;
  T *ptr_;
  T data_[SIZE];

  void Grow(std::size_t size);

  // Do not implement!
  Buffer(const Buffer &);
  void operator=(const Buffer &);

 public:
  Buffer() : size_(0), capacity_(SIZE), ptr_(data_) {}
  ~Buffer() {
    if (ptr_ != data_) delete [] ptr_;
  }

  // Returns the size of this buffer.
  std::size_t size() const { return size_; }

  // Returns the capacity of this buffer.
  std::size_t capacity() const { return capacity_; }

  // Resizes the buffer. If T is a POD type new elements are not initialized.
  void resize(std::size_t new_size) {
    if (new_size > capacity_)
      Grow(new_size);
    size_ = new_size;
  }

  void reserve(std::size_t capacity) {
    if (capacity > capacity_)
      Grow(capacity);
  }

  void clear() { size_ = 0; }

  void push_back(const T &value) {
    if (size_ == capacity_)
      Grow(size_ + 1);
    ptr_[size_++] = value;
  }

  // Appends data to the end of the buffer.
  void append(const T *begin, const T *end);

  T &operator[](std::size_t index) { return ptr_[index]; }
  const T &operator[](std::size_t index) const { return ptr_[index]; }
};

template <typename T, std::size_t SIZE>
void Buffer<T, SIZE>::Grow(std::size_t size) {
  capacity_ = std::max(size, capacity_ + capacity_ / 2);
  T *p = new T[capacity_];
  std::copy(ptr_, ptr_ + size_, p);
  if (ptr_ != data_)
    delete [] ptr_;
  ptr_ = p;
}

template <typename T, std::size_t SIZE>
void Buffer<T, SIZE>::append(const T *begin, const T *end) {
  std::ptrdiff_t num_elements = end - begin;
  if (size_ + num_elements > capacity_)
    Grow(num_elements);
  std::copy(begin, end, ptr_ + size_);
  size_ += num_elements;
}

class FormatError : public std::runtime_error {
 public:
  FormatError(const std::string &message) : std::runtime_error(message) {}
};

class BasicArgFormatter;

// Formatter provides string formatting functionality similar to Python's
// str.format. The output is stored in a memory buffer that grows dynamically.
// Usage:
//
//   Formatter out;
//   out("Current point:\n");
//   out("(-{:+f}, {:+f})") << 3.14 << -3.14;
//
// This will populate the buffer of the out object with the following output:
//
//   Current point:
//   (-3.140000, +3.140000)
//
// The buffer can be accessed using Formatter::data() or Formatter::c_str().
class Formatter {
 private:
  enum { INLINE_BUFFER_SIZE = 500 };
  Buffer<char, INLINE_BUFFER_SIZE> buffer_;  // Output buffer.

  enum Type {
    // Numeric types should go first.
    INT, UINT, LONG, ULONG, DOUBLE, LONG_DOUBLE,
    LAST_NUMERIC_TYPE = LONG_DOUBLE,
    CHAR, STRING, WSTRING, POINTER, CUSTOM
  };

  typedef void (Formatter::*FormatFunc)(const void *arg, int width);

  // A format argument.
  class Arg {
   private:
    // This method is private to disallow formatting of arbitrary pointers.
    // If you want to output a pointer cast it to const void*. Do not implement!
    template <typename T>
    Arg(const T *value);

    // This method is private to disallow formatting of arbitrary pointers.
    // If you want to output a pointer cast it to void*. Do not implement!
    template <typename T>
    Arg(T *value);

    // This method is private to disallow formatting of wide characters.
    // If you want to output a wide character cast it to integer type.
    // Do not implement!
    Arg(wchar_t value);

   public:
    Type type;
    union {
      int int_value;
      unsigned uint_value;
      double double_value;
      long long_value;
      unsigned long ulong_value;
      long double long_double_value;
      const void *pointer_value;
      struct {
        const char *string_value;
        std::size_t size;
      };
      struct {
        const void *custom_value;
        FormatFunc format;
      };
    };
    mutable Formatter **formatter;

    Arg(int value) : type(INT), int_value(value) {}
    Arg(unsigned value) : type(UINT), uint_value(value) {}
    Arg(long value) : type(LONG), long_value(value) {}
    Arg(unsigned long value) : type(ULONG), ulong_value(value) {}
    Arg(double value) : type(DOUBLE), double_value(value) {}
    Arg(long double value) : type(LONG_DOUBLE), long_double_value(value) {}
    Arg(char value) : type(CHAR), int_value(value) {}
    Arg(const char *value) : type(STRING), string_value(value), size(0) {}
    Arg(char *value) : type(STRING), string_value(value), size(0) {}
    Arg(const void *value) : type(POINTER), pointer_value(value) {}
    Arg(void *value) : type(POINTER), pointer_value(value) {}
    Arg(const std::string &value)
    : type(STRING), string_value(value.c_str()), size(value.size()) {}

    template <typename T>
    Arg(const T &value)
    : type(CUSTOM), custom_value(&value),
      format(&Formatter::FormatCustomArg<T>) {}

    ~Arg() {
      // Format is called here to make sure that the argument object
      // referred to is still alive, for example:
      //
      //   Print("{0}") << std::string("test");
      //
      // Here an Arg object refers to a temporary std::string which is
      // destroyed at the end of the statement. Since the string object is
      // constructed before the Arg object, it will be destroyed after,
      // so it will be alive in the Arg's destructor when Format is called.
      // Note that the string object will not necessarily be alive when
      // the destructor of BasicArgFormatter is called.
      if (*formatter) {
        (*formatter)->Format();
        *formatter = 0;
      }
    }
  };

  enum { NUM_INLINE_ARGS = 10 };
  Buffer<const Arg*, NUM_INLINE_ARGS> args_;  // Format arguments.

  const char *format_;  // Format string.

  friend class BasicArgFormatter;

  void Add(const Arg &arg) {
    args_.push_back(&arg);
  }

  // Formats an integer.
  template <typename T>
  void FormatInt(T value, unsigned flags, int width, char type);

  // Formats a floating point number (double or long double).
  template <typename T>
  void FormatDouble(
      T value, unsigned flags, int width, int precision, char type);

  // Formats an argument of a custom type, such as a user-defined class.
  template <typename T>
  void FormatCustomArg(const void *arg, int width);

  void Format();

  // Grows the buffer by n characters and returns a pointer to the newly
  // allocated area.
  char *GrowBuffer(std::size_t n) {
    std::size_t size = buffer_.size();
    buffer_.resize(size + n);
    return &buffer_[size];
  }

 public:
  Formatter() : format_(0) {}

  // Formats a string appending the output to the internal buffer.
  // Arguments are accepted through the returned BasicArgFormatter object
  // using inserter operator<<.
  BasicArgFormatter operator()(const char *format);

  std::size_t size() const { return buffer_.size(); }

  const char *data() const { return &buffer_[0]; }
  const char *c_str() const { return &buffer_[0]; }
};

// Argument formatter. This is a transient object that normally exists
// only as a temporary returned by one of the formatting functions.
// It stores a reference to a formatter and provides operators <<
// that feed arguments to the formatter.
class BasicArgFormatter {
 private:
  friend class Formatter;

 protected:
  mutable Formatter *formatter_;

  BasicArgFormatter(BasicArgFormatter& other)
  : formatter_(other.formatter_) {
    other.formatter_ = 0;
  }

  BasicArgFormatter& operator=(const BasicArgFormatter& other) {
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
  explicit BasicArgFormatter(Formatter &f) : formatter_(&f) {}
  ~BasicArgFormatter();

  BasicArgFormatter &operator<<(const Formatter::Arg &arg) {
    arg.formatter = &formatter_;
    formatter_->Add(arg);
    return *this;
  }

  friend const char *c_str(const BasicArgFormatter &af) {
    return af.FinishFormatting()->c_str();
  }

  friend std::string str(const BasicArgFormatter &af) {
    return af.FinishFormatting()->c_str();
  }
};

template <typename Callback>
class ArgFormatter : public BasicArgFormatter {
 public:
  explicit ArgFormatter(Formatter &f) : BasicArgFormatter(f) {}

  ~ArgFormatter() {
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
  char *out = GrowBuffer(std::max<std::size_t>(width, str.size()));
  std::copy(str.begin(), str.end(), out);
  if (width > str.size())
    std::fill_n(out + str.size(), width - str.size(), ' ');
}

inline BasicArgFormatter Formatter::operator()(const char *format) {
  BasicArgFormatter formatter(*this);
  format_ = format;
  args_.clear();
  return formatter;
}

class FullFormat : public BasicArgFormatter {
 private:
  mutable Formatter formatter_;

  // Do not implement.
  FullFormat& operator=(const FullFormat&);

 public:
  explicit FullFormat(const char *format) : BasicArgFormatter(formatter_) {
    BasicArgFormatter::operator=(formatter_(format));
  }

  FullFormat(FullFormat& other) : BasicArgFormatter(other) {}

  ~FullFormat() {
    FinishFormatting();
  }
};

inline FullFormat Format(const char *format) {
  FullFormat ff(format);
  return ff;
}

class Print : public BasicArgFormatter {
 private:
  Formatter formatter_;

  // Do not implement.
  Print(const Print&);
  Print& operator=(const Print&);

 public:
  explicit Print(const char *format) : BasicArgFormatter(formatter_) {
    BasicArgFormatter::operator=(formatter_(format));
  }

  ~Print() {
    FinishFormatting();
    std::fwrite(formatter_.data(), 1, formatter_.size(), stdout);
  }
};
}

namespace fmt = format;

#endif  // FORMAT_H_
