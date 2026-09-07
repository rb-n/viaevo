// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#ifndef VIAEVO_PROGRAM_PROGRAM_H_
#define VIAEVO_PROGRAM_PROGRAM_H_

#include <memory>
#include <string>
#include <vector>

#include "elf_image.h"
#include "sandbox.h"

namespace viaevo {

// Program couples an in-memory ELF (ElfImage) with the Sandbox that executes
// it. The ELF's evolvable code (main) and inputs are accessed and modified via
// the delegating accessors below; Execute runs the ELF in the Sandbox and
// caches the outcome for the last_* accessors.
//
// The class is not intended for subclassing other than the mock classes for
// unit testing that default-construct a Program and set the protected last_*
// state directly (see the example scorer tests). Instances for real ELFs
// should be created via the factory method Create.
class Program {
public:
  // Re-exported so callers can keep writing Program::ExecuteMode::... (the
  // modes are defined on and implemented by Sandbox).
  using ExecuteMode = Sandbox::ExecuteMode;

  // Default constructor exists for the unit-test mocks that subclass Program
  // and populate the protected last_* members directly (the ElfImage is left
  // uninitialized and Execute is never called on such instances).
  // TODO (RECOMMENDATIONS.md 2.2): give the scorers an interface to mock so
  // this subclass-and-poke pattern (and this ctor) can go away.
  Program() {}
  // cpu_timeout_usec / wall_timeout_usec are forwarded to the Sandbox that
  // executes this Program (see Sandbox for what each bounds).
  Program(const char *filename,
          long cpu_timeout_usec = Sandbox::kDefaultCpuTimeoutUsec,
          long wall_timeout_usec = Sandbox::kDefaultWallTimeoutUsec)
      : elf_image_(filename), sandbox_(cpu_timeout_usec, wall_timeout_usec) {}

  Program(const Program &) = delete;
  Program &operator=(const Program &) = delete;

  bool IsInitialized() const { return elf_image_.IsInitialized(); }

  // Factory method to create Program instances based on one of the //elfs.
  // The timeouts are forwarded to the Program's Sandbox.
  static std::shared_ptr<Program>
  Create(const std::string &filename,
         long cpu_timeout_usec = Sandbox::kDefaultCpuTimeoutUsec,
         long wall_timeout_usec = Sandbox::kDefaultWallTimeoutUsec);

  // Execute the ELF in the Sandbox according to mode (see Sandbox::ExecuteMode)
  // and cache the outcome for the last_* accessors below. Returns the number of
  // ptrace stops observed (3 for a well-behaved kTerminateInEvolvedCode run).
  int Execute(ExecuteMode mode = ExecuteMode::kTerminateInEvolvedCode);

  // Get and set the ELF's evolvable code (main).
  std::vector<char> GetElfCode() const { return elf_image_.GetCode(); }
  // Size of elf_code must match the size of the ELF's evolvable code (main).
  void SetElfCode(const std::vector<char> &elf_code) {
    elf_image_.SetCode(elf_code);
  }
  // Replace all instruction in ELF's evolvable code (main) with nop
  // instructions.
  void SetElfCodeToAllNops() { elf_image_.SetCodeToAllNops(); }

  // Get and set the ELF's inputs variable.
  std::vector<int> GetElfInputs() const { return elf_image_.GetInputs(); }
  // Size of elf_inputs must be smaller or equal to the size of the ELF's inputs
  // variable.
  void SetElfInputs(const std::vector<int> &elf_inputs) {
    elf_image_.SetInputs(elf_inputs);
  }

  // Save the current in-memory elf into filename.
  void SaveElf(const char *filename) { elf_image_.Save(filename); }

  unsigned long long last_syscall() const { return last_syscall_; }
  long long last_rip_offset() const { return last_rip_offset_; }
  int last_exit_status() const { return last_exit_status_; }
  int last_term_signal() const { return last_term_signal_; }
  int last_stop_signal() const { return last_stop_signal_; }
  const std::vector<int> &last_results() const { return last_results_; }

  long cpu_timeout_usec() const { return sandbox_.cpu_timeout_usec(); }
  long wall_timeout_usec() const { return sandbox_.wall_timeout_usec(); }

protected:
  // The in-memory ELF (memfd + symbol data) this Program executes and modifies.
  ElfImage elf_image_;

  // Runs elf_image_ in a locked-down child process. Holds no per-execution
  // state, so a single instance is reused across Execute calls.
  Sandbox sandbox_;

  // Outcome of the last Execute, copied out of the Sandbox's ExecutionResult.
  // Left as individual members (rather than an ExecutionResult) because the
  // unit-test mocks poke last_results_ / last_stop_signal_ directly. Initial
  // values are the ExecutionResult "not run yet" sentinels.
  unsigned long long last_syscall_ = ExecutionResult::kInvalidSyscall;
  long long last_rip_offset_ = ExecutionResult::kInvalidRipOffset;
  int last_exit_status_ = ExecutionResult::kInvalidExitStatus;
  int last_term_signal_ = ExecutionResult::kInvalidSignal;
  int last_stop_signal_ = ExecutionResult::kInvalidSignal;
  std::vector<int> last_results_;
};

} // namespace viaevo

#endif // VIAEVO_PROGRAM_PROGRAM_H_
