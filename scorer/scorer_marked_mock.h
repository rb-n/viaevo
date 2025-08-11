// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#ifndef VIAEVO_SCORER_SCORER_MARKED_MOCK_H_
#define VIAEVO_SCORER_SCORER_MARKED_MOCK_H_

#include "scorer.h"

#include <unordered_map>

namespace viaevo {

// ScorerMarkedMock is intended to be used for unit tests. The other ScorerMock
// assigned scores in a sequential fashion what led to flapping tests after
// program scoring was changed to be done concurrently. ScorerMarkedMock assigns
// based on a program's "mark". The "mark" is hardcoded to being the byte with
// index 100 in the program's evolvable code. Unit test are responsible for
// setting these "marks" and passing a map "mark" -> score for scoring. The
// scorer will assign predefined scores and return a predefined max score.
class ScorerMarkedMock : public Scorer {
public:
  ScorerMarkedMock(std::unordered_map<char, long long> scores,
                   long long max_score, std::vector<int> current_inputs,
                   std::vector<long long> results_history_scores);
  // Returns score for specific results.
  virtual long long Score(const Program &program) const override;
  // Returns score for specific results history.
  virtual long long ScoreResultsHistory(
      const std::vector<std::vector<int>> &results_history) const override;
  // Returns predefined max_score_.
  virtual long long MaxScore() const override;
  // Does nothing.
  virtual void ResetInputs() override {};

protected:
  // Predefined scores to be assigned in a circular fashion.
  std::unordered_map<char, long long> scores_;
  // Maximum score to be returned by MaxScore member function.
  long long max_score_;
  // Predefined scores for results history to be assigned in circular fashion.
  std::vector<long long> results_history_scores_;
  mutable int current_results_history_scores_index_ = -1;
};

} // namespace viaevo

#endif // VIAEVO_SCORER_SCORER_MARKED_MOCK_H_