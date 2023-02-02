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

TEST(ProgramTest, GetSetElfCodeSimpleSmall) {
  std::shared_ptr<viaevo::Program> program =
      viaevo::Program::CreateSimpleSmall();

  std::vector<char> elf_code = program->GetElfCode();
  EXPECT_FALSE(elf_code.empty());

  std::vector<char> nops(elf_code.size(), 0x90);
  program->SetElfCode(nops);
  EXPECT_EQ(program->GetElfCode(), nops);

  // Setting with a vector of a smaller size (10) than the code in the ELF.
  std::vector<char> nops_10(10, 0x90);
  EXPECT_DEATH(program->SetElfCode(nops_10),
               "elf code to set has incorrect size");

  // Setting with a vector of a smaller size (10k) than the code in the ELF.
  std::vector<char> nops_10k(10'000, 0x90);
  EXPECT_DEATH(program->SetElfCode(nops_10k),
               "elf code to set has incorrect size");

  // for (int i = 0; i < (int)elf_code.size(); ++i) {
  //   if (i % 16 == 0)
  //     printf("\n");
  //   printf("%3x", (unsigned char)elf_code[i]);
  // }
  // printf("\n");
}

TEST(ProgramTest, GetSetElfInputsSimpleSmall) {
  std::shared_ptr<viaevo::Program> program =
      viaevo::Program::CreateSimpleSmall();

  std::vector<int> elf_inputs = program->GetElfInputs();
  EXPECT_EQ(elf_inputs.size(), 101);
  EXPECT_EQ(elf_inputs[100], 17);
  elf_inputs.resize(5);
  std::vector<int> expected{100, 42, 17, 42, 17};
  EXPECT_EQ(elf_inputs, expected);

  // Setting with a vector of a smaller size (3) than inputs in the ELF.
  std::vector<int> short_vector{7, 7, 7};
  program->SetElfInputs(short_vector);
  elf_inputs = program->GetElfInputs();
  EXPECT_EQ(elf_inputs.size(), 101);
  EXPECT_EQ(elf_inputs[100], 17);
  elf_inputs.resize(5);
  expected = {7, 7, 7, 42, 17};
  EXPECT_EQ(elf_inputs, expected);

  // Setting with a vector of a matching size to inputs in the ELF.
  std::vector<int> zeros_101(101, 0);
  program->SetElfInputs(zeros_101);
  EXPECT_EQ(program->GetElfInputs(), zeros_101);

  // Setting with a vector of a larger size (200) than inputs in the ELF.
  std::vector<int> zeros_200(200, 0);
  EXPECT_DEATH(program->SetElfInputs(zeros_200),
               "elf inputs to set are too large");
}

} // namespace

// int main(int argc, char **argv) {
//   ::testing::InitGoogleTest(&argc, argv);
//   return RUN_ALL_TESTS();
// }