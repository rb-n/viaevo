// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "scorer_util.h"

#include <atomic>
#include <iostream>

namespace viaevo {

bool ResultsHaveMinSize(const std::vector<int> &results, std::size_t min_size,
                        const char *where) {
  if (results.size() >= min_size)
    return true;

  static std::atomic<long long> count{0};
  long long n = ++count;
  // Leading newline so the message does not mangle the '\r'-updated progress
  // line printed by the evolver.
  std::cerr << "\n[" << where << "] undersized results (size=" << results.size()
            << ", expected >= " << min_size
            << "); scoring 0. occurrences so far: " << n << std::endl;
  return false;
}

} // namespace viaevo
