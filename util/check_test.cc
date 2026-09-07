// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "check.h"

#include <string>

#include <gtest/gtest.h>

namespace {

TEST(CheckTest, PassingCheckDoesNotTerminate) {
  int value = 42;
  VIAEVO_CHECK(value == 42, "Value should be 42.");
  VIAEVO_CHECK(true, std::string("Never printed."));
  SUCCEED();
}

TEST(CheckTest, FailingCheckTerminatesWithMessage) {
  int value = 41;
  EXPECT_DEATH(VIAEVO_CHECK(value == 42, "Value should be 42."),
               "check_test.cc:[0-9]+: Check failed: value == 42: Value should "
               "be 42.");
}

TEST(CheckTest, MessageCanBeComposedFromRuntimeValues) {
  std::string filename = "missing_data_file";
  EXPECT_DEATH(VIAEVO_CHECK(false, "Failed to open " + filename + "."),
               "Failed to open missing_data_file.");
}

// The point of VIAEVO_CHECK over assert: the message expression must not be
// evaluated unless the check fails (no cost on the success path), and the
// check itself must survive NDEBUG (bazel's -c opt). The latter is covered by
// running this test in both compilation modes.
TEST(CheckTest, MessageIsNotEvaluatedWhenCheckPasses) {
  int evaluations = 0;
  auto message = [&evaluations]() {
    ++evaluations;
    return std::string("evaluated");
  };
  VIAEVO_CHECK(true, message());
  EXPECT_EQ(evaluations, 0);
}

} // namespace
