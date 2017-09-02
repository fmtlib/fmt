/*
 std::ostream support tests

 Copyright (c) 2012-2016, Victor Zverovich
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fmt/ostream.h"

#include <sstream>
#include "gmock/gmock.h"
#include "gtest-extra.h"
#include "util.h"

using fmt::format;
using fmt::FormatError;

std::ostream &operator<<(std::ostream &os, const Date &d) {
  os << d.year() << '-' << d.month() << '-' << d.day();
  return os;
}

std::wostream &operator<<(std::wostream &os, const Date &d) {
  os << d.year() << L'-' << d.month() << L'-' << d.day();
  return os;
}

enum TestEnum {};
std::ostream &operator<<(std::ostream &os, TestEnum) {
  return os << "TestEnum";
}

enum TestEnum2 {A};

TEST(OStreamTest, Enum) {
  EXPECT_FALSE(fmt::internal::ConvertToInt<TestEnum>::value);
  EXPECT_EQ("TestEnum", fmt::format("{}", TestEnum()));
  EXPECT_EQ("0", fmt::format("{}", A));
}

struct TestArgFormatter : fmt::BasicArgFormatter<TestArgFormatter, char> {
  TestArgFormatter(fmt::BasicFormatter<char, TestArgFormatter> &f,
                   fmt::FormatSpec &s, const char *fmt)
    : fmt::BasicArgFormatter<TestArgFormatter, char>(f, s, fmt) {}
};

TEST(OStreamTest, CustomArg) {
  fmt::MemoryWriter writer;
  typedef fmt::BasicFormatter<char, TestArgFormatter> Formatter;
  Formatter formatter(fmt::ArgList(), writer);
  fmt::FormatSpec spec;
  TestArgFormatter af(formatter, spec, "}");
  af.visit(fmt::internal::MakeArg<Formatter>(TestEnum()));
  EXPECT_EQ("TestEnum", writer.str());
}

TEST(OStreamTest, Format) {
  EXPECT_EQ("a string", format("{0}", TestString("a string")));
  std::string s = format("The date is {0}", Date(2012, 12, 9));
  EXPECT_EQ("The date is 2012-12-9", s);
  Date date(2012, 12, 9);
  EXPECT_EQ(L"The date is 2012-12-9",
            format(L"The date is {0}", Date(2012, 12, 9)));
}

TEST(OStreamTest, FormatSpecs) {
  EXPECT_EQ("def  ", format("{0:<5}", TestString("def")));
  EXPECT_EQ("  def", format("{0:>5}", TestString("def")));
  EXPECT_THROW_MSG(format("{0:=5}", TestString("def")),
      FormatError, "format specifier '=' requires numeric argument");
  EXPECT_EQ(" def ", format("{0:^5}", TestString("def")));
  EXPECT_EQ("def**", format("{0:*<5}", TestString("def")));
  EXPECT_THROW_MSG(format("{0:+}", TestString()),
      FormatError, "format specifier '+' requires numeric argument");
  EXPECT_THROW_MSG(format("{0:-}", TestString()),
      FormatError, "format specifier '-' requires numeric argument");
  EXPECT_THROW_MSG(format("{0: }", TestString()),
      FormatError, "format specifier ' ' requires numeric argument");
  EXPECT_THROW_MSG(format("{0:#}", TestString()),
      FormatError, "format specifier '#' requires numeric argument");
  EXPECT_THROW_MSG(format("{0:05}", TestString()),
      FormatError, "format specifier '0' requires numeric argument");
  EXPECT_EQ("test         ", format("{0:13}", TestString("test")));
  EXPECT_EQ("test         ", format("{0:{1}}", TestString("test"), 13));
  EXPECT_EQ("te", format("{0:.2}", TestString("test")));
  EXPECT_EQ("te", format("{0:.{1}}", TestString("test"), 2));
}

struct EmptyTest {};
std::ostream &operator<<(std::ostream &os, EmptyTest) {
  return os << "";
}

TEST(OStreamTest, EmptyCustomOutput) {
  EXPECT_EQ("", fmt::format("{}", EmptyTest()));
}

TEST(OStreamTest, Print) {
  std::ostringstream os;
  fmt::print(os, "Don't {}!", "panic");
  EXPECT_EQ("Don't panic!", os.str());
}

TEST(OStreamTest, WriteToOStream) {
  std::ostringstream os;
  fmt::MemoryWriter w;
  w << "foo";
  fmt::internal::write(os, w);
  EXPECT_EQ("foo", os.str());
}

TEST(OStreamTest, WriteToOStreamMaxSize) {
  std::size_t max_size = std::numeric_limits<std::size_t>::max();
  std::streamsize max_streamsize = std::numeric_limits<std::streamsize>::max();
  if (max_size <= fmt::internal::to_unsigned(max_streamsize))
    return;

  class TestWriter : public fmt::BasicWriter<char> {
   private:
    struct TestBuffer : fmt::Buffer<char> {
      explicit TestBuffer(std::size_t size) { size_ = size; }
      void grow(std::size_t) {}
    } buffer_;
   public:
    explicit TestWriter(std::size_t size)
      : fmt::BasicWriter<char>(buffer_), buffer_(size) {}
  } w(max_size);

  struct MockStreamBuf : std::streambuf {
    MOCK_METHOD2(xsputn, std::streamsize (const void *s, std::streamsize n));
    std::streamsize xsputn(const char *s, std::streamsize n) {
      const void *v = s;
      return xsputn(v, n);
    }
  } buffer;

  struct TestOStream : std::ostream {
    explicit TestOStream(MockStreamBuf &buffer) : std::ostream(&buffer) {}
  } os(buffer);

  testing::InSequence sequence;
  const char *data = 0;
  std::size_t size = max_size;
  do {
    typedef fmt::internal::MakeUnsigned<std::streamsize>::Type UStreamSize;
    UStreamSize n = std::min<UStreamSize>(
          size, fmt::internal::to_unsigned(max_streamsize));
    EXPECT_CALL(buffer, xsputn(data, static_cast<std::streamsize>(n)))
        .WillOnce(testing::Return(max_streamsize));
    data += n;
    size -= static_cast<std::size_t>(n);
  } while (size != 0);
  fmt::internal::write(os, w);
}

struct ConvertibleToInt {
  template <typename ValueType>
  operator ValueType() const {
    return 0;
  }

  friend std::ostream &operator<<(std::ostream &o, ConvertibleToInt) {
    return o << "foo";
  }
};

TEST(FormatTest, FormatConvertibleToInt) {
  EXPECT_EQ("foo", fmt::format("{}", ConvertibleToInt()));
}
