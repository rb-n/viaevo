// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "sandbox.h"

#include <gtest/gtest.h>

#include "elf_image.h"

namespace {

// Sandbox is exercised directly here (without Program) to confirm the execution
// machinery is usable and testable on its own. The observed signal values (5 =
// SIGTRAP, 9 = SIGKILL) match the expectations in program_test.cc.

TEST(SandboxTest, TerminateInEvolvedCode) {
  viaevo::ElfImage image("elfs/simple_small");
  viaevo::Sandbox sandbox;

  viaevo::ExecutionResult result = sandbox.Execute(
      image, viaevo::Sandbox::ExecuteMode::kTerminateInEvolvedCode);

  // Post-execveat SIGTRAP, breakpoint SIGTRAP at main, terminating stop.
  EXPECT_EQ(result.ptrace_stops, 3);
  EXPECT_EQ(result.last_syscall, 231u) << "expected exit as the last syscall";
  EXPECT_EQ(result.last_stop_signal, 5) << "expected SIGTRAP";
  EXPECT_EQ(result.last_term_signal, 9) << "expected SIGKILL";
  EXPECT_NE(result.last_rip_offset, -1);

  // main() ran: results[0] is changed from -1 to 20, the rest stay -1.
  std::vector<int> expected{20, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  EXPECT_EQ(result.last_results, expected);
}

TEST(SandboxTest, StopAtMainEntry) {
  viaevo::ElfImage image("elfs/simple_small");
  viaevo::Sandbox sandbox;

  viaevo::ExecutionResult result =
      sandbox.Execute(image, viaevo::Sandbox::ExecuteMode::kStopAtMainEntry);

  EXPECT_EQ(result.ptrace_stops, 2);
  EXPECT_EQ(result.last_rip_offset, 0) << "stopped at main's entry";
  // main() did not run: results hold the ELF's initialized data (all -1).
  std::vector<int> expected{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  EXPECT_EQ(result.last_results, expected);
}

TEST(SandboxTest, RunToCompletion) {
  viaevo::ElfImage image("elfs/simple_small");
  viaevo::Sandbox sandbox;

  viaevo::ExecutionResult result =
      sandbox.Execute(image, viaevo::Sandbox::ExecuteMode::kRunToCompletion);

  EXPECT_GT(result.ptrace_stops, 3);
  EXPECT_EQ(result.last_exit_status, 0);
  EXPECT_EQ(result.last_term_signal, -1) << "not terminated by a signal";
  // Results are not read in run-to-completion mode.
  EXPECT_TRUE(result.last_results.empty());
}

TEST(SandboxTest, FreshResultHasSentinels) {
  // A default-constructed ExecutionResult carries the "not run yet" sentinels
  // Program relies on for its pre-Execute state.
  viaevo::ExecutionResult result;
  EXPECT_EQ(result.last_syscall, 9999u);
  EXPECT_EQ(result.last_rip_offset, -1);
  EXPECT_EQ(result.last_exit_status, -9999);
  EXPECT_EQ(result.last_term_signal, -1);
  EXPECT_EQ(result.last_stop_signal, -1);
  EXPECT_EQ(result.ptrace_stops, 0);
  EXPECT_TRUE(result.last_results.empty());
}

} // namespace
