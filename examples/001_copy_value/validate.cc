// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "scorer_copy_value.h"

#include <iomanip>
#include <iostream>

// TODO: Remove relative paths.
#include "../../util/random.h"

int main() {
  viaevo::Random gen;
  gen.Seed(1);

  std::string filename =
      "examples/001_copy_value/evolved_elfs/best_program.elf";

  std::shared_ptr<viaevo::Program> program = viaevo::Program::Create(filename);

  viaevo::ScorerCopyValue scorer(gen, 50);

  int correct = 0, total = 0;
  for (int i = 0; i < 100; ++i) {
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
              << " | " << score << " | ";
    for (auto itm : results) {
      std::cout << itm << " ";
    }
    std::cout << std::endl;
  }

  std::cout << "SUCCESS RATE: " << std::fixed << std::setprecision(2)
            << 100 * ((double)correct) / total << "%" << std::endl;

  return 0;
}