// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "mutator_composite_random.h"

namespace viaevo {

MutatorCompositeRandom::MutatorCompositeRandom(Random &gen) : gen_(gen) {}

std::shared_ptr<Mutator> MutatorCompositeRandom::GetMutator() {
  // TODO: Add myfail when mutators_ are empty. Also update the message in the
  // test's EXPECT_DEATH.
  return mutators_[gen_() % mutators_.size()];
}

} // namespace viaevo
