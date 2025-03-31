cc_library(
    name = "scorer_guess_value",
    srcs = ["scorer_guess_value.cc"],
    hdrs = ["scorer_guess_value.h"],
    deps = ["//scorer"],
)

cc_test(
    name = "scorer_guess_value_test",
    srcs = ["scorer_guess_value_test.cc"],
    deps = [
        ":scorer_guess_value",
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "main",
    srcs = ["main.cc"],
    data = ["//elfs:simple_small"],
    deps = [
        ":scorer_guess_value",
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
        ":scorer_guess_value",
        "@abseil-cpp//absl/flags:flag",
        "@abseil-cpp//absl/flags:parse",
    ],
)
