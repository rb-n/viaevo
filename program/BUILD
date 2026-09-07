load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "elf_layout",
    srcs = ["elf_layout.cc"],
    hdrs = ["elf_layout.h"],
)

cc_test(
    name = "elf_layout_test",
    size = "small",
    srcs = ["elf_layout_test.cc"],
    data = [
        "//elfs:intermediate_medium",
        "//elfs:intermediate_small",
        "//elfs:simple_medium",
        "//elfs:simple_small",
    ],
    deps = [
        ":elf_layout",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "elf_image",
    srcs = ["elf_image.cc"],
    hdrs = ["elf_image.h"],
    deps = [
        ":elf_layout",
    ],
)

cc_test(
    name = "elf_image_test",
    size = "small",
    srcs = ["elf_image_test.cc"],
    data = [
        "//elfs:simple_small",
    ],
    deps = [
        ":elf_image",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "sandbox",
    srcs = ["sandbox.cc"],
    hdrs = ["sandbox.h"],
    linkopts = ["-lseccomp"],
    deps = [
        ":elf_image",
    ],
)

cc_test(
    name = "sandbox_test",
    size = "small",
    srcs = ["sandbox_test.cc"],
    data = [
        "//elfs:inf_loop",
        "//elfs:simple_small",
    ],
    deps = [
        ":elf_image",
        ":sandbox",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "program",
    srcs = ["program.cc"],
    hdrs = ["program.h"],
    visibility = [
        "//evolver:__pkg__",
        "//mutator:__pkg__",
        "//scorer:__pkg__",
    ],
    deps = [
        ":elf_image",
        ":sandbox",
    ],
)

cc_test(
    name = "program_test",
    size = "small",
    srcs = ["program_test.cc"],
    data = [
        "//elfs:complex_large",
        "//elfs:inf_loop",
        "//elfs:intermediate_medium",
        "//elfs:intermediate_small",
        "//elfs:simple_medium",
        "//elfs:simple_small",
    ],
    deps = [
        ":program",
        "@googletest//:gtest_main",
    ],
)
