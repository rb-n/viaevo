// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "scorer_guess_value.h"

#include <gtest/gtest.h>

namespace {

TEST(ScorerGuessValueTest, FailOnValue0) {
  EXPECT_DEATH(viaevo::ScorerGuessValue(0), "Value should not be 0.");
}

TEST(ScorerGuessValueTest, Score) {
  viaevo::ScorerGuessValue scorer1(1);
  viaevo::ScorerGuessValue scorer42(42); // 0b101010
  viaevo::ScorerGuessValue scorer1024(1024);

  // Results the same as initialized in //elfs/simple_small.c
  std::vector<int> results{10, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3};

  EXPECT_EQ(scorer1.Score(results), 0);
  EXPECT_EQ(scorer42.Score(results), 0);
  EXPECT_EQ(scorer1024.Score(results), 0);

  // Changing elements other than results[1] should increments score by 1.
  results[0] = 5;
  EXPECT_EQ(scorer1.Score(results), 1);
  EXPECT_EQ(scorer42.Score(results), 1);
  EXPECT_EQ(scorer1024.Score(results), 1);

  results[2] = 1;
  EXPECT_EQ(scorer1.Score(results), 2);
  EXPECT_EQ(scorer42.Score(results), 2);
  EXPECT_EQ(scorer1024.Score(results), 2);

  results[7] = 0;
  EXPECT_EQ(scorer1.Score(results), 3);
  EXPECT_EQ(scorer42.Score(results), 3);
  EXPECT_EQ(scorer1024.Score(results), 3);

  // Changing results[1] disregards other elements and scores only based on
  // results[1].
  results[1] = 1;
  EXPECT_EQ(scorer1.Score(results), 52);
  EXPECT_EQ(scorer42.Score(results), 48);
  EXPECT_EQ(scorer1024.Score(results), 50);

  results[1] = 1024;
  EXPECT_EQ(scorer1.Score(results), 50);
  EXPECT_EQ(scorer42.Score(results), 48);
  EXPECT_EQ(scorer1024.Score(results), 52);

  results[1] = 42;
  EXPECT_EQ(scorer1.Score(results), 48);
  EXPECT_EQ(scorer42.Score(results), 52);
  EXPECT_EQ(scorer1024.Score(results), 48);

  // Walking back changes in the other elements does not impact the score when
  // results[1] changed.
  results[2] = 0;
  EXPECT_EQ(scorer1.Score(results), 48);
  EXPECT_EQ(scorer42.Score(results), 52);
  EXPECT_EQ(scorer1024.Score(results), 48);

  results[0] = 10;
  results[7] = 3;
  EXPECT_EQ(scorer1.Score(results), 48);
  EXPECT_EQ(scorer42.Score(results), 52);
  EXPECT_EQ(scorer1024.Score(results), 48);
}

TEST(ScorerGuessValueTest, MaxScore) {
  viaevo::ScorerGuessValue scorer1(42);
  // Max score should be 52 (20 for changing results[1] and +1 for each
  // correctly set bit in results[1]).
  EXPECT_EQ(scorer1.MaxScore(), 52);

  viaevo::ScorerGuessValue scorer2(1'000'000'007);
  EXPECT_EQ(scorer2.MaxScore(), 52);

  viaevo::ScorerGuessValue scorer3(1);
  EXPECT_EQ(scorer3.MaxScore(), 52);
}

} // namespace
