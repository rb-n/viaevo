// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "random_mock.h"

#include <gtest/gtest.h>

namespace {

TEST(RandomMockTest, GeneratedValues) {
  viaevo::RandomMock mock1;

  EXPECT_DEATH(mock1(), "");

  mock1.set_values({42});
  EXPECT_EQ(mock1(), 42);
  EXPECT_EQ(mock1(), 42);
  EXPECT_EQ(mock1(), 42);

  mock1.set_values({0, 1, 2, 3, 4});
  EXPECT_EQ(mock1(), 0);
  EXPECT_EQ(mock1(), 1);
  EXPECT_EQ(mock1(), 2);
  EXPECT_EQ(mock1(), 3);
  EXPECT_EQ(mock1(), 4);
  EXPECT_EQ(mock1(), 0);
  EXPECT_EQ(mock1(), 1);

  mock1.set_values({7, 17});
  EXPECT_EQ(mock1(), 7);
  EXPECT_EQ(mock1(), 17);
  EXPECT_EQ(mock1(), 7);
  EXPECT_EQ(mock1(), 17);

  viaevo::RandomMock mock2({23, 32});
  EXPECT_EQ(mock2(), 23);
  EXPECT_EQ(mock2(), 32);
  EXPECT_EQ(mock2(), 23);
  EXPECT_EQ(mock2(), 32);

  // Test that the _virtual_ operator() works correctly.
  viaevo::Random &r = mock2;
  EXPECT_EQ(r(), 23);
  EXPECT_EQ(r(), 32);
  EXPECT_EQ(r(), 23);
  EXPECT_EQ(r(), 32);

}

} //  namespace