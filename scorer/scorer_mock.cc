// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "scorer_mock.h"

#include <algorithm>
#include <assert.h>

namespace viaevo {

ScorerMock::ScorerMock(std::vector<long long> scores, long long max_score,
                       std::vector<int> current_inputs)
    : scores_(scores), max_score_(max_score) {
  current_inputs_ = current_inputs;
  assert(max_score_ >= *std::max_element(scores_.begin(), scores_.end()) &&
         "max_score_ should not be smaller than any element in scores_");
}

long long ScorerMock::Score(const Program &program) const {
  current_scores_index_ = (current_scores_index_ + 1) % scores_.size();
  return scores_[current_scores_index_];
};

long long ScorerMock::MaxScore() const { return max_score_; };

} // namespace viaevo
