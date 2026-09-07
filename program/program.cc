// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "program.h"

#include <utility>

namespace viaevo {

std::shared_ptr<Program> Program::Create(const std::string &filename,
                                        long cpu_timeout_usec,
                                        long wall_timeout_usec) {
  return std::make_shared<Program>(filename.c_str(), cpu_timeout_usec,
                                   wall_timeout_usec);
}

int Program::Execute(ExecuteMode mode) {
  ExecutionResult result = sandbox_.Execute(elf_image_, mode);
  last_syscall_ = result.last_syscall;
  last_rip_offset_ = result.last_rip_offset;
  last_exit_status_ = result.last_exit_status;
  last_term_signal_ = result.last_term_signal;
  last_stop_signal_ = result.last_stop_signal;
  last_results_ = std::move(result.last_results);
  return result.ptrace_stops;
}

} // namespace viaevo
