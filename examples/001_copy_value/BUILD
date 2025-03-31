cc_library(
    name = "scorer_copy_value",
    srcs = ["scorer_copy_value.cc"],
    hdrs = ["scorer_copy_value.h"],
    deps = [
        "//scorer",
        "//util:random",
    ],
)

cc_test(
    name = "scorer_copy_value_test",
    srcs = ["scorer_copy_value_test.cc"],
    deps = [
        ":scorer_copy_value",
        "//util:random_mock",
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "main",
    srcs = ["main.cc"],
    data = ["//elfs:simple_small"],
    deps = [
        ":scorer_copy_value",
        "//evolver:evolver_adhoc",
        "//mutator:mutator_composite_random",
        "//mutator:mutator_point_last_instruction",
        "//mutator:mutator_point_random",
        "//mutator:mutator_recombine_random",
        "@abseil-cpp//absl/flags:flag",
        "@abseil-cpp//absl/flags:parse",
    ],
)

filegroup(
    name = "evolved_elfs",
    srcs = glob(
        ["evolved_elfs/*.elf"],
        allow_empty = True,
    ),
)

cc_binary(
    name = "validate",
    srcs = ["validate.cc"],
    data = [":evolved_elfs"],
    deps = [
        ":scorer_copy_value",
        "@abseil-cpp//absl/flags:flag",
        "@abseil-cpp//absl/flags:parse",
    ],
)
