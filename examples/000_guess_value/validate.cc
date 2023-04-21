// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "scorer_guess_value.h"

#include <iomanip>
#include <iostream>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"

ABSL_FLAG(int32_t, value_to_guess, 42,
          "the correct value expected in the results[1] global variable after "
          "the execution of the evolved program");
ABSL_FLAG(std::string, elf_filename,
          "examples/000_guess_value/evolved_elfs/best_program.elf",
          "filename of the evolved ELF program to validate");
// Each evaluation is expected to produce the same value in results[1]. Multiple
// evaluations per program help evaluate if the evolved program is
// non-deterministic and produces different results in each evaluation.
ABSL_FLAG(int32_t, num_evaluations, 20,
          "number of evaluations to be performed on the evolved ELF program");

int main(int argc, char **argv) {
  absl::SetProgramUsageMessage(
      "This program evaluates ELF programs that were evoloved to 'guess' a "
      "value and place the value in the results[1] global variable.\nNOTE: The "
      "evolution produces invalid executables, always run this program in a "
      "sandbox!\nSample usage via the bazel build system (with 'build "
      "--spawn_strategy=linux-sandbox' in .bazelrc):\nbazel run "
      "//examples/000_guess_value:validate -- --value_to_guess=1009 "
      "--elf_filename=best_program.elf");

  absl::ParseCommandLine(argc, argv);

  int value_to_guess = absl::GetFlag(FLAGS_value_to_guess);
  std::string elf_filename = absl::GetFlag(FLAGS_elf_filename);
  int num_evaluations = absl::GetFlag(FLAGS_num_evaluations);

  std::cout << "# value_to_guess: " << value_to_guess << "\n";
  std::cout << "# elf_filename: " << elf_filename << "\n";
  std::cout << "# num_evaluations: " << num_evaluations << "\n";

  std::shared_ptr<viaevo::Program> program =
      viaevo::Program::Create(elf_filename);

  viaevo::ScorerGuessValue scorer(value_to_guess);

  std::cout << std::setw(16) << "expected" << std::setw(11) << "result[1]"
            << " | " << std::setw(5)
            << "score | results                   | signals\n";

  int correct = 0, total = 0;
  for (int i = 0; i < num_evaluations; ++i) {
    scorer.ResetInputs();
    program->SetElfInputs(scorer.current_inputs());
    program->Execute();
    auto results = program->last_results();
    int expected = scorer.value();
    int result = results[1];
    long long score = scorer.Score(*program);

    ++total;
    if (result == expected) {
      std::cout << "OK!";
      ++correct;
    } else {
      std::cout << "---";
    }
    std::cout << " " << std::setw(11) << expected << " " << std::setw(11)
              << result << " | " << std::setw(5) << score << " | ";
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