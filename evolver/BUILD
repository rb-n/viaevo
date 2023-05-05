cc_library(
    name = "evolver_adhoc",
    srcs = ["evolver_adhoc.cc"],
    hdrs = ["evolver_adhoc.h"],
    visibility = [
        "//examples:__subpackages__",
    ],
    deps = [
        "//mutator",
        "//program",
        "//scorer",
        "//util:random",
    ],
)

cc_test(
    name = "evolver_adhoc_test",
    srcs = ["evolver_adhoc_test.cc"],
    data = ["//elfs:simple_small"],
    deps = [
        ":evolver_adhoc",
        "//mutator:mutator_point_random",
        "//program",
        "//scorer:scorer_mock",
        "//util:random_mock",
        "@com_google_googletest//:gtest_main",
    ],
)
