// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "scorer_guess_value.h"

#include <assert.h>

namespace viaevo {

ScorerGuessValue::ScorerGuessValue(int value) : value_(value) {
  assert(value_ != -1 && "Value should not be -1.");
}

long long ScorerGuessValue::Score(const Program &program) const {
  const std::vector<int> &results = program.last_results();
  long long score = 0;

  // value_ is expected in results[1]. If results[1] is unchanged, at least
  // score changes in other elements of results. The assumption is that a change
  // in one of the others is better that no change and a change in multiple is
  // better than change in one. It is also assumed that a change in one of these
  // is easier to lead to a change in results[1] than no change in any of the
  // elements.
  // Also, this scoring is based on results from //elfs/simple_small.c.
  if (results[1] == -1) {
    // results[0] is disregarded as it is changed in main of //elfs:simple_small
    // from 10 to 20.
    for (int i = 2; i < 11; ++i) {
      if (results[i] != -1) {
        ++score;
      }
    }
    return score;
  }

  // Reaching this path means results[1] changed. Start with a score that is
  // higher than the maximum possible score from the path above. The assumption
  // is that changing results[1] alone is better that changing any other (or
  // all other) elements in results.
  score = 20;

  // Increment the score for each correctly set bit in results[1].
  unsigned int matching_bits = ~(results[1] ^ value_);
  while (matching_bits) {
    if (matching_bits & 1) {
      ++score;
    }
    matching_bits >>= 1;
  }

  return score;
}

long long ScorerGuessValue::MaxScore() const {
  // Max score should be 52 (20 for changing results[1] and +1 for each
  // correctly set bit in results[1]).
  return 52;
}

void ScorerGuessValue::ResetInputs() {}

} // namespace viaevo