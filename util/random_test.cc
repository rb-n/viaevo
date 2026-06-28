// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "random.h"

#include <sstream>

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

// The stream operators must return the stream they were given. Returning a
// reference to a different object (or falling off the end of the function, which
// is undefined behavior) would be a bug.
TEST(RandomTest, StreamOperatorsReturnTheStream) {
  viaevo::Random random;
  random.Seed(42);

  std::stringstream ss;
  std::ostream &ost = (ss << random);
  EXPECT_EQ(&ost, &ss);

  std::istream &ist = (ss >> random);
  EXPECT_EQ(&ist, &ss);
}

// Serializing a Random's state and reading it back should reproduce the exact
// same subsequent sequence of generated values.
TEST(RandomTest, StreamSerializationRoundTrip) {
  viaevo::Random source, restored;
  source.Seed(12345);

  // Advance the source generator so its state is non-trivial.
  for (int i = 0; i < 5; ++i)
    source();

  std::stringstream ss;
  ss << source;
  ss >> restored;

  // Both generators should now produce identical sequences.
  for (int i = 0; i < 10; ++i)
    EXPECT_EQ(source(), restored());
}

} //  namespace