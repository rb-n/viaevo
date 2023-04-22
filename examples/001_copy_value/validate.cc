// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "scorer_copy_value.h"

#include <iomanip>
#include <iostream>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"

// TODO: Remove relative paths.
#include "../../util/random.h"

ABSL_FLAG(std::string, elf_filename,
          "examples/001_copy_value/evolved_elfs/best_program.elf",
          "filename of the evolved ELF program to validate");
// Each evaluation is expected to produce the same value in results[1]. Multiple
// evaluations per program help evaluate if the evolved program is
// non-deterministic and produces (in)correct results in each evaluation.
ABSL_FLAG(
    int32_t, num_value_copies_in_inputs, 10,
    "number of copies of the value to be copied in inputs (the first n items "
    "of inputs are filled with this value, use the same value here as in the "
    "evolution)");
ABSL_FLAG(
    uint32_t, random_seed, 1,
    "random seed for the validation (values to copy a selected at random)");
ABSL_FLAG(int32_t, num_evaluations, 20,
          "number of evaluations to be performed on the evolved ELF program");

int main(int argc, char **argv) {
  absl::SetProgramUsageMessage(
      "This program evaluates ELF programs that were evoloved to copy a value "
      "from inputs and place the value in the results[1] global "
      "variable.\nWARNING: The evolution produces invalid executables, always "
      "run this program in a sandbox!\nSample usage via the bazel build system "
      "(with 'build --spawn_strategy=linux-sandbox' in .bazelrc):\nbazel run "
      "//examples/001_copy_value:validate -- --num_value_copies_in_inputs=10 "
      "--elf_filename=best_program.elf");

  absl::ParseCommandLine(argc, argv);

  std::string elf_filename = absl::GetFlag(FLAGS_elf_filename);
  unsigned int random_seed = absl::GetFlag(FLAGS_random_seed);
  int num_value_copies_in_inputs =
      absl::GetFlag(FLAGS_num_value_copies_in_inputs);
  int num_evaluations = absl::GetFlag(FLAGS_num_evaluations);

  std::cout << "# elf_filename: " << elf_filename << "\n";
  std::cout << "# random_seed: " << random_seed << "\n";
  std::cout << "# num_value_copies_in_inputs: " << num_value_copies_in_inputs
            << "\n";
  std::cout << "# num_evaluations: " << num_evaluations << "\n";

  viaevo::Random gen;
  gen.Seed(random_seed);

  std::shared_ptr<viaevo::Program> program =
      viaevo::Program::Create(elf_filename);

  viaevo::ScorerCopyValue scorer(gen, num_value_copies_in_inputs);

  std::cout << std::setw(16) << "expected" << std::setw(11) << "result[1]"
            << " | " << std::setw(5)
            << "score | results                   | signals\n";

  int correct = 0, total = 0;
  for (int i = 0; i < num_evaluations; ++i) {
    scorer.ResetInputs();
    program->SetElfInputs(scorer.current_inputs());
    program->Execute();
    auto results = program->last_results();
    int input = scorer.current_inputs()[0];
    int result = results[1];
    long long score = scorer.Score(*program);

    ++total;
    if (result == input) {
      std::cout << "OK!";
      ++correct;
    } else {
      std::cout << "---";
    }
    std::cout << " " << std::setw(11) << input << " " << std::setw(11) << result
              << " | " << std::setw(5) << score << " | ";
    for (auto itm : results) {
      std::cout << itm << " ";
    }
    std::cout << " | " << program->last_stop_signal() << " "
              << program->last_term_signal();
    std::cout << std::endl;
  }

  std::cout << "SUCCESS RATE: " << std::fixed << std::setprecision(2)
            << 100 * ((double)correct) / total << "%" << std::endl;

  return 0;
}