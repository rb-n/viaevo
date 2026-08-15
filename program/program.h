// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#ifndef VIAEVO_PROGRAM_PROGRAM_H_
#define VIAEVO_PROGRAM_PROGRAM_H_

#include <elf.h>
#include <fcntl.h>

#include <memory>
#include <unordered_map>
#include <vector>

#include "elf_layout.h"

namespace viaevo {

// Program reads ELFs and manages their modifications, execution and reading of
// results.
//
// The class is not intended for subclassing (other than mock classes for unit
// testing). Instances should be created via the factory method Create for ELFs
// in //elfs.
class Program {
public:
  // TODO: Allowing the default constructor to make it easier to subclass for
  // mocking in unit testing. May want to find a different approach.
  Program() {}
  Program(const char *filename);
  Program(const char *filename, SymbolData symbol_data,
          int expected_ptrace_stops);

  Program(const Program &) = delete;
  Program &operator=(const Program &) = delete;

  ~Program();

  bool IsInitialized() const;

  // Factory method to create Program instances based on one of the //elfs.
  static std::shared_ptr<Program> Create(const std::string &filename);

  // Execute the program and populate last_results_. At most max_ptrace_stops
  // will be allowed for the elf process before the elf process is terminated.
  // If max_ptrace_stops is -1, at most expected_ptrace_stops_ will be allowed.
  // Returns the number of ptrace stops during the process lifetime.
  int Execute(int max_ptrace_stops = -1);

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
  int expected_ptrace_stops() const { return expected_ptrace_stops_; }

private:
  // Copies the ELF from filename to an in memory file referenced by the
  // elf_mem_fd_ member variable.
  void SetupElfInMemory(const char *filename);

  // Writes the contents one file (file descriptor fd_from) into another (file
  // descriptor fd_to). Used by member functions SetupElfInMemory and SaveElf.
  void WriteFile(int fd_from, int fd_to);

  // Monitors the separate ELF process via ptrace stops. Also populates
  // last_results_. The value of max_ptrace_stops is passed from the Execute
  // method and has the same meaning here as there. Returns the number of ptrace
  // stops during the lifetime of the ELF process.
  int MonitorElfProcess(pid_t elf_pid, int max_ptrace_stops);

  // Runs the ELF in a new process (created via fork prior to calling this
  // function).
  void RunElfProcess();

  // Reads last_results_ from the ELF process. Also updates last_rip_offset_ -
  // as /proc/[elf_pid]/stat is parsed here and also provides codestart address.
  void ReadLastResultsAndLastRipOffsetFromElfProcess(pid_t elf_pid,
                                                     unsigned long long rip);

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
  // elf_layout.h).
  SymbolData symbol_data_;

  // Map elf name to its symbol data to "memoize" these and avoid computing
  // these for each instance of the same elf. Symbol data fields are initialized
  // to -1 and set the correct value during the first pass of the factory method
  // for each ELF. Then the values from the map are used for subsequent
  // instances of the elf.
  static std::unordered_map<std::string, SymbolData> symbol_data_map_;

  // Number of ptrace stops (e.g. syscalls) prior to executing main().
  int expected_ptrace_stops_ = -1;

  // Memoize expected ptrace stops for different elfs to avoid computing them
  // for every instance.
  static std::unordered_map<std::string, int> expected_ptrace_stops_map_;
};

} // namespace viaevo

#endif // VIAEVO_PROGRAM_PROGRAM_H_