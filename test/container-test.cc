/*
 Tests of container utilities

 Copyright (c) 2012 - 2016, Victor Zverovich
 All rights reserved.

 For the license information refer to format.h.
 */

#include "fmt/container.h"
#include "gtest/gtest.h"

using fmt::internal::ContainerBuffer;

TEST(ContainerBufferTest, Empty) {
  std::string data;
  ContainerBuffer<std::string> buffer(data);
  EXPECT_EQ(0u, buffer.size());
  EXPECT_EQ(0u, buffer.capacity());
}

TEST(ContainerBufferTest, Reserve) {
  std::string data;
  ContainerBuffer<std::string> buffer(data);
  std::size_t capacity = std::string().capacity() + 10;
  buffer.reserve(capacity);
  EXPECT_EQ(0u, buffer.size());
  EXPECT_EQ(capacity, buffer.capacity());
}

TEST(ContainerBufferTest, Resize) {
  std::string data;
  ContainerBuffer<std::string> buffer(data);
  std::size_t size = std::string().capacity() + 10;
  buffer.resize(size);
  EXPECT_EQ(size, buffer.size());
  EXPECT_EQ(size, buffer.capacity());
}

TEST(ContainerBufferTest, Append) {
  std::string data("Why so");
  const std::string serious(" serious");
  ContainerBuffer<std::string> buffer(data);
  buffer.append(serious.c_str(), serious.c_str() + serious.length());
  EXPECT_EQ("Why so serious", data);
  EXPECT_EQ(data.length(), buffer.size());
}

TEST(BasicContainerWriterTest, String) {
  std::string data;
  fmt::BasicContainerWriter<std::string> out(data);
  out << "The answer is " << 42 << "\n";
  EXPECT_EQ("The answer is 42\n", data);
  EXPECT_EQ(17u, out.size());
}

TEST(BasicContainerWriterTest, WString) {
  std::wstring data;
  fmt::BasicContainerWriter<std::wstring> out(data);
  out << "The answer is " << 42 << "\n";
  EXPECT_EQ(L"The answer is 42\n", data);
  EXPECT_EQ(17u, out.size());
}

TEST(BasicContainerWriterTest, Vector) {
  std::vector<char> data;
  fmt::BasicContainerWriter<std::vector<char> > out(data);
  out << "The answer is " << 42 << "\n";
  EXPECT_EQ(17u, data.size());
  EXPECT_EQ(out.size(), data.size());
}

TEST(BasicContainerWriterTest, StringAppend) {
  std::string data("The");
  fmt::BasicContainerWriter<std::string> out(data);
  EXPECT_EQ(3u, data.size());
  EXPECT_EQ(3u, out.size());
  out << " answer is " << 42 << "\n";
  EXPECT_EQ("The answer is 42\n", data);
  EXPECT_EQ(17u, out.size());
}

TEST(BasicContainerWriterTest, VectorAppend) {
  std::vector<char> data;
  data.push_back('T');
  data.push_back('h');
  data.push_back('e');
  fmt::BasicContainerWriter<std::vector<char> > out(data);
  EXPECT_EQ(3u, data.size());
  EXPECT_EQ(3u, out.size());
  out << " answer is " << 42 << "\n";
  EXPECT_EQ(17u, data.size());
  EXPECT_EQ(17u, out.size());
}
