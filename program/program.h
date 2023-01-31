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

namespace viaevo {

// Program reads ELFs and manages their modifications, execution and reading of
// results.
//
// The class is not intended for subclassing and will provide a separate factory
// method for each ELF in //elfs.
class Program final {
public:
  explicit Program(const char *filename);
  Program(const char *filename, Elf64_Addr main_st_value, uint64_t main_st_size,
          Elf64_Addr results_offset_in_data, uint64_t results_st_size,
          int expected_ptrace_stops);
  ~Program();

  bool IsInitialized() const;

  // Factory methods creating a Program instance based on one of the //elfs.
  static std::shared_ptr<Program> CreateSimpleSmall(); // //elfs:simple_small

  // Execute the program and populate last_results_. At most max_ptrace_stops
  // will be allowed for the elf process before the elf process is terminated.
  // If max_ptrace_stops is -1, at most expected_ptrace_stops_ will be allowed.
  // Returns the number of ptrace stops during the process lifetime.
  int Execute(int max_ptrace_stops = -1);

  unsigned long long last_syscall() const { return last_syscall_; }
  int last_exit_status() const { return last_exit_status_; }
  int last_signal() const { return last_signal_; }
  const std::vector<int> &last_results() const { return last_results_; }
  int expected_ptrace_stops() const { return expected_ptrace_stops_; }

private:
  // Copies the ELF from filename to an in memory file referenced by the
  // elf_mem_fd_ member variable.
  void SetupElfInMemory(const char *filename);

  // Find main() address (and length) and results address and lenght in the ELF.
  void InitializeElfSymbolData();

  // Monitors the separate ELF process via ptrace stops. Also populates
  // last_results_. The value of max_ptrace_stops is passed from the Execute
  // method and has the same meaning here as there. Returns the number of ptrace
  // stops during the lifetime of the ELF process.
  int MonitorElfProcess(pid_t elf_pid, int max_ptrace_stops);

  // Runs the ELF in a new process (created via fork prior to calling this
  // function).
  void RunElfProcess();

  // Reads last_results_ from the ELF process.
  void ReadLastResultsFromElfProcess(pid_t elf_pid);

  // Clears last_* member variables.
  void ClearLastState();

  // File descriptor of the (in memory) file containig the ELF being evolved.
  // This is where the code modifications happen and where inputs are updated
  // between executions.
  int elf_mem_fd_ = -1;

  // Last syscall, status, and signal observed in the elf process.
  static constexpr unsigned long long kInvalidSyscall = 9999;
  unsigned long long last_syscall_ = kInvalidSyscall;

  static constexpr int kInvalidExitStatus = -9999;
  int last_exit_status_ = kInvalidExitStatus;

  static constexpr int kInvalidSignal = -1;
  int last_signal_ = kInvalidSignal;

  // Results from the last completed execution of the program.
  std::vector<int> last_results_;

  // ELF symbol table values and sizes for main and results.
  Elf64_Addr main_st_value_ = -1;
  uint64_t main_st_size_ = -1;
  Elf64_Addr results_offset_in_data_ = -1;
  uint64_t results_st_size_ = -1;

  // Number of ptrace stops (e.g. syscalls) prior to executing main().
  int expected_ptrace_stops_ = -1;

  // Default values for respective member variables for individual ELFs in
  // //elfs. These should be initialized to -1 and set the correct value during
  // the first pass of the factory method for each ELF.
  static Elf64_Addr main_st_value_simple_small_;
  static uint64_t main_st_size_simple_small_;
  static Elf64_Addr results_offset_in_data_simple_small_;
  static uint64_t results_st_size_simple_small_;

  static int expected_ptrace_stops_simple_small_;
};

} // namespace viaevo

#endif // VIAEVO_PROGRAM_PROGRAM_H_