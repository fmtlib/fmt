/*
 Tests of string utilities

 Copyright (c) 2012 - 2016, Victor Zverovich
 All rights reserved.

 For the license information refer to format.h.
 */

#include "fmt/string.h"
#include "gtest/gtest.h"

using fmt::internal::StringBuffer;

TEST(StringBufferTest, Empty) {
  StringBuffer<char> buffer;
  EXPECT_EQ(0, buffer.size());
  EXPECT_EQ(0, buffer.capacity());
  std::string data;
  // std::string may have initial capacity.
  std::size_t capacity = data.capacity();
  buffer.move_to(data);
  EXPECT_EQ("", data);
  EXPECT_EQ(capacity, data.capacity());
}

TEST(StringBufferTest, Reserve) {
  StringBuffer<char> buffer;
  std::size_t capacity = std::string().capacity() + 10;
  buffer.reserve(capacity);
  EXPECT_EQ(0, buffer.size());
  EXPECT_EQ(capacity, buffer.capacity());
  std::string data;
  buffer.move_to(data);
  EXPECT_EQ("", data);
}

TEST(StringBufferTest, Resize) {
  StringBuffer<char> buffer;
  std::size_t size = std::string().capacity() + 10;
  buffer.resize(size);
  EXPECT_EQ(size, buffer.size());
  EXPECT_EQ(size, buffer.capacity());
  std::string data;
  buffer.move_to(data);
  EXPECT_EQ(size, data.size());
}

TEST(StringBufferTest, MoveTo) {
  StringBuffer<char> buffer;
  std::size_t size = std::string().capacity() + 10;
  buffer.resize(size);
  const char *p = &buffer[0];
  std::string data;
  buffer.move_to(data);
  EXPECT_EQ(p, &data[0]);
  EXPECT_EQ(0, buffer.size());
  EXPECT_EQ(0, buffer.capacity());
}

TEST(StringWriterTest, MoveTo) {
  fmt::StringWriter out;
  out << "The answer is " << 42 << "\n";
  std::string s;
  out.move_to(s);
  EXPECT_EQ("The answer is 42\n", s);
  EXPECT_EQ(0, out.size());
}

TEST(StringWriterTest, WString) {
  fmt::WStringWriter out;
  out << "The answer is " << 42 << "\n";
  std::wstring s;
  out.move_to(s);
  EXPECT_EQ(L"The answer is 42\n", s);
}

TEST(StringTest, ToString) {
  EXPECT_EQ("42", fmt::to_string(42));
}
