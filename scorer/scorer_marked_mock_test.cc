// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "scorer_marked_mock.h"

#include <gtest/gtest.h>

namespace {

TEST(ScorerMarkedMockTest, Score) {
  viaevo::ScorerMarkedMock scorer({{0xA0, 7}}, 23, {1}, {0});

  EXPECT_EQ(scorer.current_inputs(), std::vector<int>{1});

  std::vector<int> results;

  auto program = viaevo::Program::Create("elfs/simple_small");
  // "Mark" the program.
  std::vector<char> code = program->GetElfCode();
  code[100] = 0xA0;
  program->SetElfCode(code);

  auto program_no_mark = viaevo::Program::Create("elfs/simple_small");

  EXPECT_EQ(scorer.Score(*program), 7);
  EXPECT_EQ(scorer.Score(*program), 7);
  EXPECT_EQ(scorer.Score(*program_no_mark), 0);
  EXPECT_EQ(scorer.Score(*program_no_mark), 0);
  EXPECT_EQ(scorer.Score(*program), 7);
  EXPECT_EQ(scorer.Score(*program_no_mark), 0);

  EXPECT_EQ(scorer.MaxScore(), 23);

  // max_score_ smaller than at least one of the elements in scores_.
  EXPECT_DEATH(viaevo::ScorerMarkedMock scorer({{'a', 4}}, 2, {}, {1}),
               "max_score_ should not be smaller");
}

TEST(ScorerMarkedMockTest, ScoreResultsHistory) {
  viaevo::ScorerMarkedMock scorer({{0xA0, 1}}, 23, {91}, {17, 11});

  EXPECT_EQ(scorer.current_inputs(), std::vector<int>{91});

  EXPECT_EQ(scorer.ScoreResultsHistory({}), 17);
  EXPECT_EQ(scorer.ScoreResultsHistory({}), 11);
  EXPECT_EQ(scorer.ScoreResultsHistory({}), 17);
  EXPECT_EQ(scorer.ScoreResultsHistory({}), 11);
  EXPECT_EQ(scorer.ScoreResultsHistory({}), 17);
  EXPECT_EQ(scorer.ScoreResultsHistory({}), 11);

  EXPECT_EQ(scorer.MaxScore(), 23);

  // max_score_ smaller than the sum of max element in scores_ and max element
  // in results_history_scores_.
  EXPECT_DEATH(viaevo::ScorerMarkedMock scorer({{'a', 4}}, 7, {}, {0, 5}),
               "max_score_ should not be smaller");
}

} // namespace
