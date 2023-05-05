cc_library(
    name = "program",
    srcs = ["program.cc"],
    hdrs = ["program.h"],
    linkopts = ["-lseccomp"],
    visibility = [
        "//evolver:__pkg__",
        "//mutator:__pkg__",
        "//scorer:__pkg__",
    ],
)

cc_test(
    name = "program_test",
    srcs = ["program_test.cc"],
    data = [
        "//elfs:inf_loop",
        "//elfs:intermediate_medium",
        "//elfs:intermediate_small",
        "//elfs:simple_medium",
        "//elfs:simple_small",
    ],
    deps = [
        ":program",
        "@com_google_googletest//:gtest_main",
    ],
)
