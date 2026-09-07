// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#ifndef VIAEVO_EVOLVER_EVOLVER_ADHOC_H_
#define VIAEVO_EVOLVER_EVOLVER_ADHOC_H_

#include <memory>
#include <vector>

#include "evolver.h"
// TODO: Remove relative paths.
#include "../mutator/mutator.h"
#include "../program/program.h"
#include "../scorer/scorer.h"
#include "../util/random.h"

namespace viaevo {

// Evolver manages a population of Programs and rounds of mutation,
// evaluation, and selection. EvolverAdHoc's implementation is inspired by
// Genetic Programming and (mu + lambda) Evolution Strategy (with stochastic
// ranking). The current implementation probably does not match any of these
// exactly. EvolverAdHoc is one concrete strategy behind the abstract Evolver
// interface; further strategies reflecting common approaches in Evolutionary
// Computation can be added as additional Evolver subclasses.
class EvolverAdHoc : public Evolver {
public:
  // cpu_timeout_usec / wall_timeout_usec bound the execution of each program in
  // the population and are forwarded to each Program's Sandbox (see Sandbox for
  // what each bounds and for the defaults).
  EvolverAdHoc(std::string elf_filename, int mu, int phi, int lambda,
               Scorer &scorer, Mutator &mutator, Random &gen,
               int evaluations_per_program, int max_generations,
               bool score_results_history = false,
               std::string output_filename_prefix = "",
               bool initialize_programs_to_all_nops = false,
               long cpu_timeout_usec = Sandbox::kDefaultCpuTimeoutUsec,
               long wall_timeout_usec = Sandbox::kDefaultWallTimeoutUsec);
  // Selects mu_ parents by bringing them to the front of programs_: the top
  // (mu_ - phi_) programs by score (ties broken randomly via a pre-sort
  // shuffle), plus phi_ programs sampled at random from the rest of the
  // population. The random (phi_) sample prefers programs with a positive
  // score; zero-score programs (which includes anything that timed out on every
  // execution - see Scorer::Score returning 0 on SIGALRM) are drawn only when a
  // slot has no positive-scoring candidates left.
  virtual void SelectParents(std::vector<long long> &current_scores);
  // Creates lambda_ offspring in the last lambda_ elements of programs_ from
  // the first mu_ elements (parents), using mutator_.
  virtual void CreateOffspring();
  // Counts of the two timeout kinds the Sandbox can deliver in a generation:
  // SIGPROF (the primary CPU-time bound) and SIGALRM (the wall-clock backstop).
  // See RECOMMENDATIONS.md 12.7.
  struct TimeoutCounts {
    int sigprofs = 0;
    int sigalrms = 0;
  };
  // Resets current_scores to zero, then runs evaluations_per_program_ rounds of
  // (ResetInputs + parallel execute + score), followed by results-history
  // scoring. current_scores must already be sized mu_ + lambda_. Returns the
  // per-kind timeout counts observed across the generation.
  virtual TimeoutCounts EvaluatePrograms(std::vector<long long> &current_scores);
  // Runs the evolution.
  void Run() override;

  const std::vector<std::shared_ptr<Program>> &programs() { return programs_; }
  bool score_results_history() const { return score_results_history_; }

protected:
  // Size of population in each generation (iteration) is (mu_ + lambda_).
  // Number of parents selected in each iteration.
  int mu_ = 30;
  // Number of parents selected randomly in each iterations as opposed to (mu_ -
  // phi_) parents that are selected based on their scores. mu_ >= phi_;
  int phi_ = 5;
  // Number of offspring created from mu_ parents in each iteration.
  int lambda_ = 70;

  // Population of Programs.
  std::vector<std::shared_ptr<Program>> programs_;

  // Scorer used to provide input data and score results in each iteration.
  Scorer &scorer_;
  // Mutator used to create offspring from parents in each iteration.
  Mutator &mutator_;
  // Random number generator.
  Random &gen_;

  // Number of evaluations (with different inputs) a Scorer performs on each
  // program in each generation. Scores are accumulated.
  int evaluations_per_program_ = 1;

  // Current and the maximum number of generations (iterations) for the
  // evolution.
  int current_generation_ = 0;
  int max_generations_ = 10'000;

  // When set to true, score Programs also on their results_history_ after all
  // evaluations in a generation are completed This is intended for multiple
  // executions of the same program on different inputs. The intention is to
  // "reward" programs that produce different results on different inputs to set
  // them apart from programs producing the same result on different input (try
  // to steer away from the "broken clock is right twice a day" phenomenon).
  bool score_results_history_ = false;

  // Prefix to prepend to output file names (e.g. for saved evolved elfs).
  std::string output_filename_prefix_;
};

} // namespace viaevo

#endif // VIAEVO_EVOLVER_EVOLVER_ADHOC_H_