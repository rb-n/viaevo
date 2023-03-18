// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "mutator_point_last_instruction.h"

#include <gtest/gtest.h>

// TODO: Remove relative path.
#include "../util/random_mock.h"

namespace {

TEST(MutatorPointLastInstructionTest, Mutate) {
  std::shared_ptr<viaevo::Program> target =
      viaevo::Program::Create("elfs/simple_small");

  std::shared_ptr<viaevo::Program> parent =
      viaevo::Program::Create("elfs/simple_small");

  viaevo::RandomMock gen({42});
  EXPECT_EQ(gen(), 42);
  EXPECT_EQ(gen(), 42);
  EXPECT_EQ(gen(), 42);

  std::vector<char> old_parent_code = parent->GetElfCode();

  // The code below is taken from TEST(ProgramTest, LastRipOffset) to set the
  // last_rip_offset_ in the parent.
  int nop_position = 23;
  EXPECT_EQ(old_parent_code[nop_position], '\x90')
      << "Instruction at offset " << nop_position
      << " vs main in the simple_small binary is not nop (\\x90). This may be "
         "compiler dependent. The value of the nop_position variable in this "
         "test needs to be changed accordingly.";

  // Create an illegal instruction.
  old_parent_code[nop_position] = '\x06';
  parent->SetElfCode(old_parent_code);

  parent->Execute();
  EXPECT_EQ(parent->last_rip_offset(), nop_position)
      << "Last rip offset should be " << nop_position
      << "  for 'default' Execute";

  viaevo::MutatorPointLastInstruction mutator(gen);
  // The third argument is ignored by MutatorPointLastInstruction.
  mutator.Mutate(target, parent, parent);

  std::vector<char> new_target_code = target->GetElfCode();
  std::vector<char> new_parent_code = parent->GetElfCode();

  // The modified old_parent_code should be the same as new_parent_code.
  EXPECT_EQ(old_parent_code, new_parent_code);
  // New target code should be different from parent1 codes due to the
  // introduced illegal instruction and the mutation.
  EXPECT_NE(old_parent_code, new_target_code);
  EXPECT_NE(new_parent_code, new_target_code);

  std::vector<char> parent_start(old_parent_code.begin(),
                                 old_parent_code.begin() + nop_position);
  std::vector<char> target_start(new_target_code.begin(),
                                 new_target_code.begin() + nop_position);
  EXPECT_EQ(parent_start, target_start);

  // The added illegal instruction expected the corresponding position in the
  // new_target_code.
  EXPECT_EQ(new_target_code[nop_position], '\x06');
  // The third bit five bytes down from nop_position should be flipped, hence
  // ^4.
  EXPECT_EQ(old_parent_code[nop_position + 5],
            new_target_code[nop_position + 5] ^ 4);

  std::vector<char> parent_end(old_parent_code.begin() + nop_position + 6,
                               old_parent_code.end());
  std::vector<char> target_end(new_target_code.begin() + nop_position + 6,
                               new_target_code.end());
  EXPECT_EQ(parent_end, target_end);
}

} // namespace
