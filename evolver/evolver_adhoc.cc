// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "evolver_adhoc.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <unordered_map>

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
                           Mutator &mutator, Random &gen,
                           int evaluations_per_program, int max_generations)
    : mu_(mu), phi_(phi), lambda_(lambda), scorer_(scorer), mutator_(mutator),
      gen_(gen), evaluations_per_program_(evaluations_per_program),
      max_generations_(max_generations) {
  for (int i = 0; i < mu_ + lambda_; ++i) {
    programs_.push_back(Program::Create("elfs/simple_small"));
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
  long long best_overall_score = 0;
  long long max_score = evaluations_per_program_ * scorer_.MaxScore();
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
    scorer_.ResetInputs();
    long long best_generation_score = 0;
    std::unordered_map<unsigned long long, int> rip_offset_counts;
    unsigned long long top_rip_offset = -1;
    int top_rip_offset_count = 0;
    std::vector<int> best_generation_results;
    for (int i = 0; i < mu_ + lambda_; ++i) {
      programs_[i]->ResetCurrentScore();
      for (int j = 0; j < evaluations_per_program_; ++j) {
        programs_[i]->SetElfInputs(scorer_.current_inputs());
        programs_[i]->Execute();
        programs_[i]->IncrementCurrentScoreBy(scorer_.Score(*programs_[i]));
      }
      if (best_generation_score < programs_[i]->current_score()) {
        best_generation_score = programs_[i]->current_score();
        best_generation_results = programs_[i]->last_results();
      }
      unsigned long long rip_offset = programs_[i]->last_rip_offset();
      ++rip_offset_counts[rip_offset];
      if (rip_offset_counts[rip_offset] > top_rip_offset_count) {
        top_rip_offset_count = rip_offset_counts[rip_offset];
        top_rip_offset = rip_offset;
      }
    }
    std::cout << "\rG: " << std::setw(8) << current_generation_
              << " | best score: " << best_generation_score << " (overall: "
              << std::max(best_overall_score, best_generation_score) << "/"
              << max_score << ") "
              << " | rip distinct: " << rip_offset_counts.size()
              << " top: " << top_rip_offset
              << " count: " << top_rip_offset_count << std::flush;
    if (best_overall_score < best_generation_score) {
      best_overall_score = best_generation_score;
      std::cout << "\n            | best results: ";
      for (auto itm : best_generation_results)
        std::cout << itm << " ";
      std::cout << "\n";
    }
    if (best_overall_score == max_score) {
      std::cout << "DONE! :)\n";
      break;
    }
  }
}

} // namespace viaevo
