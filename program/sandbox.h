// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#ifndef VIAEVO_PROGRAM_SANDBOX_H_
#define VIAEVO_PROGRAM_SANDBOX_H_

#include <sys/types.h> // pid_t

#include <vector>

#include "elf_image.h"

namespace viaevo {

// Outcome of a single sandboxed execution of an ELF. The fields mirror the
// last_* observations Program exposes; a default-constructed ExecutionResult
// carries the "not run yet" sentinels.
struct ExecutionResult {
  static constexpr unsigned long long kInvalidSyscall = 9999;
  static constexpr long long kInvalidRipOffset = -1;
  static constexpr int kInvalidExitStatus = -9999;
  static constexpr int kInvalidSignal = -1;

  // Last syscall, rip offset (vs. main), exit status and signals observed.
  unsigned long long last_syscall = kInvalidSyscall;
  // Offset of the last observed instruction pointer (rip) relative to main;
  // signed so kInvalidRipOffset (-1) is a genuine sentinel and out-of-main
  // offsets are negative (see MutatorPointLastInstruction).
  long long last_rip_offset = kInvalidRipOffset;
  int last_exit_status = kInvalidExitStatus;
  int last_term_signal = kInvalidSignal;
  int last_stop_signal = kInvalidSignal;
  // Results read from the ELF process (empty if it terminated before its
  // results could be read back).
  std::vector<int> last_results;
  // Number of ptrace stops observed during the process lifetime; 3 for a
  // well-behaved kTerminateInEvolvedCode run (post-execveat SIGTRAP, breakpoint
  // SIGTRAP at main, and the terminating stop from the evolved code).
  int ptrace_stops = 0;
};

// Sandbox runs an in-memory ELF (held in an ElfImage) in a locked-down child
// process and reports the outcome. It owns the seccomp policy, the
// vfork/execveat spawn, the int3-breakpoint-driven ptrace monitor loop, and the
// execution timeout - the execution machinery previously interleaved in
// Program.
//
// The timeout is bounded primarily by CPU time (ITIMER_PROF -> SIGPROF): the
// quantity we actually want to cap is how much work an evolved program does,
// which - unlike wall-clock time - is immune to scheduling delays under load
// (see RECOMMENDATIONS.md 12.7). A looser wall-clock timer (ITIMER_REAL ->
// SIGALRM) runs alongside it purely as a backstop for a program that blocks
// without burning CPU (e.g. hung in a blocking syscall during startup). Either
// signal terminating in the evolved code is a timeout; the two are reported
// separately by the evolver.
//
// "Evolvable code reached" is detected via an int3 (0xCC) software breakpoint
// at the entry of main (see Execute and the State enum in the implementation),
// so it is independent of how many system calls the loader/libc make before
// main. A Sandbox holds no per-execution state and is safe to reuse across
// executions (Execute is const).
class Sandbox {
public:
  // How Execute runs the ELF process:
  // - kTerminateInEvolvedCode (default, used during evolution): run to an int3
  //   breakpoint at main, remove it, then terminate the process at the first
  //   system call or signal originating from the (evolved) code in main.
  //   Results are read from the process memory just before termination.
  // - kStopAtMainEntry: terminate the process at the breakpoint at main; main
  //   never executes and the results read reflect the ELF's initialized data.
  // - kRunToCompletion: no breakpoint; trace system calls until the process
  //   exits on its own. Intended for tests/diagnostics only - the code in main
  //   runs unrestrained (still under seccomp and the timeout) and results are
  //   not read.
  enum class ExecuteMode {
    kTerminateInEvolvedCode,
    kStopAtMainEntry,
    kRunToCompletion,
  };

  // cpu_timeout_usec is the CPU-time (ITIMER_PROF -> SIGPROF) budget given to
  // the child - the primary bound on runaway/looping evolved code (default
  // 50 ms). wall_timeout_usec is the wall-clock (ITIMER_REAL -> SIGALRM)
  // backstop for a child that blocks without consuming CPU (default 500 ms); it
  // must comfortably exceed the CPU budget so it only fires when the CPU timer
  // cannot.
  explicit Sandbox(long cpu_timeout_usec = 50000,
                   long wall_timeout_usec = 500000)
      : cpu_timeout_usec_(cpu_timeout_usec),
        wall_timeout_usec_(wall_timeout_usec) {}

  // Executes image according to mode and returns the outcome. Terminates the
  // whole process (via an internal perror + exit) on an unexpected ptrace/OS
  // error, matching the previous behavior; recoverable-error handling is
  // tracked in RECOMMENDATIONS.md 2.1.
  ExecutionResult Execute(const ElfImage &image, ExecuteMode mode) const;

  long cpu_timeout_usec() const { return cpu_timeout_usec_; }
  long wall_timeout_usec() const { return wall_timeout_usec_; }

private:
  // Monitors the vforked ELF process via ptrace stops (breakpoint-driven, see
  // the State enum in the implementation), filling *result. Returns the number
  // of ptrace stops during the lifetime of the ELF process.
  int MonitorElfProcess(pid_t elf_pid, const ElfImage &image, ExecuteMode mode,
                        ExecutionResult *result) const;

  // Runs the ELF in the vfork child: arms the timeout, installs the pre-built
  // seccomp filter, and execs from the in-memory ELF fd. Never returns (execs
  // on success, _exit()s on failure).
  [[noreturn]] void RunElfProcess(const ElfImage &image) const;

  // Reads result->last_results and updates result->last_rip_offset from the
  // stopped ELF process. start_code/start_data are the process's runtime text
  // and data segment start addresses (from /proc/[pid]/stat at the post-exec
  // stop).
  void ReadResultsAndRipOffset(pid_t elf_pid, const ElfImage &image,
                               unsigned long long rip, unsigned long start_code,
                               unsigned long start_data,
                               ExecutionResult *result) const;

  long cpu_timeout_usec_;
  long wall_timeout_usec_;
};

} // namespace viaevo

#endif // VIAEVO_PROGRAM_SANDBOX_H_
