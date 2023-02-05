// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "mutator_point_random.h"

namespace viaevo {

MutatorPointRandom::MutatorPointRandom(Random &gen) : gen_(gen) {}

void MutatorPointRandom::Mutate(std::shared_ptr<Program> target,
                                std::shared_ptr<Program> parent1,
                                std::shared_ptr<Program> parent2) {
  std::vector<char> code = parent1->GetElfCode();

  auto random_number = gen_();
  auto element_size = 8 * sizeof(decltype(code)::value_type);
  auto pos = random_number % (code.size() * element_size);
  auto index = pos / element_size;
  auto bit_pos = pos % element_size;
  code[index] ^= (1 << bit_pos);

  target->SetElfCode(code);
}

} // namespace viaevo
