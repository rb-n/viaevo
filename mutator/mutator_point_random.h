// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#ifndef VIAEVO_MUTATOR_MUTATOR_POINT_RANDOM_H_
#define VIAEVO_MUTATOR_MUTATOR_POINT_RANDOM_H_

#include "mutator.h"

// TODO: Remove relative path.
#include "../util/random.h"

namespace viaevo {

// MutatorPointRandom creates new code for target based on parent1 (parent2 is
// ignored). Target's code will have a single bit flip in a random location
// compared to parent1's code.
class MutatorPointRandom : public Mutator {
public:
  explicit MutatorPointRandom(Random &gen);
  // Creates new code for target based on parent1 with an added bit flip in
  // target's code in a random location.
  virtual void Mutate(std::shared_ptr<Program> target,
                      std::shared_ptr<Program> parent1,
                      std::shared_ptr<Program> parent2) override;

protected:
  // Random number generator.
  Random &gen_;
};

} // namespace viaevo

#endif // VIAEVO_MUTATOR_MUTATOR_POINT_RANDOM_H_