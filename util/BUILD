cc_library(
  name="random",
  srcs=["random.cc"],
  hdrs=["random.h"],
  visibility=[
    "//mutator:__pkg__",
    "//evolver:__pkg__",
    "//examples:__subpackages__",
  ],
)

cc_test(
  name="random_test",
  srcs=["random_test.cc"],
  deps=[
    ":random",
    "@com_google_googletest//:gtest_main",
  ],
)

cc_library(
  name="random_mock",
  srcs=["random_mock.cc"],
  hdrs=["random_mock.h"],
  deps=[":random"],
  visibility=["//mutator:__pkg__"],
)

cc_test(
  name="random_mock_test",
  srcs=["random_mock_test.cc"],
  deps=[
    ":random_mock",
    "@com_google_googletest//:gtest_main",
  ],
)