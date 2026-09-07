load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_shell//shell:sh_test.bzl", "sh_test")

# The machine code of these template programs is the substrate the evolution
# operates on, not ordinary application code: main is a sled of nops
# interspersed with dummy operations, and inputs/results are globals that main
# deliberately never touches. The flags below pin how that substrate is
# generated so the template ELFs are byte-identical regardless of the
# compilation mode the rest of the project is built with. Without them a
# -c opt build (RECOMMENDATIONS 13.3) silently evolves a different program:
#
#   -O0                    gcc -O2 folds away the dummy assignments that give
#                          the evolution something to repurpose (main shrinks
#                          from 3304 to 2573 bytes).
#   -fPIC                  bazel links the PIC object in fastbuild but the
#                          non-PIC one in opt; PIC global accesses are longer,
#                          which alone changes main by ~700 bytes.
#   -fno-{function,data}-sections
#   -Wl,--no-gc-sections   opt turns both on, and --gc-sections then drops the
#                          inputs global outright (nothing in main references
#                          it by design), so symbol lookup fails at runtime.
#   -U_FORTIFY_SOURCE      opt defines _FORTIFY_SOURCE=1; keep the templates
#                          free of fortification regardless.
TEMPLATE_COPTS = [
    "-O0",
    "-fPIC",
    "-fno-function-sections",
    "-fno-data-sections",
    "-U_FORTIFY_SOURCE",
]

TEMPLATE_LINKOPTS = ["-Wl,--no-gc-sections"]

cc_binary(
    name = "simple_small",
    srcs = ["simple_small.c"],
    copts = TEMPLATE_COPTS,
    linkopts = TEMPLATE_LINKOPTS,
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
    copts = TEMPLATE_COPTS,
    linkopts = TEMPLATE_LINKOPTS,
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
    copts = TEMPLATE_COPTS,
    linkopts = TEMPLATE_LINKOPTS,
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
    copts = TEMPLATE_COPTS,
    linkopts = TEMPLATE_LINKOPTS,
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
    copts = TEMPLATE_COPTS + ["-mavx2"],
    linkopts = TEMPLATE_LINKOPTS,
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
    copts = TEMPLATE_COPTS,
    linkopts = TEMPLATE_LINKOPTS,
    visibility = ["//program:__pkg__"],
)
