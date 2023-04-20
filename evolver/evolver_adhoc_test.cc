// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "evolver_adhoc.h"

#include <algorithm>

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

  for (int i = 0; i < 3; ++i) {
    std::vector<char> code = programs[i]->GetElfCode();
    EXPECT_TRUE(code.size() > 0);
    // Code should not be all nop by default.
    EXPECT_FALSE(std::all_of(code.begin(), code.end(),
                             [](char value) { return value == '\x90'; }));
  }

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

  EXPECT_EQ(programs[0]->results_history().size(), 1);

  evolver.SelectParents();

  EXPECT_EQ(programs[0]->current_score(), 7);
  EXPECT_EQ(programs[1]->current_score(), 1);
  EXPECT_EQ(programs[2]->current_score(), 0);

  viaevo::EvolverAdHoc evolver_3_generations("elfs/simple_small", 2, 0, 1,
                                             scorer, mutator, gen, 1, 3, true);

  evolver_3_generations.Run();

  EXPECT_EQ(evolver_3_generations.programs()[0]->results_history().size(), 1)
      << "Program's results history should be of size one after three "
         "generations of one evaluation per generation - results from the one "
         "evaluation from the last generation only.";

  viaevo::EvolverAdHoc evolver_3_generations_2_evaluations(
      "elfs/simple_small", 2, 0, 1, scorer, mutator, gen, 2, 3, true);

  evolver_3_generations_2_evaluations.Run();

  EXPECT_EQ(evolver_3_generations_2_evaluations.programs()[0]
                ->results_history()
                .size(),
            2)
      << "Program's results history should be of size two after three "
         "generations of two evaluation per generation - results from the two "
         "evaluation from the last generation only.";
}

TEST(EvolverAdHocTest, InitializeProgramsToAllNops) {
  viaevo::RandomMock gen({7, 17});

  viaevo::MutatorPointRandom mutator(gen);

  // results_history_scores_ should be ignored by the evolver by default.
  viaevo::ScorerMock scorer({0, 0, 5}, 10, {}, {0, 1, 2});
  std::vector<int> results;

  viaevo::EvolverAdHoc evolver("elfs/simple_small", 2, 1, 1, scorer, mutator,
                               gen, 1, 1, false, "", true);

  auto &programs = evolver.programs();

  for (int i = 0; i < 3; ++i) {
    std::vector<char> code = programs[i]->GetElfCode();
    EXPECT_TRUE(code.size() > 0);
    // Code expected to be all nop here.
    EXPECT_TRUE(std::all_of(code.begin(), code.end(),
                            [](char value) { return value == '\x90'; }));
  }
}

} // namespace
