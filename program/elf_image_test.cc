// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "elf_image.h"

#include <gtest/gtest.h>

#include <algorithm>

namespace {

TEST(ElfImageTest, DefaultConstructedIsUninitialized) {
  viaevo::ElfImage image;
  EXPECT_FALSE(image.IsInitialized());
  EXPECT_EQ(image.fd(), -1);
}

TEST(ElfImageTest, CreateSimpleSmall) {
  viaevo::ElfImage image("elfs/simple_small");
  EXPECT_TRUE(image.IsInitialized());
  EXPECT_NE(image.fd(), -1);

  const viaevo::SymbolData &symbol_data = image.symbol_data();
  EXPECT_NE(symbol_data.main_offset_in_elf_, (Elf64_Addr)-1);
  EXPECT_NE(symbol_data.main_st_size_, (uint64_t)-1);
  EXPECT_NE(symbol_data.inputs_offset_in_elf_, (Elf64_Addr)-1);
  EXPECT_NE(symbol_data.results_offset_in_data_, (Elf64_Addr)-1);
}

TEST(ElfImageTest, GetSetCode) {
  viaevo::ElfImage image("elfs/simple_small");

  std::vector<char> code = image.GetCode();
  EXPECT_FALSE(code.empty());
  // Code should not be all nop by default.
  EXPECT_FALSE(std::all_of(code.begin(), code.end(),
                           [](char value) { return value == '\x90'; }));

  std::vector<char> nops(code.size(), 0x90);
  image.SetCode(nops);
  EXPECT_EQ(image.GetCode(), nops);

  image.SetCode(code);
  EXPECT_EQ(image.GetCode(), code);

  image.SetCodeToAllNops();
  EXPECT_EQ(image.GetCode(), nops);

  // Setting with a vector of a smaller size (10) than the code in the ELF.
  std::vector<char> nops_10(10, 0x90);
  EXPECT_DEATH(image.SetCode(nops_10), "elf code to set has incorrect size");

  // Setting with a vector of a larger size (10k) than the code in the ELF.
  std::vector<char> nops_10k(10'000, 0x90);
  EXPECT_DEATH(image.SetCode(nops_10k), "elf code to set has incorrect size");
}

TEST(ElfImageTest, GetSetInputs) {
  viaevo::ElfImage image("elfs/simple_small");

  std::vector<int> inputs = image.GetInputs();
  EXPECT_EQ(inputs.size(), 101);
  EXPECT_EQ(inputs[0], -1);
  EXPECT_EQ(inputs[100], -1);

  // Setting with a vector of a smaller size (3) than inputs in the ELF.
  std::vector<int> short_vector{7, 7, 7};
  image.SetInputs(short_vector);
  inputs = image.GetInputs();
  EXPECT_EQ(inputs.size(), 101);
  EXPECT_EQ(inputs[0], 7);
  EXPECT_EQ(inputs[2], 7);
  EXPECT_EQ(inputs[3], -1);

  // Setting with a vector of a matching size to inputs in the ELF.
  std::vector<int> zeros_101(101, 0);
  image.SetInputs(zeros_101);
  EXPECT_EQ(image.GetInputs(), zeros_101);

  // Setting with a vector of a larger size (200) than inputs in the ELF.
  std::vector<int> zeros_200(200, 0);
  EXPECT_DEATH(image.SetInputs(zeros_200), "elf inputs to set are too large");
}

TEST(ElfImageTest, SaveRoundTrip) {
  viaevo::ElfImage image("elfs/simple_small");

  std::vector<char> code = image.GetCode();
  std::vector<char> nops(code.size(), 0x90);
  image.SetCode(nops);

  std::string filename = testing::TempDir() + "unit_test_elf_image_save.elf";
  image.Save(filename.c_str());

  viaevo::ElfImage saved_image(filename.c_str());
  EXPECT_TRUE(saved_image.IsInitialized());
  // The saved image's code should be all nops.
  EXPECT_EQ(saved_image.GetCode(), nops);
  // Inputs should round-trip as well.
  EXPECT_EQ(saved_image.GetInputs(), image.GetInputs());
}

} // namespace
