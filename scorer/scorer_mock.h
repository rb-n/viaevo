// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#ifndef VIAEVO_SCORER_SCORER_MOCK_H_
#define VIAEVO_SCORER_SCORER_MOCK_H_

#include "scorer.h"

namespace viaevo {

// ScorerMock is intended to be used for unit tests. The scorer will assign
// predefined scores and return a predefined max score.
class ScorerMock : public Scorer {
public:
  ScorerMock(std::vector<long long> scores_, long long max_score,
             std::vector<int> current_inputs);
  // Returns score for specific results.
  virtual long long Score(const std::vector<int> &results) const override;
  // Returns predefined max_score_.
  virtual long long MaxScore() const override;
  // Does nothing.
  virtual void ResetInputs() override{};

protected:
  // Predefined scores to be assigned in a circular fashion.
  std::vector<long long> scores_;
  mutable int current_scores_index_ = -1;
  // Maximum score to be returned by MaxScore member function.
  long long max_score_;
};

} // namespace viaevo

#endif // VIAEVO_SCORER_SCORER_MOCK_H_