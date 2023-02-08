// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#ifndef VIAEVO_SCORER_SCORER_H_
#define VIAEVO_SCORER_SCORER_H_

#include <vector>

namespace viaevo {

// Scorer is an abstract base class defining the interface to provide inputs and
// score results for a Program. Scorer is intended to be subclassed for specific
// applications (see examples in //examples).
class Scorer {
public:
  // Returns score for specific results.
  virtual long long Score(const std::vector<int> &results) const = 0;
  // Returns maximum possible score for "perfect" results. Should not depend on
  // current_inputs_.
  virtual long long MaxScore() const = 0;
  // Generates new current_inputs_ for a new round of evaluation of Programs.
  virtual void ResetInputs() = 0;

  const std::vector<int> &current_inputs() { return current_inputs_; };

protected:
  // Inputs for Programs for the current iteration of evaluation.
  std::vector<int> current_inputs_;
};

} // namespace viaevo

#endif // VIAEVO_SCORER_SCORER_H_