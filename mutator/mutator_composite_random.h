// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#ifndef VIAEVO_MUTATOR_MUTATOR_COMPOSITE_RANDOM_H_
#define VIAEVO_MUTATOR_MUTATOR_COMPOSITE_RANDOM_H_

#include "mutator_composite.h"

#include <memory>

#include "../util/random.h"

namespace viaevo {

// MutatorCompositeRandom will select a mutator from mutators_ at random with
// equal probability.
class MutatorCompositeRandom : public MutatorComposite {
public:
  MutatorCompositeRandom(Random &gen);

protected:
  // Random number genrator.
  Random &gen_;

  // Selects a mutator from mutators_ at random with equal probability.
  virtual std::shared_ptr<Mutator> GetMutator();
};

} // namespace viaevo

#endif // VIAEVO_MUTATOR_MUTATOR_COMPOSITE_RANDOM_H_