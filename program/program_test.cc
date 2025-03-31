// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "program.h"

#include <gtest/gtest.h>

namespace {

TEST(ProgramTest, CreateExecuteSimpleSmall) {
  std::shared_ptr<viaevo::Program> program =
      viaevo::Program::Create("elfs/simple_small");

  std::vector<int> default_results{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  std::vector<int> changed_results = default_results;
  // results[0] is changed in main() of //elfs:simple_small from 10 to 20.
  changed_results[0] = 20;

  EXPECT_EQ(program->last_syscall(), 9999)
      << "Last syscall not initialized correctly";
  EXPECT_EQ(program->last_rip_offset(), -1)
      << "Last rip offset not initialized correctly";
  EXPECT_EQ(program->last_exit_status(), -9999)
      << "Last exit status not initialized correctly";
  EXPECT_EQ(program->last_term_signal(), -1)
      << "Last term signal not initialized correctly";
  EXPECT_EQ(program->last_stop_signal(), -1)
      << "Last stop signal not initialized correctly";
  EXPECT_TRUE(program->last_results().empty())
      << "last_results not empty before first Execute";

  // Execute the elf, should be terminated when 'attempting' exit.
  int ptrace_stops_count_default = program->Execute();
  EXPECT_EQ(program->last_syscall(), 231)
      << "Last syscall should be exit for 'default' Execute (#1)\nMay want to "
         "add this syscall to allowed seccomp rules in program.cc if this "
         "syscall was newly added to elfs by a compiler/linker.";
  EXPECT_NE(program->last_rip_offset(), -1)
      << "Last rip offset should not be -1 for 'default' Execute (#1)";
  EXPECT_EQ(program->last_exit_status(), -9999)
      << "Last exit status should be invalid for 'default' Execute (#1)";
  EXPECT_EQ(program->last_term_signal(), 9)
      << "Last term signal should be 9 (SIGKILL) for 'default' Execute (#1)";
  EXPECT_EQ(program->last_stop_signal(), 5)
      << "Last stop signal should be 5 (SIGTRAP) for 'default' Execute (#1)";
  // main() in //elfs:simple_small executes therefore the value of results[0] is
  // changed to 20.
  EXPECT_EQ(program->last_results(), changed_results)
      << "Unexpected last results after a 'default' Execute (#1)";

  // Terminate the elf process before entering main (small max_ptrace_stops
  // passed to Execute).
  EXPECT_EQ(program->Execute(5), 5)
      << "observed ptrace stops different from max (#2)";
  EXPECT_NE(program->last_syscall(), 231)
      << "last_syscall should not be exit for 'short' Execute (#2)";
  EXPECT_NE(program->last_rip_offset(), -1)
      << "Last rip offset should not be -1 for 'short' Execute (#2)";
  EXPECT_EQ(program->last_exit_status(), -9999)
      << "Last exit status should be invalid for 'short' Execute (#2)";
  EXPECT_EQ(program->last_term_signal(), 9)
      << "Last term signal should be 9 (SIGKILL) for 'short' Execute (#2)";
  EXPECT_EQ(program->last_stop_signal(), 5)
      << "Last stop signal should be 5 (SIGTRAP) for 'short' Execute (#2)";
  // main() in //elfs:simple_small does not execute therefore the value of
  // results[0] remains 10.
  EXPECT_EQ(program->last_results(), default_results)
      << "Unexpected last results after a 'short' Execute (#2)";

  // Run the elf to completion (large max_ptrace_stops passed to Execute).
  int ptrace_stops_count_full = program->Execute(999'999);
  EXPECT_GT(ptrace_stops_count_full, 0)
      << "Too few ptrace stops for 'full' Execute (#3)";
  EXPECT_EQ(program->last_syscall(), 231)
      << "Last syscall should be exit for 'full' Execute (#3)\nMay want to "
         "add this syscall to allowed seccomp rules in program.cc if this "
         "syscall was newly added to elfs by a compiler/linker.";
  EXPECT_EQ(program->last_exit_status(), 0)
      << "Last exit status should be 0 for 'full' Execute (#3)";
  EXPECT_EQ(program->last_term_signal(), -1)
      << "Last term signal should be -1 (not set) for 'full' Execute (#3)";
  EXPECT_EQ(program->last_stop_signal(), 5)
      << "Last stop signal should be 5 (SIGTRAP) for 'full' Execute (#3)";
  EXPECT_TRUE(program->last_results().empty())
      << "Last results should be empty after 'full' Execute (#3)";

  EXPECT_EQ(ptrace_stops_count_full, ptrace_stops_count_default)
      << "'Full' Execute should have the same amount of ptrace stops compared "
         "to 'default' Execute.";
}

TEST(ProgramTest, CreateExecuteSimpleMedium) {
  std::shared_ptr<viaevo::Program> program =
      viaevo::Program::Create("elfs/simple_medium");

  std::vector<int> default_results{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  std::vector<int> changed_results = default_results;
  // results[0] is changed in main() of //elfs:simple_small from 10 to 20.
  changed_results[0] = 20;

  EXPECT_EQ(program->last_syscall(), 9999)
      << "Last syscall not initialized correctly";
  EXPECT_EQ(program->last_rip_offset(), -1)
      << "Last rip offset not initialized correctly";
  EXPECT_EQ(program->last_exit_status(), -9999)
      << "Last exit status not initialized correctly";
  EXPECT_EQ(program->last_term_signal(), -1)
      << "Last term signal not initialized correctly";
  EXPECT_EQ(program->last_stop_signal(), -1)
      << "Last stop signal not initialized correctly";
  EXPECT_TRUE(program->last_results().empty())
      << "last_results not empty before first Execute";

  // Execute the elf, should be terminated when 'attempting' exit.
  int ptrace_stops_count_default = program->Execute();
  EXPECT_EQ(program->last_syscall(), 231)
      << "Last syscall should be exit for 'default' Execute (#1)\nMay want to "
         "add this syscall to allowed seccomp rules in program.cc if this "
         "syscall was newly added to elfs by a compiler/linker.";
  EXPECT_NE(program->last_rip_offset(), -1)
      << "Last rip offset should not be -1 for 'default' Execute (#1)";
  EXPECT_EQ(program->last_exit_status(), -9999)
      << "Last exit status should be invalid for 'default' Execute (#1)";
  EXPECT_EQ(program->last_term_signal(), 9)
      << "Last term signal should be 9 (SIGKILL) for 'default' Execute (#1)";
  EXPECT_EQ(program->last_stop_signal(), 5)
      << "Last stop signal should be 5 (SIGTRAP) for 'default' Execute (#1)";
  // main() in //elfs:simple_small executes therefore the value of results[0] is
  // changed to 20.
  EXPECT_EQ(program->last_results(), changed_results)
      << "Unexpected last results after a 'default' Execute (#1)";

  // Terminate the elf process before entering main (small max_ptrace_stops
  // passed to Execute).
  EXPECT_EQ(program->Execute(5), 5)
      << "observed ptrace stops different from max (#2)";
  EXPECT_NE(program->last_syscall(), 231)
      << "last_syscall should not be exit for 'short' Execute (#2)";
  EXPECT_NE(program->last_rip_offset(), -1)
      << "Last rip offset should not be -1 for 'short' Execute (#2)";
  EXPECT_EQ(program->last_exit_status(), -9999)
      << "Last exit status should be invalid for 'short' Execute (#2)";
  EXPECT_EQ(program->last_term_signal(), 9)
      << "Last term signal should be 9 (SIGKILL) for 'short' Execute (#2)";
  EXPECT_EQ(program->last_stop_signal(), 5)
      << "Last stop signal should be 5 (SIGTRAP) for 'short' Execute (#2)";
  // main() in //elfs:simple_small does not execute therefore the value of
  // results[0] remains 10.
  EXPECT_EQ(program->last_results(), default_results)
      << "Unexpected last results after a 'short' Execute (#2)";

  // Run the elf to completion (large max_ptrace_stops passed to Execute).
  int ptrace_stops_count_full = program->Execute(999'999);
  EXPECT_GT(ptrace_stops_count_full, 0)
      << "Too few ptrace stops for 'full' Execute (#3)";
  EXPECT_EQ(program->last_syscall(), 231)
      << "Last syscall should be exit for 'full' Execute (#3)\nMay want to "
         "add this syscall to allowed seccomp rules in program.cc if this "
         "syscall was newly added to elfs by a compiler/linker.";
  EXPECT_EQ(program->last_exit_status(), 0)
      << "Last exit status should be 0 for 'full' Execute (#3)";
  EXPECT_EQ(program->last_term_signal(), -1)
      << "Last term signal should be -1 (not set) for 'full' Execute (#3)";
  EXPECT_EQ(program->last_stop_signal(), 5)
      << "Last stop signal should be 5 (SIGTRAP) for 'full' Execute (#3)";
  EXPECT_TRUE(program->last_results().empty())
      << "Last results should be empty after 'full' Execute (#3)";

  EXPECT_EQ(ptrace_stops_count_full, ptrace_stops_count_default)
      << "'Full' Execute should have the same amount of ptrace stops compared "
         "to 'default' Execute.";
}

TEST(ProgramTest, CreateExecuteIntermediateSmall) {
  std::shared_ptr<viaevo::Program> program =
      viaevo::Program::Create("elfs/intermediate_small");

  std::vector<int> default_results{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  std::vector<int> changed_results = default_results;
  // results[0] is changed in main() of //elfs:intermediate_small from 10 to 20.
  changed_results[0] = 20;

  EXPECT_EQ(program->last_syscall(), 9999)
      << "Last syscall not initialized correctly";
  EXPECT_EQ(program->last_rip_offset(), -1)
      << "Last rip offset not initialized correctly";
  EXPECT_EQ(program->last_exit_status(), -9999)
      << "Last exit status not initialized correctly";
  EXPECT_EQ(program->last_term_signal(), -1)
      << "Last term signal not initialized correctly";
  EXPECT_EQ(program->last_stop_signal(), -1)
      << "Last stop signal not initialized correctly";
  EXPECT_TRUE(program->last_results().empty())
      << "last_results not empty before first Execute";

  // Execute the elf, should be terminated when 'attempting' exit.
  int ptrace_stops_count_default = program->Execute();
  EXPECT_EQ(program->last_syscall(), 231)
      << "Last syscall should be exit for 'default' Execute (#1)\nMay want to "
         "add this syscall to allowed seccomp rules in program.cc if this "
         "syscall was newly added to elfs by a compiler/linker.";
  EXPECT_NE(program->last_rip_offset(), -1)
      << "Last rip offset should not be -1 for 'default' Execute (#1)";
  EXPECT_EQ(program->last_exit_status(), -9999)
      << "Last exit status should be invalid for 'default' Execute (#1)";
  EXPECT_EQ(program->last_term_signal(), 9)
      << "Last term signal should be 9 (SIGKILL) for 'default' Execute (#1)";
  EXPECT_EQ(program->last_stop_signal(), 5)
      << "Last stop signal should be 5 (SIGTRAP) for 'default' Execute (#1)";
  // main() in //elfs:simple_small executes therefore the value of results[0] is
  // changed to 20.
  EXPECT_EQ(program->last_results(), changed_results)
      << "Unexpected last results after a 'default' Execute (#1)";

  // Terminate the elf process before entering main (small max_ptrace_stops
  // passed to Execute).
  EXPECT_EQ(program->Execute(5), 5)
      << "observed ptrace stops different from max (#2)";
  EXPECT_NE(program->last_syscall(), 231)
      << "last_syscall should not be exit for 'short' Execute (#2)";
  EXPECT_NE(program->last_rip_offset(), -1)
      << "Last rip offset should not be -1 for 'short' Execute (#2)";
  EXPECT_EQ(program->last_exit_status(), -9999)
      << "Last exit status should be invalid for 'short' Execute (#2)";
  EXPECT_EQ(program->last_term_signal(), 9)
      << "Last term signal should be 9 (SIGKILL) for 'short' Execute (#2)";
  EXPECT_EQ(program->last_stop_signal(), 5)
      << "Last stop signal should be 5 (SIGTRAP) for 'short' Execute (#2)";
  // main() in //elfs:simple_small does not execute therefore the value of
  // results[0] remains 10.
  EXPECT_EQ(program->last_results(), default_results)
      << "Unexpected last results after a 'short' Execute (#2)";

  // Run the elf to completion (large max_ptrace_stops passed to Execute).
  int ptrace_stops_count_full = program->Execute(999'999);
  EXPECT_GT(ptrace_stops_count_full, 0)
      << "Too few ptrace stops for 'full' Execute (#3)";
  EXPECT_EQ(program->last_syscall(), 231)
      << "Last syscall should be exit for 'full' Execute (#3)\nMay want to "
         "add this syscall to allowed seccomp rules in program.cc if this "
         "syscall was newly added to elfs by a compiler/linker.";
  EXPECT_EQ(program->last_exit_status(), 0)
      << "Last exit status should be 0 for 'full' Execute (#3)";
  EXPECT_EQ(program->last_term_signal(), -1)
      << "Last term signal should be -1 (not set) for 'full' Execute (#3)";
  EXPECT_EQ(program->last_stop_signal(), 5)
      << "Last stop signal should be 5 (SIGTRAP) for 'full' Execute (#3)";
  EXPECT_TRUE(program->last_results().empty())
      << "Last results should be empty after 'full' Execute (#3)";

  EXPECT_EQ(ptrace_stops_count_full, ptrace_stops_count_default)
      << "'Full' Execute should have the same amount of ptrace stops compared "
         "to 'default' Execute.";
}

TEST(ProgramTest, CreateExecuteIntermediateMedium) {
  std::shared_ptr<viaevo::Program> program =
      viaevo::Program::Create("elfs/intermediate_medium");

  std::vector<int> default_results{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  std::vector<int> changed_results = default_results;
  // results[0] is changed in main() of //elfs:intermediate_medium from 10
  // to 20.
  changed_results[0] = 20;

  EXPECT_EQ(program->last_syscall(), 9999)
      << "Last syscall not initialized correctly";
  EXPECT_EQ(program->last_rip_offset(), -1)
      << "Last rip offset not initialized correctly";
  EXPECT_EQ(program->last_exit_status(), -9999)
      << "Last exit status not initialized correctly";
  EXPECT_EQ(program->last_term_signal(), -1)
      << "Last term signal not initialized correctly";
  EXPECT_EQ(program->last_stop_signal(), -1)
      << "Last stop signal not initialized correctly";
  EXPECT_TRUE(program->last_results().empty())
      << "last_results not empty before first Execute";

  // Execute the elf, should be terminated when 'attempting' exit.
  int ptrace_stops_count_default = program->Execute();
  EXPECT_EQ(program->last_syscall(), 231)
      << "Last syscall should be exit for 'default' Execute (#1)\nMay want to "
         "add this syscall to allowed seccomp rules in program.cc if this "
         "syscall was newly added to elfs by a compiler/linker.";
  EXPECT_NE(program->last_rip_offset(), -1)
      << "Last rip offset should not be -1 for 'default' Execute (#1)";
  EXPECT_EQ(program->last_exit_status(), -9999)
      << "Last exit status should be invalid for 'default' Execute (#1)";
  EXPECT_EQ(program->last_term_signal(), 9)
      << "Last term signal should be 9 (SIGKILL) for 'default' Execute (#1)";
  EXPECT_EQ(program->last_stop_signal(), 5)
      << "Last stop signal should be 5 (SIGTRAP) for 'default' Execute (#1)";
  // main() in //elfs:simple_small executes therefore the value of results[0] is
  // changed to 20.
  EXPECT_EQ(program->last_results(), changed_results)
      << "Unexpected last results after a 'default' Execute (#1)";

  // Terminate the elf process before entering main (small max_ptrace_stops
  // passed to Execute).
  EXPECT_EQ(program->Execute(5), 5)
      << "observed ptrace stops different from max (#2)";
  EXPECT_NE(program->last_syscall(), 231)
      << "last_syscall should not be exit for 'short' Execute (#2)";
  EXPECT_NE(program->last_rip_offset(), -1)
      << "Last rip offset should not be -1 for 'short' Execute (#2)";
  EXPECT_EQ(program->last_exit_status(), -9999)
      << "Last exit status should be invalid for 'short' Execute (#2)";
  EXPECT_EQ(program->last_term_signal(), 9)
      << "Last term signal should be 9 (SIGKILL) for 'short' Execute (#2)";
  EXPECT_EQ(program->last_stop_signal(), 5)
      << "Last stop signal should be 5 (SIGTRAP) for 'short' Execute (#2)";
  // main() in //elfs:simple_small does not execute therefore the value of
  // results[0] remains 10.
  EXPECT_EQ(program->last_results(), default_results)
      << "Unexpected last results after a 'short' Execute (#2)";

  // Run the elf to completion (large max_ptrace_stops passed to Execute).
  int ptrace_stops_count_full = program->Execute(999'999);
  EXPECT_GT(ptrace_stops_count_full, 0)
      << "Too few ptrace stops for 'full' Execute (#3)";
  EXPECT_EQ(program->last_syscall(), 231)
      << "Last syscall should be exit for 'full' Execute (#3)\nMay want to "
         "add this syscall to allowed seccomp rules in program.cc if this "
         "syscall was newly added to elfs by a compiler/linker.";
  EXPECT_EQ(program->last_exit_status(), 0)
      << "Last exit status should be 0 for 'full' Execute (#3)";
  EXPECT_EQ(program->last_term_signal(), -1)
      << "Last term signal should be -1 (not set) for 'full' Execute (#3)";
  EXPECT_EQ(program->last_stop_signal(), 5)
      << "Last stop signal should be 5 (SIGTRAP) for 'full' Execute (#3)";
  EXPECT_TRUE(program->last_results().empty())
      << "Last results should be empty after 'full' Execute (#3)";

  EXPECT_EQ(ptrace_stops_count_full, ptrace_stops_count_default)
      << "'Full' Execute should have the same amount of ptrace stops compared "
         "to 'default' Execute.";
}

TEST(ProgramTest, GetSetElfCodeSimpleSmall) {
  std::shared_ptr<viaevo::Program> program =
      viaevo::Program::Create("elfs/simple_small");

  std::vector<char> elf_code = program->GetElfCode();
  EXPECT_FALSE(elf_code.empty());

  std::vector<char> nops(elf_code.size(), 0x90);
  program->SetElfCode(nops);
  EXPECT_EQ(program->GetElfCode(), nops);

  // Setting with a vector of a smaller size (10) than the code in the ELF.
  std::vector<char> nops_10(10, 0x90);
  EXPECT_DEATH(program->SetElfCode(nops_10),
               "elf code to set has incorrect size");

  // Setting with a vector of a larger size (10k) than the code in the ELF.
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

TEST(ProgramTest, GetSetElfCodeSimpleMedium) {
  std::shared_ptr<viaevo::Program> program =
      viaevo::Program::Create("elfs/simple_medium");

  std::vector<char> elf_code = program->GetElfCode();
  EXPECT_FALSE(elf_code.empty());

  std::vector<char> nops(elf_code.size(), 0x90);
  program->SetElfCode(nops);
  EXPECT_EQ(program->GetElfCode(), nops);

  // Setting with a vector of a smaller size (10) than the code in the ELF.
  std::vector<char> nops_10(10, 0x90);
  EXPECT_DEATH(program->SetElfCode(nops_10),
               "elf code to set has incorrect size");

  // Setting with a vector of a larger size (10k) than the code in the ELF.
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

TEST(ProgramTest, GetSetElfCodeIntermediateSmall) {
  std::shared_ptr<viaevo::Program> program =
      viaevo::Program::Create("elfs/intermediate_small");

  std::vector<char> elf_code = program->GetElfCode();
  EXPECT_FALSE(elf_code.empty());

  std::vector<char> nops(elf_code.size(), 0x90);
  program->SetElfCode(nops);
  EXPECT_EQ(program->GetElfCode(), nops);

  // Setting with a vector of a smaller size (10) than the code in the ELF.
  std::vector<char> nops_10(10, 0x90);
  EXPECT_DEATH(program->SetElfCode(nops_10),
               "elf code to set has incorrect size");

  // Setting with a vector of a larger size (10k) than the code in the ELF.
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

TEST(ProgramTest, GetSetElfCodeIntermediateMedium) {
  std::shared_ptr<viaevo::Program> program =
      viaevo::Program::Create("elfs/intermediate_medium");

  std::vector<char> elf_code = program->GetElfCode();
  EXPECT_FALSE(elf_code.empty());

  std::vector<char> nops(elf_code.size(), 0x90);
  program->SetElfCode(nops);
  EXPECT_EQ(program->GetElfCode(), nops);

  // Setting with a vector of a smaller size (10) than the code in the ELF.
  std::vector<char> nops_10(10, 0x90);
  EXPECT_DEATH(program->SetElfCode(nops_10),
               "elf code to set has incorrect size");

  // Setting with a vector of a larger size (10k) than the code in the ELF.
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

TEST(ProgramTest, SetElfCodeToAllNopsSimpleSmall) {
  std::shared_ptr<viaevo::Program> program =
      viaevo::Program::Create("elfs/simple_small");

  std::vector<int> default_results{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  std::vector<int> changed_results = default_results;
  // results[0] is changed in main() of //elfs:simple_small from 10 to 20.
  changed_results[0] = 20;

  // Execute the elf, should be terminated when 'attempting' exit.
  int ptrace_stops_count_default = program->Execute();
  EXPECT_EQ(program->last_syscall(), 231)
      << "Last syscall should be exit for 'default' Execute (#1)";
  EXPECT_NE(program->last_rip_offset(), -1)
      << "Last rip offset should not be -1 for 'default' Execute (#1)";
  EXPECT_EQ(program->last_exit_status(), -9999)
      << "Last exit status should be invalid for 'default' Execute (#1)";
  EXPECT_EQ(program->last_term_signal(), 9)
      << "Last term signal should be 9 (SIGKILL) for 'default' Execute (#1)";
  EXPECT_EQ(program->last_stop_signal(), 5)
      << "Last stop signal should be 5 (SIGTRAP) for 'default' Execute (#1)";
  // main() in //elfs:simple_small executes therefore the value of results[0] is
  // changed to 20.
  EXPECT_EQ(program->last_results(), changed_results)
      << "Unexpected last results after a 'default' Execute (#1)";

  std::vector<char> elf_code = program->GetElfCode();
  EXPECT_FALSE(elf_code.empty());

  std::vector<char> nops(elf_code.size(), 0x90);

  program->SetElfCodeToAllNops();
  EXPECT_EQ(program->GetElfCode(), nops);

  int ptrace_stops_count_all_nops = program->Execute();
  EXPECT_NE(program->last_syscall(), 231)
      << "Last syscall should not be exit for 'default' Execute (#2)";
  EXPECT_NE(program->last_rip_offset(), -1)
      << "Last rip offset should not be -1 for 'default' Execute (#2)";
  EXPECT_EQ(program->last_exit_status(), -9999)
      << "Last exit status should be invalid for 'default' Execute (#2)";
  EXPECT_EQ(program->last_term_signal(), 9)
      << "Last term signal should be 9 (SIGKILL) for 'default' Execute (#2)";
  EXPECT_NE(program->last_stop_signal(), 5)
      << "Last stop signal should not be 5 (SIGTRAP) for 'default' Execute "
         "(#2)";
  // main() change to all nops in //elfs:simple_small, therefore the value of
  // results[0] is _not_ changed to 20. Expecting default results here.
  EXPECT_EQ(program->last_results(), default_results)
      << "Unexpected last results after a 'default' Execute (#2)";

  EXPECT_EQ(ptrace_stops_count_default, ptrace_stops_count_all_nops)
      << "Default execute should have the same number of ptrace stops as "
         "execute after changing all instructions to nops.";
}

TEST(ProgramTest, GetSetElfInputsSimpleSmall) {
  std::shared_ptr<viaevo::Program> program =
      viaevo::Program::Create("elfs/simple_small");

  std::vector<int> elf_inputs = program->GetElfInputs();
  EXPECT_EQ(elf_inputs.size(), 101);
  EXPECT_EQ(elf_inputs[100], -1);
  elf_inputs.resize(5);
  std::vector<int> expected{-1, -1, -1, -1, -1};
  EXPECT_EQ(elf_inputs, expected);

  // Setting with a vector of a smaller size (3) than inputs in the ELF.
  std::vector<int> short_vector{7, 7, 7};
  program->SetElfInputs(short_vector);
  elf_inputs = program->GetElfInputs();
  EXPECT_EQ(elf_inputs.size(), 101);
  EXPECT_EQ(elf_inputs[100], -1);
  elf_inputs.resize(5);
  expected = {7, 7, 7, -1, -1};
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

TEST(ProgramTest, GetSetElfInputsSimpleMedium) {
  std::shared_ptr<viaevo::Program> program =
      viaevo::Program::Create("elfs/simple_medium");

  std::vector<int> elf_inputs = program->GetElfInputs();
  EXPECT_EQ(elf_inputs.size(), 201);
  EXPECT_EQ(elf_inputs[100], -1);
  elf_inputs.resize(5);
  std::vector<int> expected{-1, -1, -1, -1, -1};
  EXPECT_EQ(elf_inputs, expected);

  // Setting with a vector of a smaller size (3) than inputs in the ELF.
  std::vector<int> short_vector{7, 7, 7};
  program->SetElfInputs(short_vector);
  elf_inputs = program->GetElfInputs();
  EXPECT_EQ(elf_inputs.size(), 201);
  EXPECT_EQ(elf_inputs[100], -1);
  elf_inputs.resize(5);
  expected = {7, 7, 7, -1, -1};
  EXPECT_EQ(elf_inputs, expected);

  // Setting with a vector of a matching size to inputs in the ELF.
  std::vector<int> zeros_201(201, 0);
  program->SetElfInputs(zeros_201);
  EXPECT_EQ(program->GetElfInputs(), zeros_201);

  // Setting with a vector of a larger size (200) than inputs in the ELF.
  std::vector<int> zeros_2000(2000, 0);
  EXPECT_DEATH(program->SetElfInputs(zeros_2000),
               "elf inputs to set are too large");
}

TEST(ProgramTest, GetSetElfInputsIntermediateSmall) {
  std::shared_ptr<viaevo::Program> program =
      viaevo::Program::Create("elfs/intermediate_small");

  std::vector<int> elf_inputs = program->GetElfInputs();
  EXPECT_EQ(elf_inputs.size(), 101);
  EXPECT_EQ(elf_inputs[100], -1);
  elf_inputs.resize(5);
  std::vector<int> expected{-1, -1, -1, -1, -1};
  EXPECT_EQ(elf_inputs, expected);

  // Setting with a vector of a smaller size (3) than inputs in the ELF.
  std::vector<int> short_vector{7, 7, 7};
  program->SetElfInputs(short_vector);
  elf_inputs = program->GetElfInputs();
  EXPECT_EQ(elf_inputs.size(), 101);
  EXPECT_EQ(elf_inputs[100], -1);
  elf_inputs.resize(5);
  expected = {7, 7, 7, -1, -1};
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

TEST(ProgramTest, GetSetElfInputsIntermediateMedium) {
  std::shared_ptr<viaevo::Program> program =
      viaevo::Program::Create("elfs/intermediate_medium");

  std::vector<int> elf_inputs = program->GetElfInputs();
  EXPECT_EQ(elf_inputs.size(), 201);
  EXPECT_EQ(elf_inputs[100], -1);
  elf_inputs.resize(5);
  std::vector<int> expected{-1, -1, -1, -1, -1};
  EXPECT_EQ(elf_inputs, expected);

  // Setting with a vector of a smaller size (3) than inputs in the ELF.
  std::vector<int> short_vector{7, 7, 7};
  program->SetElfInputs(short_vector);
  elf_inputs = program->GetElfInputs();
  EXPECT_EQ(elf_inputs.size(), 201);
  EXPECT_EQ(elf_inputs[100], -1);
  elf_inputs.resize(5);
  expected = {7, 7, 7, -1, -1};
  EXPECT_EQ(elf_inputs, expected);

  // Setting with a vector of a matching size to inputs in the ELF.
  std::vector<int> zeros_201(201, 0);
  program->SetElfInputs(zeros_201);
  EXPECT_EQ(program->GetElfInputs(), zeros_201);

  // Setting with a vector of a larger size (200) than inputs in the ELF.
  std::vector<int> zeros_2000(2000, 0);
  EXPECT_DEATH(program->SetElfInputs(zeros_2000),
               "elf inputs to set are too large");
}

TEST(ProgramTest, ResetIncrementCurrentScore) {
  std::shared_ptr<viaevo::Program> program =
      viaevo::Program::Create("elfs/simple_small");

  EXPECT_EQ(program->current_score(), 0);

  program->IncrementCurrentScoreBy(10);
  EXPECT_EQ(program->current_score(), 10);
  program->IncrementCurrentScoreBy(5);
  EXPECT_EQ(program->current_score(), 15);

  program->ResetCurrentScore();
  EXPECT_EQ(program->current_score(), 0);
}

TEST(ProgramTest, LastRipOffset) {
  std::shared_ptr<viaevo::Program> program =
      viaevo::Program::Create("elfs/simple_small");

  std::vector<int> expected_results{20, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

  std::vector<char> elf_code = program->GetElfCode();

  // Change all instructions to nops so to make sure the illegal instruction
  // below replaces a nop instruction and indeed causes an illegal instruction
  // (and does not land in the middle of a different instruction instead).
  program->SetElfCodeToAllNops();

  int nop_position = 23;
  EXPECT_EQ(elf_code[nop_position], '\x90')
      << "Instruction at offset " << nop_position
      << " vs main in the simple_small binary is not nop (\\x90). This may be "
         "compiler dependent. The value of the nop_position variable in this "
         "test needs to be changed accordingly.";

  // Create an illegal instruction.
  elf_code[nop_position] = '\x06';
  program->SetElfCode(elf_code);

  program->Execute();
  EXPECT_EQ(program->last_syscall(), -1)
      << "Last syscall should be -1 for 'default' Execute";
  EXPECT_EQ(program->last_rip_offset(), nop_position)
      << "Last rip offset should be " << nop_position
      << "  for 'default' Execute";
  EXPECT_EQ(program->last_exit_status(), -9999)
      << "Last exit status should be invalid (" << -9999
      << ") for 'default' Execute";
  EXPECT_EQ(program->last_term_signal(), 9)
      << "Last term signal should be " << 9
      << " (SIGKILL) for 'default' Execute";
  EXPECT_EQ(program->last_stop_signal(), 4)
      << "Last term signal should be " << 4
      << " (SIGILL) for 'default' Execute";
  EXPECT_EQ(program->last_results(), expected_results)
      << "Unexpected last results should be for 'default' Execute";
}

TEST(ProgramTest, SaveElfSimpleSmall) {
  std::shared_ptr<viaevo::Program> program =
      viaevo::Program::Create("elfs/simple_small");

  std::vector<char> elf_code = program->GetElfCode();
  EXPECT_FALSE(elf_code.empty());

  // Set program's code to all nops.
  std::vector<char> nops(elf_code.size(), 0x90);
  program->SetElfCode(nops);
  EXPECT_EQ(program->GetElfCode(), nops);

  std::string filename = testing::TempDir() + "unit_test_save_elf.elf";

  program->SaveElf(filename.c_str());

  std::shared_ptr<viaevo::Program> saved_program =
      viaevo::Program::Create(filename);

  // Saved program's code should be all nops.
  EXPECT_EQ(saved_program->GetElfCode(), nops);
}

TEST(ProgramTest, ResultsHistorySimpleSmall) {
  std::shared_ptr<viaevo::Program> program =
      viaevo::Program::Create("elfs/simple_small");

  EXPECT_EQ(program->track_results_history(), false)
      << "Results history should not be tracked in Programs by default. (#1)";
  EXPECT_EQ(program->results_history().size(), 0)
      << "Results history should be empty after program creation.";

  std::vector<int> default_results{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  std::vector<int> changed_results = default_results;
  // results[0] is changed in main() of //elfs:simple_small from 10 to 20.
  changed_results[0] = 20;

  program->Execute();

  EXPECT_EQ(program->track_results_history(), false)
      << "Results history should not be tracked in Programs by default. (#2)";
  EXPECT_EQ(program->results_history().size(), 0)
      << "Results history should be empty after program execution(s) by "
         "default. (#2)";

  program->Execute(5);

  EXPECT_EQ(program->track_results_history(), false)
      << "Results history should not be tracked in Programs by default. (#3)";
  EXPECT_EQ(program->results_history().size(), 0)
      << "Results history should be empty after program execution(s) by "
         "default. (#3)";

  program->set_track_results_history(true);

  EXPECT_EQ(program->track_results_history(), true)
      << "Results history should be tracked in Programs after setting it so.";

  program->Execute();

  EXPECT_EQ(program->results_history().size(), 1)
      << "Results history should have one item after first execution since "
         "tracking the results history.";

  std::vector<std::vector<int>> results_history{changed_results};
  EXPECT_EQ(program->results_history(), results_history)
      << "Results history should have changed results (first item 20) after "
         "one full execution with results history tracking on.";

  program->Execute(5);

  EXPECT_EQ(program->results_history().size(), 2)
      << "Results history should have two items after first execution since "
         "tracking the results history.";

  // Add default results to the expected results history after "short" execute
  // (with only 5 ptrace stops allowed) which will keep the value of the first
  // item in last_results_ at 10.
  results_history.push_back(default_results);

  EXPECT_EQ(program->results_history(), results_history)
      << "Results history should have changed results (first item 20) and "
         "default results (first item 10) after one full execution and one "
         "'short' execution with results history tracking on.";

  std::vector<char> elf_code = program->GetElfCode();
  EXPECT_FALSE(elf_code.empty());

  // Set program's code to all nops.
  std::vector<char> nops(elf_code.size(), 0x90);
  program->SetElfCode(nops);

  EXPECT_EQ(program->results_history(), results_history)
      << "Results history should be unaffected after SetElfCode.";

  program->ClearResultsHistory();

  EXPECT_EQ(program->results_history().size(), 0)
      << "Results history should be empty (cleared) after ClearResultsHistory.";
}

TEST(ProgramTest, CreateExecuteInfLoop) {
  std::shared_ptr<viaevo::Program> program =
      viaevo::Program::Create("elfs/inf_loop");

  std::vector<int> default_results{10, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3};

  EXPECT_EQ(program->last_syscall(), 9999)
      << "Last syscall not initialized correctly";
  EXPECT_EQ(program->last_rip_offset(), -1)
      << "Last rip offset not initialized correctly";
  EXPECT_EQ(program->last_exit_status(), -9999)
      << "Last exit status not initialized correctly";
  EXPECT_EQ(program->last_term_signal(), -1)
      << "Last term signal not initialized correctly";
  EXPECT_EQ(program->last_stop_signal(), -1)
      << "Last stop signal not initialized correctly";
  EXPECT_TRUE(program->last_results().empty())
      << "last_results not empty before first Execute";

  program->Execute();
  EXPECT_NE(program->last_syscall(), 231)
      << "Last syscall should not be exit for 'default' Execute (#1)";
  EXPECT_NE(program->last_rip_offset(), -1)
      << "Last rip offset should not be -1 for 'default' Execute (#1)";
  EXPECT_EQ(program->last_exit_status(), -9999)
      << "Last exit status should be invalid for 'default' Execute (#1)";
  EXPECT_EQ(program->last_term_signal(), 9)
      << "Last term signal should be 9 (SIGKILL) for 'default' Execute (#1)";
  EXPECT_EQ(program->last_stop_signal(), 14)
      << "Last stop signal should be 14 (SIGALRM) for 'default' Execute "
         "(#1)\nMay want to add this syscall to allowed seccomp rules in "
         "program.cc if this syscall was newly added to elfs by a "
         "compiler/linker.";
  // main() in //elfs:simple_small executes therefore the value of results[0] is
  // changed to 20.
  EXPECT_EQ(program->last_results(), default_results)
      << "Unexpected last results after a 'default' Execute (#1)";
}

} // namespace
