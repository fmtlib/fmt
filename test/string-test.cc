/*
 Tests of string utilities

 Copyright (c) 2012 - 2016, Victor Zverovich
 All rights reserved.

 For the license information refer to format.h.
 */

#include "fmt/string.h"
#include "gtest/gtest.h"

using fmt::string_buffer;

TEST(StringBufferTest, Empty) {
  string_buffer buffer;
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
  string_buffer buffer;
  std::size_t capacity = std::string().capacity() + 10;
  buffer.reserve(capacity);
  EXPECT_EQ(0, buffer.size());
  EXPECT_EQ(capacity, buffer.capacity());
  std::string data;
  buffer.move_to(data);
  EXPECT_EQ("", data);
}

TEST(StringBufferTest, Resize) {
  string_buffer buffer;
  std::size_t size = std::string().capacity() + 10;
  buffer.resize(size);
  EXPECT_EQ(size, buffer.size());
  EXPECT_EQ(size, buffer.capacity());
  std::string data;
  buffer.move_to(data);
  EXPECT_EQ(size, data.size());
}

TEST(StringBufferTest, MoveTo) {
  string_buffer buffer;
  std::size_t size = std::string().capacity() + 10;
  buffer.resize(size);
  const char *p = &buffer[0];
  std::string data;
  buffer.move_to(data);
  EXPECT_EQ(p, &data[0]);
  EXPECT_EQ(0, buffer.size());
  EXPECT_EQ(0, buffer.capacity());
}

TEST(StringBufferTest, WString) {
  fmt::wstring_buffer out;
  out.push_back(L'x');
  std::wstring s;
  out.move_to(s);
  EXPECT_EQ(L"x", s);
}

TEST(StringTest, ToString) {
  EXPECT_EQ("42", fmt::to_string(42));
}
