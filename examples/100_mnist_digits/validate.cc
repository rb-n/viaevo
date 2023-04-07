// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "scorer_mnist_digits.h"

#include <iomanip>
#include <iostream>

// TODO: Remove relative paths.
#include "../../util/random.h"

int main() {
  viaevo::Random gen;
  gen.Seed(1);

  std::string filename =
      "examples/100_mnist_digits/evolved_elfs/v7_gen_1230_best_program.elf";

  std::shared_ptr<viaevo::Program> program = viaevo::Program::Create(filename);

  // TODO: Validate on the test data set.
  viaevo::ScorerMnistDigits scorer(
      gen, "examples/100_mnist_digits/data/train-images-idx3-ubyte",
      "examples/100_mnist_digits/data/train-labels-idx1-ubyte");

  int correct = 0, total = 0;
  for (int i = 0; i < 100; ++i) {
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
              << " | " << std::setw(5) << score << " | ";
    for (auto itm : results) {
      std::cout << itm << " ";
    }
    std::cout << " | " << program->last_stop_signal() << " " << program->last_term_signal();
    std::cout << std::endl;
  }

  std::cout << "SUCCESS RATE: " << std::fixed << std::setprecision(2)
            << 100 * ((double)correct) / total << "%" << std::endl;

  return 0;
}