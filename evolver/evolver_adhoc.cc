// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "evolver_adhoc.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <memory>

namespace viaevo {

namespace {
class ProgramCompare {
public:
  bool operator()(const std::shared_ptr<Program> &lhs,
                  const std::shared_ptr<Program> &rhs) const {
    return lhs->current_score() > rhs->current_score();
  }
};
} // namespace

EvolverAdHoc::EvolverAdHoc(int mu, int phi, int lambda, Scorer &scorer,
                           Mutator &mutator, Random &gen, int max_generations)
    : mu_(mu), phi_(phi), lambda_(lambda), scorer_(scorer), mutator_(mutator),
      gen_(gen), max_generations_(max_generations) {
  for (int i = 0; i < mu_ + lambda_; ++i) {
    programs_.push_back(Program::CreateSimpleSmall());
  }
}

void EvolverAdHoc::SelectParents() {
  // Shuffle programs_ to prevent breaking ties the same way in each
  // generation.
  // TODO: Provide own random number generator.
  std::random_shuffle(programs_.begin(), programs_.end());
  // Bring the (mu_ - phi_) parents (selected on score) to the "front" of
  // programs_.
  std::nth_element(programs_.begin(), programs_.begin() + (mu_ - phi_) - 1,
                   programs_.end(), ProgramCompare());
  // Shuffle the elements after (mu_ - phi_) to obtain the remaining phi_
  // parents at random.
  // TODO: Provide own random number generator.
  std::random_shuffle(programs_.begin() + (mu_ - phi_), programs_.end());
}

// TODO: Put the three stages in individual member functions and add unit
// tests for those.
void EvolverAdHoc::Run() {
  while (current_generation_ < max_generations_) {
    ++current_generation_;

    // Stage 1 (SelectParents): Bring the mu_ parents to the "front" of
    // programs_.
    // --------------------------------------------------------------
    SelectParents();
    // Stage 2 (CreateOffspring): Create lambda_ offspring in the last lambda_
    // elements of programs_ using the first mu_ elements of programs as
    // parents.
    // --------------------------------------------------------------------
    for (int i = 0; i < lambda_; ++i) {
      // Note: The same program may be selected as both parent1 and parent2
      // and this is ok for e.g. random recombinations.
      mutator_.Mutate(programs_[mu_ + i], programs_[gen_() % mu_],
                      programs_[gen_() % mu_]);
    }

    // Stage 3 (EvaluatePrograms): Update programs_ inputs, execute and score
    // programs_.
    // -----------------------------------------
    // TODO: Enable multiple evaluation rounds (new
    // inputs/execution/evaluation) per generation.
    long long best_score = 0;
    scorer_.ResetInputs();
    for (int i = 0; i < mu_ + lambda_; ++i) {
      programs_[i]->ResetCurrentScore();
      programs_[i]->SetElfInputs(scorer_.current_inputs());
      programs_[i]->Execute();
      programs_[i]->IncrementCurrentScoreBy(
          scorer_.Score(programs_[i]->last_results()));
      best_score = std::max(best_score, programs_[i]->current_score());
    }
    if (best_score > 0) {
      std::cout << "\nG: " << std::setw(8) << current_generation_
                << " best score: " << best_score << "\n"
                << std::flush;
    }

    std::cout << "\rG: " << std::setw(8) << current_generation_
              << " best score: " << best_score << std::flush;
    if (current_generation_ % 10000 == 0) {
      std::cout << "\n";
    }
  }
}

} // namespace viaevo
