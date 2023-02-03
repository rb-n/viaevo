// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "random.h"

#include <gtest/gtest.h>

namespace {

TEST(RandomTest, GeneratedValues) {
  viaevo::mt_type mt_gen;
  viaevo::Random random;

  // Consecutive values from a random number generator may in theory be equal on
  // an extremely rare occasion? Therefore this could flap?
  EXPECT_NE(random(), random());

  random.Seed(42);
  mt_gen.seed(42);

  EXPECT_EQ(random(), mt_gen());
}

} //  namespace