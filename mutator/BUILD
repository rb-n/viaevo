cc_library(
    name = "mutator",
    hdrs = ["mutator.h"],
    visibility = ["//evolver:__pkg__"],
    deps = ["//program"],
)

cc_library(
    name = "mutator_point_random",
    srcs = ["mutator_point_random.cc"],
    hdrs = ["mutator_point_random.h"],
    visibility = [
        "//evolver:__pkg__",
        "//examples:__subpackages__",
    ],
    deps = [
        ":mutator",
        "//util:random",
    ],
)

cc_test(
    name = "mutator_point_random_test",
    srcs = ["mutator_point_random_test.cc"],
    data = ["//elfs:simple_small"],
    deps = [
        ":mutator_point_random",
        "//util:random_mock",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "mutator_point_last_instruction",
    srcs = ["mutator_point_last_instruction.cc"],
    hdrs = ["mutator_point_last_instruction.h"],
    visibility = [
        "//examples:__subpackages__",
    ],
    deps = [
        ":mutator_point_random",
    ],
)

cc_test(
    name = "mutator_point_last_instruction_test",
    srcs = ["mutator_point_last_instruction_test.cc"],
    data = ["//elfs:simple_small"],
    deps = [
        ":mutator_point_last_instruction",
        "//util:random_mock",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "mutator_recombine_random",
    srcs = ["mutator_recombine_random.cc"],
    hdrs = ["mutator_recombine_random.h"],
    visibility = [
        "//examples:__subpackages__",
    ],
    deps = [
        ":mutator",
        "//util:random",
    ],
)

cc_test(
    name = "mutator_recombine_random_test",
    srcs = ["mutator_recombine_random_test.cc"],
    data = ["//elfs:simple_small"],
    deps = [
        ":mutator_recombine_random",
        "//util:random_mock",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "mutator_recombine_plain_elf",
    srcs = ["mutator_recombine_plain_elf.cc"],
    hdrs = ["mutator_recombine_plain_elf.h"],
    visibility = [
        "//examples:__subpackages__",
    ],
    deps = [
        ":mutator",
        "//util:random",
    ],
)

cc_test(
    name = "mutator_recombine_plain_elf_test",
    srcs = ["mutator_recombine_plain_elf.cc"],
    data = [
        "//elfs:intermediate_small",
        "//elfs:simple_small",
    ],
    deps = [
        ":mutator_recombine_plain_elf",
        "//util:random_mock",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "mutator_composite",
    srcs = ["mutator_composite.cc"],
    hdrs = ["mutator_composite.h"],
    deps = [":mutator"],
)

cc_library(
    name = "mutator_composite_random",
    srcs = ["mutator_composite_random.cc"],
    hdrs = ["mutator_composite_random.h"],
    visibility = [
        "//examples:__subpackages__",
    ],
    deps = [
        ":mutator_composite",
        "//util:random",
    ],
)

cc_test(
    name = "mutator_composite_random_test",
    srcs = ["mutator_composite_random_test.cc"],
    data = ["//elfs:simple_small"],
    deps = [
        ":mutator_composite_random",
        ":mutator_point_random",
        ":mutator_recombine_random",
        "//util:random_mock",
        "@com_google_googletest//:gtest_main",
    ],
)
