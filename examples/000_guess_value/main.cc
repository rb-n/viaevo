// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "scorer_guess_value.h"

#include <iostream>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"

// TODO: Remove relative paths.
#include "../../evolver/evolver_adhoc.h"
#include "../../mutator/mutator_composite_random.h"
#include "../../mutator/mutator_point_last_instruction.h"
#include "../../mutator/mutator_point_random.h"
#include "../../mutator/mutator_recombine_random.h"
#include "../../util/random.h"

ABSL_FLAG(int32_t, value_to_guess, 42,
          "value to be 'guessed' by the evolved program and placed into the "
          "results[1] global variable");
ABSL_FLAG(std::string, elf_filename, "elfs/simple_small",
          "filename of the ELF executable to be used as the starting template "
          "for evolution (NOTE: results are expected to be initialized to {10, "
          "0, 0, 0, 0, 0, 3, 3, 3, 3, 3})");
ABSL_FLAG(
    int32_t, mu, 60,
    "number of parents selected in each generation to generate lambda "
    "offspring (the size of the population in each generation is mu + lambda)");
ABSL_FLAG(int32_t, phi, 10,
          "number of parents selected randomly in each generation as opposed "
          "to (mu - phi) parents that are selected based on their scores (mu "
          ">= phi)");
ABSL_FLAG(int32_t, lambda, 140,
          "number of offspring to generate from mu parents in each generation "
          "(the size of the population in each generation is mu + lambda)");
// Each evaluation is expected to produce the same value in results[1]. Multiple
// evaluations per program help "penalize" programs that are non-deterministic
// and produce different results in each evaluation.
ABSL_FLAG(int32_t, evaluations_per_program, 10,
          "number of evaluations to be performed on each program in each "
          "generation (scores are accumulated across evaluations)");
ABSL_FLAG(int32_t, max_generations, 10000,
          "maximum number of generations for the evolution");
ABSL_FLAG(
    bool, score_results_history, false,
    "track and score the set of evolved program's results across evaluations "
    "within a generation (not applicable for 000_guess_value)");
ABSL_FLAG(
    std::string, output_filename_prefix, "",
    "prefix to prepend to output file names (e.g. for saved evolved elfs)");
ABSL_FLAG(uint32_t, random_seed, 1,
          "random seed for the evolution (NOTE: repeated evolutions with the "
          "same random seed may diverge if the evolved programs compute "
          "results non-deterministically)");
ABSL_FLAG(bool, initialize_programs_to_all_nops, false,
          "set all instructions in the evolvable code of the template ELF "
          "executable to nop prior to starting the evolution");

int main(int argc, char **argv) {
  absl::SetProgramUsageMessage(
      "This program evolves ELFs to 'guess' a value and place the value in the "
      "results[1] global variable of the evolved program.\n\nWARNING: The "
      "evolution produces invalid executables. To protect your system, always "
      "run this program in a sandbox!\n\nSample usage via the bazel build "
      "system (with 'build --spawn_strategy=linux-sandbox' in "
      ".bazelrc):\n\nbazel run //examples/000_guess_value:main -- "
      "--value_to_guess=1009");

  absl::ParseCommandLine(argc, argv);

  int value_to_guess = absl::GetFlag(FLAGS_value_to_guess);
  std::string elf_filename = absl::GetFlag(FLAGS_elf_filename);
  int mu = absl::GetFlag(FLAGS_mu);
  int phi = absl::GetFlag(FLAGS_phi);
  int lambda = absl::GetFlag(FLAGS_lambda);
  int evaluations_per_program = absl::GetFlag(FLAGS_evaluations_per_program);
  int max_generations = absl::GetFlag(FLAGS_max_generations);
  bool score_results_history = absl::GetFlag(FLAGS_score_results_history);
  std::string output_filename_prefix =
      absl::GetFlag(FLAGS_output_filename_prefix);
  unsigned int random_seed = absl::GetFlag(FLAGS_random_seed);
  bool initialize_programs_to_all_nops =
      absl::GetFlag(FLAGS_initialize_programs_to_all_nops);

  std::cout << "# value_to_guess: " << value_to_guess << "\n";
  std::cout << "# elf_filename: " << elf_filename << "\n";
  std::cout << "# mu: " << mu << "\n";
  std::cout << "# phi: " << phi << "\n";
  std::cout << "# lambda: " << lambda << "\n";
  std::cout << "# evaluations_per_program: " << evaluations_per_program << "\n";
  std::cout << "# max_generations: " << max_generations << "\n";
  std::cout << "# score_results_history: " << std::boolalpha
            << score_results_history << "\n";
  std::cout << "# output_filename_prefix: " << output_filename_prefix << "\n";
  std::cout << "# random_seed: " << random_seed << "\n";
  std::cout << "# initialize_programs_to_all_nops: " << std::boolalpha
            << initialize_programs_to_all_nops << "\n";

  viaevo::Random gen;
  gen.Seed(random_seed);

  std::shared_ptr<viaevo::MutatorPointRandom> mutator_point =
      std::make_shared<viaevo::MutatorPointRandom>(gen);
  std::shared_ptr<viaevo::MutatorRecombineRandom> mutator_recombine =
      std::make_shared<viaevo::MutatorRecombineRandom>(gen);
  std::shared_ptr<viaevo::MutatorPointLastInstruction>
      mutator_last_instruction =
          std::make_shared<viaevo::MutatorPointLastInstruction>(gen);

  viaevo::MutatorCompositeRandom mutator_composite(gen);
  mutator_composite.AppendMutator(mutator_point);
  mutator_composite.AppendMutator(mutator_recombine);
  mutator_composite.AppendMutator(mutator_last_instruction);

  // viaevo::ScorerGuessValue scorer(42);
  // viaevo::ScorerGuessValue scorer(0xFFFFFFFF);
  // viaevo::ScorerGuessValue scorer(63'451'913);
  viaevo::ScorerGuessValue scorer(value_to_guess);

  viaevo::EvolverAdHoc evolver(
      elf_filename, mu, phi, lambda, scorer, mutator_composite, gen,
      evaluations_per_program, max_generations, score_results_history,
      output_filename_prefix, initialize_programs_to_all_nops);
  evolver.Run();

  return 0;
}