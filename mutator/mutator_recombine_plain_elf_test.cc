// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "mutator_recombine_plain_elf.h"

#include <algorithm>
#include <random>

#include <gtest/gtest.h>

// TODO: Remove relative path.
#include "../util/random_mock.h"

namespace {

TEST(MutatorRecombinePlainElfTest, Mutate) {
  std::shared_ptr<viaevo::Program> target =
      viaevo::Program::Create("elfs/simple_small");

  std::shared_ptr<viaevo::Program> parent1 =
      viaevo::Program::Create("elfs/simple_small");

  // Use a different elf for parent2 which should be ingored by the mutator.
  std::shared_ptr<viaevo::Program> parent2 =
      viaevo::Program::Create("elfs/intermediate_small");

  // Simple mutation: Base the new code on parent1 and starting from index 20
  // place elements [0..10) from parent2's code.
  viaevo::RandomMock gen({0, 10, 20});
  EXPECT_EQ(gen(), 0);
  EXPECT_EQ(gen(), 10);
  EXPECT_EQ(gen(), 20);
  EXPECT_EQ(gen(), 0);
  EXPECT_EQ(gen(), 10);
  EXPECT_EQ(gen(), 20);

  viaevo::MutatorRecombinePlainElf mutator(gen, "elfs/simple_small");

  std::vector<char> old_parent1_code = parent1->GetElfCode();
  std::vector<char> old_standard_elf_code =
      mutator.standard_program()->GetElfCode();

  EXPECT_FALSE(std::all_of(old_standard_elf_code.begin(),
                           old_standard_elf_code.end(),
                           [](char value) { return value == '\x90'; }));

  mutator.Mutate(target, parent1, parent2);

  std::vector<char> new_target_code = target->GetElfCode();
  std::vector<char> new_parent1_code = parent1->GetElfCode();
  std::vector<char> new_standard_elf_code =
      mutator.standard_program()->GetElfCode();

  // Parent code should be unaltered.
  EXPECT_EQ(old_parent1_code, new_parent1_code);
  EXPECT_EQ(old_standard_elf_code, new_standard_elf_code);
  // New target code should be different from parent1 code due to the introduced
  // mutation.
  EXPECT_NE(old_parent1_code, new_target_code);

  std::vector<char> parent1_start(old_parent1_code.begin(),
                                  old_parent1_code.begin() + 20);
  std::vector<char> target_start(new_target_code.begin(),
                                 new_target_code.begin() + 20);
  EXPECT_EQ(parent1_start, target_start);

  std::vector<char> parent1_mid(old_parent1_code.begin() + 20,
                                old_parent1_code.begin() + 30);
  std::vector<char> parent2_insert(old_standard_elf_code.begin(),
                                   old_standard_elf_code.begin() + 10);
  std::vector<char> target_mid(new_target_code.begin() + 20,
                               new_target_code.begin() + 30);
  EXPECT_NE(parent1_mid, target_mid);
  EXPECT_EQ(parent2_insert, target_mid);

  std::vector<char> parent1_end(old_parent1_code.begin() + 30,
                                old_parent1_code.end());
  std::vector<char> target_end(new_target_code.begin() + 30,
                               new_target_code.end());
  EXPECT_EQ(parent1_end, target_end);

  // Test large random numbers that would be out of bounds of the underlying
  // code vectors (if mod was not done in the code) do not cause a crash.
  gen.set_values({0, 5'000'000, 10'000'000});
  EXPECT_EQ(gen(), 0);
  EXPECT_EQ(gen(), 5'000'000);
  EXPECT_EQ(gen(), 10'000'000);

  mutator.Mutate(target, parent1, parent2);
}

TEST(MutatorRecombinePlainElfTest, InitializeProgramToAllNops) {
  viaevo::RandomMock gen({0});

  viaevo::MutatorRecombinePlainElf mutator(gen, "elfs/simple_small", true);

  std::vector<char> code = mutator.standard_program()->GetElfCode();
  EXPECT_TRUE(code.size() > 0);
  EXPECT_TRUE(std::all_of(code.begin(), code.end(),
                          [](char value) { return value == '\x90'; }));
}

} // namespace
