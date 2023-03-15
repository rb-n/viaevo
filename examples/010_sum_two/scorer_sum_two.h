// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#ifndef VIAEVO_EXAMPLES_010_SUM_TWO_SCORER_SUM_TWO_H_
#define VIAEVO_EXAMPLES_010_SUM_TWO_SCORER_SUM_TWO_H_

#include <vector>

// TODO: Remove relative path.
#include "../../scorer/scorer.h"
#include "../../util/random.h"

namespace viaevo {

// ScorerSumTwo expects the value of current_inputs_[0] + current_inputs_[1] in
// Program's results[1]. The sum may not be 0 for the scorer to work correctly.
class ScorerSumTwo : public Scorer {
public:
  explicit ScorerSumTwo(Random &gen, int number_of_copies_in_current_inputs);
  // Returns score for specific results.
  virtual long long Score(const std::vector<int> &results) const override;
  // Returns maximum possible score for "perfect" results. Should be the same
  // for different values of current_inputs_[0].
  virtual long long MaxScore() const override;
  // Initializes new value(s) for current_inputs_.
  virtual void ResetInputs() override;

  int expected_value() { return expected_value_; }

protected:
  // Random number generator.
  Random &gen_;
  // Number of copies of the two values to be added in current_inputs_. The
  // assumptions is that multiple copies of the values in inputs will make it
  // easier to evolve the Program to sum these values to results.
  int number_of_copies_in_current_inputs_ = 1;
  // expected_value_ holds the value expected in results[1]. In this case, it is
  // current_inputs_[0] + current_inputs_[1].
  int expected_value_ = 0;
};

} // namespace viaevo

#endif // VIAEVO_EXAMPLES_010_SUM_TWO_SCORER_SUM_TWO_H_