// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "program.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <seccomp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

#include <fstream>
#include <string>
#include <vector>

namespace viaevo {

namespace {

void myfail(const char *s) {
  perror(s);
  exit(EXIT_FAILURE);
}

// Error handler for the vfork() child. It must NOT call exit(): the child runs
// in the parent's shared address space until it execs, so running atexit
// handlers or flushing stdio buffers would corrupt the (suspended) parent's
// state. Uses _exit() instead. The parent is suspended during the vfork window,
// so the perror() call here races with nothing.
[[noreturn]] void child_fail(const char *s) {
  perror(s);
  _exit(127);
}

// Compiles the seccomp allow-list into a raw BPF program exactly once and
// returns a reference to a cached, never-destroyed sock_fprog.
//
// The compilation (libseccomp rule building + BPF generation, which allocates)
// happens here in the parent process. The forked child then only needs to
// install this pre-built program via async-signal-safe syscalls (see
// RunElfProcess). Building the filter directly in the post-fork child of a
// multithreaded process is unsafe: libseccomp is not async-signal-safe and can
// deadlock on locks (e.g. malloc's) held by other threads at fork() time.
//
// First-call (lazy) initialization of the function-local static is thread-safe;
// Program::Execute also forces this build before fork() so the very first build
// never happens in a child.
const struct sock_fprog &GetSeccompProgram() {
  static const std::vector<struct sock_filter> filter = [] {
    scmp_filter_ctx ctx = seccomp_init(SCMP_ACT_KILL);
    if (!ctx)
      myfail("seccomp_init failed");

    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(read), 0);
    // The write system call does not seem to be necessary, which is good.
    // seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(write), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(close), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit_group), 0);

    // These were added one by one by looking into syslog after "killed by
    // signal 31" failures. The syscall sequence can also be made visible via
    // unit tests (program_test.cc) by uncommenting the corresponding printf
    // statements in MonitorElfProcess. Useful if the compiler/linker adds more
    // syscalls to elfs and the unit tests start failing.
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(execveat), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(brk), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(fstat), 0);
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

    // NOTE: ptrace is deliberately NOT in the allowlist. The child's only
    // ptrace call (PTRACE_TRACEME in RunElfProcess) happens before this filter
    // is installed, so the ELF process itself - including its evolved code -
    // can never invoke ptrace.

    // Export the compiled classic-BPF program through a memfd, then read it
    // back into a vector of sock_filter instructions.
    int bpf_fd = memfd_create("viaevo_seccomp_bpf", MFD_CLOEXEC);
    if (bpf_fd == -1)
      myfail("memfd_create for seccomp bpf failed");
    if (seccomp_export_bpf(ctx, bpf_fd) != 0)
      myfail("seccomp_export_bpf failed");
    seccomp_release(ctx);

    off_t size = lseek(bpf_fd, 0, SEEK_END);
    if (size <= 0 || size % (off_t)sizeof(struct sock_filter) != 0)
      myfail("unexpected seccomp bpf size");
    if (lseek(bpf_fd, 0, SEEK_SET) == -1)
      myfail("lseek on seccomp bpf failed");

    std::vector<struct sock_filter> instructions(size /
                                                 sizeof(struct sock_filter));
    if (read(bpf_fd, instructions.data(), size) != size)
      myfail("reading seccomp bpf failed");
    close(bpf_fd);

    return instructions;
  }();

  static const struct sock_fprog prog = {
      static_cast<unsigned short>(filter.size()),
      const_cast<struct sock_filter *>(filter.data())};
  return prog;
}

// Parses the runtime start addresses of the text (start_code) and data
// (start_data) segments of the process from /proc/[pid]/stat. Valid once the
// process completed exec (called at the post-execveat ptrace stop).
void ReadProcStatAddresses(pid_t pid, unsigned long *start_code,
                           unsigned long *start_data) {
  std::string proc_file_name =
      std::string("/proc/") + std::to_string(pid) + std::string("/stat");

  int proc_pid;
  std::string proc_comm;
  char proc_state;
  unsigned long dummy_ul;
  unsigned long end_code, end_data, kstkeip;

  std::ifstream ifs(proc_file_name);
  ifs >> proc_pid >> proc_comm >> proc_state;
  // Skip to start_code.
  // NOTE: some of the fields are not unsigned, using the unsigned long dummy_ul
  // variable may not be appropriate.
  for (int i = 0; i < 22; ++i)
    ifs >> dummy_ul;
  ifs >> *start_code >> end_code;
  for (int i = 0; i < 2; ++i)
    ifs >> dummy_ul;
  ifs >> kstkeip;
  for (int i = 0; i < 14; ++i)
    ifs >> dummy_ul;
  ifs >> *start_data >> end_data;

  if (!ifs)
    myfail("parsing /proc/[pid]/stat failed");
  if (*start_data > end_data)
    myfail("start_data > end_data");
}

// Replaces the byte at addr in the traced (stopped) process's text with an
// int3 (0xCC) breakpoint. Returns the original text word so the breakpoint can
// be removed again via RemoveBreakpoint. PTRACE_POKETEXT is used (rather than
// e.g. process_vm_writev) as it can write to the read-only text mapping.
long InstallBreakpoint(pid_t pid, unsigned long long addr) {
  errno = 0;
  long original_word = ptrace(PTRACE_PEEKTEXT, pid, addr, 0);
  if (original_word == -1 && errno != 0)
    myfail("PTRACE_PEEKTEXT failed");
  long patched_word = (original_word & ~0xFFL) | 0xCCL;
  if (ptrace(PTRACE_POKETEXT, pid, addr, patched_word) == -1)
    myfail("PTRACE_POKETEXT (install breakpoint) failed");
  return original_word;
}

// Restores the original text word previously replaced by InstallBreakpoint.
void RemoveBreakpoint(pid_t pid, unsigned long long addr, long original_word) {
  if (ptrace(PTRACE_POKETEXT, pid, addr, original_word) == -1)
    myfail("PTRACE_POKETEXT (remove breakpoint) failed");
}

} // namespace

std::shared_ptr<Program> Program::Create(const std::string &filename) {
  return std::make_shared<Program>(filename.c_str());
}

int Program::Execute(ExecuteMode mode) {
  ClearLastState();

  // Compile the seccomp BPF program here in the parent (idempotent after the
  // first call) so the child only has to install the pre-built program via
  // async-signal-safe syscalls.
  GetSeccompProgram();

  pid_t pid;

  // Use vfork() rather than fork(): the parent is suspended and the child runs
  // in the parent's address space until it execs, so no page tables are copied
  // and there is no window in which a multithreaded parent's address space is
  // duplicated with foreign locks held (the "fork-in-a-thread" hazard). The
  // child (RunElfProcess) performs only async-signal-safe syscalls -- no
  // allocation, as the seccomp BPF is pre-built -- and never returns into this
  // frame: it ends in execveat() on success or _exit() (via child_fail) on
  // error.
  pid = vfork();

  if (pid == -1)
    myfail("vfork failed");

  if (pid > 0) {
    return MonitorElfProcess(pid, mode);
  } else {
    RunElfProcess();
  }

  // This should be unreachable code.
  myfail("Program::Execute failed");
  return -1; // Keep the linter happy.
}

int Program::MonitorElfProcess(pid_t elf_pid, ExecuteMode mode) {
  int status, ptrace_stops_count = 0;
  pid_t w;
  struct user_regs_struct regs;

  // Runtime addresses of the ELF process, established at the post-execveat
  // stop.
  unsigned long start_code = 0, start_data = 0;
  unsigned long long main_addr = 0;
  // Original text word at main's entry, replaced by the int3 breakpoint.
  long original_main_word = 0;

  // Breakpoint-driven monitoring. The expected stop sequence in
  // kTerminateInEvolvedCode mode is:
  //   1. the SIGTRAP from the successful execveat (kAwaitingExecTrap) - the
  //      breakpoint is installed at main and the process continues untraced
  //      (startup syscalls are not stopped; they remain constrained by
  //      seccomp),
  //   2. the SIGTRAP from the int3 breakpoint at main (kAwaitingMainTrap) -
  //      the breakpoint is removed, rip is rewound to main, and the process
  //      continues with syscall tracing,
  //   3. the first syscall-entry stop or signal originating from the (evolved)
  //      code in main (kInEvolvedCode) - results are read from the process
  //      memory and the process is killed (a stopped syscall never executes).
  // This replaces the previous approach of counting an ELF-specific expected
  // number of ptrace stops before main, which was brittle across
  // compiler/libc versions and required a calibration run per ELF.
  enum class State {
    kAwaitingExecTrap,
    kAwaitingMainTrap,
    kInEvolvedCode,
    kRunningToCompletion,
    kTerminating,
  } state = State::kAwaitingExecTrap;

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
      // printf("killed by signal %d\n", last_term_signal_);
    } else if (WIFSTOPPED(status)) {
      ++ptrace_stops_count;
      last_stop_signal_ = WSTOPSIG(status);
      // printf("%4d stopped by signal %d", ptrace_stops_count,
      // last_stop_signal_);

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
        return ptrace_stops_count;
      }

      last_syscall_ = regs.orig_rax;

      // printf(", syscall: %lld\n", last_syscall_);

      switch (state) {
      case State::kAwaitingExecTrap:
        if (last_stop_signal_ == SIGTRAP) {
          // The ELF process completed execveat; its runtime memory layout is
          // now in place.
          ReadProcStatAddresses(elf_pid, &start_code, &start_data);
          main_addr =
              start_code + elf_image_.symbol_data().main_offset_in_text_;
          if (mode == ExecuteMode::kRunToCompletion) {
            state = State::kRunningToCompletion;
            if (ptrace(PTRACE_SYSCALL, elf_pid, 0, 0) == -1)
              myfail("PTRACE_SYSCALL failed");
          } else {
            original_main_word = InstallBreakpoint(elf_pid, main_addr);
            state = State::kAwaitingMainTrap;
            if (ptrace(PTRACE_CONT, elf_pid, 0, 0) == -1)
              myfail("PTRACE_CONT failed");
          }
        } else {
          // E.g. SIGALRM if the timeout expires before exec completes on a
          // heavily loaded machine. Forward the signal (typically fatal).
          if (ptrace(PTRACE_CONT, elf_pid, 0, last_stop_signal_) == -1)
            myfail("PTRACE_CONT failed");
        }
        break;

      case State::kAwaitingMainTrap:
        if (last_stop_signal_ == SIGTRAP) {
          // int3 leaves rip one byte past the trap instruction. Only the
          // template's fixed startup code (loader/libc) ran so far, so no
          // other SIGTRAP source is possible - verify rather than assume.
          if (regs.rip != main_addr + 1)
            myfail("unexpected SIGTRAP location before main");
          RemoveBreakpoint(elf_pid, main_addr, original_main_word);
          regs.rip = main_addr;
          if (ptrace(PTRACE_SETREGS, elf_pid, 0, &regs) == -1)
            myfail("PTRACE_SETREGS failed");
          if (mode == ExecuteMode::kStopAtMainEntry) {
            ReadLastResultsAndLastRipOffsetFromElfProcess(
                elf_pid, regs.rip, start_code, start_data);
            if (kill(elf_pid, SIGKILL) == -1)
              myfail("kill failed");
            state = State::kTerminating;
          } else {
            state = State::kInEvolvedCode;
            if (ptrace(PTRACE_SYSCALL, elf_pid, 0, 0) == -1)
              myfail("PTRACE_SYSCALL failed");
          }
        } else {
          // A signal before main (e.g. the SIGALRM timeout during startup);
          // forward it (typically fatal).
          if (ptrace(PTRACE_CONT, elf_pid, 0, last_stop_signal_) == -1)
            myfail("PTRACE_CONT failed");
        }
        break;

      case State::kInEvolvedCode:
        // The first stop after main was entered: a syscall-entry stop (SIGTRAP
        // from PTRACE_SYSCALL) or a signal (e.g. SIGSEGV/SIGILL for an invalid
        // program, SIGALRM for a long running one). The (result) data are
        // explored at this point and the process is killed - a stopped
        // syscall-entry never executes the system call.
        ReadLastResultsAndLastRipOffsetFromElfProcess(elf_pid, regs.rip,
                                                      start_code, start_data);
        if (kill(elf_pid, SIGKILL) == -1)
          myfail("kill failed");
        state = State::kTerminating;
        break;

      case State::kRunningToCompletion:
        if (last_stop_signal_ == SIGTRAP) {
          if (ptrace(PTRACE_SYSCALL, elf_pid, 0, 0) == -1)
            myfail("PTRACE_SYSCALL failed");
        } else {
          // E.g. SIGSEGV for an invalid program.
          if (ptrace(PTRACE_CONT, elf_pid, 0, last_stop_signal_) == -1)
            myfail("PTRACE_CONT failed");
        }
        break;

      case State::kTerminating:
        // A stop racing the pending SIGKILL; nothing more to do here.
        if (ptrace(PTRACE_CONT, elf_pid, 0, 0) == -1)
          myfail("PTRACE_CONT failed");
        break;
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
  //
  // This function runs in the vfork() child, sharing the parent's address space
  // until the execveat() below. It must therefore use only async-signal-safe
  // syscalls (no allocation, no stdio) and must terminate via exec or _exit()
  // (see child_fail) -- never return into Program::Execute.
  //
  // Install the pre-built seccomp BPF program (compiled once in the parent, see
  // GetSeccompProgram). This replaces an earlier libseccomp
  // seccomp_init/seccomp_rule_add/seccomp_load sequence that allocated memory in
  // the child.
  //
  // PR_SET_NO_NEW_PRIVS is required to install a filter without privileges;
  // libseccomp's seccomp_load used to set this for us.
  //
  // PTRACE_TRACEME is called BEFORE the filter is installed so that ptrace
  // does not need to be in the seccomp allowlist - the ELF process (and any
  // evolved code in it) can then never invoke ptrace itself.
  if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == -1)
    child_fail("prctl(PR_SET_NO_NEW_PRIVS) failed");

  if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) == -1)
    child_fail("PTRACE_TRACEME failed");

  if (syscall(SYS_seccomp, SECCOMP_SET_MODE_FILTER, 0, &GetSeccompProgram()) !=
      0)
    child_fail("seccomp(SECCOMP_SET_MODE_FILTER) failed");

  // Execute directly from the in-memory ELF file descriptor. execveat with an
  // empty path and AT_EMPTY_PATH is the underlying mechanism fexecve uses; we
  // call it directly to avoid any dependency on /proc and to match the intended
  // design. Use the raw syscall for portability across glibc versions (the
  // execveat() wrapper was only added in glibc 2.34).
  syscall(SYS_execveat, elf_image_.fd(), "", (char *const *)av,
          (char *const *)ep, AT_EMPTY_PATH);
  // execveat only returns on failure.
  child_fail("execveat failed");
}

void Program::ReadLastResultsAndLastRipOffsetFromElfProcess(
    pid_t elf_pid, unsigned long long rip, unsigned long start_code,
    unsigned long start_data) {
  const SymbolData &symbol_data = elf_image_.symbol_data();

  // Compute in signed arithmetic so that an rip outside main (e.g. before main
  // starts) yields a negative offset rather than a huge unsigned value.
  last_rip_offset_ = (long long)rip - (long long)start_code -
                     (long long)symbol_data.main_offset_in_text_;

  struct iovec local[1];
  struct iovec remote[1];
  ssize_t nread;

  if (symbol_data.results_st_size_ %
          sizeof(decltype(last_results_)::value_type) !=
      0)
    myfail("results_st_size_ mismatch");

  last_results_.resize(symbol_data.results_st_size_ /
                       sizeof(decltype(last_results_)::value_type));
  // printf("last_results_ size: %ld\n", last_results_.size());

  local[0].iov_base = last_results_.data();
  local[0].iov_len = symbol_data.results_st_size_;
  remote[0].iov_base =
      (void *)(start_data + symbol_data.results_offset_in_data_);
  remote[0].iov_len = symbol_data.results_st_size_;

  nread = process_vm_readv(elf_pid, local, 1, remote, 1, 0);
  if (nread != (ssize_t)symbol_data.results_st_size_)
    myfail("process_vm_readv failed");
}

void Program::ClearLastState() {
  last_syscall_ = kInvalidSyscall;
  last_rip_offset_ = kInvalidRipOffset;
  last_exit_status_ = kInvalidExitStatus;
  last_term_signal_ = kInvalidSignal;
  last_stop_signal_ = kInvalidSignal;
  last_results_.clear();
}

} // namespace viaevo
