// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#ifndef VIAEVO_PROGRAM_PROGRAM_H_
#define VIAEVO_PROGRAM_PROGRAM_H_

#include <elf.h>
#include <fcntl.h>

#include <memory>
#include <vector>

#include "elf_layout.h"

namespace viaevo {

// Program reads ELFs and manages their modifications, execution and reading of
// results.
//
// Execution places an int3 (0xCC) software breakpoint at the entry of main in
// the traced ELF process, so "evolvable code reached" is detected exactly and
// independently of how many system calls the loader/libc make before main.
//
// The class is not intended for subclassing (other than mock classes for unit
// testing). Instances should be created via the factory method Create for ELFs
// in //elfs.
class Program {
public:
  // How Execute runs the ELF process:
  // - kTerminateInEvolvedCode (default, used during evolution): run untraced
  //   to an int3 breakpoint at main, remove the breakpoint, then terminate the
  //   process at the first system call or signal originating from the
  //   (evolved) code in main. Results are read from the process memory just
  //   before termination.
  // - kStopAtMainEntry: terminate the process at the breakpoint at main
  //   instead; main never executes and the results read reflect the ELF's
  //   initialized data.
  // - kRunToCompletion: no breakpoint; trace system calls until the process
  //   exits on its own. Intended for tests/diagnostics only - the code in main
  //   runs unrestrained (still under seccomp and the timeout) and results are
  //   not read.
  enum class ExecuteMode {
    kTerminateInEvolvedCode,
    kStopAtMainEntry,
    kRunToCompletion,
  };

  // TODO: Allowing the default constructor to make it easier to subclass for
  // mocking in unit testing. May want to find a different approach.
  Program() {}
  Program(const char *filename);

  Program(const Program &) = delete;
  Program &operator=(const Program &) = delete;

  ~Program();

  bool IsInitialized() const;

  // Factory method to create Program instances based on one of the //elfs.
  static std::shared_ptr<Program> Create(const std::string &filename);

  // Execute the program according to mode (see ExecuteMode above) and populate
  // last_* member variables (last_results_ is populated unless mode is
  // kRunToCompletion). Returns the number of ptrace stops observed during the
  // process lifetime; in kTerminateInEvolvedCode mode this is 3 for a
  // well-behaved run: the post-execveat SIGTRAP, the breakpoint SIGTRAP at
  // main, and the terminating stop from the evolved code.
  int Execute(ExecuteMode mode = ExecuteMode::kTerminateInEvolvedCode);

  // Get and set the ELF's evolvable code (main).
  std::vector<char> GetElfCode() const;
  // Size of elf_code must match the size of the ELF's evolvable code (main).
  void SetElfCode(const std::vector<char> &elf_code);
  // Replace all instruction in ELF's evolvable code (main) with nop
  // instructions.
  void SetElfCodeToAllNops();

  // Get and set the ELF's inputs variable.
  std::vector<int> GetElfInputs() const;
  // Size of elf_inputs must be smaller or equal to the size of the ELF's inputs
  // variable.
  void SetElfInputs(const std::vector<int> &elf_inputs);

  // Save the current elf in the memory file referenced by the elf_mem_fd_ file
  // descriptor into filename.
  void SaveElf(const char *filename);

  unsigned long long last_syscall() const { return last_syscall_; }
  long long last_rip_offset() const { return last_rip_offset_; }
  int last_exit_status() const { return last_exit_status_; }
  int last_term_signal() const { return last_term_signal_; }
  int last_stop_signal() const { return last_stop_signal_; }
  const std::vector<int> &last_results() const { return last_results_; }

private:
  // Copies the ELF from filename to an in memory file referenced by the
  // elf_mem_fd_ member variable.
  void SetupElfInMemory(const char *filename);

  // Writes the contents one file (file descriptor fd_from) into another (file
  // descriptor fd_to). Used by member functions SetupElfInMemory and SaveElf.
  void WriteFile(int fd_from, int fd_to);

  // Monitors the separate ELF process via ptrace stops (breakpoint-driven, see
  // the class comment and the State enum in the implementation). Also
  // populates last_results_ (unless mode is kRunToCompletion). Returns the
  // number of ptrace stops during the lifetime of the ELF process.
  int MonitorElfProcess(pid_t elf_pid, ExecuteMode mode);

  // Runs the ELF in a new process (created via vfork prior to calling this
  // function).
  void RunElfProcess();

  // Reads last_results_ from the ELF process and updates last_rip_offset_.
  // start_code and start_data are the process's runtime text and data segment
  // start addresses (parsed from /proc/[elf_pid]/stat at the post-exec stop).
  void ReadLastResultsAndLastRipOffsetFromElfProcess(pid_t elf_pid,
                                                     unsigned long long rip,
                                                     unsigned long start_code,
                                                     unsigned long start_data);

  // Clears last_* member variables.
  void ClearLastState();

  // File descriptor of the (in memory) file containig the ELF being evolved.
  // This is where the code modifications happen and where inputs are updated
  // between executions.
  int elf_mem_fd_ = -1;

protected:
  // Last syscall, rip (instruction pointer) offset (vs. main), status, and
  // signal observed in the elf process.
  static constexpr unsigned long long kInvalidSyscall = 9999;
  unsigned long long last_syscall_ = kInvalidSyscall;

  // Offset of the last observed instruction pointer (rip) relative to main.
  // Signed so that kInvalidRipOffset (-1) is a genuine sentinel and so that
  // out-of-main offsets can be detected via a "< 0" check (see
  // MutatorPointLastInstruction).
  static constexpr long long kInvalidRipOffset = -1;
  long long last_rip_offset_ = kInvalidRipOffset;

  static constexpr int kInvalidExitStatus = -9999;
  int last_exit_status_ = kInvalidExitStatus;

  static constexpr int kInvalidSignal = -1;
  int last_term_signal_ = kInvalidSignal;
  int last_stop_signal_ = kInvalidSignal;

  // Results from the last completed execution of the program.
  std::vector<int> last_results_;

  // ELF symbol table values and sizes for main, inputs and results (see
  // elf_layout.h). Resolved from the in-memory ELF on construction.
  SymbolData symbol_data_;
};

} // namespace viaevo

#endif // VIAEVO_PROGRAM_PROGRAM_H_
