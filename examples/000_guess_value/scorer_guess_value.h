// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#ifndef VIAEVO_EXAMPLES_001_GUESS_VALUE_SCORER_GUESS_VALUE_H_
#define VIAEVO_EXAMPLES_001_GUESS_VALUE_SCORER_GUESS_VALUE_H_

#include <vector>

// TODO: Remove relative path.
#include "../../scorer/scorer.h"

namespace viaevo {

// ScorerGuessValue expects value_ in Program's last_results_[1]. Inputs are not
// relevant here as the only objecive for Program's is to guess the constant
// value passed to the scorer. The value may not be 0 for the scorer to work
// correctly.
class ScorerGuessValue : public Scorer {
public:
  explicit ScorerGuessValue(int value);
  // Returns score for specific results.
  virtual long long Score(const std::vector<int> &results) const override;
  // Returns maximum possible score for "perfect" results. Should not depend on
  // current_inputs_.
  virtual long long MaxScore() const override;
  // Does nothing for this scorer.
  virtual void ResetInputs() override;

protected:
  // Value to guess. Value MAY NOT be 0 for the scorer to work correctly.
  int value_ = 0;
};

} // namespace viaevo

#endif // VIAEVO_EXAMPLES_001_GUESS_VALUE_SCORER_GUESS_VALUE_H_