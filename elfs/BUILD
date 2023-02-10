cc_binary(
  name = "simple_small",
  srcs = ["simple_small.c"],
  visibility=[
    "//program:__pkg__",
    "//mutator:__pkg__",
    "//examples:__subpackages__",
    "//evolver:__pkg__",
  ],
)

sh_test(
  name = "simple_small_test",
  size = "small",
  srcs = ["simple_small_test.sh"],
  data = [":simple_small"],
)