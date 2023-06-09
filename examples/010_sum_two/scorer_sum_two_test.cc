// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "scorer_sum_two.h"

#include <gtest/gtest.h>

// TODO: Remove relative path.
#include "../../util/random_mock.h"

namespace {

class ProgramMock : public viaevo::Program {
public:
  void set_last_results(const std::vector<int> &results) {
    last_results_ = results;
  }
};

TEST(ScorerSumTwoTest, Score) {
  viaevo::RandomMock gen({28, 56});
  EXPECT_EQ(gen(), 28);
  EXPECT_EQ(gen(), 56);
  EXPECT_EQ(gen(), 28);
  EXPECT_EQ(gen(), 56);

  viaevo::ScorerSumTwo scorer(gen, 2);
  // The generated random values are divided by two as a hack to "prevent
  // overflow/underflow for the sum".
  std::vector<int> expected_inputs{14, 28, 14, 28};
  EXPECT_EQ(scorer.current_inputs(), expected_inputs);
  EXPECT_EQ(scorer.expected_value(), 42);
  scorer.ResetInputs();
  EXPECT_EQ(scorer.current_inputs(), expected_inputs);
  EXPECT_EQ(scorer.expected_value(), 42);

  // Results the same as initialized in //elfs/simple_small.c
  std::vector<int> results{20, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3};

  ProgramMock program;

  program.set_last_results(results);
  EXPECT_EQ(scorer.Score(program), 0);

  // Changing elements other than results[1] should increments score by 1.
  results[5] = 5;
  program.set_last_results(results);
  EXPECT_EQ(scorer.Score(program), 1);

  results[2] = 1;
  program.set_last_results(results);
  EXPECT_EQ(scorer.Score(program), 2);

  results[7] = 0;
  program.set_last_results(results);
  EXPECT_EQ(scorer.Score(program), 3);

  // Setting one of the other results to the correct value should add
  // additonal 60.
  results[4] = 42;
  program.set_last_results(results);
  EXPECT_EQ(scorer.Score(program), 64);

  results[4] = 0;
  program.set_last_results(results);
  EXPECT_EQ(scorer.Score(program), 3);

  // Changing results[1] disregards other elements and scores only based on
  // results[1].
  results[1] = 1;
  program.set_last_results(results);
  EXPECT_EQ(scorer.Score(program), 48);

  results[1] = 1024;
  program.set_last_results(results);
  EXPECT_EQ(scorer.Score(program), 48);

  results[1] = 42;
  program.set_last_results(results);
  EXPECT_EQ(scorer.Score(program), 112);

  // Walking back changes in the other elements does not impact the score when
  // results[1] changed.
  results[2] = 0;
  program.set_last_results(results);
  EXPECT_EQ(scorer.Score(program), 112);

  results[5] = 0;
  results[7] = 3;
  program.set_last_results(results);
  EXPECT_EQ(scorer.Score(program), 112);
}

TEST(ScorerSumTwoTest, MaxScore) {
  viaevo::RandomMock gen({14, 42});
  EXPECT_EQ(gen(), 14);
  EXPECT_EQ(gen(), 42);
  EXPECT_EQ(gen(), 14);
  EXPECT_EQ(gen(), 42);

  viaevo::ScorerSumTwo scorer(gen, 1);
  // Max score should be 112 (20 for changing results[1] and +1 for each
  // correctly set bit in results[1] and +60 for at least one results element
  // equalling the correct value).
  EXPECT_EQ(scorer.MaxScore(), 112);
  scorer.ResetInputs();
  EXPECT_EQ(scorer.MaxScore(), 112);
}

} // namespace
