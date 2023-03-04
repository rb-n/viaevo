// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "scorer_guess_value.h"

// TODO: Remove relative paths.
#include "../../evolver/evolver_adhoc.h"
#include "../../mutator/mutator_composite_random.h"
#include "../../mutator/mutator_point_random.h"
#include "../../mutator/mutator_recombine_random.h"
#include "../../util/random.h"

int main() {
  viaevo::Random gen;
  gen.Seed(1);

  std::shared_ptr<viaevo::MutatorPointRandom> mutator_point =
      std::make_shared<viaevo::MutatorPointRandom>(gen);
  std::shared_ptr<viaevo::MutatorRecombineRandom> mutator_recombine =
      std::make_shared<viaevo::MutatorRecombineRandom>(gen);

  viaevo::MutatorCompositeRandom mutator_composite(gen);
  mutator_composite.AppendMutator(mutator_point);
  mutator_composite.AppendMutator(mutator_recombine);

  // viaevo::ScorerGuessValue scorer(42);
  // viaevo::ScorerGuessValue scorer(0xFFFFFFFF);
  viaevo::ScorerGuessValue scorer(63'451'913);

  viaevo::EvolverAdHoc evolver(60, 10, 140, scorer, mutator_composite, gen, 5,
                               1000000);
  evolver.Run();

  return 0;
}