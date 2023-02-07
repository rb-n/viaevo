// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "mutator_composite.h"

namespace viaevo {

void MutatorComposite::Mutate(std::shared_ptr<Program> target,
                              std::shared_ptr<Program> parent1,
                              std::shared_ptr<Program> parent2) {
  GetMutator()->Mutate(target, parent1, parent2);
}

void MutatorComposite::AppendMutator(std::shared_ptr<Mutator> mutator) {
  mutators_.push_back(mutator);
}

void MutatorComposite::Clear() {
  mutators_.clear();
}

} // namespace viaevo
