cc_library(
  name="mutator",
  hdrs=["mutator.h"],
  deps=["//program:program"],
)

cc_library(
  name="mutator_point_random",
  srcs=["mutator_point_random.cc"],
  hdrs=["mutator_point_random.h"],
  deps=[
    ":mutator",
    "//util:random",
  ],
)

cc_test(
  name="mutator_point_random_test",
  srcs=["mutator_point_random_test.cc"],
  deps=[
    ":mutator_point_random",
    "//util:random_mock",
    "@com_google_googletest//:gtest_main",
  ],
  data=["//elfs:simple_small"],
)

cc_library(
  name="mutator_recombine_random",
  srcs=["mutator_recombine_random.cc"],
  hdrs=["mutator_recombine_random.h"],
  deps=[
    ":mutator",
    "//util:random",
  ],
)

cc_test(
  name="mutator_recombine_random_test",
  srcs=["mutator_recombine_random_test.cc"],
  deps=[
    ":mutator_recombine_random",
    "//util:random_mock",
    "@com_google_googletest//:gtest_main",
  ],
  data=["//elfs:simple_small"],
)

