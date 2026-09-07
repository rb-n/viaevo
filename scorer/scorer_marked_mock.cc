// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "scorer_marked_mock.h"

#include <algorithm>

// TODO: Remove relative path.
#include "../util/check.h"

namespace viaevo {

ScorerMarkedMock::ScorerMarkedMock(
    std::unordered_map<char, long long> scores, long long max_score,
    std::vector<int> current_inputs,
    std::vector<long long> results_history_scores)
    : scores_(scores), max_score_(max_score),
      results_history_scores_(results_history_scores) {
  current_inputs_ = current_inputs;
  long long highest_score = 0;
  for (auto &p : scores_) {
    highest_score = std::max(highest_score, p.second);
  }
  VIAEVO_CHECK(max_score_ >= highest_score +
                                 *std::max_element(
                                     results_history_scores_.begin(),
                                     results_history_scores_.end()),
               "max_score_ should not be smaller than max element in scores_ + "
               "plus max element in results_history_scores_");
}

long long ScorerMarkedMock::Score(const Program &program) const {
  // Program's "mark" hard coded to index 100 of evolvable code.
  char mark = program.GetElfCode()[100];
  auto it = scores_.find(mark);
  // Default to score 0 if "mark" not found.
  if (it == scores_.end()) {
    return 0;
  }
  return it->second;
};

long long ScorerMarkedMock::ScoreResultsHistory(
    const std::vector<std::vector<int>> &results_history) const {
  current_results_history_scores_index_ =
      (current_results_history_scores_index_ + 1) %
      results_history_scores_.size();
  return results_history_scores_[current_results_history_scores_index_];
}

long long ScorerMarkedMock::MaxScore() const { return max_score_; };

} // namespace viaevo
