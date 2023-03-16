cc_library(
  name="scorer",
  hdrs=["scorer.h"],
  deps=["//program:program"],
  visibility=[
    "//evolver:__pkg__",
    "//examples:__subpackages__",
  ],
)

cc_library(
  name="scorer_mock",
  srcs=["scorer_mock.cc"],
  hdrs=["scorer_mock.h"],
  deps=[":scorer"],
  visibility=["//evolver:__pkg__"],
)

cc_test(
  name="scorer_mock_test",
  srcs=["scorer_mock_test.cc"],
  deps=[
    ":scorer_mock",
    "@com_google_googletest//:gtest_main",
  ],
)