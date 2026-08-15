// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#ifndef VIAEVO_PROGRAM_ELF_LAYOUT_H_
#define VIAEVO_PROGRAM_ELF_LAYOUT_H_

#include <elf.h>

namespace viaevo {

// ELF symbol table values and sizes for the symbols Program cares about (main,
// inputs, results). Fields are initialized to -1 and set to the correct value
// by ResolveElfSymbolData.
struct SymbolData {
  Elf64_Addr main_offset_in_elf_ = -1;  // offset from elf beginning
  Elf64_Addr main_offset_in_text_ = -1; // offset from .text beginning
  uint64_t main_st_size_ = -1;
  Elf64_Addr inputs_offset_in_elf_ = -1; // offset from elf beginning
  uint64_t inputs_st_size_ = -1;
  Elf64_Addr results_offset_in_data_ = -1;
  uint64_t results_st_size_ = -1;
};

// Parses the ELF referenced by the (open, readable) file descriptor fd and
// returns the offsets and sizes of the main, inputs and results symbols. The
// descriptor is not closed; its file offset is repositioned during parsing.
//
// This is pure ELF parsing (no fork/ptrace), factored out of Program so it can
// be unit tested on its own. It currently terminates the process on malformed
// input (see RECOMMENDATIONS.md 2.1 for the planned recoverable-error work).
SymbolData ResolveElfSymbolData(int fd);

} // namespace viaevo

#endif // VIAEVO_PROGRAM_ELF_LAYOUT_H_
