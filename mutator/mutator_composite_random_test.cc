// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "mutator_composite_random.h"

#include <gtest/gtest-death-test.h>
#include <gtest/gtest.h>
#include <memory>

// TODO: Remove relative path.
#include "../util/random_mock.h"

#include "mutator_point_random.h"
#include "mutator_recombine_random.h"

namespace {

TEST(MutatorRecombineRandomTest, Mutate) {
  std::shared_ptr<viaevo::Program> target =
      viaevo::Program::CreateSimpleSmall();

  std::shared_ptr<viaevo::Program> parent1 =
      viaevo::Program::CreateSimpleSmall();

  std::shared_ptr<viaevo::Program> parent2 =
      viaevo::Program::CreateSimpleSmall();

  // The first mutator (point) will be selected first and used to mutate the
  // code, then the second mutator (recombine) will be selected and used to
  // mutate the code. The mocked random values for point and recombine mutators
  // are the same as in mutator_point_random_test.cc and
  // mutator_recombine_random_test.cc.
  viaevo::RandomMock gen({0, 42, 1, 0, 10, 20});
  EXPECT_EQ(gen(), 0);
  EXPECT_EQ(gen(), 42);
  EXPECT_EQ(gen(), 1);
  EXPECT_EQ(gen(), 0);
  EXPECT_EQ(gen(), 10);
  EXPECT_EQ(gen(), 20);

  std::vector<char> old_parent1_code = parent1->GetElfCode();
  std::vector<char> old_parent2_code = parent2->GetElfCode();

  std::shared_ptr<viaevo::MutatorPointRandom> mutator_point =
      std::make_shared<viaevo::MutatorPointRandom>(gen);
  std::shared_ptr<viaevo::MutatorRecombineRandom> mutator_recombine =
      std::make_shared<viaevo::MutatorRecombineRandom>(gen);

  viaevo::MutatorCompositeRandom mutator_composite(gen);

  mutator_composite.AppendMutator(mutator_point);
  mutator_composite.AppendMutator(mutator_recombine);

  // Selects the first mutator (MutatorPointRandom).
  mutator_composite.Mutate(target, parent1, parent2);

  std::vector<char> new_target_code = target->GetElfCode();
  std::vector<char> new_parent_code = parent1->GetElfCode();

  // Parent code should be unaltered. Assertions below are the same as in
  // mutator_point_random_test.cc.
  EXPECT_EQ(old_parent1_code, new_parent_code);
  // New target code should be different from parent1 code due to the introduced
  // mutation.
  EXPECT_NE(old_parent1_code, new_target_code);

  std::vector<char> parent_start(old_parent1_code.begin(),
                                 old_parent1_code.begin() + 5);
  std::vector<char> target_start(new_target_code.begin(),
                                 new_target_code.begin() + 5);
  EXPECT_EQ(parent_start, target_start);

  EXPECT_NE(old_parent1_code[5], new_target_code[5]);
  // The third bit is flipped, hence ^4.
  EXPECT_EQ(old_parent1_code[5], new_target_code[5] ^ 4);

  std::vector<char> parent_end(old_parent1_code.begin() + 6,
                               old_parent1_code.end());
  std::vector<char> target_end(new_target_code.begin() + 6,
                               new_target_code.end());
  EXPECT_EQ(parent_end, target_end);

  // Selects the second mutator (MutatorRecombineRandom).
  mutator_composite.Mutate(target, parent1, parent2);

  new_target_code = target->GetElfCode();
  std::vector<char> new_parent1_code = parent1->GetElfCode();
  std::vector<char> new_parent2_code = parent1->GetElfCode();

  // Parent code should be unaltered. Assertions below are the same as in
  // mutator_recombine_random_test.cc.
  EXPECT_EQ(old_parent1_code, new_parent1_code);
  EXPECT_EQ(old_parent2_code, new_parent2_code);
  // New target code should be different from parent1 code due to the introduced
  // mutation.
  EXPECT_NE(old_parent1_code, new_target_code);

  std::vector<char> parent1_start(old_parent1_code.begin(),
                                  old_parent1_code.begin() + 20);
  target_start =
      std::vector<char>(new_target_code.begin(), new_target_code.begin() + 20);
  EXPECT_EQ(parent1_start, target_start);

  std::vector<char> parent1_mid(old_parent1_code.begin() + 20,
                                old_parent1_code.begin() + 30);
  std::vector<char> parent2_insert(old_parent2_code.begin(),
                                   old_parent2_code.begin() + 10);
  std::vector<char> target_mid(new_target_code.begin() + 20,
                               new_target_code.begin() + 30);
  EXPECT_NE(parent1_mid, target_mid);
  EXPECT_EQ(parent2_insert, target_mid);

  std::vector<char> parent1_end(old_parent1_code.begin() + 30,
                                old_parent1_code.end());
  target_end =
      std::vector<char>(new_target_code.begin() + 30, new_target_code.end());
  EXPECT_EQ(parent1_end, target_end);

  // Expect death when executing Mutate and mutators_ is empty.
  mutator_composite.Clear();
  EXPECT_DEATH(mutator_composite.Mutate(target, parent1, parent2), "");
}

} // namespace
