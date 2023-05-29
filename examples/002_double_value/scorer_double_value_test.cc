// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "scorer_double_value.h"

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

TEST(ScorerDoubleValueTest, Score) {
  viaevo::RandomMock gen({21});
  EXPECT_EQ(gen(), 21);
  EXPECT_EQ(gen(), 21);
  EXPECT_EQ(gen(), 21);

  viaevo::ScorerDoubleValue scorer(gen, 5);
  EXPECT_EQ(scorer.current_inputs(), std::vector<int>(5, 21));
  EXPECT_EQ(scorer.expected_value(), 42);
  scorer.ResetInputs();
  EXPECT_EQ(scorer.current_inputs(), std::vector<int>(5, 21));
  EXPECT_EQ(scorer.expected_value(), 42);

  // Results the same as initialized in //elfs/simple_small.c
  std::vector<int> results{20, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

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

  // Setting one of the other results to the correct value.
  results[4] = 42;
  program.set_last_results(results);
  EXPECT_EQ(scorer.Score(program), 64);
  // EXPECT_EQ(scorer.Score(program), 1'000'000'000);

  results[4] = 0;
  program.set_last_results(results);
  EXPECT_EQ(scorer.Score(program), 4);

  results[1] = 1;
  program.set_last_results(results);
  EXPECT_EQ(scorer.Score(program), 48);
  // EXPECT_EQ(scorer.Score(program), 1'000);

  // Other multiple of the correct value.
  results[1] = 3*42;
  program.set_last_results(results);
  EXPECT_EQ(scorer.Score(program), 49);
  // EXPECT_EQ(scorer.Score(program), 1'000'000);

  results[1] = 42;
  program.set_last_results(results);
  EXPECT_EQ(scorer.Score(program), 112);
  EXPECT_EQ(scorer.Score(program), scorer.MaxScore());
  // EXPECT_EQ(scorer.Score(program), 1'000'000'000'000);

  // Walking back changes in the other elements does not impact the score when
  // results[1] changed.
  results[2] = 0;
  program.set_last_results(results);
  EXPECT_EQ(scorer.Score(program), 112);
  EXPECT_EQ(scorer.Score(program), scorer.MaxScore());
  // EXPECT_EQ(scorer.Score(program), 1'000'000'000'000);

  results[5] = 0;
  results[7] = 3;
  program.set_last_results(results);
  EXPECT_EQ(scorer.Score(program), 112);
  EXPECT_EQ(scorer.Score(program), scorer.MaxScore());
  // EXPECT_EQ(scorer.Score(program), 1'000'000'000'000);
}

TEST(ScorerDoubleValueTest, MaxScore) {
  viaevo::RandomMock gen({21});
  EXPECT_EQ(gen(), 21);
  EXPECT_EQ(gen(), 21);
  EXPECT_EQ(gen(), 21);
  viaevo::ScorerDoubleValue scorer(gen, 1);
  EXPECT_EQ(scorer.MaxScore(), 112);
  // EXPECT_EQ(scorer.MaxScore(), 1'000'000'000'000);
  scorer.ResetInputs();
  EXPECT_EQ(scorer.MaxScore(), 112);
  // EXPECT_EQ(scorer.MaxScore(), 1'000'000'000'000);
}

} // namespace
