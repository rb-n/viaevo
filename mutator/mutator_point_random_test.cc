// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "mutator_point_random.h"

#include <gtest/gtest.h>

// TODO: Remove relative path.
#include "../util/random_mock.h"

namespace {

TEST(MutatorPointRandomTest, Mutate) {
  std::shared_ptr<viaevo::Program> target =
      viaevo::Program::Create("elfs/simple_small");

  std::shared_ptr<viaevo::Program> parent =
      viaevo::Program::Create("elfs/simple_small");

  viaevo::RandomMock gen({42});
  EXPECT_EQ(gen(), 42);
  EXPECT_EQ(gen(), 42);
  EXPECT_EQ(gen(), 42);

  std::vector<char> old_parent_code = parent->GetElfCode();

  viaevo::MutatorPointRandom mutator(gen);
  // The third argument is ignored by MutatorPointRandom.
  mutator.Mutate(target, parent, parent);

  std::vector<char> new_target_code = target->GetElfCode();
  std::vector<char> new_parent_code = parent->GetElfCode();

  // Parent code should be unaltered.
  EXPECT_EQ(old_parent_code, new_parent_code);
  // New target code should be different from parent1 code due to the introduced
  // mutation.
  EXPECT_NE(old_parent_code, new_target_code);

  std::vector<char> parent_start(old_parent_code.begin(),
                                 old_parent_code.begin() + 5);
  std::vector<char> target_start(new_target_code.begin(),
                                 new_target_code.begin() + 5);
  EXPECT_EQ(parent_start, target_start);

  EXPECT_NE(old_parent_code[5], new_target_code[5]);
  // The third bit should be flipped, hence ^4.
  EXPECT_EQ(old_parent_code[5], new_target_code[5] ^ 4);

  std::vector<char> parent_end(old_parent_code.begin() + 6,
                               old_parent_code.end());
  std::vector<char> target_end(new_target_code.begin() + 6,
                               new_target_code.end());
  EXPECT_EQ(parent_end, target_end);
}

} // namespace
