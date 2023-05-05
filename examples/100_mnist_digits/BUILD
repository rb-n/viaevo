cc_library(
    name = "scorer_mnist_digits",
    srcs = ["scorer_mnist_digits.cc"],
    hdrs = ["scorer_mnist_digits.h"],
    deps = [
        "//scorer",
        "//util:random",
    ],
)

cc_test(
    name = "scorer_mnist_digits_test",
    srcs = ["scorer_mnist_digits_test.cc"],
    data = [":train_data"],
    deps = [
        ":scorer_mnist_digits",
        "//util:random_mock",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_binary(
    name = "main",
    srcs = ["main.cc"],
    data = [
        ":train_data",
        "//elfs:intermediate_medium",
        "//elfs:simple_medium",
    ],
    deps = [
        ":scorer_mnist_digits",
        "//evolver:evolver_adhoc",
        "//mutator:mutator_composite_random",
        "//mutator:mutator_point_last_instruction",
        "//mutator:mutator_point_random",
        "//mutator:mutator_recombine_plain_elf",
        "//mutator:mutator_recombine_random",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
    ],
)

filegroup(
    name = "train_data",
    srcs = [
        "data/train-images-idx3-ubyte",
        "data/train-labels-idx1-ubyte",
    ],
)

filegroup(
    name = "evolved_elfs",
    srcs = glob(["evolved_elfs/*.elf"]),
)

cc_binary(
    name = "validate",
    srcs = ["validate.cc"],
    data = [
        ":evolved_elfs",
        ":train_data",
    ],
    deps = [
        ":scorer_mnist_digits",
    ],
)
