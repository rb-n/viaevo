// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#ifndef VIAEVO_EXAMPLES_001_COPY_VALUE_SCORER_GUESS_VALUE_H_
#define VIAEVO_EXAMPLES_001_COPY_VALUE_SCORER_GUESS_VALUE_H_

#include <vector>

// TODO: Remove relative path.
#include "../../scorer/scorer.h"
#include "../../util/random.h"

namespace viaevo {

// ScorerCopyValue expects the value of current_inputs_[0] in Program's
// results[1]. The value may not be 0 for the scorer to work correctly.
class ScorerCopyValue : public Scorer {
public:
  explicit ScorerCopyValue(Random &gen, int number_of_copies_in_current_inputs);
  // Returns score for specific results.
  virtual long long Score(const std::vector<int> &results) const override;
  // Returns maximum possible score for "perfect" results. Should be the same
  // for different values of current_inputs_[0].
  virtual long long MaxScore() const override;
  // Initializes new value(s) for current_inputs_.
  virtual void ResetInputs() override;

protected:
  // Random number generator.
  Random &gen_;
  // Number of copies of the value of interest in current_inputs_. The
  // assumptions is that multiple copies of the value in inputs will make it
  // easier to evolve the Program to copy one of these values to results.
  int number_of_copies_in_current_inputs_ = 1;
};

} // namespace viaevo

#endif // VIAEVO_EXAMPLES_001_COPY_VALUE_SCORER_GUESS_VALUE_H_