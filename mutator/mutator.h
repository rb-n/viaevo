// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#ifndef VIAEVO_MUTATOR_MUTATOR_H_
#define VIAEVO_MUTATOR_MUTATOR_H_

#include <memory>

// TODO: Remove relative path.
#include "../program/program.h"

namespace viaevo {

// Mutator is an abstract base class defining the interface to change Program's
// evolvable code.
class Mutator {
public:
  virtual ~Mutator() = default;
  // Creates new code for target based on parent1 or both parent1 and parent2.
  virtual void Mutate(std::shared_ptr<Program> target,
                      std::shared_ptr<Program> parent1,
                      std::shared_ptr<Program> parent2) = 0;
};

} // namespace viaevo

#endif // VIAEVO_MUTATOR_MUTATOR_H_