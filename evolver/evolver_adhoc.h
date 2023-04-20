// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#ifndef VIAEVO_EVOLVER_EVOLVER_ADHOC_H_
#define VIAEVO_EVOLVER_EVOLVER_ADHOC_H_

#include <memory>
#include <vector>

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
// exactly. Eventually, it would be worthwhile to create an abstract base class
// Evolver specifying a generic interface and a set of concrete derived classes
// reflecting common approaches in Evolutionary Computation.
class EvolverAdHoc {
public:
  EvolverAdHoc(std::string elf_filename, int mu, int phi, int lambda,
               Scorer &scorer, Mutator &mutator, Random &gen,
               int evaluations_per_program, int max_generations,
               bool score_results_history = false,
               std::string output_filename_prefix = "");
  // Selects mu_ parents by bringing them to the front of programs_.
  virtual void SelectParents();
  // Runs the evolution.
  virtual void Run();

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
  // evaluations in a generation are completed.
  bool score_results_history_ = false;

  // Prefix to prepend to output file names (e.g. for saved evolved elfs).
  std::string output_filename_prefix_;
};

} // namespace viaevo

#endif // VIAEVO_EVOLVER_EVOLVER_ADHOC_H_