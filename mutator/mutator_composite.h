// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#ifndef VIAEVO_MUTATOR_MUTATOR_COMPOSITE_H_
#define VIAEVO_MUTATOR_MUTATOR_COMPOSITE_H_

#include "mutator.h"

#include <memory>

namespace viaevo {

// MutatorComposite is an abstract base class for composite mutators. The
// virtual function GetMutator is intended to define the behavior of derived
// classes in terms of selecting a specific a mutator from mutators_. This
// mutator is then used in the Mutate function.
class MutatorComposite : public Mutator {
public:
  virtual void Mutate(std::shared_ptr<Program> target,
                      std::shared_ptr<Program> parent1,
                      std::shared_ptr<Program> parent2) override;

  // Appends a mutator to mutators_.
  void AppendMutator(std::shared_ptr<Mutator> mutator);

  // Clears mutators_.
  void Clear();

protected:
  std::vector<std::shared_ptr<Mutator>> mutators_;

  // The virtual function GetMutator is intended to define the behavior of
  // derived classes in terms of selecting a specific a mutator from mutators_.
  virtual std::shared_ptr<Mutator> GetMutator() = 0;
};

} // namespace viaevo

#endif // VIAEVO_MUTATOR_MUTATOR_COMPOSITE_H_