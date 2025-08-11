cc_library(
    name = "scorer",
    hdrs = ["scorer.h"],
    visibility = [
        "//evolver:__pkg__",
        "//examples:__subpackages__",
    ],
    deps = ["//program"],
)

cc_library(
    name = "scorer_mock",
    srcs = ["scorer_mock.cc"],
    hdrs = ["scorer_mock.h"],
    visibility = ["//evolver:__pkg__"],
    deps = [":scorer"],
)

cc_test(
    name = "scorer_mock_test",
    srcs = ["scorer_mock_test.cc"],
    deps = [
        ":scorer_mock",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "scorer_marked_mock",
    srcs = ["scorer_marked_mock.cc"],
    hdrs = ["scorer_marked_mock.h"],
    visibility = ["//evolver:__pkg__"],
    deps = [":scorer"],
)

cc_test(
    name = "scorer_marked_mock_test",
    srcs = ["scorer_marked_mock_test.cc"],
    data = ["//elfs:simple_small"],
    deps = [
        ":scorer_marked_mock",
        "@googletest//:gtest_main",
    ],
)
