// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "evolver_adhoc.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <execution>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <signal.h>
#include <string>
#include <unordered_map>

namespace viaevo {

EvolverAdHoc::EvolverAdHoc(std::string elf_filename, int mu, int phi,
                           int lambda, Scorer &scorer, Mutator &mutator,
                           Random &gen, int evaluations_per_program,
                           int max_generations, bool score_results_history,
                           std::string output_filename_prefix,
                           bool initialize_programs_to_all_nops)
    : mu_(mu), phi_(phi), lambda_(lambda), scorer_(scorer), mutator_(mutator),
      gen_(gen), evaluations_per_program_(evaluations_per_program),
      max_generations_(max_generations),
      score_results_history_(score_results_history),
      output_filename_prefix_(output_filename_prefix) {
  assert(phi_ >= 0 && mu_ >= phi_);
  for (int i = 0; i < mu_ + lambda_; ++i) {
    auto program = Program::Create(elf_filename);
    if (initialize_programs_to_all_nops) {
      program->SetElfCodeToAllNops();
    }
    programs_.push_back(program);
  }
}

void EvolverAdHoc::SelectParents(std::vector<long long> &scores) {
  std::size_t n = programs_.size();
  assert(n == scores.size());

  std::vector<int> indices(n, 0);
  std::iota(indices.begin(), indices.end(), 0);

  // Break score ties randomly: shuffle the index array (Fisher-Yates driven by
  // gen_ so mocked generators keep unit tests deterministic) before the stable
  // sort below. Without this, ties resolve by index and the current parents
  // (occupying the front of programs_) could never be displaced by
  // equal-scoring offspring, making neutral drift impossible.
  for (std::size_t i = n - 1; i > 0; --i) {
    std::swap(indices[i], indices[gen_() % (i + 1)]);
  }

  // Use stable_sort instead of e.g. nth_element to simplify unit testing.
  std::stable_sort(
      indices.begin(), indices.end(),
      [&scores](int a, int b) -> bool { return scores[a] > scores[b]; });

  // The first (mu_ - phi_) parents are the top scorers. Fill the remaining
  // phi_ parent slots with a random sample of the rest of the population
  // (partial Fisher-Yates) per the "phi parents selected at random" scheme
  // described in the README. The sample prefers programs with a positive score:
  // at each slot the candidate pool is the remaining programs whose score is
  // greater than zero, and only if that pool is empty (every remaining program
  // scored zero) does the slot fall back to the full remainder. This
  // deprioritizes programs that produced nothing useful - including anything
  // that timed out on every execution, which scores zero. When every remaining
  // program scores zero the pool is the full remainder and this reduces to the
  // previous uniform sample (identical RNG use).
  std::vector<std::size_t> candidates;
  for (std::size_t i = mu_ - phi_; i < static_cast<std::size_t>(mu_); ++i) {
    candidates.clear();
    for (std::size_t j = i; j < n; ++j) {
      if (scores[indices[j]] > 0)
        candidates.push_back(j);
    }
    if (candidates.empty()) {
      for (std::size_t j = i; j < n; ++j)
        candidates.push_back(j);
    }
    std::swap(indices[i], indices[candidates[gen_() % candidates.size()]]);
  }

  // Should be possible to do this without the extra space, e.g. cyclic sort.
  std::vector<std::shared_ptr<Program>> aux;
  for (auto i : indices) {
    aux.push_back(programs_[i]);
  }

  for (std::size_t i = 0; i < n; ++i) {
    programs_[i] = aux[i];
  }
}

void EvolverAdHoc::CreateOffspring() {
  for (int i = 0; i < lambda_; ++i) {
    // Note: The same program may be selected as both parent1 and parent2
    // and this is ok for e.g. random recombinations.
    mutator_.Mutate(programs_[mu_ + i], programs_[gen_() % mu_],
                    programs_[gen_() % mu_]);
  }
  // Note: Consider shuffling programs in the vector, would complicate unit
  // testing though.
}

EvolverAdHoc::TimeoutCounts
EvolverAdHoc::EvaluatePrograms(std::vector<long long> &current_scores) {
  std::fill(current_scores.begin(), current_scores.end(), 0);

  // results_history is local to a single generation: it is accumulated across
  // the evaluations below and consumed by ScoreResultsHistory before returning.
  std::vector<std::vector<std::vector<int>>> results_history;
  if (score_results_history_) {
    results_history.resize(mu_ + lambda_);
  }

  // Count timeouts of a long running program (e.g. inf loop) by kind: SIGPROF
  // is the CPU-time bound, SIGALRM the wall-clock backstop (see Sandbox).
  std::atomic<int> sigprofs_count(0), sigalarms_count(0);
  std::vector<int> indices(mu_ + lambda_, 0);
  std::iota(indices.begin(), indices.end(), 0);
  for (int j = 0; j < evaluations_per_program_; ++j) {
    scorer_.ResetInputs();
    std::for_each(
        std::execution::par, indices.begin(), indices.end(),
        [this, &current_scores, &results_history, &sigprofs_count,
         &sigalarms_count](int i) {
          programs_[i]->SetElfInputs(scorer_.current_inputs());
          programs_[i]->Execute();
          long long score = scorer_.Score(*programs_[i]);
          current_scores[i] += score;
          if (score_results_history_) {
            results_history[i].push_back(programs_[i]->last_results());
          }
          if (programs_[i]->last_stop_signal() == SIGPROF) {
            ++sigprofs_count;
          } else if (programs_[i]->last_stop_signal() == SIGALRM) {
            ++sigalarms_count;
          }
        });
  }

  if (score_results_history_) {
    for (int i = 0; i < mu_ + lambda_; ++i) {
      current_scores[i] += scorer_.ScoreResultsHistory(results_history[i]);
    }
  }

  return {sigprofs_count.load(), sigalarms_count.load()};
}

void EvolverAdHoc::Run() {
  std::cout.imbue(std::locale(""));
  long long best_overall_score = 0;
  long long max_score = evaluations_per_program_ * scorer_.MaxScore() +
                        scorer_.MaxScoreResultsHistory();

  // Current scores set by the scorer_ reflect an accumulated performance of
  // programs on recent (sets of) inputs.
  std::vector<long long> current_scores(mu_ + lambda_, 0);

  while (current_generation_ < max_generations_) {
    ++current_generation_;

    // Stage 1 (SelectParents): Bring the mu_ parents to the "front" of
    // programs_.
    SelectParents(current_scores);
    // Stage 2 (CreateOffspring): Create lambda_ offspring in the last lambda_
    // elements of programs_ using the first mu_ elements as parents.
    CreateOffspring();
    // Stage 3 (EvaluatePrograms): Update programs_ inputs, execute and score
    // programs_.
    TimeoutCounts timeouts = EvaluatePrograms(current_scores);

    long long best_generation_score = 0;
    std::unordered_map<long long, int> rip_offset_counts;
    long long top_rip_offset = -1;
    int top_rip_offset_count = 0;
    std::vector<int> best_generation_results;
    int best_generation_program_index = -1;
    for (int i = 0; i < mu_ + lambda_; ++i) {
      if (best_generation_score < current_scores[i]) {
        best_generation_score = current_scores[i];
        best_generation_results = programs_[i]->last_results();
        best_generation_program_index = i;
      }
      long long rip_offset = programs_[i]->last_rip_offset();
      ++rip_offset_counts[rip_offset];
      if (rip_offset_counts[rip_offset] > top_rip_offset_count) {
        top_rip_offset_count = rip_offset_counts[rip_offset];
        top_rip_offset = rip_offset;
      }
    }
    std::cout << "\33[2K\rG: " << std::setw(8) << current_generation_
              << " | best score: " << best_generation_score
              << " (max: " << max_score << ") "
              << " | rip distinct: " << rip_offset_counts.size()
              << " top: " << top_rip_offset
              << " count: " << top_rip_offset_count
              << " sigprofs/alrms: " << timeouts.sigprofs << "/"
              << timeouts.sigalrms << std::flush;
    if (best_overall_score < best_generation_score) {
      best_overall_score = best_generation_score;
      std::cout << "\n            | best last results: ";
      for (auto itm : best_generation_results)
        std::cout << itm << " ";
      std::cout << "\n" << std::flush;

      std::string filename = output_filename_prefix_ + "gen_" +
                             std::to_string(current_generation_) +
                             "_best_program.elf";
      programs_[best_generation_program_index]->SaveElf(filename.c_str());
    }
    if (best_overall_score == max_score) {
      std::cout << "DONE! :)\n";
      std::string best_filename = output_filename_prefix_ + "best_program.elf";
      programs_[best_generation_program_index]->SaveElf(best_filename.c_str());
      break;
    }
  }
  std::cout << "\n";
}

} // namespace viaevo
