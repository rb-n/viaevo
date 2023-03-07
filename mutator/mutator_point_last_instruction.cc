// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "mutator_point_last_instruction.h"
#include "mutator_point_random.h"
#include <algorithm>

namespace viaevo {

MutatorPointLastInstruction::MutatorPointLastInstruction(Random &gen)
    : MutatorPointRandom(gen) {}

void MutatorPointLastInstruction::Mutate(std::shared_ptr<Program> target,
                                         std::shared_ptr<Program> parent1,
                                         std::shared_ptr<Program> parent2) {
  auto last_rip_offset = parent1->last_rip_offset();
  std::vector<char> code = parent1->GetElfCode();

  // If program1's last instruction offset is outside the mutable code, revert
  // to MutatorPointRandom behavior (random bit flip anywhere in the mutable
  // code).
  if (last_rip_offset >= code.size() * sizeof(decltype(code)::value_type) ||
      last_rip_offset < 0) {
    MutatorPointRandom::Mutate(target, parent1, parent2);
    return;
  }

  auto random_number = gen_();
  auto element_size_in_bits = 8 * sizeof(decltype(code)::value_type);
  auto bits_left = code.size() * element_size_in_bits - last_rip_offset * 8;
  auto pos =
      last_rip_offset * 8 +
      random_number % std::min(kMaxInstructionSizeInBytes * 8, bits_left);
  auto index = pos / element_size_in_bits;
  auto bit_pos = pos % element_size_in_bits;
  code[index] ^= (1 << bit_pos);

  target->SetElfCode(code);
}

} // namespace viaevo
