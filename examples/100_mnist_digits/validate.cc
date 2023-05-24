// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "scorer_mnist_digits.h"

#include <iomanip>
#include <iostream>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"

// TODO: Remove relative paths.
#include "../../util/random.h"

ABSL_FLAG(std::string, elf_filename,
          "examples/100_mnist_digits/evolved_elfs/"
          "no_9_labels_intermediate_medium_digits_rs_1_gen_2606_best_program.elf",
          "filename of the evolved ELF program to validate");
ABSL_FLAG(
    uint32_t, random_seed, 1,
    "random seed for the validation (values to copy a selected at random)");
ABSL_FLAG(int32_t, num_evaluations, 1000,
          "number of evaluations to be performed on the evolved ELF program");

int main(int argc, char **argv) {
  absl::SetProgramUsageMessage(
      "This program evaluates ELF programs that were evoloved to recognize handwritten digits and place the digit value in the results[1] global "
      "variable.\nWARNING: The evolution produces invalid executables, always "
      "run this program in a sandbox!\nSample usage via the bazel build system "
      "(with 'build --spawn_strategy=linux-sandbox' in .bazelrc):\nbazel run "
      "//examples/100_mnist_digits:validate -- --elf_filename=best_program.elf");

  absl::ParseCommandLine(argc, argv);

  std::string elf_filename = absl::GetFlag(FLAGS_elf_filename);
  unsigned int random_seed = absl::GetFlag(FLAGS_random_seed);
  int num_evaluations = absl::GetFlag(FLAGS_num_evaluations);

  std::cout << "# elf_filename: " << elf_filename << "\n";
  std::cout << "# random_seed: " << random_seed << "\n";
  std::cout << "# num_evaluations: " << num_evaluations << "\n";

  viaevo::Random gen;
  gen.Seed(random_seed);

  // std::string filename =
  //     "examples/100_mnist_digits/evolved_elfs/v5_gen_7152_best_program.elf";
  // std::string filename =
  //     "examples/100_mnist_digits/evolved_elfs/"
  //     "intermediate_medium_digits_rs_1_gen_2606_best_program.elf";
  // "intermediate_medium_digits_rs_1_gen_1757_best_program.elf";

  // Consistently over 10% success rate (?).
  // std::string filename =
  //     "examples/100_mnist_digits/evolved_elfs/v7_gen_11192_best_program.elf";
  // std::string filename =
  //     "examples/100_mnist_digits/evolved_elfs/v7_gen_10575_best_program.elf";
  // std::string filename =
  //     "examples/100_mnist_digits/evolved_elfs/v7_gen_10167_best_program.elf";

  std::shared_ptr<viaevo::Program> program = viaevo::Program::Create(elf_filename);

  // TODO: Validate on the test data set.
  viaevo::ScorerMnistDigits scorer(
      gen, "examples/100_mnist_digits/data/train-images-idx3-ubyte",
      "examples/100_mnist_digits/data/train-labels-idx1-ubyte");

  std::cout << std::setw(16) << "expected" << std::setw(11) << "result[1]"
            << " | " << std::setw(11)
            << "score " << " | results                   | signals\n";

  int correct = 0, total = 0;
  for (int i = 0; i < num_evaluations; ++i) {
    scorer.ResetInputs();
    program->SetElfInputs(scorer.current_inputs());
    program->Execute();
    auto results = program->last_results();
    int input = scorer.expected_value();
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
              << " | " << std::setw(11) << score << " | ";
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