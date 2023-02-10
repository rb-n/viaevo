// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "scorer_mock.h"

#include <gtest/gtest.h>

namespace {

TEST(ScorerMockTest, Score) {
  viaevo::ScorerMock scorer({7, 17, 0, 5}, 23, {1});

  EXPECT_EQ(scorer.current_inputs(), std::vector<int>{1});

  std::vector<int> results;
  EXPECT_EQ(scorer.Score(results), 7);
  EXPECT_EQ(scorer.Score(results), 17);
  EXPECT_EQ(scorer.Score(results), 0);
  EXPECT_EQ(scorer.Score(results), 5);
  EXPECT_EQ(scorer.Score(results), 7);
  EXPECT_EQ(scorer.Score(results), 17);
  EXPECT_EQ(scorer.Score(results), 0);
  EXPECT_EQ(scorer.Score(results), 5);

  EXPECT_EQ(scorer.MaxScore(), 23);

  // max_score_ smaller than at least one of the elements in scores_.
  EXPECT_DEATH(viaevo::ScorerMock scorer({3, 4, 0, 1}, 2, {}),
               "max_score_ should not be smaller");
}

} // namespace
