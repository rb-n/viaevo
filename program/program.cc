// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "program.h"

#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <seccomp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

#include <fstream>
#include <string>
#include <unordered_map>

namespace viaevo {

namespace {

void myfail(const char *s) {
  perror(s);
  exit(EXIT_FAILURE);
}

} // namespace

std::unordered_map<std::string, Program::SymbolData> Program::symbol_data_map_;

std::unordered_map<std::string, int> Program::expected_ptrace_stops_map_;

Program::Program(const char *filename) { SetupElfInMemory(filename); }

Program::Program(const char *filename, SymbolData symbol_data,
                 int expected_ptrace_stops)
    : symbol_data_(symbol_data), expected_ptrace_stops_(expected_ptrace_stops) {
  SetupElfInMemory(filename);
}

Program::~Program() {
  if (elf_mem_fd_ != -1)
    close(elf_mem_fd_);
}

bool Program::IsInitialized() const {
  return (elf_mem_fd_ != -1 &&
          symbol_data_.main_offset_in_elf_ != (Elf64_Addr)-1 &&
          symbol_data_.main_st_size_ != (uint64_t)-1 &&
          symbol_data_.results_offset_in_data_ != (Elf64_Addr)-1 &&
          symbol_data_.results_st_size_ != (uint64_t)-1 &&
          expected_ptrace_stops_ != -1);
}

std::shared_ptr<Program> Program::Create(const std::string filename) {
  if (symbol_data_map_.count(filename) == 0 ||
      expected_ptrace_stops_map_.count(filename) == 0) {
    Program p(filename.c_str());
    p.InitializeElfSymbolData();
    symbol_data_map_[filename] = p.symbol_data_;
    expected_ptrace_stops_map_[filename] = p.Execute();
  }
  return std::make_shared<Program>(filename.c_str(), symbol_data_map_[filename],
                                   expected_ptrace_stops_map_[filename]);
}

void Program::SetupElfInMemory(const char *filename) {
  int fd_from;

  if (elf_mem_fd_ != -1)
    close(elf_mem_fd_);

  // In memory files for all instances of Program have the same filename. This
  // should be ok as per memfd_create(2): "... as such multiple files can have
  // the same name without any side effects."
  elf_mem_fd_ = memfd_create("viaevo_program", 0);
  if (elf_mem_fd_ == -1)
    myfail("memfd_create failed");

  fd_from = open(filename, O_RDONLY);
  if (fd_from == -1)
    myfail("open failed");

  WriteFile(fd_from, elf_mem_fd_);

  close(fd_from);
}

void Program::WriteFile(int fd_from, int fd_to) {
  char buffer[4096];
  ssize_t nread;

  // Based on https://stackoverflow.com/a/2180788
  while (nread = read(fd_from, buffer, sizeof buffer), nread > 0) {
    char *out_ptr = buffer;
    ssize_t nwritten;

    do {
      nwritten = write(fd_to, out_ptr, nread);
      if (nwritten >= 0) {
        nread -= nwritten;
        out_ptr += nwritten;
      } else if (errno != EINTR) {
        myfail("write failed");
      }
    } while (nread > 0);
  }
}

void Program::SaveElf(const char *filename) {
  int fd_to;

  if (elf_mem_fd_ == -1)
    myfail("invalid elf_mem_fd_");

  off_t offset = lseek(elf_mem_fd_, 0, SEEK_SET);
  if (offset != 0)
    myfail("lseek to 0 failed");

  fd_to = creat(filename, 0666);
  if (fd_to == -1)
    myfail("creat failed");

  WriteFile(elf_mem_fd_, fd_to);

  close(fd_to);
}

void Program::ClearResultsHistory() { results_history_.clear(); }

void Program::InitializeElfSymbolData() {
  if (elf_mem_fd_ == -1)
    myfail("invalid elf_mem_fd_");

  ssize_t nread;

  // Read the ELF header.
  Elf64_Ehdr ehdr;
  off_t offset = lseek(elf_mem_fd_, 0, SEEK_SET);
  if (offset != 0)
    myfail("lseek to 0 failed");

  nread = read(elf_mem_fd_, &ehdr, sizeof(ehdr));
  if (nread != sizeof(ehdr))
    myfail("read ehdr failed");

  // printf("e_phoff : %ld, e_phentsize : %d, e_phnum : %d\n", ehdr.e_phoff,
  //        ehdr.e_phentsize, ehdr.e_phnum);
  // printf("e_shoff : %ld, e_shentsize : %d, e_shnum : %d\n", ehdr.e_shoff,
  //        ehdr.e_shentsize, ehdr.e_shnum);

  // Read section headers.
  std::vector<Elf64_Shdr> shdrs(ehdr.e_shnum);
  offset = lseek(elf_mem_fd_, ehdr.e_shoff, SEEK_SET);
  if (offset != (off_t)ehdr.e_shoff)
    myfail("lseek to ehdr.e_shoff failed");
  nread = read(elf_mem_fd_, shdrs.data(), ehdr.e_shnum * ehdr.e_shentsize);
  if (nread != ehdr.e_shnum * ehdr.e_shentsize)
    myfail("read shdrs failed");

    // for (auto &shdr : shdrs) {
    //   printf("name: %d\n", shdr.sh_name);
    // }

    // Read the section header string table.
#define SBUF_SIZE 512
  char sbuf[SBUF_SIZE];
  if (shdrs[ehdr.e_shstrndx].sh_size >= sizeof(sbuf))
    myfail("sbuf too small for shstrtab");

  offset = lseek(elf_mem_fd_, shdrs[ehdr.e_shstrndx].sh_offset, SEEK_SET);
  if (offset != (off_t)shdrs[ehdr.e_shstrndx].sh_offset)
    myfail("lseek to shdrs[ehdr.e_shstrndx].sh_offset failed");

  nread = read(elf_mem_fd_, sbuf, shdrs[ehdr.e_shstrndx].sh_size);
  if (nread != (off_t)shdrs[ehdr.e_shstrndx].sh_size)
    myfail("section header string table read failed");

  std::unordered_map<std::string, int> name_to_shdrs_index;
  for (int i = 0; i < (int)shdrs.size(); ++i) {
    std::string str(&sbuf[shdrs[i].sh_name]);
    name_to_shdrs_index[str] = i;
    // printf("%s: %d\n", str.c_str(), i);
  }

  // Read the symbol table.
  if (name_to_shdrs_index.count(".symtab") < 1)
    myfail(".symtab not found");
  int symtab_index = name_to_shdrs_index[".symtab"];

  if (shdrs[symtab_index].sh_size % sizeof(Elf64_Sym) != 0)
    myfail("symtab size misunderstood");

  int symtab_num = shdrs[symtab_index].sh_size / sizeof(Elf64_Sym);
  std::vector<Elf64_Sym> syms(symtab_num);

  offset = lseek(elf_mem_fd_, shdrs[symtab_index].sh_offset, SEEK_SET);
  if (offset != (off_t)shdrs[symtab_index].sh_offset)
    myfail("lseek to shdrs[symtab_index].sh_offset failed");

  nread = read(elf_mem_fd_, syms.data(), shdrs[symtab_index].sh_size);
  if (nread != (ssize_t)shdrs[symtab_index].sh_size)
    myfail("read symtab failed");

  // for (auto sym : syms) {
  //   printf("name: %d\n", sym.st_name);
  // }

  // Read the symbol table string table.
  if (name_to_shdrs_index.count(".strtab") < 1)
    myfail(".strtab not found");
  int strtab_index = name_to_shdrs_index[".strtab"];

  if (shdrs[strtab_index].sh_size >= sizeof(sbuf))
    myfail("sbuf too small for strtab");

  offset = lseek(elf_mem_fd_, shdrs[strtab_index].sh_offset, SEEK_SET);
  if (offset != (off_t)shdrs[strtab_index].sh_offset)
    myfail("lseek to shdrs[ehdr.e_shstrndx].sh_offset failed");

  nread = read(elf_mem_fd_, sbuf, shdrs[strtab_index].sh_size);
  if (nread != (ssize_t)shdrs[strtab_index].sh_size)
    myfail("symbol table string table read failed");

  std::unordered_map<std::string, int> name_to_syms_index;
  for (int i = 0; i < (int)syms.size(); ++i) {
    std::string str(&sbuf[syms[i].st_name]);
    name_to_syms_index[str] = i;
    // printf("%s: %d\n", str.c_str(), i);
  }

  if (name_to_shdrs_index.count(".text") < 1)
    myfail("section header .text not found");
  int text_index = name_to_shdrs_index[".text"];

  if (name_to_syms_index.count("main") < 1)
    myfail("symbol main not found");
  int main_index = name_to_syms_index["main"];

  symbol_data_.main_offset_in_text_ =
      syms[main_index].st_value - shdrs[text_index].sh_addr;
  // NOTE: Risk of underflow for an unsigned variable? (Same below for inputs.)
  symbol_data_.main_offset_in_elf_ =
      syms[main_index].st_value -
      (shdrs[text_index].sh_addr - shdrs[text_index].sh_offset);
  symbol_data_.main_st_size_ = syms[main_index].st_size;

  if (name_to_shdrs_index.count(".data") < 1)
    myfail("section header .data not found");
  int data_index = name_to_shdrs_index[".data"];

  if (name_to_syms_index.count("inputs") < 1)
    myfail("symbol inputs not found");
  int inputs_index = name_to_syms_index["inputs"];

  symbol_data_.inputs_offset_in_elf_ =
      syms[inputs_index].st_value -
      (shdrs[data_index].sh_addr - shdrs[data_index].sh_offset);
  symbol_data_.inputs_st_size_ = syms[inputs_index].st_size;

  if (name_to_syms_index.count("data_start") < 1)
    myfail("symbol data_start not found");
  int data_start_index = name_to_syms_index["data_start"];

  if (name_to_syms_index.count("results") < 1)
    myfail("symbol results not found");
  int results_index = name_to_syms_index["results"];
  symbol_data_.results_offset_in_data_ =
      syms[results_index].st_value - syms[data_start_index].st_value;
  symbol_data_.results_st_size_ = syms[results_index].st_size;
}

int Program::Execute(int max_ptrace_stops) {
  ClearLastState();

  pid_t pid;

  pid = fork();

  // TODO: Fork sometimes fails with "resource not available", retry?
  if (pid == -1)
    myfail("fork failed");

  if (pid > 0) {
    return MonitorElfProcess(pid, max_ptrace_stops);
  } else {
    RunElfProcess();
  }

  // This should be unreachable code.
  myfail("Program::Execute failed");
  return -1; // Keep the linter happy.
}

int Program::MonitorElfProcess(pid_t elf_pid, int max_ptrace_stops) {
  int status, ptrace_stops_count = 0;
  pid_t w;
  struct user_regs_struct regs;

  if (max_ptrace_stops == -1)
    max_ptrace_stops = expected_ptrace_stops_;

  // printf("In parent, child pid: %d\n", elf_pid);
  // printf("In parent, parent pid: %d\n", getpid());

  // From: https://linux.die.net/man/2/waitpid
  do {
    w = waitpid(elf_pid, &status, WUNTRACED | WCONTINUED);
    if (w == -1)
      myfail("waitpid failed");

    if (WIFEXITED(status)) {
      last_exit_status_ = WEXITSTATUS(status);
      // printf("exited, status=%d\n", last_exit_status_);
    } else if (WIFSIGNALED(status)) {
      last_term_signal_ = WTERMSIG(status);
      // printf("killed by signal %d\n", last_signal_);
    } else if (WIFSTOPPED(status)) {
      ++ptrace_stops_count;
      last_stop_signal_ = WSTOPSIG(status);
      // printf("%4d stopped by signal %d", ptrace_stops_count, last_signal_);

      if (ptrace(PTRACE_GETREGS, elf_pid, 0, &regs) == -1) {
        // myfail("PTRACE_GETREGS failed");
        // This call sometimes fails with "No such process". These failures seem
        // random and not clear at this point what is the cause. These failures
        // seem to be prevented by adding a sleep after the if block with the
        // call to kill further below. The last_*_ member variables are set
        // (with the exception of last_term_signal_ and last_exit_status_). This
        // means ReadLastResultsAndLastRipOffsetFromElfProcess was called. Retry
        // ptrace call does not seem to help (as the process seems to be gone).
        // The failure seems innocent, so let's ignore it and just return
        // ptrace_stops_count.
        // TODO: Find/fix the root cause, also check for resource leaks.
        printf("\n");
        perror("PTRACE_GETREGS failed (ignoring)");
        // printf("last_stop_signal_: %d, last_term_signal_: %d,
        // last_rip_offset: "
        //        "%lld, last_exit_status_: %d, last_results_[0]: %d, "
        //        "ptrace_stops_count: %d\n",
        //        last_stop_signal_, last_term_signal_, last_rip_offset_,
        //        last_exit_status_, last_results_[0], ptrace_stops_count);
        return ptrace_stops_count;
      }

      last_syscall_ = regs.orig_rax;

      // printf(", syscall: %lld\n", last_syscall_);

      if (ptrace_stops_count >= max_ptrace_stops && max_ptrace_stops != -1) {
        // We are done with the elf process after the initial expected
        // syscalls. If this is happening at expected_ptrace_stops_, any
        // additional syscall or signal is assumed to originate from the
        // program's (evolved) code and ends the process. The (result) data are
        // explored at this point. The child process is killed.
        ReadLastResultsAndLastRipOffsetFromElfProcess(elf_pid, regs.rip);
        if (kill(elf_pid, SIGKILL) == -1)
          myfail("kill failed");
      } else {
        if (last_stop_signal_ != 5) {
          // E.g. SISGSEGV for and invalid program.
          if (ptrace(PTRACE_CONT, elf_pid, 0, last_stop_signal_) == -1)
            myfail("PTRACE_CONT failed");

        } else {
          // if (ptrace_stops_count == max_ptrace_stops - 1) {
          //   --ptrace_stops_count;
          //   ReadLastResultsFromElfProcess(elf_pid, regs.rip);
          //   printf(" last_rip_offset_: %lld\n", last_rip_offset_);
          //   if (ptrace(PTRACE_SINGLESTEP, elf_pid, NULL, NULL) == -1)
          //     myfail("PTRACE_SINGLESTEP failed");
          // } else {
          if (ptrace(PTRACE_SYSCALL, elf_pid, 0, 0) == -1)
            myfail("PTRACE_SYSCALL failed");
          // }
        }
      }
    } else if (WIFCONTINUED(status)) {
      printf("continued\n");
    }

  } while (!WIFEXITED(status) && !WIFSIGNALED(status));

  // printf("ptrace_stops_count: %d\n", ptrace_stops_count);
  return ptrace_stops_count;
}

void Program::RunElfProcess() {
  // "Ask for a SIGALRM" to be delivered to the child process. This should cause
  // a termination of the process if e.g. an infinite loop is present.
  struct itimerval alarm_timer;

  alarm_timer.it_interval.tv_sec = 0;
  alarm_timer.it_interval.tv_usec = 0;
  alarm_timer.it_value.tv_sec = 0;
  // TODO: use a command line flag to set the duration.
  alarm_timer.it_value.tv_usec = 50000;

  setitimer(ITIMER_REAL, &alarm_timer, NULL);

  // From:
  // https://stackoverflow.com/questions/63208333/using-memfd-create-and-fexecve-to-run-elf-from-memory
  const char *const av[] = {"memprogram", NULL};
  const char *const ep[] = {NULL};

  // Limit the allowed syscalls for the elf_process to the necessary minimum.
  // The parent process is only intended to run in a sandbox anyway, but let's
  // try to be cautious here as well.

  // prctl(PR_SET_SECCOMP, SECCOMP_MODE_STRICT); // Does now work with fexecve
  // further below, using libseccomp instead.

  // From:
  // https://adil.medium.com/allow-disallow-syscalls-via-seccomp-d5fc8816d34e
  scmp_filter_ctx ctx = seccomp_init(SCMP_ACT_KILL);
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(read), 0);
  // seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(write), 0); // The write
  // system call does not seem to be necessary what is good.
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(close), 0);
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit_group), 0);

  // These were added one by one by looking into syslog after "killed by
  // signal 31" failures.
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, 322, 0); // stub_execveat
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(brk), 0);
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(mmap), 0);
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(access), 0);
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(openat), 0);
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(newfstatat), 0);
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(pread64), 0);
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(arch_prctl), 0);
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(set_tid_address), 0);
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(set_robust_list), 0);
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(rseq), 0);
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(mprotect), 0);
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(prlimit64), 0);
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(munmap), 0);
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(getrandom), 0);

  // The system call below is required for ptrace.
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(ptrace), 0);

  seccomp_load(ctx);

  if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) == -1)
    myfail("PTRACE_TRACEME failed");

  if (fexecve(elf_mem_fd_, (char *const *)av, (char *const *)ep) == -1)
    myfail("fexecve failed");
}

void Program::ReadLastResultsAndLastRipOffsetFromElfProcess(
    pid_t elf_pid, unsigned long long rip) {
  std::string proc_file_name =
      std::string("/proc/") + std::to_string(elf_pid) + std::string("/stat");

  int proc_pid;
  std::string proc_comm;
  char proc_state;
  unsigned long dummy_ul;
  unsigned long start_code, end_code, kstkeip, start_data, end_data;

  std::ifstream ifs(proc_file_name);
  ifs >> proc_pid >> proc_comm >> proc_state;
  // Skip to start_data.
  // NOTE: some of the fields are not unsigned, using the unsigned long dummy_ul
  // variable may not be appropriate.
  for (int i = 0; i < 22; ++i)
    ifs >> dummy_ul;
  ifs >> start_code >> end_code;
  for (int i = 0; i < 2; ++i)
    ifs >> dummy_ul;
  ifs >> kstkeip;
  for (int i = 0; i < 14; ++i)
    ifs >> dummy_ul;
  ifs >> start_data >> end_data;

  if (start_data > end_data)
    myfail("start_data > end_data");

  last_rip_offset_ = rip - start_code - symbol_data_.main_offset_in_text_;

  // printf("pid: %d, comm: %s, state: %c\n", proc_pid, proc_comm.c_str(),
  //        proc_state);
  // printf("proc_comm: %s\n", proc_comm.c_str());
  // printf("start_data: %ld, end_data %ld, diff: %ld\n", start_data, end_data,
  //        end_data - start_data);
  // printf("start_data: %lx, end_data %lx, diff: %lx\n", start_data, end_data,
  //        end_data - start_data);

  struct iovec local[1];
  struct iovec remote[1];
  ssize_t nread;

  if (symbol_data_.results_st_size_ %
          sizeof(decltype(last_results_)::value_type) !=
      0)
    myfail("results_st_size_ mismatch");

  last_results_.resize(symbol_data_.results_st_size_ /
                       sizeof(decltype(last_results_)::value_type));
  // printf("last_results_ size: %ld\n", last_results_.size());

  local[0].iov_base = last_results_.data();
  local[0].iov_len = symbol_data_.results_st_size_;
  remote[0].iov_base =
      (void *)(start_data + symbol_data_.results_offset_in_data_);
  remote[0].iov_len = symbol_data_.results_st_size_;

  nread = process_vm_readv(elf_pid, local, 1, remote, 1, 0);
  if (nread != (ssize_t)symbol_data_.results_st_size_)
    myfail("process_vm_readv failed");

  if (track_results_history_) {
    results_history_.push_back(last_results_);
  }
}

void Program::ClearLastState() {
  last_syscall_ = kInvalidSyscall;
  last_rip_offset_ = -1;
  last_exit_status_ = kInvalidExitStatus;
  last_term_signal_ = kInvalidSignal;
  last_stop_signal_ = kInvalidSignal;
  last_results_.clear();
}

std::vector<char> Program::GetElfCode() const {
  if (symbol_data_.main_offset_in_elf_ == (Elf64_Addr)-1)
    myfail("location to get main unknown");

  off_t offset = lseek(elf_mem_fd_, symbol_data_.main_offset_in_elf_, SEEK_SET);
  if (offset != (off_t)symbol_data_.main_offset_in_elf_)
    myfail("get elf code lseek failed");

  std::vector<char> elf_code(symbol_data_.main_st_size_);
  ssize_t nread =
      read(elf_mem_fd_, elf_code.data(), symbol_data_.main_st_size_);
  if (nread != (ssize_t)symbol_data_.main_st_size_)
    myfail("getting elf code failed");

  return elf_code;
}

void Program::SetElfCode(const std::vector<char> &elf_code) {
  if (elf_code.size() != symbol_data_.main_st_size_)
    myfail("elf code to set has incorrect size");

  if (symbol_data_.main_offset_in_elf_ == (Elf64_Addr)-1)
    myfail("location to set main unknown");

  off_t offset = lseek(elf_mem_fd_, symbol_data_.main_offset_in_elf_, SEEK_SET);
  if (offset != (off_t)symbol_data_.main_offset_in_elf_)
    myfail("set elf code lseek failed");

  ssize_t nwritten = write(elf_mem_fd_, elf_code.data(), elf_code.size());
  if (nwritten != (off_t)elf_code.size())
    myfail("setting elf code failed");
}

void Program::SetElfCodeToAllNops() {
  std::vector<char> new_elf_code(symbol_data_.main_st_size_, 0x90);
  SetElfCode(new_elf_code);
}

std::vector<int> Program::GetElfInputs() const {
  if (symbol_data_.inputs_offset_in_elf_ == (Elf64_Addr)-1)
    myfail("location to get inputs unknown");

  std::vector<int> elf_inputs;

  if (symbol_data_.inputs_st_size_ % sizeof(decltype(elf_inputs)::value_type) !=
      0)
    myfail("inputs_st_size_ mismatch");

  elf_inputs.resize(symbol_data_.inputs_st_size_ /
                    sizeof(decltype(elf_inputs)::value_type));

  off_t offset =
      lseek(elf_mem_fd_, symbol_data_.inputs_offset_in_elf_, SEEK_SET);
  if (offset != (off_t)symbol_data_.inputs_offset_in_elf_)
    myfail("get elf inputs lseek failed");

  ssize_t nread =
      read(elf_mem_fd_, elf_inputs.data(), symbol_data_.inputs_st_size_);
  if (nread != (ssize_t)symbol_data_.inputs_st_size_)
    myfail("getting elf inputs failed");

  return elf_inputs;
}

void Program::SetElfInputs(const std::vector<int> &elf_inputs) {
  if (symbol_data_.inputs_offset_in_elf_ == (Elf64_Addr)-1)
    myfail("location to set inputs unknown");

  auto element_size =
      sizeof(std::remove_reference_t<decltype(elf_inputs)>::value_type);

  if (symbol_data_.inputs_st_size_ % element_size != 0)
    myfail("inputs_st_size_ mismatch");

  if (elf_inputs.size() * element_size > symbol_data_.inputs_st_size_)
    myfail("elf inputs to set are too large");

  off_t offset =
      lseek(elf_mem_fd_, symbol_data_.inputs_offset_in_elf_, SEEK_SET);
  if (offset != (off_t)symbol_data_.inputs_offset_in_elf_)
    myfail("set elf inputs lseek failed");

  ssize_t nwritten =
      write(elf_mem_fd_, elf_inputs.data(), elf_inputs.size() * element_size);
  if (nwritten != (off_t)(elf_inputs.size() * element_size))
    myfail("setting elf inputs failed");
}

void Program::ResetCurrentScore() { current_score_ = 0; }

void Program::IncrementCurrentScoreBy(long long increment) {
  current_score_ += increment;
}

} // namespace viaevo