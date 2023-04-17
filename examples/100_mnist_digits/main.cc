// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "scorer_mnist_digits.h"

// TODO: Remove relative paths.
#include "../../evolver/evolver_adhoc.h"
#include "../../mutator/mutator_composite_random.h"
#include "../../mutator/mutator_point_last_instruction.h"
#include "../../mutator/mutator_point_random.h"
#include "../../mutator/mutator_recombine_plain_elf.h"
#include "../../mutator/mutator_recombine_random.h"
#include "../../util/random.h"

int main() {
  viaevo::Random gen;
  gen.Seed(1);

  std::string elf_filename("elfs/simple_medium");

  std::shared_ptr<viaevo::MutatorPointRandom> mutator_point =
      std::make_shared<viaevo::MutatorPointRandom>(gen);
  std::shared_ptr<viaevo::MutatorRecombineRandom> mutator_recombine =
      std::make_shared<viaevo::MutatorRecombineRandom>(gen);
  std::shared_ptr<viaevo::MutatorRecombinePlainElf>
      mutator_recombine_plain_elf =
          std::make_shared<viaevo::MutatorRecombinePlainElf>(
              gen, elf_filename);
  std::shared_ptr<viaevo::MutatorPointLastInstruction>
      mutator_last_instruction =
          std::make_shared<viaevo::MutatorPointLastInstruction>(gen);

  viaevo::MutatorCompositeRandom mutator_composite(gen);
  mutator_composite.AppendMutator(mutator_point);
  mutator_composite.AppendMutator(mutator_recombine);
  mutator_composite.AppendMutator(mutator_recombine_plain_elf);
  // Naive "implementation of a weighted random composite mutator - just append
  // a mutator mutiple times."
  // TODO: Implement a weighted random composite mutator.
  for (int i = 0; i < 7; ++i) {
    mutator_composite.AppendMutator(mutator_last_instruction);
  }

  viaevo::ScorerMnistDigits scorer(
      gen, "examples/100_mnist_digits/data/train-images-idx3-ubyte",
      "examples/100_mnist_digits/data/train-labels-idx1-ubyte");

  viaevo::EvolverAdHoc evolver(elf_filename, 60, 10, 140, scorer,
                               mutator_composite, gen, 200, 1000000, true);
  evolver.Run();

  return 0;
}