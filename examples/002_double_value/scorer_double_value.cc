// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "scorer_double_value.h"

#include <assert.h>

namespace viaevo {

ScorerDoubleValue::ScorerDoubleValue(Random &gen,
                                     int number_of_copies_in_current_inputs)
    : gen_(gen),
      number_of_copies_in_current_inputs_(number_of_copies_in_current_inputs) {
  current_inputs_ = std::vector<int>(number_of_copies_in_current_inputs_, 0);
  ResetInputs();
}

long long ScorerDoubleValue::Score(const Program &program) const {
  const std::vector<int> &results = program.last_results();
  long long score = 0;

  // If any of the results exactly matches 2 * current_inputs_[0], increase the
  // score significantly. The assumption being that if 2 * current_inputs_[0]
  // ends up in any of the results, it should be easier to learn to make it end
  // up in results[1].
  for (std::vector<int>::size_type i = 0; i < results.size(); ++i) {
    if (results[i] == expected_value_) {
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
  score += 20;

  // Increment the score for each correctly set bit in results[1].
  unsigned int matching_bits = ~(results[1] ^ expected_value_);
  while (matching_bits) {
    if (matching_bits & 1) {
      ++score;
    }
    matching_bits >>= 1;
  }

  return score;
}

// long long ScorerDoubleValue::Score(const Program &program) const {
//   const std::vector<int> &results = program.last_results();

//   // The highest possible score for a correct result.
//   if (results[1] == expected_value_) {
//     return 1'000'000'000'000;
//   }

//   // Second highest score if any of the results has the correct score.
//   for (std::vector<int>::size_type i = 0; i < results.size(); ++i) {
//     if (results[i] == expected_value_) {
//       return 1'000'000'000;
//     }
//   }

//   if (results[1] != -1) {
//     // Third highest score if results[1] is other multiple of inputs[0].
//     if (results[1] % (expected_value_ / 2) == 0) {
//       return 1'000'000;
//     } else {
//       // Some score if results[1] at least changes.
//       return 1'000;
//     }
//   }

//   // If results[1] is unchanged,
//   // at least score changes in other elements of results. The assumption is that
//   // a change in one of the others is better that no change and a change in
//   // multiple is better than a change in one. It is also assumed that a change
//   // in one of these is easier to lead to a change in results[1] than no change
//   // in any of the elements. Also, this scoring is based on results from
//   // //elfs/simple_small.c.

//   long long score = 0;

//   // results[0] is disregarded as it is changed in main of //elfs:simple_small
//   // from 10 to 20.
//   for (std::size_t i = 2; i < 11; ++i) {
//     if (results[i] != -1) {
//       ++score;
//     }
//   }

//   return score;
// }


long long ScorerDoubleValue::MaxScore() const {
  // Max score should be 112 (20 for changing results[1] and +1 for each
  // correctly set bit in results[1] and +60 for at least one results element
  // equalling the correct value).
  return 112;
  // return 1'000'000'000'000;
}

void ScorerDoubleValue::ResetInputs() {
  current_inputs_[0] = gen_();
  // Avoid -1 as the scorer does not work correctly in this case. Also prevent
  // overflow or underfow when multiplied by 2.
  while (current_inputs_[0] == -1 || current_inputs_[0] < -1'000'000'000 ||
         current_inputs_[0] > 1'000'000'000) {
    current_inputs_[0] = gen_();
  }
  for (int i = 1; i < number_of_copies_in_current_inputs_; ++i) {
    current_inputs_[i] = current_inputs_[0];
  }
  expected_value_ = 2 * current_inputs_[0];
}

} // namespace viaevo