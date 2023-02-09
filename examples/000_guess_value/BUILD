cc_library(
  name="scorer_guess_value",
  srcs=["scorer_guess_value.cc"],
  hdrs=["scorer_guess_value.h"],
  deps=["//scorer:scorer"],
)

cc_test(
  name="scorer_guess_value_test",
  srcs=["scorer_guess_value_test.cc"],
  deps=[
    ":scorer_guess_value",
    "@com_google_googletest//:gtest_main",
  ],
)