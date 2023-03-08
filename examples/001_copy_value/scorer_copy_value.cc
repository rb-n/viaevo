// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "scorer_copy_value.h"

#include <assert.h>

namespace viaevo {

ScorerCopyValue::ScorerCopyValue(Random &gen,
                                 int number_of_copies_in_current_inputs)
    : gen_(gen),
      number_of_copies_in_current_inputs_(number_of_copies_in_current_inputs) {
  current_inputs_ = std::vector<int>(number_of_copies_in_current_inputs_, 0);
  ResetInputs();
}

long long ScorerCopyValue::Score(const std::vector<int> &results) const {
  long long score = 0;

  // If any of the results exactly matches current_inputs_[0], increase the
  // score significantly. The assumption being that if current_inputs_[0] is
  // copied to any of the results, it should be easier to learn to copy it to
  // results[1].
  for (std::vector<int>::size_type i = 0; i < results.size(); ++i) {
    if (results[i] == current_inputs_[0]) {
      score += 60;
      break;
    }
  }

  // current_inputs_[0] is expected in results[1]. If results[1] is unchanged,
  // at least score changes in other elements of results. The assumption is that
  // a change in one of the others is better that no change and a change in
  // multiple is better than a change in one. It is also assumed that a change
  // in one of these is easier to lead to a change in results[1] than no change
  // in any of the elements. Also, this scoring is based on results from
  // //elfs/simple_small.c.
  if (results[1] == 0) {
    // results[0] is disregarded as it is changed in main of //elfs:simple_small
    // from 10 to 20.
    for (int i = 2; i < 6; ++i) {
      if (results[i] != 0) {
        ++score;
      }
    }

    for (int i = 6; i < 11; ++i) {
      if (results[i] != 3) {
        ++score;
      }
    }

    return score;
  }

  // Reaching this path means results[1] changed. Start with a score that is
  // higher than the maximum possible score from the path above. The assumption
  // is that changing results[1] alone is better that changing any other (or
  // all other) elements in results.
  score += 20;

  // Increment the score for each correctly set bit in results[1].
  unsigned int matching_bits = ~(results[1] ^ current_inputs_[0]);
  while (matching_bits) {
    if (matching_bits & 1) {
      ++score;
    }
    matching_bits >>= 1;
  }

  return score;
}

long long ScorerCopyValue::MaxScore() const {
  std::vector<int> results(11, current_inputs_[0]);
  return Score(results);
}

void ScorerCopyValue::ResetInputs() {
  current_inputs_[0] = gen_();
  // Avoid 0 as the scorer does not work correctly in this case.
  while (current_inputs_[0] == 0) {
    current_inputs_[0] = gen_();
  }
  for (int i = 1; i < number_of_copies_in_current_inputs_; ++i) {
    current_inputs_[i] = current_inputs_[0];
  }
}

} // namespace viaevo