// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#ifndef VIAEVO_MUTATOR_MUTATOR_POINT_LAST_INSTRUCTION_H_
#define VIAEVO_MUTATOR_MUTATOR_POINT_LAST_INSTRUCTION_H_

#include "mutator_point_random.h"

namespace viaevo {

// MutatorPointLastInstruction creates new code for target based on parent1
// (parent2 is ignored). Target's code will have a single bit flip in a random
// location compared to parent1's code. The location of the bit flip will be in
// the last instruction (parent1's last_rip_offset). The code of parent1 is not
// changed.
class MutatorPointLastInstruction : public MutatorPointRandom {
public:
  explicit MutatorPointLastInstruction(Random &gen);
  // Creates new code for target based on parent1 with an added bit flip in
  // target's code in a random location.
  virtual void Mutate(std::shared_ptr<Program> target,
                      std::shared_ptr<Program> parent1,
                      std::shared_ptr<Program> parent2) override;

protected:
  // Maximum instruction size for x86-64 should be 15 bytes.
  static constexpr unsigned long long kMaxInstructionSizeInBytes = 15;
};

} // namespace viaevo

#endif // VIAEVO_MUTATOR_MUTATOR_POINT_LAST_INSTRUCTION_H_