// ... (existing tests)
#ifdef __STDCPP_FLOAT16_T__
TEST(StdTest, ComplexFloat16) {
  std::complex<std::float16_t> c(1.0f16, 2.0f16);
  EXPECT_EQ(fmt::format("{}", c), "(1, 2)");
}

TEST(StdTest, Float16) {
  std::float16_t f = 1.5f16;
  EXPECT_EQ(fmt::format("{}", f), "1.5");
}
#endif