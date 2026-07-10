// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#ifndef VIAEVO_EVOLVER_EVOLVER_H_
#define VIAEVO_EVOLVER_EVOLVER_H_

namespace viaevo {

// Evolver is an abstract base class defining the interface for driving an
// evolution run. Concrete strategies (e.g. EvolverAdHoc) implement Run().
//
// Extracting this interface lets alternative strategies ((mu + lambda),
// tournament selection, MAP-Elites, ...) be swapped in without touching callers
// such as main.cc (see RECOMMENDATIONS.md 2.6).
class Evolver {
public:
  virtual ~Evolver() = default;

  // Runs the evolution to completion (until a "perfect" score is reached or the
  // maximum number of generations is exhausted).
  virtual void Run() = 0;
};

} // namespace viaevo

#endif // VIAEVO_EVOLVER_EVOLVER_H_
