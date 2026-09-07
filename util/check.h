// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#ifndef VIAEVO_UTIL_CHECK_H_
#define VIAEVO_UTIL_CHECK_H_

#include <string>

namespace viaevo {
namespace internal {

// Reports a failed VIAEVO_CHECK on stderr and terminates the process with
// EXIT_FAILURE. Not to be called directly, use the macro below.
[[noreturn]] void CheckFailed(const char *condition, const std::string &message,
                              const char *file, int line);

} // namespace internal
} // namespace viaevo

// VIAEVO_CHECK(condition, message) validates `condition` and, if it does not
// hold, prints "file:line: Check failed: condition: message" on stderr and
// terminates the process with EXIT_FAILURE.
//
// Unlike assert(), the check is always compiled in, including under NDEBUG
// (which bazel's -c opt defines). It is therefore the tool for validating
// anything that comes from outside the program - data files, command line
// flags and constructor arguments - where silently proceeding with invalid
// state in an optimized build would corrupt an evolution run.
//
// Keep assert() for internal invariants that no input can violate.
//
// `message` is any expression convertible to std::string; it is only evaluated
// when the check fails, so composing it from runtime values (e.g. a filename)
// costs nothing on the success path.
#define VIAEVO_CHECK(condition, message)                                       \
  (static_cast<bool>(condition)                                                \
       ? (void)0                                                               \
       : ::viaevo::internal::CheckFailed(#condition, (message), __FILE__,      \
                                         __LINE__))

#endif // VIAEVO_UTIL_CHECK_H_
