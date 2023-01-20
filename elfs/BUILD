cc_binary(
  name = "simple_small",
  srcs = ["simple_small.c"],
)

sh_test(
  name = "simple_small_test",
  size = "small",
  srcs = ["simple_small_test.sh"],
  data = [":simple_small"],
)