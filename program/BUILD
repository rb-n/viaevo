cc_library(
  name="program",
  srcs=["program.cc"],
  hdrs=["program.h"],
  linkopts=["-lseccomp"],
  visibility=["//mutator:__pkg__"],
)

cc_test(
  name="program_test",
  srcs=["program_test.cc"],
  deps=[
    ":program",
    "@com_google_googletest//:gtest_main",
  ],
  data=["//elfs:simple_small"],
)