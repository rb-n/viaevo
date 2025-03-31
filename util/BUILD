cc_library(
    name = "random",
    srcs = ["random.cc"],
    hdrs = ["random.h"],
    visibility = [
        "//evolver:__pkg__",
        "//examples:__subpackages__",
        "//mutator:__pkg__",
    ],
)

cc_test(
    name = "random_test",
    srcs = ["random_test.cc"],
    deps = [
        ":random",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "random_mock",
    srcs = ["random_mock.cc"],
    hdrs = ["random_mock.h"],
    visibility = [
        "//evolver:__pkg__",
        "//examples:__subpackages__",
        "//mutator:__pkg__",
    ],
    deps = [":random"],
)

cc_test(
    name = "random_mock_test",
    srcs = ["random_mock_test.cc"],
    deps = [
        ":random_mock",
        "@googletest//:gtest_main",
    ],
)
