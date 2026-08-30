// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "evolver_adhoc.h"

#include <algorithm>

#include <gtest/gtest.h>

// TODO: Remove relative path.
#include "../mutator/mutator_point_random.h"
#include "../scorer/scorer_marked_mock.h"
#include "../scorer/scorer_mock.h"
#include "../util/random_mock.h"

namespace {

TEST(EvolverAdHocTest, SelectParents) {
  viaevo::RandomMock gen({7, 17});

  viaevo::MutatorPointRandom mutator(gen);

  std::vector<long long> scores{0, 2, 0, 5, 0};
  // results_history_scores_ should be ignored by the evolver by default.
  viaevo::ScorerMarkedMock scorer({}, 20, {}, {0, 1, 2, 3, 9});
  std::vector<int> results;

  viaevo::EvolverAdHoc evolver("elfs/simple_small", 3, 1, 2, scorer, mutator,
                               gen, 1, 1);

  EXPECT_EQ(evolver.score_results_history(), false)
      << "score_results_history_ should be false in Evolver by default.";

  auto &programs = evolver.programs();

  for (int i = 0; i < 5; ++i) {
    std::vector<char> code = programs[i]->GetElfCode();
    EXPECT_TRUE(code.size() > 0);
    // Code should not be all nop by default.
    EXPECT_FALSE(std::all_of(code.begin(), code.end(),
                             [](char value) { return value == '\x90'; }));
    // "Mark" programs so that it is possible to tell which is which after the
    // mocked scoring.
    code[100] = 0xA0 + i;
    programs[i]->SetElfCode(code);
  }

  EXPECT_EQ(programs.size(), 5); // mu + lambda

  EXPECT_EQ(programs[0]->GetElfCode()[100], '\xA0');
  EXPECT_EQ(programs[1]->GetElfCode()[100], '\xA1');
  EXPECT_EQ(programs[2]->GetElfCode()[100], '\xA2');
  EXPECT_EQ(programs[3]->GetElfCode()[100], '\xA3');
  EXPECT_EQ(programs[4]->GetElfCode()[100], '\xA4');

  evolver.SelectParents(scores);

  // SelectParents consumes the mocked values {7, 17} cyclically: first the
  // tie-breaking Fisher-Yates shuffle of the index array {0,1,2,3,4}
  // (i=4: 7%5=2 swaps 4<->2; i=3: 17%4=1 swaps 3<->1; i=2: 7%3=1 swaps 2<->1;
  // i=1: 17%2=1 is a no-op) yielding {0,4,3,1,2}. The stable sort by score
  // then gives {3,1,0,4,2}. Finally the single (phi = 1) random parent slot
  // (index 2 = mu - phi) is filled by sampling from the remainder
  // (7%3=1 swaps index 2 with index 3), yielding {3,1,4,0,2}.
  // First two programs are the ones with score 5 and 2.
  EXPECT_EQ(programs[0]->GetElfCode()[100], '\xA3');
  EXPECT_EQ(programs[1]->GetElfCode()[100], '\xA1');
  // The third parent slot is the randomly sampled (phi) parent.
  EXPECT_EQ(programs[2]->GetElfCode()[100], '\xA4');
  // The remaining programs are not parents.
  EXPECT_EQ(programs[3]->GetElfCode()[100], '\xA0');
  EXPECT_EQ(programs[4]->GetElfCode()[100], '\xA2');
}

TEST(EvolverAdHocTest, SelectParentsIdentityShuffle) {
  // Mocked values {4, 3, 2, 1, 0} make both the tie-breaking shuffle
  // (i: gen % (i + 1) == i for i = 4..1) and the phi sampling
  // (gen % (n - i) == 0 at i = 2) identity operations, so the result is a pure
  // stable sort by score - the pre-phi behavior.
  viaevo::RandomMock gen({4, 3, 2, 1, 0});

  viaevo::MutatorPointRandom mutator(gen);

  std::vector<long long> scores{0, 2, 0, 5, 0};
  viaevo::ScorerMarkedMock scorer({}, 20, {}, {0, 1, 2, 3, 9});

  viaevo::EvolverAdHoc evolver("elfs/simple_small", 3, 1, 2, scorer, mutator,
                               gen, 1, 1);

  auto &programs = evolver.programs();

  for (int i = 0; i < 5; ++i) {
    std::vector<char> code = programs[i]->GetElfCode();
    code[100] = 0xA0 + i;
    programs[i]->SetElfCode(code);
  }

  evolver.SelectParents(scores);

  // First two programs are the ones with score 5 and 2, the remaining ones are
  // stable sorted (all have a score of 0).
  EXPECT_EQ(programs[0]->GetElfCode()[100], '\xA3');
  EXPECT_EQ(programs[1]->GetElfCode()[100], '\xA1');
  EXPECT_EQ(programs[2]->GetElfCode()[100], '\xA0');
  EXPECT_EQ(programs[3]->GetElfCode()[100], '\xA2');
  EXPECT_EQ(programs[4]->GetElfCode()[100], '\xA4');
}

TEST(EvolverAdHocTest, SelectParentsPhiPicksRandomParent) {
  // Identity shuffle ({4, 3, 2, 1, ...} as above), then the phi slot samples
  // the last-ranked program: at i = 2 (mu - phi), gen % (n - i) = 2 % 3 = 2
  // swaps index 2 with index 4.
  viaevo::RandomMock gen({4, 3, 2, 1, 2});

  viaevo::MutatorPointRandom mutator(gen);

  std::vector<long long> scores{0, 2, 0, 5, 0};
  viaevo::ScorerMarkedMock scorer({}, 20, {}, {0, 1, 2, 3, 9});

  viaevo::EvolverAdHoc evolver("elfs/simple_small", 3, 1, 2, scorer, mutator,
                               gen, 1, 1);

  auto &programs = evolver.programs();

  for (int i = 0; i < 5; ++i) {
    std::vector<char> code = programs[i]->GetElfCode();
    code[100] = 0xA0 + i;
    programs[i]->SetElfCode(code);
  }

  evolver.SelectParents(scores);

  // Stable sort gives {3,1,0,2,4}; the phi slot then picks the zero-scoring
  // program 4 as the random parent even though program 0 outranks it.
  EXPECT_EQ(programs[0]->GetElfCode()[100], '\xA3');
  EXPECT_EQ(programs[1]->GetElfCode()[100], '\xA1');
  EXPECT_EQ(programs[2]->GetElfCode()[100], '\xA4');
  EXPECT_EQ(programs[3]->GetElfCode()[100], '\xA2');
  EXPECT_EQ(programs[4]->GetElfCode()[100], '\xA0');
}

TEST(EvolverAdHocTest, SelectParentsPhiPrefersPositiveScore) {
  // Identity shuffle ({4,3,2,1,...}), scores {0,5,3,8,1}. The stable sort by
  // score gives {3,1,2,4,0}, so the top two parents are programs 3 and 1 and
  // the phi pool (i = 2) is {2, 4, 0} with scores {3, 1, 0}. The phi slot must
  // draw only from the positive-scoring candidates {2, 4}; with the 5th mocked
  // value 2 (2 % 2 == 0) it picks program 2 - whereas a plain uniform sample
  // over the whole pool (2 % 3 == 2) would have picked the zero-scoring program
  // 0.
  viaevo::RandomMock gen({4, 3, 2, 1, 2});

  viaevo::MutatorPointRandom mutator(gen);

  std::vector<long long> scores{0, 5, 3, 8, 1};
  viaevo::ScorerMarkedMock scorer({}, 20, {}, {0, 1, 2, 3, 9});

  viaevo::EvolverAdHoc evolver("elfs/simple_small", 3, 1, 2, scorer, mutator,
                               gen, 1, 1);

  auto &programs = evolver.programs();

  for (int i = 0; i < 5; ++i) {
    std::vector<char> code = programs[i]->GetElfCode();
    code[100] = 0xA0 + i;
    programs[i]->SetElfCode(code);
  }

  evolver.SelectParents(scores);

  EXPECT_EQ(programs[0]->GetElfCode()[100], '\xA3');
  EXPECT_EQ(programs[1]->GetElfCode()[100], '\xA1');
  // The phi parent skips the zero-scoring program 0 for a positive scorer.
  EXPECT_EQ(programs[2]->GetElfCode()[100], '\xA2');
}

TEST(EvolverAdHocTest, SelectParentsPhiFallsBackToZeroScoreWhenNoOtherCandidates) {
  // If every candidate for the phi slot scored zero, there is no positive
  // scorer to draw and the slot falls back to sampling the full remainder - so
  // a zero-scoring program can still become a parent when nothing else is
  // available (this is exactly SelectParentsPhiPicksRandomParent). The stable
  // sort gives {3,1,0,2,4}; the phi pool (programs 0, 2, 4) all scored zero, so
  // the 5th mocked value 2 (2 % 3 == 2) selects program 4.
  viaevo::RandomMock gen({4, 3, 2, 1, 2});

  viaevo::MutatorPointRandom mutator(gen);

  std::vector<long long> scores{0, 2, 0, 5, 0};
  viaevo::ScorerMarkedMock scorer({}, 20, {}, {0, 1, 2, 3, 9});

  viaevo::EvolverAdHoc evolver("elfs/simple_small", 3, 1, 2, scorer, mutator,
                               gen, 1, 1);

  auto &programs = evolver.programs();

  for (int i = 0; i < 5; ++i) {
    std::vector<char> code = programs[i]->GetElfCode();
    code[100] = 0xA0 + i;
    programs[i]->SetElfCode(code);
  }

  evolver.SelectParents(scores);

  EXPECT_EQ(programs[0]->GetElfCode()[100], '\xA3');
  EXPECT_EQ(programs[1]->GetElfCode()[100], '\xA1');
  // No positive-scoring candidate remained, so the zero-scoring program 4 is
  // still selected.
  EXPECT_EQ(programs[2]->GetElfCode()[100], '\xA4');
}

TEST(EvolverAdHocTest, SelectParentsBreaksTiesRandomly) {
  // All scores equal: the pre-sort shuffle alone determines the order, so
  // equal-scoring programs can displace the current parents (neutral drift).
  viaevo::RandomMock gen({7, 17});

  viaevo::MutatorPointRandom mutator(gen);

  std::vector<long long> scores{0, 0, 0, 0, 0};
  viaevo::ScorerMarkedMock scorer({}, 20, {}, {0, 1, 2, 3, 9});

  viaevo::EvolverAdHoc evolver("elfs/simple_small", 3, 1, 2, scorer, mutator,
                               gen, 1, 1);

  auto &programs = evolver.programs();

  for (int i = 0; i < 5; ++i) {
    std::vector<char> code = programs[i]->GetElfCode();
    code[100] = 0xA0 + i;
    programs[i]->SetElfCode(code);
  }

  evolver.SelectParents(scores);

  // Shuffle yields {0,4,3,1,2} (see SelectParents test for the trace), the
  // stable sort keeps it (all scores tie), and the phi slot (i=2) swaps with
  // index 3 (7%3=1), yielding {0,4,1,3,2}. Programs 4 and 1 - not the
  // pre-selection parents 1 and 2 - land in parent slots 1 and 2.
  EXPECT_EQ(programs[0]->GetElfCode()[100], '\xA0');
  EXPECT_EQ(programs[1]->GetElfCode()[100], '\xA4');
  EXPECT_EQ(programs[2]->GetElfCode()[100], '\xA1');
  EXPECT_EQ(programs[3]->GetElfCode()[100], '\xA3');
  EXPECT_EQ(programs[4]->GetElfCode()[100], '\xA2');
}

TEST(EvolverAdHocTest, Run) {
  viaevo::RandomMock gen({7, 17});

  viaevo::MutatorPointRandom mutator(gen);

  std::vector<long long> scores{0, 2, 0, 5, 0};
  // results_history_scores_ should be ignored by the evolver by default.
  viaevo::ScorerMarkedMock scorer({{0xA3, 5}, {0xA1, 2}}, 20, {},
                                  {0, 1, 2, 3, 9});
  std::vector<int> results;

  viaevo::EvolverAdHoc evolver("elfs/simple_small", 3, 1, 2, scorer, mutator,
                               gen, 1, 1);

  EXPECT_EQ(evolver.score_results_history(), false)
      << "score_results_history_ should be false in Evolver by default.";

  auto &programs = evolver.programs();

  for (int i = 0; i < 5; ++i) {
    std::vector<char> code = programs[i]->GetElfCode();
    EXPECT_TRUE(code.size() > 0);
    // Code should not be all nop by default.
    EXPECT_FALSE(std::all_of(code.begin(), code.end(),
                             [](char value) { return value == '\x90'; }));
    // "Mark" programs so that it is possible to tell which is which after the
    // mocked scoring.
    code[100] = 0xA0 + i;
    programs[i]->SetElfCode(code);
  }

  EXPECT_EQ(programs.size(), 5); // mu + lambda

  EXPECT_EQ(programs[0]->GetElfCode()[100], '\xA0');
  EXPECT_EQ(programs[1]->GetElfCode()[100], '\xA1');
  EXPECT_EQ(programs[2]->GetElfCode()[100], '\xA2');
  EXPECT_EQ(programs[3]->GetElfCode()[100], '\xA3');
  EXPECT_EQ(programs[4]->GetElfCode()[100], '\xA4');

  evolver.Run();

  // The initial round of parent selection (all scores zero) is determined by
  // the tie-breaking shuffle and the phi sampling with the mocked values
  // {7, 17}: the parent slots end up as programs 0, 4 and 1 (same trace as in
  // the SelectParentsBreaksTiesRandomly test). The offspring slots (3, 4) are
  // mutated copies of these parents and are not asserted here.
  EXPECT_EQ(programs[0]->GetElfCode()[100], '\xA0');
  EXPECT_EQ(programs[1]->GetElfCode()[100], '\xA4');
  EXPECT_EQ(programs[2]->GetElfCode()[100], '\xA1');
}

TEST(EvolverAdHocTest, RunAndScore) {
  viaevo::RandomMock gen({7, 17});

  viaevo::MutatorPointRandom mutator(gen);

  std::vector<long long> scores{2, 5, 0, 0, 0};
  viaevo::ScorerMarkedMock scorer({{0xA0, 2}, {0xA1, 5}}, 20, {}, {1});
  std::vector<int> results;

  // Run for two generation to see the ranking for the top two programs.
  // I.e. need to run for two generations to see the ranking after the first
  // generation.
  viaevo::EvolverAdHoc evolver("elfs/simple_small", 3, 1, 2, scorer, mutator,
                               gen, 1, 2, true);

  EXPECT_EQ(evolver.score_results_history(), true)
      << "score_results_history_ should be false in Evolver by default.";

  auto &programs = evolver.programs();

  for (int i = 0; i < 5; ++i) {
    std::vector<char> code = programs[i]->GetElfCode();
    EXPECT_TRUE(code.size() > 0);
    // Code should not be all nop by default.
    EXPECT_FALSE(std::all_of(code.begin(), code.end(),
                             [](char value) { return value == '\x90'; }));
    // "Mark" programs so that it is possible to tell which is which after the
    // mocked scoring.
    code[100] = 0xA0 + i;
    programs[i]->SetElfCode(code);
  }

  EXPECT_EQ(programs.size(), 5); // mu + lambda

  EXPECT_EQ(programs[0]->GetElfCode()[100], '\xA0');
  EXPECT_EQ(programs[1]->GetElfCode()[100], '\xA1');
  EXPECT_EQ(programs[2]->GetElfCode()[100], '\xA2');
  EXPECT_EQ(programs[3]->GetElfCode()[100], '\xA3');
  EXPECT_EQ(programs[4]->GetElfCode()[100], '\xA4');

  evolver.Run();

  // This reflects the order after scoring after the first round: 5+1 pts for
  // the second program (0xA1). Order of the others is unpredictable since the
  // generation of offspring already took place after the first round and there
  // may be multiple programs with the 0xA1 "mark".
  EXPECT_EQ(programs[0]->GetElfCode()[100], '\xA1');
}

TEST(EvolverAdHocTest, ScoreResultsHistory) {
  viaevo::RandomMock gen({7, 17});

  viaevo::MutatorPointRandom mutator(gen);

  viaevo::ScorerMock scorer({0, 0, 5}, 10, {}, {0, 0, 1, 1, 2, 2});
  std::vector<int> results;

  viaevo::EvolverAdHoc evolver("elfs/simple_small", 2, 0, 1, scorer, mutator,
                               gen, 1, 1, true);

  EXPECT_EQ(evolver.score_results_history(), true);

  auto &programs = evolver.programs();

  EXPECT_EQ(programs.size(), 3); // mu + lambda

  evolver.Run();

  // TODO: This test was gutted after the migration from keeping scores in
  // programs to managing them in the evolver. Also, enabling concurrent program
  // execution and scoring complicates this test given that previously the test
  // depended on sequential score assignments. Update this (and the previous)
  // test accordingly.

  std::vector<long long> scores{0, 1, 7};
  evolver.SelectParents(scores);

  viaevo::EvolverAdHoc evolver_3_generations("elfs/simple_small", 2, 0, 1,
                                             scorer, mutator, gen, 1, 3, true);

  evolver_3_generations.Run();

  viaevo::EvolverAdHoc evolver_3_generations_2_evaluations(
      "elfs/simple_small", 2, 0, 1, scorer, mutator, gen, 2, 3, true);

  evolver_3_generations_2_evaluations.Run();
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

TEST(EvolverAdHocTest, CreateOffspring) {
  viaevo::RandomMock gen({7, 17});

  viaevo::MutatorPointRandom mutator(gen);

  viaevo::ScorerMarkedMock scorer({}, 20, {}, {0});

  viaevo::EvolverAdHoc evolver("elfs/simple_small", 3, 1, 2, scorer, mutator,
                               gen, 1, 1);

  auto &programs = evolver.programs();
  EXPECT_EQ(programs.size(), 5); // mu + lambda

  // Mark the mu_ parents (0..2) distinctly; mark the lambda_ offspring slots
  // (3..4) with sentinels that are not any parent mark.
  for (int i = 0; i < 5; ++i) {
    std::vector<char> code = programs[i]->GetElfCode();
    code[100] = (i < 3) ? (0xA0 + i) : (0xF0 + i);
    programs[i]->SetElfCode(code);
  }

  evolver.CreateOffspring();

  // Each offspring is a (single-bit-mutated) copy of one of the mu_ parents, so
  // its mark must now equal one of the parent marks (the bit flip lands well
  // away from index 100 for the mocked RNG stream).
  for (int i = 3; i < 5; ++i) {
    char mark = programs[i]->GetElfCode()[100];
    EXPECT_TRUE(mark == '\xA0' || mark == '\xA1' || mark == '\xA2')
        << "offspring " << i << " mark: " << (int)(unsigned char)mark;
  }
}

TEST(EvolverAdHocTest, EvaluatePrograms) {
  viaevo::RandomMock gen({7, 17});

  viaevo::MutatorPointRandom mutator(gen);

  viaevo::ScorerMarkedMock scorer({{0xA0, 2}, {0xA1, 5}}, 20, {},
                                  {0, 1, 2, 3, 9});

  viaevo::EvolverAdHoc evolver("elfs/simple_small", 3, 1, 2, scorer, mutator,
                               gen, 1, 1);

  auto &programs = evolver.programs();
  for (int i = 0; i < 5; ++i) {
    std::vector<char> code = programs[i]->GetElfCode();
    code[100] = 0xA0 + i;
    programs[i]->SetElfCode(code);
  }

  // Pre-fill with garbage to confirm EvaluatePrograms clears before scoring.
  std::vector<long long> current_scores{99, 99, 99, 99, 99};
  int sigalarms = evolver.EvaluatePrograms(current_scores);

  // Scores reflect each program's mark; unknown marks (0xA2..0xA4) score 0.
  EXPECT_EQ(current_scores[0], 2); // 0xA0
  EXPECT_EQ(current_scores[1], 5); // 0xA1
  EXPECT_EQ(current_scores[2], 0); // 0xA2 (unknown mark)
  EXPECT_EQ(current_scores[3], 0);
  EXPECT_EQ(current_scores[4], 0);
  EXPECT_GE(sigalarms, 0);
}

} // namespace
