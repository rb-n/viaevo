// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "evolver_adhoc.h"

#include <gtest/gtest.h>

// TODO: Remove relative path.
#include "../mutator/mutator_point_random.h"
#include "../program/program.h"
#include "../scorer/scorer_mock.h"
#include "../util/random_mock.h"

namespace {

TEST(EvolverAdHocTest, RunSelectParents) {
  viaevo::RandomMock gen({7, 17});

  viaevo::MutatorPointRandom mutator(gen);

  // results_history_scores_ should be ignored by the evolver by default.
  viaevo::ScorerMock scorer({0, 0, 5}, 10, {}, {0, 1, 2});
  std::vector<int> results;

  viaevo::EvolverAdHoc evolver("elfs/simple_small", 2, 1, 1, scorer, mutator,
                               gen, 1, 1);

  EXPECT_EQ(evolver.score_results_history(), false)
      << "score_results_history_ should be false in Evolver by default.";

  auto &programs = evolver.programs();

  EXPECT_EQ(programs[0]->track_results_history(), false);

  EXPECT_EQ(programs.size(), 3); // mu + lambda

  EXPECT_EQ(programs[0]->current_score(), 0);
  EXPECT_EQ(programs[1]->current_score(), 0);
  EXPECT_EQ(programs[2]->current_score(), 0);

  evolver.Run();

  EXPECT_EQ(programs[0]->current_score(), 0);
  EXPECT_EQ(programs[1]->current_score(), 0);
  EXPECT_EQ(programs[2]->current_score(), 5);

  evolver.SelectParents();

  EXPECT_EQ(programs[0]->current_score(), 5);
  EXPECT_EQ(programs[1]->current_score(), 0);
  EXPECT_EQ(programs[2]->current_score(), 0);
}

TEST(EvolverAdHocTest, ScoreResultsHistory) {
  viaevo::RandomMock gen({7, 17});

  viaevo::MutatorPointRandom mutator(gen);

  viaevo::ScorerMock scorer({0, 0, 5}, 10, {}, {0, 1, 2});
  std::vector<int> results;

  viaevo::EvolverAdHoc evolver("elfs/simple_small", 2, 0, 1, scorer, mutator,
                               gen, 1, 1, true);

  EXPECT_EQ(evolver.score_results_history(), true);

  auto &programs = evolver.programs();

  EXPECT_EQ(programs[0]->track_results_history(), true);

  EXPECT_EQ(programs.size(), 3); // mu + lambda

  EXPECT_EQ(programs[0]->current_score(), 0);
  EXPECT_EQ(programs[1]->current_score(), 0);
  EXPECT_EQ(programs[2]->current_score(), 0);

  evolver.Run();

  EXPECT_EQ(programs[0]->current_score(), 0);
  EXPECT_EQ(programs[1]->current_score(), 1);
  EXPECT_EQ(programs[2]->current_score(), 7);

  evolver.SelectParents();

  EXPECT_EQ(programs[0]->current_score(), 7);
  EXPECT_EQ(programs[1]->current_score(), 1);
  EXPECT_EQ(programs[2]->current_score(), 0);
}

} // namespace
