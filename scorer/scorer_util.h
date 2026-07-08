// Copyright (c) 2023 Richard Baran
//
// All components of viaevo are licensed under the MIT License.
// See LICENSE.txt in the root of the repository.

#ifndef VIAEVO_SCORER_SCORER_UTIL_H_
#define VIAEVO_SCORER_SCORER_UTIL_H_

#include <cstddef>
#include <vector>

namespace viaevo {

// Returns true if results has at least min_size elements.
//
// Otherwise logs (to stderr, with a running occurrence count) that a Program
// returned fewer result values than the scorer expects and returns false.
// Scorers should call this before indexing into last_results(): the vector is
// left empty whenever an ELF process terminates before its results are read
// back (a crash, an early exit, or a skipped PTRACE_GETREGS read; see
// Program::MonitorElfProcess), and indexing it would otherwise trip
// libstdc++'s hardened operator[] assertion and abort the run.
//
// `where` is a short label (e.g. "ScorerGuessValue::Score") used in the log
// message. Thread-safe, as scoring runs under std::execution::par.
bool ResultsHaveMinSize(const std::vector<int> &results, std::size_t min_size,
                        const char *where);

} // namespace viaevo

#endif // VIAEVO_SCORER_SCORER_UTIL_H_
