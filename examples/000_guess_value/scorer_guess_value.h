// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#ifndef VIAEVO_EXAMPLES_000_GUESS_VALUE_SCORER_GUESS_VALUE_H_
#define VIAEVO_EXAMPLES_000_GUESS_VALUE_SCORER_GUESS_VALUE_H_

#include <vector>

// TODO: Remove relative path.
#include "../../scorer/scorer.h"

namespace viaevo {

// ScorerGuessValue expects value_ in Program's last_results_[1]. Inputs are not
// relevant here as the only objecive for Program's is to guess the constant
// value passed to the scorer. The value may not be -1 for the scorer to work
// correctly.
class ScorerGuessValue : public Scorer {
public:
  explicit ScorerGuessValue(int value);
  // Returns score for a specific Program.
  virtual long long Score(const Program &program) const override;
  // Returns maximum possible score for "perfect" results. Should not depend on
  // current_inputs_.
  virtual long long MaxScore() const override;
  // Does nothing for this scorer.
  virtual void ResetInputs() override;

  int value() { return value_; }

protected:
  // Value to guess. Value MAY NOT be -1 for the scorer to work correctly.
  int value_ = -1;
};

} // namespace viaevo

#endif // VIAEVO_EXAMPLES_000_GUESS_VALUE_SCORER_GUESS_VALUE_H_