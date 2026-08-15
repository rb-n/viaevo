// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "elf_layout.h"

#include <fcntl.h>
#include <unistd.h>

#include <gtest/gtest.h>

namespace {

void ExpectAllFieldsPopulated(const viaevo::SymbolData &sd) {
  // The default sentinel for every field is -1; a successful resolve replaces
  // all of them.
  EXPECT_NE(sd.main_offset_in_elf_, (Elf64_Addr)-1);
  EXPECT_NE(sd.main_offset_in_text_, (Elf64_Addr)-1);
  EXPECT_NE(sd.main_st_size_, (uint64_t)-1);
  EXPECT_NE(sd.inputs_offset_in_elf_, (Elf64_Addr)-1);
  EXPECT_NE(sd.inputs_st_size_, (uint64_t)-1);
  EXPECT_NE(sd.results_offset_in_data_, (Elf64_Addr)-1);
  EXPECT_NE(sd.results_st_size_, (uint64_t)-1);
}

TEST(ElfLayoutTest, ResolveAllTemplates) {
  for (const char *filename :
       {"elfs/simple_small", "elfs/simple_medium", "elfs/intermediate_small",
        "elfs/intermediate_medium"}) {
    int fd = open(filename, O_RDONLY);
    ASSERT_NE(fd, -1) << "could not open " << filename;

    viaevo::SymbolData sd = viaevo::ResolveElfSymbolData(fd);
    close(fd);

    ExpectAllFieldsPopulated(sd);

    // main is the evolvable code region and must be non-empty.
    EXPECT_GT(sd.main_st_size_, 0u) << filename;
    // inputs and results are int arrays, so their sizes are multiples of an
    // int (this is what Program relies on when reading them).
    EXPECT_EQ(sd.inputs_st_size_ % sizeof(int), 0u) << filename;
    EXPECT_EQ(sd.results_st_size_ % sizeof(int), 0u) << filename;
  }
}

TEST(ElfLayoutTest, ResolveIsRepeatable) {
  int fd = open("elfs/simple_small", O_RDONLY);
  ASSERT_NE(fd, -1);

  // Resolving twice on the same descriptor must return identical data (the
  // function repositions the descriptor itself before reading).
  viaevo::SymbolData a = viaevo::ResolveElfSymbolData(fd);
  viaevo::SymbolData b = viaevo::ResolveElfSymbolData(fd);
  close(fd);

  EXPECT_EQ(a.main_offset_in_elf_, b.main_offset_in_elf_);
  EXPECT_EQ(a.main_offset_in_text_, b.main_offset_in_text_);
  EXPECT_EQ(a.main_st_size_, b.main_st_size_);
  EXPECT_EQ(a.inputs_offset_in_elf_, b.inputs_offset_in_elf_);
  EXPECT_EQ(a.inputs_st_size_, b.inputs_st_size_);
  EXPECT_EQ(a.results_offset_in_data_, b.results_offset_in_data_);
  EXPECT_EQ(a.results_st_size_, b.results_st_size_);
}

} // namespace
