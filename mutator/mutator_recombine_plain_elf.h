// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#ifndef VIAEVO_MUTATOR_MUTATOR_RECOMBINE_PLAIN_ELF_H_
#define VIAEVO_MUTATOR_MUTATOR_RECOMBINE_PLAIN_ELF_H_

#include "mutator.h"

// TODO: Remove relative path.
#include "../util/random.h"
#include <memory>

namespace viaevo {

// MutatorRecombinePlainElf creates new code for target based on parent1 and
// and a standard program held by the mutator. The standard program serves as
// parent2 and the parent2 passed to Mutate member function is ignored. Target's
// code is based on parent1's code with a random subarray of standard program's
// code placed in a random position of parent1' code overwriting the
// corresponding part of parent1's code. The insert can be truncated not to
// exceed the code size limit. All positions are "byte-aligned". Parent1 and
// parent2 are not modified.
//
// Evolving programs using other mutators may cause erosion of these programs
// such that most of the machine code may be invalid. This mutator may
// reintroduce valid instructions into these programs.
class MutatorRecombinePlainElf : public Mutator {
public:
  explicit MutatorRecombinePlainElf(
      Random &gen, std::string elf_filename,
      bool initialize_program_to_all_nops = false);
  // Creates new code for target based on the description above.
  virtual void Mutate(std::shared_ptr<Program> target,
                      std::shared_ptr<Program> parent1,
                      std::shared_ptr<Program> parent2) override;

  const std::shared_ptr<Program> standard_program() {
    return standard_program_;
  }

protected:
  // Random number generator.
  Random &gen_;
  // Standard program to serve as parent2.
  std::shared_ptr<Program> standard_program_;
};

} // namespace viaevo

#endif // VIAEVO_MUTATOR_MUTATOR_RECOMBINE_PLAIN_ELF_H_