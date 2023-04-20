// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "mutator_recombine_plain_elf.h"

#include <algorithm>
#include <memory>

namespace viaevo {

MutatorRecombinePlainElf::MutatorRecombinePlainElf(
    Random &gen, std::string elf_filename, bool initialize_program_to_all_nops)
    : gen_(gen), standard_program_(Program::Create(elf_filename)) {
  if (initialize_program_to_all_nops) {
    standard_program_->SetElfCodeToAllNops();
  }
}

void MutatorRecombinePlainElf::Mutate(std::shared_ptr<Program> target,
                                      std::shared_ptr<Program> parent1,
                                      std::shared_ptr<Program> parent2) {
  std::vector<char> code1 = parent1->GetElfCode();
  std::vector<char> code2 = standard_program_->GetElfCode();

  auto code2_start = gen_() % code2.size();
  auto code2_size = gen_() % (code2.size() - code2_start);

  auto code1_position = gen_() % code1.size();

  code2_size = std::min(code2_size, code1.size() - code1_position);

  for (unsigned long i = 0; i < code2_size; ++i) {
    code1[code1_position + i] = code2[code2_start + i];
  }

  target->SetElfCode(code1);
}

} // namespace viaevo
