// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "program.h"

#include <errno.h>
#include <fcntl.h>
#include <memory>
#include <seccomp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

namespace viaevo {

namespace {

void myfail(const char *s) {
  perror(s);
  exit(EXIT_FAILURE);
}

} // namespace

int Program::expected_ptrace_stops_simple_small_ = -1;

Program::Program(const char *filename) { SetupElfInMemory(filename); }

Program::~Program() {
  if (elf_mem_fd_ != -1)
    close(elf_mem_fd_);
}

std::shared_ptr<Program> Program::CreateSimpleSmall() {
  std::shared_ptr<Program> p = std::make_shared<Program>("elfs/simple_small");
  if (expected_ptrace_stops_simple_small_ == -1) {
    // The expected number of ptrace stops prior to executing is the full number
    // minus one (minus the final exit syscall).
    expected_ptrace_stops_simple_small_ = p->Execute() - 1;
    p->expected_ptrace_stops_ = expected_ptrace_stops_simple_small_;
    p->ClearLastState();
  }
  return p;
}

void Program::SetupElfInMemory(const char *filename) {
  int fd_from;
  char buffer[4096];
  ssize_t nread;

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

  // Based on https://stackoverflow.com/a/2180788
  while (nread = read(fd_from, buffer, sizeof buffer), nread > 0) {
    char *out_ptr = buffer;
    ssize_t nwritten;

    do {
      nwritten = write(elf_mem_fd_, out_ptr, nread);

      if (nwritten >= 0) {
        nread -= nwritten;
        out_ptr += nwritten;
      } else if (errno != EINTR) {
        myfail("write failed");
      }
    } while (nread > 0);
  }

  close(fd_from);
}

int Program::Execute(int max_ptrace_stops) {
  pid_t pid;

  pid = fork();

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

  ClearLastState();

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
      last_signal_ = WTERMSIG(status);
      // printf("killed by signal %d\n", last_signal_);
    } else if (WIFSTOPPED(status)) {
      ++ptrace_stops_count;
      last_signal_ = WSTOPSIG(status);
      // printf("%4d stopped by signal %d", ptrace_stops_count, last_signal_);

      if (ptrace(PTRACE_GETREGS, elf_pid, 0, &regs) == -1)
        myfail("PTRACE_GETREGS failed");

      last_syscall_ = regs.orig_rax;

      // printf(", syscall: %lld\n", last_syscall_);

      if (ptrace_stops_count >= max_ptrace_stops && max_ptrace_stops != -1) {
        // We are done with the elf process after the initial expected
        // syscalls. Any additional syscall or signal is assumed to originate
        // from the program's (evolved) code and ends the process. The (result)
        // data are evaluated at this point. The child process is killed.
        // ExploreProcess(elf_pid);
        if (kill(elf_pid, SIGKILL) == -1)
          myfail("kill failed");
        // getchar();
      } else {
        if (last_signal_ != 5) {
          // E.g. SISGSEGV for and invalid program.
          if (ptrace(PTRACE_CONT, elf_pid, 0, last_signal_) == -1)
            myfail("PTRACE_CONT failed");

        } else {
          if (ptrace(PTRACE_SYSCALL, elf_pid, 0, 0) == -1)
            myfail("PTRACE_SYSCALL failed");
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
  // "Ask for a SIGALRM" to be delivered to the child process in 1 second.
  // This should cause a termination of the process if e.g. an infinite loop is
  // present.
  // NOTE: A shorter time interval may be more approriate.
  alarm(1);

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

void Program::ClearLastState() {
  last_syscall_ = kInvalidSyscall;
  last_exit_status_ = kInvalidExitStatus;
  last_signal_ = kInvalidSignal;
  last_results_.clear();
}

} // namespace viaevo