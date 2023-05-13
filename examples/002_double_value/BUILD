cc_library(
    name = "scorer_double_value",
    srcs = ["scorer_double_value.cc"],
    hdrs = ["scorer_double_value.h"],
    deps = [
        "//scorer",
        "//util:random",
    ],
)

cc_test(
    name = "scorer_double_value_test",
    srcs = ["scorer_double_value_test.cc"],
    deps = [
        ":scorer_double_value",
        "//util:random_mock",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_binary(
    name = "main",
    srcs = ["main.cc"],
    data = [
        "//elfs:intermediate_small",
        "//elfs:simple_small",
    ],
    deps = [
        ":scorer_double_value",
        "//evolver:evolver_adhoc",
        "//mutator:mutator_composite_random",
        "//mutator:mutator_point_last_instruction",
        "//mutator:mutator_point_random",
        "//mutator:mutator_recombine_random",
    ],
)
