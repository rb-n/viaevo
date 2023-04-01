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

  viaevo::Program program;
  EXPECT_EQ(scorer.Score(program), 7);
  EXPECT_EQ(scorer.Score(program), 17);
  EXPECT_EQ(scorer.Score(program), 0);
  EXPECT_EQ(scorer.Score(program), 5);
  EXPECT_EQ(scorer.Score(program), 7);
  EXPECT_EQ(scorer.Score(program), 17);
  EXPECT_EQ(scorer.Score(program), 0);
  EXPECT_EQ(scorer.Score(program), 5);

  EXPECT_EQ(scorer.MaxScore(), 23);

  // max_score_ smaller than at least one of the elements in scores_.
  EXPECT_DEATH(viaevo::ScorerMock scorer({3, 4, 0, 1}, 2, {}),
               "max_score_ should not be smaller");
}

TEST(ScorerMockTest, ScoreResultsHistory) {
  viaevo::ScorerMock scorer({1}, 23, {91}, {7, 17, 0, 5});

  EXPECT_EQ(scorer.current_inputs(), std::vector<int>{91});

  std::vector<int> results;

  viaevo::Program program;
  EXPECT_EQ(scorer.ScoreResultsHistory(program.results_history()), 7);
  EXPECT_EQ(scorer.ScoreResultsHistory(program.results_history()), 17);
  EXPECT_EQ(scorer.ScoreResultsHistory(program.results_history()), 0);
  EXPECT_EQ(scorer.ScoreResultsHistory(program.results_history()), 5);
  EXPECT_EQ(scorer.ScoreResultsHistory(program.results_history()), 7);
  EXPECT_EQ(scorer.ScoreResultsHistory(program.results_history()), 17);
  EXPECT_EQ(scorer.ScoreResultsHistory(program.results_history()), 0);
  EXPECT_EQ(scorer.ScoreResultsHistory(program.results_history()), 5);

  EXPECT_EQ(scorer.MaxScore(), 23);

  // max_score_ smaller than the sum of max element in scores_ and max element
  // in results_history_scores_.
  EXPECT_DEATH(viaevo::ScorerMock scorer({3, 4, 0, 1}, 7, {}, {0, 5}),
               "max_score_ should not be smaller");
}

} // namespace
