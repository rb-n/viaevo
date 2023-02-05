// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#ifndef VIAEVO_MUTATOR_MUTATOR_RECOMBINE_RANDOM_H_
#define VIAEVO_MUTATOR_MUTATOR_RECOMBINE_RANDOM_H_

#include "mutator.h"

// TODO: Remove relative path.
#include "../util/random.h"

namespace viaevo {

// MutatorRecombineRandom creates new code for target based on parent1 and
// parent2. Target's code is based on parent1's code with a random subarray of
// parent2's code placed in a random position of parent1' code overwriting the
// corresponding part of parent1's code. The insert can be truncated not to
// exceed the code size limit. All positions are "byte-aligned". Parent1 and
// parent2 are not modified.
class MutatorRecombineRandom : public Mutator {
public:
  explicit MutatorRecombineRandom(Random &gen);
  // Creates new code for target based on the description above.
  virtual void Mutate(std::shared_ptr<Program> target,
                      std::shared_ptr<Program> parent1,
                      std::shared_ptr<Program> parent2) override;

protected:
  // Random number generator.
  Random &gen_;
};

} // namespace viaevo

#endif // VIAEVO_MUTATOR_MUTATOR_RECOMBINE_RANDOM_H_