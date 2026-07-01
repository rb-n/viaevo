load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "evolver_adhoc",
    srcs = ["evolver_adhoc.cc"],
    hdrs = ["evolver_adhoc.h"],
    linkopts = ["-ltbb"],
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
    size = "small",
    srcs = ["evolver_adhoc_test.cc"],
    data = ["//elfs:simple_small"],
    deps = [
        ":evolver_adhoc",
        "//mutator:mutator_point_random",
        "//program",
        "//scorer:scorer_marked_mock",
        "//scorer:scorer_mock",
        "//util:random_mock",
        "@googletest//:gtest_main",
    ],
)
