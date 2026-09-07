// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#include "check.h"

#include <cstdlib>
#include <iostream>

namespace viaevo {
namespace internal {

void CheckFailed(const char *condition, const std::string &message,
                 const char *file, int line) {
  std::cerr << file << ":" << line << ": Check failed: " << condition << ": "
            << message << std::endl;
  // TODO: Throw a viaevo exception here instead of exiting once the
  // library-wide error path replaces myfail (RECOMMENDATIONS 2.1). Keeping the
  // termination in this single place makes that a one-line change.
  exit(EXIT_FAILURE);
}

} // namespace internal
} // namespace viaevo
