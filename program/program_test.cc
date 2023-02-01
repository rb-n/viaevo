// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "program.h"

#include <gtest/gtest.h>

namespace {

TEST(ProgramTest, CreateExecuteSimpleSmall) {
  std::shared_ptr<viaevo::Program> program =
      viaevo::Program::CreateSimpleSmall();

  std::vector<int> default_results{10, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3};

  EXPECT_EQ(program->last_syscall(), 9999)
      << "Last syscall not initialized correctly";
  EXPECT_EQ(program->last_exit_status(), -9999)
      << "Last exit status not initialized correctly";
  EXPECT_EQ(program->last_signal(), -1)
      << "Last signal not initialized correctly";
  EXPECT_TRUE(program->last_results().empty())
      << "last_results not empty before first Execute";

  // Execute the elf, should be terminated when 'attempting' exit.
  int ptrace_stops_count_default = program->Execute();
  EXPECT_NE(program->last_syscall(), 231)
      << "Last syscall should not be exit for 'default' Execute (#1)";
  EXPECT_EQ(program->last_exit_status(), -9999)
      << "Last exit status should be invalid for 'default' Execute (#1)";
  EXPECT_EQ(program->last_signal(), 9)
      << "Last signal should be 9 (SIGKILL) for 'default' Execute (#1)";
  EXPECT_EQ(program->last_results(), default_results)
      << "Unexpected last results after a 'default' Execute (#1)";

  // Terminate the elf process before entering main (small max_ptrace_stops
  // passed to Execute).
  EXPECT_EQ(program->Execute(5), 5)
      << "observed ptrace stops different from max (#2)";
  EXPECT_NE(program->last_syscall(), 231)
      << "last_syscall should not be exit for 'short' Execute (#2)";
  EXPECT_EQ(program->last_exit_status(), -9999)
      << "Last exit status should be invalid for 'short' Execute (#2)";
  EXPECT_EQ(program->last_signal(), 9)
      << "Last signal should be 9 (SIGKILL) for 'short' Execute (#2)";
  EXPECT_EQ(program->last_results(), default_results)
      << "Unexpected last results after a 'short' Execute (#2)";

  // Run the elf to completion (large max_ptrace_stops passed to Execute).
  int ptrace_stops_count_full = program->Execute(999'999);
  EXPECT_GT(ptrace_stops_count_full, 0)
      << "Too few ptrace stops for 'full' Execute (#3)";
  EXPECT_EQ(program->last_syscall(), 231)
      << "last_syscall should be exit for 'full' Execute (#3)";
  EXPECT_EQ(program->last_exit_status(), 0)
      << "Last exit status should be 0 for 'full' Execute (#3)";
  EXPECT_EQ(program->last_signal(), 5)
      << "Last signal should be 5 (SIGTRAP) for 'full' Execute (#3)";
  EXPECT_TRUE(program->last_results().empty())
      << "Last results should be empty after 'full' Execute (#3)";

  EXPECT_EQ(ptrace_stops_count_full, ptrace_stops_count_default + 1)
      << "'Full' Execute should have one more ptrace stop compared to "
         "'default' Execute.";
}

TEST(ProgramTest, GetElfCodeSimpleSmall) {
  std::shared_ptr<viaevo::Program> program =
      viaevo::Program::CreateSimpleSmall();

  std::vector<char> elf_code = program->GetElfCode();
  EXPECT_FALSE(elf_code.empty());

  // for (int i = 0; i < (int)elf_code.size(); ++i) {
  //   if (i % 16 == 0)
  //     printf("\n");
  //   printf("%3x", (unsigned char)elf_code[i]);
  // }
  // printf("\n");
}

} // namespace

// int main(int argc, char **argv) {
//   ::testing::InitGoogleTest(&argc, argv);
//   return RUN_ALL_TESTS();
// }