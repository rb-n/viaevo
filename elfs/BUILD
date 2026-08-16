load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_shell//shell:sh_test.bzl", "sh_test")

cc_binary(
    name = "simple_small",
    srcs = ["simple_small.c"],
    visibility = [
        "//evolver:__pkg__",
        "//examples:__subpackages__",
        "//mutator:__pkg__",
        "//program:__pkg__",
        "//scorer:__pkg__",
    ],
)

sh_test(
    name = "simple_small_test",
    size = "small",
    srcs = ["simple_small_test.sh"],
    data = [":simple_small"],
)

cc_binary(
    name = "simple_medium",
    srcs = ["simple_medium.c"],
    visibility = [
        "//evolver:__pkg__",
        "//examples:__subpackages__",
        "//mutator:__pkg__",
        "//program:__pkg__",
    ],
)

sh_test(
    name = "simple_medium_test",
    size = "small",
    srcs = ["simple_medium_test.sh"],
    data = [":simple_medium"],
)

cc_binary(
    name = "intermediate_small",
    srcs = ["intermediate_small.c"],
    visibility = [
        "//evolver:__pkg__",
        "//examples:__subpackages__",
        "//mutator:__pkg__",
        "//program:__pkg__",
    ],
)

sh_test(
    name = "intermediate_small_test",
    size = "small",
    srcs = ["intermediate_small_test.sh"],
    data = [":intermediate_small"],
)

cc_binary(
    name = "intermediate_medium",
    srcs = ["intermediate_medium.c"],
    visibility = [
        "//evolver:__pkg__",
        "//examples:__subpackages__",
        "//mutator:__pkg__",
        "//program:__pkg__",
    ],
)

sh_test(
    name = "intermediate_medium_test",
    size = "small",
    srcs = ["intermediate_medium_test.sh"],
    data = [":intermediate_medium"],
)

cc_binary(
    name = "complex_large",
    srcs = ["complex_large.c"],
    # The template contains AVX2 vector operations (immintrin.h).
    copts = ["-mavx2"],
    visibility = [
        "//evolver:__pkg__",
        "//examples:__subpackages__",
        "//mutator:__pkg__",
        "//program:__pkg__",
    ],
)

sh_test(
    name = "complex_large_test",
    size = "small",
    srcs = ["complex_large_test.sh"],
    data = [":complex_large"],
)

cc_binary(
    name = "inf_loop",
    srcs = ["inf_loop.c"],
    visibility = ["//program:__pkg__"],
)
