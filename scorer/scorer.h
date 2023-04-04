// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#ifndef VIAEVO_SCORER_SCORER_H_
#define VIAEVO_SCORER_SCORER_H_

#include <vector>

#include "../program/program.h"
namespace viaevo {

// Scorer is an abstract base class defining the interface to provide inputs and
// score results for a Program. Scorer is intended to be subclassed for specific
// applications (see examples in //examples).
class Scorer {
public:
  virtual ~Scorer() = default;
  // Returns score for a specific Program.
  virtual long long Score(const Program &program) const = 0;
  // Optional additional scoring based on Program's results_history_ enables
  // e.g. "rewarding" Programs that compute different results on different
  // inputs for the same code.
  virtual long long
  ScoreResultsHistory(const std::vector<std::vector<int>> &) const {
    return 0;
  }
  // Returns maximum possible score for "perfect" results. Should not depend on
  // current_inputs_.
  virtual long long MaxScore() const = 0;
  // Returns maximum possible score for "perfect" results history.
  virtual long long MaxScoreResultsHistory() const { return 0; }

  // Generates new current_inputs_ for a new round of evaluation of Programs.
  virtual void ResetInputs() = 0;

  const std::vector<int> &current_inputs() { return current_inputs_; };

protected:
  // Inputs for Programs for the current iteration of evaluation.
  std::vector<int> current_inputs_;
};

} // namespace viaevo

#endif // VIAEVO_SCORER_SCORER_H_