cc_library(
  name="random",
  srcs=["random.cc"],
  hdrs=["random.h"],
)

cc_test(
  name="random_test",
  srcs=["random_test.cc"],
  deps=[
    ":random",
    "@com_google_googletest//:gtest_main",
  ],
)