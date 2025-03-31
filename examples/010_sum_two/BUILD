cc_library(
    name = "scorer_sum_two",
    srcs = ["scorer_sum_two.cc"],
    hdrs = ["scorer_sum_two.h"],
    deps = [
        "//scorer",
        "//util:random",
    ],
)

cc_test(
    name = "scorer_sum_two_test",
    srcs = ["scorer_sum_two_test.cc"],
    deps = [
        ":scorer_sum_two",
        "//util:random_mock",
        "@googletest//:gtest_main",
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
        ":scorer_sum_two",
        "//evolver:evolver_adhoc",
        "//mutator:mutator_composite_random",
        "//mutator:mutator_point_last_instruction",
        "//mutator:mutator_point_random",
        "//mutator:mutator_recombine_plain_elf",
        "//mutator:mutator_recombine_random",
    ],
)
