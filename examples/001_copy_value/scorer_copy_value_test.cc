// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "scorer_copy_value.h"

#include <gtest/gtest.h>

// TODO: Remove relative path.
#include "../../util/random_mock.h"

namespace {

TEST(ScorerGuessValueTest, Score) {
  viaevo::RandomMock gen({42});
  EXPECT_EQ(gen(), 42);
  EXPECT_EQ(gen(), 42);
  EXPECT_EQ(gen(), 42);

  viaevo::ScorerCopyValue scorer(gen, 5);
  EXPECT_EQ(scorer.current_inputs(), std::vector<int>(5, 42));
  scorer.ResetInputs();
  EXPECT_EQ(scorer.current_inputs(), std::vector<int>(5, 42));

  // Results the same as initialized in //elfs/simple_small.c
  std::vector<int> results{20, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3};

  EXPECT_EQ(scorer.Score(results), 0);

  // Changing elements other than results[1] should increments score by 1.
  results[5] = 5;
  EXPECT_EQ(scorer.Score(results), 1);

  results[2] = 1;
  EXPECT_EQ(scorer.Score(results), 2);

  results[7] = 0;
  EXPECT_EQ(scorer.Score(results), 3);

  // Setting one of the other results to the correct value should add
  // additonal 60.
  results[4] = 42;
  EXPECT_EQ(scorer.Score(results), 64);

  results[4] = 0;
  EXPECT_EQ(scorer.Score(results), 3);

  // Changing results[1] disregards other elements and scores only based on
  // results[1].
  results[1] = 1;
  EXPECT_EQ(scorer.Score(results), 48);

  results[1] = 1024;
  EXPECT_EQ(scorer.Score(results), 48);

  results[1] = 42;
  EXPECT_EQ(scorer.Score(results), 112);

  // Walking back changes in the other elements does not impact the score when
  // results[1] changed.
  results[2] = 0;
  EXPECT_EQ(scorer.Score(results), 112);

  results[5] = 0;
  results[7] = 3;
  EXPECT_EQ(scorer.Score(results), 112);
}

TEST(ScorerCopyValueTest, MaxScore) {
  viaevo::RandomMock gen({42});
  EXPECT_EQ(gen(), 42);
  EXPECT_EQ(gen(), 42);
  EXPECT_EQ(gen(), 42);
  viaevo::ScorerCopyValue scorer(gen, 1);
  // Max score should be 112 (20 for changing results[1] and +1 for each
  // correctly set bit in results[1] and +60 for at least one results element
  // equalling the correct value).
  EXPECT_EQ(scorer.MaxScore(), 112);
  scorer.ResetInputs();
  EXPECT_EQ(scorer.MaxScore(), 112);
}

} // namespace
