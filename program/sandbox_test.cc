// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "sandbox.h"

#include <gtest/gtest.h>

#include <sys/resource.h>
#include <sys/time.h>

#include "elf_image.h"

namespace {

// Total CPU time (user + sys) consumed by reaped child processes, in
// microseconds. The Sandbox waits for and reaps its child, so the delta across
// an Execute call is that execution's CPU consumption.
long ChildCpuUsec() {
  struct rusage usage;
  getrusage(RUSAGE_CHILDREN, &usage);
  return (long)usage.ru_utime.tv_sec * 1000000 + usage.ru_utime.tv_usec +
         (long)usage.ru_stime.tv_sec * 1000000 + usage.ru_stime.tv_usec;
}

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

// The CPU budget is the primary bound on runaway evolved code and is now
// configurable (and reported) rather than hardcoded. Because ITIMER_PROF
// bounds CPU rather than wall-clock time, the assertion below holds regardless
// of how loaded the machine is (RECOMMENDATIONS.md 12.7, 13.3).
TEST(SandboxTest, CpuTimeoutIsConfigurableAndEnforced) {
  EXPECT_EQ(viaevo::Sandbox().cpu_timeout_usec(),
            viaevo::Sandbox::kDefaultCpuTimeoutUsec);
  EXPECT_EQ(viaevo::Sandbox().wall_timeout_usec(),
            viaevo::Sandbox::kDefaultWallTimeoutUsec);
  EXPECT_EQ(viaevo::Sandbox::kDefaultCpuTimeoutUsec, 10000)
      << "the default CPU budget is 10 ms";

  viaevo::ElfImage image("elfs/inf_loop");
  viaevo::Sandbox sandbox(/*cpu_timeout_usec=*/5000);
  EXPECT_EQ(sandbox.cpu_timeout_usec(), 5000);

  long before = ChildCpuUsec();
  viaevo::ExecutionResult result = sandbox.Execute(
      image, viaevo::Sandbox::ExecuteMode::kTerminateInEvolvedCode);
  long consumed = ChildCpuUsec() - before;

  EXPECT_EQ(result.last_stop_signal, 27)
      << "a busy loop should trip the CPU-time timeout (SIGPROF)";
  EXPECT_EQ(result.last_term_signal, 9) << "expected SIGKILL";

  // Budget plus headroom for itimer granularity (4 ms at CONFIG_HZ=250) and
  // the loader/libc startup that also counts against the budget. The point is
  // that the flag is honored: this stays far below the 50 ms that used to be
  // spent on every runaway execution.
  EXPECT_LT(consumed, 25000)
      << "inf_loop consumed " << consumed
      << " usec of child CPU under a 5000 usec budget";
}

} // namespace
