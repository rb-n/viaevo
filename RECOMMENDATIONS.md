# Viaevo — Recommendations for the Future

A structured review of the `viaevo` project (genetic programming via random
changes at the machine-code level). The goal is to flag antipatterns, surface
missed opportunities, and propose concrete directions. Observations reference
the current code where relevant.

The sections are independent and roughly ordered as: bugs to fix first, then
C++ design, the SIGUSR idea, template-program evolvability, benchmark problems,
prior art, scoring, infrastructure/reproducibility, documentation, operability
features (checkpointing, program libraries, interactive control), a
prioritized roadmap, and two follow-up review passes (§12, 2026-08; §13,
2026-09) whose items amend the roadmap.

---

## 1. Correctness bugs to fix first

These are concrete defects found while reading the code. They undermine
everything built on top, so fix them before the larger refactors.

- **[DONE]** **`Random` stream operators have undefined behavior.** In
  `@/home/baran/prjs/viaevo/util/random.h:38-48`, both `operator<<` and
  `operator>>` are declared to return a stream reference but contain no
  `return` statement. Falling off the end of a non-`void` function is UB. Add
  `return ost;` / `return ist;`. This matters because these operators are the
  natural way to serialize/restore RNG state for reproducible runs (see §8).

- **[DONE]** **Signed/unsigned mismatch in `last_rip_offset_`.** It is
  `unsigned long long` initialized to `-1`
  (`@/home/baran/prjs/viaevo/program/program.h:119`). In
  `@/home/baran/prjs/viaevo/mutator/mutator_point_last_instruction.cc:24-25`
  the guard `last_rip_offset < 0` can never be true for an unsigned type, so
  the "outside mutable code" fallback only triggers via the `>= code.size()`
  branch and the sentinel `(unsigned long long)-1` accidentally happens to be
  caught by that branch. This works by luck, not design. Use a signed type or
  an explicit `std::optional<uint64_t>` sentinel and compare against that.

- **[DONE]** **`std::execution::par` over `fork()`/`ptrace()` is fragile.** The
  seccomp filter is now compiled once in the parent (`GetSeccompProgram` in
  `program.cc`) and the child only installs the pre-built BPF program via
  async-signal-safe `prctl`/`seccomp` syscalls, removing the libseccomp
  allocation from the post-fork child. `Program::Execute` now uses `vfork()`
  instead of `fork()`, and the child execs directly from the in-memory ELF fd
  via `execveat(elf_mem_fd_, "", ..., AT_EMPTY_PATH)`, using only
  async-signal-safe syscalls and a `_exit()`-based error path (`child_fail`).
  This eliminates the page-table copy and the fork-in-a-thread window.
  (`posix_spawn` was evaluated but rejected: it has no hook to run the required
  child-side `PTRACE_TRACEME` / seccomp install / `setitimer`.) A persistent
  worker-process pool (§8) remains as a separate, larger throughput
  optimization.

  *Historical context — why plain `fork()` was a problem and why the
  alternatives were preferred:* programs are executed in parallel
  (`std::execution::par` in `EvaluatePrograms`), and the old `fork()`-based
  `Program::Execute` ran allocation-heavy, non-async-signal-safe code
  (libseccomp) in the child of a multithreaded process — a latent source of the
  intermittent "random PTRACE_GETREGS failures" noted in the TODOs.

  - **The core problem with `fork()` here is the "fork-in-a-thread" hazard.**
    `fork()` duplicates only the calling thread but the *entire* address space,
    including the internal locks of the other threads that did **not** come
    along. If another worker thread happened to hold, say, the allocator's lock
    (or any libc/internal lock) at the instant of `fork()`, that lock is now
    permanently held in the child by a thread that no longer exists. The child
    may then deadlock the moment it touches `malloc`, stdio, or the dynamic
    loader. POSIX therefore restricts the child to *async-signal-safe* calls
    only between `fork()` and `exec()`. The current child does far more than
    that (libseccomp — now fixed — `setitimer`, `ptrace`, `fexecve`, and any
    implicit allocations), so correctness rests on timing luck. Under
    `std::execution::par` (`evolver_adhoc.cc:114-130`) there are many threads
    forking concurrently, which maximizes the probability of forking while some
    lock is held — a strong fit for the intermittent failures observed.

  - **`posix_spawn` is preferred because it removes that window by design.** It
    is purpose-built to create-and-exec a process and, on Linux/glibc, is
    implemented on top of `clone(CLONE_VM | CLONE_VFORK)`: the child shares the
    parent's address space and runs in a tiny, controlled helper that performs
    only async-signal-safe operations before `execve`, while the parent is
    suspended so there is no concurrent mutation of shared state. The set of
    pre-exec actions (fd setup, signal mask/`setitimer`-equivalents via
    `posix_spawn` file actions/attributes) is fixed and audited by the C
    library, so the application never executes arbitrary, allocation-heavy code
    in the fragile post-fork context. It is also typically faster than `fork()`
    because no page tables are copied (no copy-on-write setup for a space we are
    about to discard at `exec`).

  - **`vfork`+`execveat` is preferred for the same address-space reason, with
    even less overhead.** `vfork` suspends the parent and lends its address
    space to the child until `execveat`, so there is zero page-table duplication
    and no risk of concurrent threads mutating shared state during the child's
    brief life. `execveat(elf_mem_fd_, "", ..., AT_EMPTY_PATH)` then execs
    directly from the existing in-memory ELF fd (`elf_mem_fd_`), matching what
    `fexecve` already does. The catch is that the contract is strict (the child
    must not return or touch the parent's stack before `exec`), so it is best
    reserved for the case where the pre-exec work is genuinely minimal; given a
    *pre-built* seccomp program and an fd-based exec, the remaining work fits
    that contract. `posix_spawn` is the safer default; raw `vfork` is the
    lowest-overhead option when every microsecond per evaluation matters.

  - **A pre-forked worker-process pool is preferred when throughput dominates,
    and it sidesteps the problem entirely.** Instead of spawning a process per
    evaluation, fork a small set of sandboxed workers **once, at startup, before
    any threads are created** (so the fork-in-a-thread hazard never arises).
    Each worker installs the seccomp filter and then loops: receive "here is new
    code + inputs" over a pipe/socket, run and `ptrace` it, send back results.
    This amortizes the fixed per-run costs that currently dominate (fork, ELF
    setup, filter install, alarm arming) across thousands of evaluations,
    converts the parallelism from "fork under `par`" into ordinary message
    passing to long-lived processes, and makes evaluation naturally
    deterministic/reproducible per worker (§8). The trade-off is more
    implementation complexity (a request/response protocol and worker lifecycle
    management), which is why it is the larger, higher-payoff step rather than a
    drop-in.

  In short: plain `fork()` is fragile here because it leaves the child in a
  multithreaded-inherited, allocation-unsafe state; `posix_spawn` and
  `vfork`+`execveat` eliminate that state window (and copy less memory), while a
  worker pool avoids paying the per-evaluation spawn cost at all.

- **[DONE]** **MNIST scorer re-parses the whole file header on every sample.**
  `ScorerMnistDigits::LoadSample` used to open both files, validate the
  magic/dimension header, and seek for *every* input draw. The constructor now
  calls `LoadData()` once to validate the headers and cache all pixel/label
  bytes into `images_data_`/`labels_data_`; `LoadSample` is now a pure in-memory
  `memcpy` from those caches with no file I/O. With `evaluations_per_program`
  draws × population × generations this removes a large amount of redundant I/O.

- **[DONE]** **Hardcoded magic numbers for signals/syscalls.** The raw
  `last_stop_signal() == 14` (in `evolver_adhoc.cc` and `scorer_mnist_digits.cc`)
  and `last_stop_signal_ != 5` (in `program.cc`) now use the named `SIGALRM` and
  `SIGTRAP` constants (`<signal.h>`), and the raw syscall number in
  `seccomp_rule_add(ctx, SCMP_ACT_ALLOW, 322, 0)` is now `SCMP_SYS(execveat)`,
  which libseccomp resolves per-architecture (the raw `322` was x86-64-specific
  and would have silently broken on other arches). The `program_test.cc`
  assertions still compare against literal `5`/`9` for the observed
  stop/term signals, but those are test expectations with explanatory comments
  rather than production logic.

- **[DONE]** **`Score()` indexes `results[1..10]` without checking size.**
  Scorers assume `results` has ≥11 elements. This is not only a smaller-template
  concern: at runtime `Program::last_results()` is left empty whenever the ELF
  terminates before its results are read back (crash/SIGSEGV, early exit, or the
  ignored `PTRACE_GETREGS` early-return in `MonitorElfProcess`), and only
  `SIGALRM` was special-cased. The unchecked `results[1]` then trips libstdc++'s
  hardened `operator[]` assertion and aborts a run. Fixed across all scorers via
  a shared `ResultsHaveMinSize` helper (`scorer/scorer_util.{h,cc}`) that guards
  a minimum size and logs undersized occurrences to stderr with a running count
  so their frequency can be gauged. `ScorerMnistDigits` (`Score` +
  `ScoreResultsHistory`) and the four example scorers (`000_guess_value`,
  `001_copy_value`, `002_double_value`, `010_sum_two`) all use it and
  return/skip with a score of 0; each has a `ScoreUndersizedResults` regression
  test. Longer term, consider making the upstream contract explicit (e.g. have
  the monitor always populate `last_results_`, or expose a "results valid"
  flag).

---

## 2. General C++ software design

The code is readable and the module boundaries (`program`, `mutator`,
`scorer`, `evolver`, `util`) are sensible. The main weaknesses are around error
handling, the `Program` god-object, and a few idiomatic issues.

### 2.1 Error handling: replace `myfail`/`exit` with exceptions or `expected`

`myfail` (`@/home/baran/prjs/viaevo/program/program.cc:30-33`) calls
`perror` + `exit(EXIT_FAILURE)` on any error. This is fine for a script but bad
for a library:

- It kills the whole evolution (and all sibling processes) on a single
  recoverable per-program error such as a transient `fork` failure.
- It is untestable — you cannot unit-test the failure path without
  `EXPECT_DEATH`.
- It prevents graceful degradation (e.g. "this offspring failed to execute,
  score it 0 and move on").

Prefer exceptions for truly exceptional setup failures (bad ELF, no `.symtab`)
and a `Status`/`std::expected<T,E>`-style return for per-execution failures that
the evolver should tolerate. At minimum, distinguish *fatal* (config) from
*recoverable* (one bad run) errors.

### 2.2 `Program` is doing too much (SRP violation)

`Program` currently owns: ELF parsing, in-memory file management, symbol-table
memoization, process spawning, ptrace monitoring, seccomp policy, result
reading, and code mutation accessors. Consider splitting:

- **[DONE]** `ElfImage` — owns the `memfd` and the resolved `SymbolData`, with
  `GetCode/SetCode/SetCodeToAllNops/GetInputs/SetInputs/Save`
  (`program/elf_image.{h,cc}`, unit-tested without forking in
  `program/elf_image_test.cc`). `Program` now holds an `ElfImage` member and
  delegates its `GetElfCode`-style accessors to it, keeping the external API
  unchanged; `program.cc` retains only execution (vfork/exec, seccomp, ptrace
  monitoring, results reading).
- **[DONE]** `ElfLayout`/`SymbolData` resolver — the `InitializeElfSymbolData`
  logic (~140 lines) is now a standalone free function `ResolveElfSymbolData(int
  fd)` with the `SymbolData` struct in `@/home/baran/prjs/viaevo/program/elf_layout.h`
  / `elf_layout.cc`, unit-tested in `program/elf_layout_test.cc`. `Program` now
  calls it from `Create`. (It still terminates on malformed input; the
  recoverable-error split is tracked in §2.1.)
- **[DONE]** `Sandbox` — owns the seccomp policy, the `vfork`/`execveat` spawn,
  the int3-breakpoint-driven `ptrace` monitor loop, and the wall-clock timeout
  (`program/sandbox.{h,cc}`). `Sandbox::Execute(const ElfImage&, ExecuteMode)`
  returns an `ExecutionResult` (the `last_*` observations + `ptrace_stops`);
  it holds no per-execution state (`Execute` is `const`) and the timeout is now
  a configurable member (`timeout_usec`, default 50 ms) rather than a hardcoded
  literal. `ExecuteMode` moved to `Sandbox` (re-exported as `Program::ExecuteMode`).
  `Program` now holds an `ElfImage` + a `Sandbox`, and `Program::Execute` just
  delegates and caches the result — `program.cc` shrank from ~500 lines to ~25.
  `Sandbox` is unit-tested directly, without `Program`, in
  `program/sandbox_test.cc`.

The awkward "default constructor for mocking" TODO
(`@/home/baran/prjs/viaevo/program/program.h`) is **still open**: the example
scorer tests subclass `Program`, default-construct it, and set the `protected`
`last_results_` / `last_stop_signal_` directly (`ProgramMock` in the four
example `scorer_*_test.cc` files, plus a bare `viaevo::Program program;` in
`scorer/scorer_mock_test.cc`). Removing the default ctor requires giving the
scorers a small "execution result" interface to mock instead of subclassing
`Program` — a separate change touching those five test doubles, left for later.
The `Sandbox` split above is a prerequisite that makes it straightforward (the
scorers only need `last_results()` / `last_stop_signal()`, which an
`ExecutionResult`-shaped interface already provides).

### 2.3 Static mutable global state

**[DONE]** `Program::symbol_data_map_` and `expected_ptrace_stops_map_` were
static `unordered_map`s populated lazily in `Create` — a latent data race the
first time a given ELF was seen. Both are gone with the `int3` switch (§3):
the stops calibration no longer exists, and each `Program` now resolves its
own `SymbolData` from its in-memory ELF at construction (a one-time parse per
instance, negligible at startup). The remaining static in `program.cc` is the
seccomp BPF cache in `GetSeccompProgram`, which is immutable after thread-safe
first initialization.

### 2.4 Const-correctness and small idioms

- **[DONE]** `Scorer::current_inputs()` (`scorer.h:38`) is non-const but returns
  a const ref; it should be `const`. Several `Program` getters are fine.
- **[DONE]** `Program::Create(const std::string filename)` takes the string by
  value with no need; now `const std::string&`. (`std::string_view` would need
  `.c_str()`/map-key adjustments; the const ref was the minimal fix.)
- The `// TODO: Remove relative paths.` includes (`../mutator/...`) appear
  throughout. Set up include paths in the Bazel `BUILD` files (`includes` /
  `strip_include_prefix`) and use `#include "viaevo/mutator/mutator.h"`. This
  prevents fragility when files move.
- `RunElfProcess` builds the seccomp allowlist by hand-appending syscalls
  discovered "by looking into syslog" (`program.cc:429-450`). This is brittle
  across libc/compiler versions. Generate the policy from a named list with a
  comment per entry, and consider `SCMP_ACT_TRAP` + logging in a debug build to
  discover additions automatically.

### 2.5 Configuration object instead of long ctor parameter lists

`EvolverAdHoc`'s constructor takes 11 parameters
(`@/home/baran/prjs/viaevo/evolver/evolver_adhoc.h:29-34`) and `main.cc` mirrors
them as flags. Introduce an `EvolverConfig` struct (designated initializers in
C++20, or a builder). This makes adding parameters non-breaking and lets you
serialize the exact config alongside results for reproducibility.

### 2.6 Abstract the `Evolver`

**[PARTIALLY DONE]** Extracted an abstract `Evolver` interface
(`@/home/baran/prjs/viaevo/evolver/evolver.h`) with a pure-virtual `Run()`, and
made `EvolverAdHoc` derive from it. The three stages are now separate,
unit-tested member functions: `SelectParents`, `CreateOffspring`, and
`EvaluatePrograms` (see `EvolverAdHocTest.CreateOffspring` /
`EvolverAdHocTest.EvaluatePrograms`). Still open: adding concrete alternative
strategies (tournament selection, NSGA-II for multi-objective MNIST, MAP-Elites,
etc.) behind the interface so `main.cc` can select among them.

### 2.7 Minor

- Prefer `std::byte` or `uint8_t` over `char` for machine code buffers to avoid
  signedness surprises in the bit-flip math.
- **[DONE]** The bit-flip in `mutator_point_last_instruction.cc` (and
  `mutator_point_random.cc`) now uses `(1u << bit_pos)` to avoid a signed shift
  into the char code buffer. (The broader `char` -> `uint8_t`/`std::byte` buffer
  type change is deferred to the `Program`/`ElfImage` refactor, §2.2.)
- Consider `std::span` (C++20) for the code/inputs accessors to avoid copying
  whole `std::vector<char>` on every `GetElfCode()` call — currently every
  mutation copies the full 3,300-byte `main` twice.

---

## 3. Detecting evolvable code via SIGUSR vs. counting syscalls

**[DONE — option 1 implemented]** `Program` now detects "evolvable code
reached" via an `int3` breakpoint: at the post-`execveat` SIGTRAP the tracer
reads the process's `start_code` from `/proc/[pid]/stat`, pokes `0xCC` at
`main`'s runtime address (`PTRACE_POKETEXT`), and continues *untraced* to the
breakpoint; there it restores the original byte, rewinds `rip`, and switches to
`PTRACE_SYSCALL` so the first syscall/signal from evolved code still terminates
the process (before any stopped syscall executes). A well-behaved run is
exactly 3 ptrace stops (down from ~44 syscall-stepped stops). The
`expected_ptrace_stops_` calibration run, both static memoization maps (§2.3),
and the per-ELF toolchain coupling are gone; `Execute` takes an `ExecuteMode`
(`kTerminateInEvolvedCode` / `kStopAtMainEntry` / `kRunToCompletion`) instead
of a stop budget. The breakpoint lives only in the traced process's memory —
the ELF image and evolvable code are untouched (regression-tested). Original
discussion below.

You asked specifically whether to raise a signal (e.g. `raise(SIGUSR1)`) at the
start of `main` to mark "evolvable code reached", instead of counting the
expected number of ptrace stops/syscalls.

**Recommendation: yes, switch to an explicit marker — but a software
breakpoint (`int3`) or a `PTRACE_SETOPTIONS` event is even better than a
signal.** Reasons and options, best first:

1. **`int3` breakpoint at the first byte of `main`.** Since you already parse
   the symbol table and know `main`'s address, write an `0xCC` byte at the
   entry, run with `PTRACE_CONT`, and you get a `SIGTRAP` exactly when `main`
   begins. Restore the original byte, set up the timeout, then switch to
   `PTRACE_SYSCALL` for the "first syscall/signal ends it" logic. This is
   deterministic, independent of how many syscalls the loader/libc makes, and
   robust across compiler/glibc upgrades — which is exactly the brittleness
   that `expected_ptrace_stops_` suffers from today
   (`program.cc:435-450` exists only because syscall counts drift).

2. **`raise(SIGUSR1)` (your idea).** Cleaner than counting and easy to
   implement: insert it via the template's prologue (not the evolvable region!)
   and watch for `WSTOPSIG == SIGUSR1`. Downsides: it costs a syscall
   (`tgkill`) you must allow through seccomp; the signal must be emitted from
   *non-evolvable* code so a mutation can't remove/forge it; and you must ensure
   the marker instruction sits outside the bytes the mutators touch.

3. **A dedicated marker syscall.** E.g. a `getpid()` or a write to a sentinel —
   but this is just a less-clean version of (2).

Whichever you choose, the deeper win is **eliminating the `expected_ptrace_
stops_` calibration run entirely** (`Program::Create` currently executes the
template once just to count stops, `program.cc:69`). A breakpoint at `main`
removes that fragility and the architecture/toolchain coupling.

One caveat: with a marker you still want the "terminate on first syscall/signal
after `main`" semantics for safety. Keep `PTRACE_SYSCALL` after the marker
fires; the marker only replaces the *counting* of pre-`main` stops.

---

## 4. Making template programs more evolvable

This is the highest-leverage research lever. Today `simple_small.c`
(`@/home/baran/prjs/viaevo/elfs/simple_small.c`) is mostly `nop`s + chained
`dummy[i]=dummy[i+1]` assignments + a few `jmp .+127`. The README already
observes that all-`nop` starts never make progress, and that having *something*
(assignments) bootstraps evolution. Build on that insight deliberately:

### 4.1 Provide a richer "instruction vocabulary" in the template

**[PARTIALLY DONE]** `elfs/complex_large.c` now carries a generic (benchmark
agnostic) version of this vocabulary in named sections: loads/stores touching
`inputs`/`results`/`scratchspace` directly (all stores into `results` read
from locations holding `-1` in the template image, so an unevolved run leaves
`results` visibly unchanged — "neutral vocabulary"), a balanced ALU mix
(`+ - * & | ^ << >>` plus the existing `/` and `%`), register-register
operations via inline asm with register constraints, immediate-bearing
instructions (4-byte immediate fields as smooth mutation targets), the
carried-over control flow, AVX2 vector ops, and nop-dense "junk DNA" rounds
behind relative jumps (each jump's landing verified via objdump to fall inside
a nop sled). The template is wired into `//elfs`, both example binaries, and
`program_test` (`CreateExecuteComplexLarge`). Still open: "no I/O access" vs
"with I/O access" template *variants* for a quantitative comparison, pushing
the same vocabulary into the smaller templates, and generating templates from
a spec (§4.5).

The available raw material constrains what point mutations can reach. Seed the
evolvable region with a diverse, balanced palette of *valid* building blocks so
that a single bit flip is more likely to land on something useful:

- **Loads/stores touching `inputs` and `results` directly.** The README notes
  there are deliberately *no* instructions accessing `inputs`/`results`. That
  is precisely why "guess from all-nops" fails and why copy tasks are hard —
  the evolver must first invent an addressing-mode instruction by chance.
  Including a few `results[k] = inputs[j];` and `results[k] = dummy[j];`
  templates gives evolution a foothold to repurpose, dramatically widening the
  reachable space. (You may want both "no I/O access" and "with I/O access"
  template variants to study the effect quantitatively.)
- **Arithmetic and bitwise ops between scratch variables:** `+ - * & | ^ <<`.
  `intermediate_medium.c` already adds some — push further and make the mix
  uniform.
- **A few register-only operations** (so mutations can stumble into
  register-based computation, which is cheaper than memory round-trips).
- **Immediate-bearing instructions** (`mov $imm, reg`): mutating the immediate
  is a smooth, low-disruption knob — flipping bits in an immediate keeps the
  instruction valid, which is ideal for the "guess value" task.

### 4.2 Favor instruction-length stability

Random bit flips are most productive when they keep the byte stream
re-decodable. Instructions whose *encoding length is insensitive* to operand
bits (register-register ALU ops, fixed-width immediates) act as "smooth"
genes; variable-length/prefix-heavy encodings cause frame-shift cascades. Bias
the template toward fixed-length encodings, and consider a mutation operator
that flips bits only within immediate/operand fields of already-decoded
instructions (requires a lightweight x86 length disassembler — see §6, Orlov &
Sipper's FINCH and fleshy/“soft” representations).

### 4.3 Add structured "junk DNA" for recombination

Your `jmp .+127` idea (accumulating silent mutations in unreached code) is
good. Extend it: include several self-contained, currently-unreachable
*subroutines* (blocks ending in a jump back) that compute useful primitives
(copy, add, compare-and-set). Recombination can then splice a working block in
wholesale rather than rebuilding it bit-by-bit. This is "scaffolding" and is
well supported by the homologous-crossover literature (Nordin/Banzhaf, §6).

### 4.4 Align mutable regions to ease crossover

Homologous crossover works best when parents share structure. If every template
instance starts identical and you track instruction boundaries, recombination
can be made *boundary-aware* (cut at instruction starts) — drastically reducing
invalid-instruction generation versus the current byte-offset cuts in
`mutator_recombine_random.cc:20-29`. See §13.4 for why the *position* of the
cut matters even more than its alignment: x86-64 RIP-relative operands and
relative jumps change meaning when moved to a different offset.

### 4.5 Make the palette a generated artifact

Rather than the hand-rolled, copy-pasted "10 rounds" in `simple_small.c`,
generate templates from a small spec (counts per instruction class, NOP
padding, jump density). This makes "what vocabulary helps?" an experiment you
can sweep, not a manual edit.

---

## 5. Additional benchmark problems

Current suite: guess constant, copy value, double value, sum two, MNIST. Good
progression. Suggested additions, grouped by what capability they probe:

### 5.1 Integer/data-flow primitives (extend the "simple" family)

- **max/min of two (or N) inputs** — requires a conditional + select.
- **conditional/branching:** `results[1] = (inputs[0] > inputs[1]) ? a : b`.
- **negate / absolute value** — sign handling.
- **multiply by constant / by input** — beyond `double_value`.
- **sum/product of an array**, **dot product of two arrays** — loops with
  accumulation (a natural bridge to MNIST).
- **count occurrences / find index of a value** — search loops.
- **identity over a vector** (copy `inputs[0..n]` to `results[0..n]`) —
  multi-output, tests whether evolution can write several results coherently.

### 5.2 Classic GP/program-synthesis benchmarks

- **Boolean circuits:** N-bit parity, multiplexer (6-/11-mux), comparator —
  staples of GP that have known difficulty curves and let you compare against
  the literature.
- **Symbolic regression** on integer-quantized polynomials (e.g. Koza's
  quartic) — even quantized, it's a recognized benchmark.
- **Sorting networks / sort 3 numbers** — small but combinatorially rich.
- **Program Synthesis Benchmark Suite (PSB1/PSB2)** problems that fit an
  integer-in/integer-out harness (e.g. "smallest", "compare string lengths"
  analogues, "last index of zero"). These come with published baselines.

### 5.3 Harder ML-style tasks (after MNIST)

- **Iris / other small tabular classification** — fewer inputs than MNIST,
  faster iteration, still non-trivial.
- **Binary MNIST (0 vs 1)** as a stepping stone before 10-class — your 22%
  result suggests the 10-class problem is near the method's current ceiling; a
  2-class variant gives a cleaner signal for measuring improvements.
- **1-D signal tasks:** threshold detection, peak counting.

### 5.4 Benchmark hygiene

- Define each benchmark with a **fixed max score, a fixed seed set, and a
  success criterion** (you already do this informally). Record **median
  generations-to-solution and success rate over K seeds** as the standard
  metric — you already produce these tables; formalize them into a script.
- Add a **trivial control** (random search with the same evaluation budget) per
  benchmark. Showing you beat random search is the single most convincing
  result for the "is this hopeless?" question the README raises.

---

## 6. Prior art and publications you may have missed

You cite the core machine-code GP lineage (Nordin/Banzhaf AIMGP, Kühling,
Orlov & Sipper, GenProg, STOKE, Langdon). Gaps worth reading and citing:

### 6.1 Directly adjacent (machine-code / binary evolution)

- **Orlov & Sipper, "FINCH: A System for Evolving Java (Unrestricted)
  Bytecode" (IEEE TEC 2011)** — the journal expansion of the 2009 paper you
  cite; details *how they keep mutated bytecode valid* (the "compatible
  crossover" idea). Highly relevant to your invalid-instruction problem.
- **Nordin, Banzhaf, Francone — AIMGP / "Discipulus" / CGPS** — you cite the
  1999 chapter; the broader AIMGP body (and the commercial Discipulus system)
  is the canonical "evolve machine code directly" work and discusses register
  machines and instruction-block validity.
- **Schulte et al., "Software Mutational Robustness" and "Genetic
  Programming for Repair of Binaries / ASM" (GECCO/GPEM, ~2013–2014)** —
  Eric Schulte's thesis on *evolving assembler and ELF binaries* is almost
  exactly your setting; covers neutral mutations and post-compilation
  evolution.
- **Le Goues et al. follow-ups to GenProg** and **Schkufza STOKE follow-ups**
  (you cite the originals) — relevant for the "edit existing valid code" vs
  "random bytes" trade-off.

### 6.2 Representation & validity (the central problem here)

- **Linear GP** (Brameier & Banzhaf, *Linear Genetic Programming*, 2007) — the
  textbook treatment of register-machine/linear representations; directly
  informs §4 (instruction vocabulary, effective vs intron code).
- **Grammatical Evolution** (O'Neill & Ryan) — maps a linear genome through a
  grammar so *every* genome is valid. A "grammar of valid x86 instructions"
  could give you unrestricted search *without* the invalid-instruction tax.
- **Cartesian GP** (Miller) — graph representation with high neutrality;
  relevant to your "silent mutations in unreached code" observation.

### 6.3 Search dynamics & neutrality (explains your empirical results)

- **Neutral networks / neutral theory in EC** (Banzhaf, Vanneschi, "neutrality
  in GP") — your finding that all-`nop` starts stall while assignment-seeded
  starts progress is a *fitness-landscape/neutrality* story; this literature
  gives you the vocabulary and predictions.
- **Novelty Search** (Lehman & Stanley) and **Quality-Diversity / MAP-Elites**
  (Mouret & Clune, 2015) — instead of (or alongside) the scalar score, maintain
  an archive of behaviorally diverse programs. This directly addresses the
  "broken clock right twice a day" problem your `ScoreResultsHistory` hack is
  fighting (`scorer_mnist_digits.cc:60-98`).
- **Fitness-distance / deception and the design of smooth fitness functions** —
  your bit-match scoring for "guess value" is essentially a Hamming-distance
  gradient; cite the "graduated/lexicase" selection work below for principled
  alternatives.
- **Lexicase selection** (Spector, Helmuth & McPhee) — selects on individual
  test cases rather than an aggregate sum. For multi-input tasks (copy, MNIST)
  this is markedly better than your summed score and would likely raise success
  rates. Strong recommendation to try.

### 6.4 Superoptimization & synthesis context

- **Massalin, "Superoptimizer" (1987)** — the origin of exhaustive machine-code
  search; good for framing.
- **Bansal & Aiken peephole superoptimization**, and **Sasnauskas et al.
  Souper** — modern superoptimizers; useful contrast for "random vs guided"
  machine-code search.

---

## 7. The scoring / fitness design (cross-cutting)

The scorers encode a lot of hand-tuned "assumption" logic with large magic
multipliers (`1'000'000`, `1'000'000'000`, `…'000'000'000'000'000` in
`scorer_mnist_digits.cc`). This lexicographic-via-magnitude trick is clever but
fragile and hard to reason about (one term can silently dominate another if a
count is larger than expected; note the `std::min(999UL, …)` guard you already
needed).

Recommendations:

- **Make multi-objective explicit.** Use a tuple/vector of objectives with
  Pareto comparison, or **lexicase selection** (§6.3), instead of packing
  priorities into integer magnitudes. This removes the magic numbers and the
  overflow worries (`long long` is already brushing 1.2e16 in MaxScore).
- **Separate "made progress" shaping from "correct" reward** into named
  components so you can log each and see *which* gradient is driving evolution.
- **The `score_results_history` mechanism is a behavioral-diversity proxy** —
  reframe it via QD/novelty (§6.3) for a principled version.
- **Centralize the SIGALRM penalty.** Both scorers special-case
  `last_stop_signal()==14`; this belongs in the evolver/runner as a
  "non-viable" flag, not duplicated per scorer.

---

## 8. Infrastructure, performance, reproducibility

- **Reproducibility:** repeated runs with the same seed can diverge (noted in
  the `--random_seed` flag help) because non-deterministic programs + parallel
  scheduling perturb selection. Fixing the `Random` serialization (§1) and
  optionally a deterministic (single-thread or seeded-per-program) evaluation
  mode would make trials reproducible — important for a research artifact.
- **Per-program RNG.** A single shared `Random` consumed under
  `std::execution::par` (the mutators capture `gen_`) is itself a data race
  (mutation happens single-threaded today, evaluation parallel — but verify).
  Give each worker its own stream (seeded deterministically from the master) to
  make parallel runs both safe and reproducible.
- **Worker-process pool.** Forking + seccomp-init + ELF setup per execution is
  the dominant cost. A pool of persistent sandboxed workers that accept "here's
  new code + inputs, run it" over a pipe would cut overhead dramatically and fix
  the fork-in-threads issue (§1).
- **Persist run metadata.** Emit a machine-readable record per run (config,
  seed, RNG end-state, generations, final score, git commit) so the README
  tables can be regenerated and audited. Currently the many `*.log` files in the
  repo root are ad hoc; move them to a `results/` dir (and out of version
  control) and standardize the format.
- **[PARTIALLY DONE]** **Repo hygiene:** `.gitignore` now covers `/bazel-*` and
  `/*.log`, and no logs are tracked in git. The dozens of
  `simple_small_guess_*_rs_*.log` files still clutter the working directory,
  though — move them into an ignored `results/` directory (which pairs with the
  machine-readable run-record item above).
- **CI:** add a GitHub Actions (or similar) job that builds and runs the unit
  tests in the sandbox. You have good test coverage (`*_test.cc` throughout) —
  make it gate changes.
- **Portability honesty:** the project is x86-64/Linux-specific (instruction
  encodings, `/proc/[pid]/stat` parsing, raw syscall 322). Document this
  explicitly and isolate the arch-specific bits behind an interface so an
  ARM/AArch64 port is conceivable later.

---

## 9. Documentation & framing

- The README is genuinely strong. Add a short **"Limitations & threats to
  validity"** section (toolchain-dependent ELF layout, non-determinism, x86-64
  only, results vs random-search baseline).
- Add an **architecture diagram of the code** (not just the workflow) showing
  `Program`/`Mutator`/`Scorer`/`Evolver` responsibilities — it will also force
  the SRP discussion in §2.2.
- Consider writing this up as a short workshop paper (the GECCO/EuroGP
  community would find the "raw random bytes vs validity tax" question
  interesting), citing the §6 additions.

---

## 10. Operability: checkpointing, program libraries, and interactive control

Three requested capabilities: (a) save/resume an evolution, (b) seed a new run
by mixing previously evolved programs and/or template ELFs, and (c) interactive
terminal monitoring/control. Much of the plumbing already exists; the notes
below flag where each depends on the `int3` marker (§3) and the worker pool
(§8).

### 10.1 Checkpoint and resume a run

The state to persist is small and mostly already serializable:

- **Population.** Each `Program`'s evolvable code can be written out with
  `Program::SaveElf` (`@/home/baran/prjs/viaevo/program/program.h:68`) and
  reconstructed via `Program::Create`
  (`@/home/baran/prjs/viaevo/program/program.h:44`). A checkpoint is the
  `mu_+lambda_` ELF blobs in `programs_` order plus their current scores.
- **RNG state.** `Random`'s `operator<<`/`operator>>` (fixed in §1,
  `@/home/baran/prjs/viaevo/util/random.h:32-50`) round-trip the generator.
  Persist the master `gen_`, and once per-program/per-worker streams exist
  (§8), each stream's state too.
- **Evolver bookkeeping.** `current_generation_`
  (`@/home/baran/prjs/viaevo/evolver/evolver_adhoc.h:69`), the full config
  (mu/phi/lambda/evaluations/max_generations/score_results_history), and the
  scorer's input schedule so the same inputs are drawn after resume.

Recommendations:

- **Define a versioned, self-describing snapshot** (a directory with a
  `manifest` + N `.elf` files, or a single archive). Include the git commit and
  the full `EvolverConfig` (§2.5) so a resumed run is auditable and reproducible
  (§8).
- **Add `EvolverAdHoc::SaveCheckpoint(path)` / `LoadCheckpoint(path)` and a
  `--resume <path>` flag.** Checkpoint every K generations and on `SIGINT`, so
  Ctrl-C saves a long run instead of losing it.
- **Determinism caveat:** exact bit-identical resumption requires the
  reproducibility fixes in §8 (per-program RNG, deterministic evaluation).
  Without them a resumed run continues correctly but won't match an
  uninterrupted one step-for-step — document which guarantee you offer.

### 10.2 Seed a run by mixing evolved programs and/or template ELFs

Today the entire population is cloned from a single `elf_filename`
(`@/home/baran/prjs/viaevo/evolver/evolver_adhoc.cc:33-39`). Generalize seeding:

- **Accept a list of seed ELFs with weights/counts** (evolved champions from
  prior runs and/or `//elfs` templates) and fill the initial `programs_` from
  them (round-robin or proportional), optionally topping up the remainder by
  mutating the seeds.
- **This turns saved champions into a reusable program library:** promising
  individuals from one task can bootstrap another (a form of transfer /
  island-model seeding — see §6 on Cartesian GP neutrality and island models).

**Why this pairs with `int3` (§3) — now unblocked:** mixing *different*
templates in one population used to be unsafe because each ELF carried its own
`expected_ptrace_stops_` calibration while the evolver assumed a single value.
With the `int3` breakpoint implemented (§3), "evolvable code reached" is
independent of pre-`main` syscalls and each `Program` self-resolves its symbol
data, so heterogeneous seeds can coexist. **Remaining prerequisite for safe
mixing:** all seeds must share the evolvable-region contract (same `main` size,
and matching `inputs`/`results` layout); validate this at load and
reject/pad mismatches explicitly rather than silently.

### 10.3 Interactive terminal monitoring and control

`Run()` currently streams a single fixed progress line
(`@/home/baran/prjs/viaevo/evolver/evolver_adhoc.cc:67-129`) and offers no
control once started. A terminal UI/REPL would help:

- **Monitoring:** live best/median/worst score, a score histogram,
  `sigalarms_count`, the ip-offset distribution (already computed in `Run`),
  generations/sec, and the current champion's disassembly. A lightweight
  ncurses / `ftxui`-style dashboard — or even a structured status line plus a
  key-driven menu — stays entirely in the terminal.
- **Control:** pause/resume, checkpoint-now, dump the current champion
  (`SaveElf`), adjust the mutation mix / `mu`/`phi`/`lambda` /
  `evaluations_per_program` on the fly, swap the target seed set, and
  quit-with-save. Implement via a non-blocking key reader on the main thread or
  a small command pipe/socket, keeping evaluation on the worker pool (§8).
- **Keep the machine-readable run record (§8) as the source of truth;** the
  interactive view is just a consumer of the same event stream, so headless
  (CI/batch) runs still log identically.

**Why this pairs with `int3` (§3) and the worker pool (§8):** interactive
pause/resume and mid-run reconfiguration are far safer once evaluation is a
robust, restartable step rather than the current fragile fork + syscall-count
loop (the source of the "random PTRACE_GETREGS failures", §1). A worker pool
also gives a clean seam to inject "pause" / "reconfigure" between generations.

---

## 11. Prioritized roadmap

A suggested order that front-loads correctness and high-leverage research:

1. **Fix the §1 bugs** (Random UB, signed rip offset, magic numbers, MNIST I/O
   caching). Low effort, removes latent failures.
2. **[DONE]** **Switch evolvable-code detection to an `int3` breakpoint at
   `main`** (§3), deleting the `expected_ptrace_stops_` calibration. Big
   robustness win (and a throughput win: 3 ptrace stops per execution instead
   of ~44).
3. **Make evaluation a worker-process pool** and give each worker its own RNG
   stream (§8). Fixes fork-in-threads, speeds everything up, enables
   reproducibility.
4. **Try lexicase selection** and add a **random-search baseline** per
   benchmark (§5.4, §6.3). Likely the biggest quality jump for least code.
5. **Enrich templates** with I/O-touching and arithmetic vocabulary; make
   templates generated from a spec (§4). Run the all-nops-vs-seeded comparison
   quantitatively.
6. **Refactor `Program`** into `ElfImage` + `Runner` + `Sandbox` and introduce
   `EvolverConfig` + an `Evolver` interface (§2).
7. **Add benchmarks** (parity, multiplexer, max/min, dot-product) and formalize
   the metrics-reporting script (§5).
8. **Explore QD/MAP-Elites or novelty search** for the MNIST-class tasks (§6.3).
9. **Checkpoint/resume and a seedable program library** (§10.1, §10.2): persist
   population + RNG + config, add `--resume`, and generalize seeding to a
   weighted set of evolved/template ELFs. Best after the `int3` switch (item 2)
   so mixed templates are safe, and it composes with the reproducibility work
   (item 3).
10. **Interactive terminal dashboard and controls** (§10.3): live monitoring
    plus pause / checkpoint / reconfigure without leaving the terminal. Best
    after the `int3` switch (item 2) and the worker pool (item 3), which make
    evaluation robust enough to pause and reconfigure safely.

---

## 12. Additional findings (2026-08 review)

New items found in a follow-up review after the §1 fixes and the
`elf_layout`/`Evolver`-interface refactors landed. The first two are
regressions in the selection logic and belong at the top of the roadmap
alongside the remaining §1 work.

### 12.1 φ (random-parent selection) is no longer implemented — regression

**[DONE]** `SelectParents` now selects the top `mu_ - phi_` programs by score
and fills the remaining `phi_` parent slots with a uniform random sample of
the rest of the population (partial Fisher-Yates driven by `gen_`, so
`RandomMock` keeps unit tests deterministic; `std::random_shuffle` is removed
in C++17). The constructor asserts `0 <= phi_ <= mu_`. Covered by the
`SelectParents` and `SelectParentsPhiPicksRandomParent` tests. Original
finding below.

`phi_` is stored by the `EvolverAdHoc` constructor but **never read anywhere**.
The original implementation (commit `70db730`) selected the top `mu_ - phi_`
programs via `std::nth_element` and then `random_shuffle`d the remainder so
that `phi_` parents were chosen at random, exactly as the README's Methods
section and every example's `--phi` flag help describe. The refactor in commit
`56d4473` ("Select parents using scores kept in the Evolver") replaced this
with a plain descending `stable_sort` in `SelectParents`
(`@/home/baran/prjs/viaevo/evolver/evolver_adhoc.cc:42-63`) and silently
dropped the φ behavior. Consequences:

- Selection is now purely elitist; the stochastic-ranking-inspired mechanism
  the README documents (and that the published trial results are attributed
  to: "All trial runs were performed with µ = 60, φ = 10") does not exist in
  the current code. Either reinstate it or update the README and deprecate the
  flag — but reinstating is recommended, since random parents are the main
  diversity-preservation mechanism this GA has.
- Fix sketch: partition the top `mu_ - phi_` by score, then fill the remaining
  `phi_` parent slots by sampling uniformly (via `gen_`, not
  `std::random_shuffle`, which is removed in C++17) from the rest.

### 12.2 Tie-breaking shuffle was dropped with it — neutral drift is impossible

**[DONE]** `SelectParents` now Fisher-Yates-shuffles the index array (via
`gen_`) before the stable sort, so score ties are broken uniformly at random
and equal-scoring offspring can displace parents again. Covered by the
`SelectParentsBreaksTiesRandomly` test (all-tie population reordered away from
the incumbent parents) and `SelectParentsIdentityShuffle` (pure-sort behavior
isolated via an identity-shuffle RNG sequence). Re-running the all-`nop`
experiment to gauge the effect remains open. Original finding below.

The same `56d4473` refactor also removed the pre-selection shuffle whose stated
purpose was "prevent breaking ties the same way in each generation". With
`stable_sort` and per-generation score reset, ties are resolved by index, and
parents occupy indices `0..mu_-1`: an offspring with a score *equal* to a
parent's can never displace it, and among zero-score programs (the common case
early on, and the *only* case in the all-`nop` experiments) the initial parent
set persists unchanged forever. This eliminates neutral drift — mutations
cannot accumulate silently in the parent pool — which §4.3 and the neutrality
literature in §6.3 identify as an important enabler for this kind of search.
The README's all-`nop` stall result may be partly an artifact of this.
Restore randomized tie-breaking (shuffle first, or sort a randomly-permuted
index array), and consider re-running the all-`nop` experiment afterwards.

### 12.3 Champion tracking compares scores from different input batches

`Run()` updates `best_overall_score` by comparing generation-best scores that
were measured on *different* randomly drawn inputs
(`@/home/baran/prjs/viaevo/evolver/evolver_adhoc.cc:166`). For input-dependent
tasks (MNIST), a program can become the saved champion merely by drawing easy
samples. Evaluate would-be champions on a fixed held-out input set before
updating `best_overall_score`/saving the ELF, and report both numbers. (Also
minor: `best_generation_results` holds only the results of the program's
*last* evaluation in the generation, which is a weak summary when
`evaluations_per_program > 1`.)

### 12.4 Seccomp hardening: `ptrace` need not be in the allowlist

**[DONE]** `RunElfProcess` now calls `PTRACE_TRACEME` before installing the
seccomp filter, and `ptrace` is removed from the allowlist — the ELF process
(including its evolved code) can no longer invoke `ptrace` at all. Original
finding below.

`RunElfProcess` installs the seccomp filter *before* calling `PTRACE_TRACEME`,
which forces `ptrace` into the allowlist
(`@/home/baran/prjs/viaevo/program/program.cc:100`) — so the evolved program
itself is allowed to call `ptrace`. Reordering to `PTRACE_TRACEME` first, then
installing the filter, lets `ptrace` be removed from the allowlist entirely.
One less syscall the evolved code can reach.

### 12.5 Small robustness nits

- `Program::WriteFile` treats `read()` returning `-1` as EOF: the
  `while (nread = read(...), nread > 0)` loop exits silently on error and does
  not retry `EINTR` on the read side (`program.cc:199`). Fail loudly on
  `nread == -1` (or retry on `EINTR`).
- `GetSeccompProgram` ignores the return values of every `seccomp_rule_add`
  call; a failed rule would surface only as a mysterious `SIGSYS` later. Check
  them (a small macro/lambda keeps it readable).
- The `google_benchmark` dependency in `MODULE.bazel` is unused — either drop
  it or (better) use it: a microbenchmark of `Execute()` would give the worker
  pool work (§8) a baseline to beat.
- `.bazelrc` pins `-std=c++17`, while §2.5/§2.7 recommend C++20 features
  (designated initializers, `std::span`). Bumping to `c++20` is a one-line
  change and unblocks both.

### 12.6 Documentation staleness to fix

- **[DONE — resolved by §12.1]** The README's Methods section describes the φ
  random-parent selection; with §12.1 reinstating that behavior, the README
  and the `--phi` flag help match the code again.
- Several `@path:line` references in §§1–2 and §10 of this document have
  drifted after the `elf_layout` extraction and other refactors (e.g.
  `program.h:119` → the `last_rip_offset_` block is now around
  `program.h:116-120`; `Program::Create` is now `program.cc:160-170`). Treat
  line numbers in this file as approximate; the symbol names remain the
  reliable anchors.

### 12.7 The execution timeout should bound CPU time, not wall-clock time

**[DONE — dual-timer belt-and-suspenders implemented]** `Sandbox` now arms two
independent itimers in the child (both survive `execveat` and are set before the
seccomp filter, so neither needs to be allowlisted): a **CPU-time** timer
(`ITIMER_PROF` → `SIGPROF`, default 50 ms) as the primary bound on
runaway/looping evolved code, plus a looser **wall-clock** timer (`ITIMER_REAL`
→ `SIGALRM`, default 500 ms) purely as a backstop for a child that blocks
without consuming CPU. The CPU timer does not advance while the process is
descheduled or ptrace-stopped, so a legitimate program is no longer killed
merely because the machine is loaded. `Sandbox`'s constructor takes
`cpu_timeout_usec` and `wall_timeout_usec`. Either signal terminating in the
evolved code counts as a timeout: the `ScorerMnistDigits` timeout penalty now
tests `SIGPROF || SIGALRM`, and `EvaluatePrograms` returns a `TimeoutCounts
{sigprofs, sigalrms}` that `Run()` reports as `sigprofs/alrms: XXX/YYY`. The
`inf_loop` regression test (a busy `while(1)`) now observes `SIGPROF` (27),
confirming the CPU timer fires first for a runaway program; an 8-generation
MNIST smoke run showed nonzero `sigprofs` with `sigalrms` at 0, i.e. the
wall-clock backstop stays quiet in normal operation. Still open: the §7
centralization of the timeout/"non-viable" penalty out of the individual
scorers (only `ScorerMnistDigits` special-cases it today), and exposing the two
timeouts as CLI flags. Original finding below.

`Sandbox::RunElfProcess` arms `ITIMER_REAL` (`program/sandbox.cc:397`), a
**wall-clock** timer, in the child right before `execveat` (itimers survive
`execve`, so the 50 ms budget covers the exec'd program's whole run up to its
first syscall/breakpoint). The default `timeout_usec_` is 50 ms
(`program/sandbox.h:77`). Wall-clock means the budget counts time the process
spends **descheduled** — waiting in the run queue for a CPU while other work
runs — not just time it spends working. This is the wrong quantity to bound and
can kill legitimate programs for reasons unrelated to their behavior:

- **For the compute the evolved programs actually do, 50 ms is enormous.** These
  are a few hundred machine instructions in `main`; the intrinsic cost is
  microseconds. The dominant real cost of a normal run is the `execveat` plus
  the three ptrace round-trips — well under a millisecond in isolation. So 50 ms
  is ~1000× more than a legitimate program needs; it is *not* too short for the
  program to finish its work.
- **But because it is a wall-clock timer, a legitimate program can be killed
  purely from scheduling jitter.** Evaluation runs with `std::execution::par`
  across all `mu_ + lambda_` (~200) programs, each spawning a ptraced child with
  many stop/continue context switches. On a loaded machine a trivial program can
  genuinely sit descheduled for tens of ms and trip the timeout without ever
  running away. This is the "eliminated due to other delays" failure mode.
- The impact is bounded, though: with the §12.1 φ selection plus the
  positive-score preference now in `SelectParents` (a program that times out on
  every execution scores zero and is deprioritized), a *single* false timeout
  only zeroes one evaluation. A program drops out entirely only if it times out
  on *all* evaluations in a generation, which a transient blip will not cause.
  So wall-clock false positives add scoring noise, not wholesale loss of good
  lineages.

An observed *drop* in `sigalarms_count` after the selection change is expected
from the selection change itself (timeout/inf-loop lineages no longer propagate
into parents), not evidence about the timeout value — do not read it as a signal
that 50 ms is now correct.

**There is no good wall-clock number.** Raising it cuts false positives but
makes every inf-loop program (common in random code) waste proportionally more
wall time; since a generation finishes when its slowest members finish, doubling
the timeout roughly doubles the generation-time floor. Lowering it raises false
positives. The knob trades the two off because it measures the wrong thing.

**Recommendation: bound CPU time instead of wall time** — `ITIMER_PROF`
(user+system CPU) or `ITIMER_VIRTUAL` (user only) rather than `ITIMER_REAL`,
keeping the same 50 ms number as a CPU budget. Then a legitimate program using
<1 ms of CPU never trips the timeout *regardless of system load* (descheduling
does not advance a CPU timer, and ptrace-stop time does not count either), while
a real infinite loop still burns CPU and is still killed promptly. This
preserves the current safety posture — a hard, short bound on runaway programs —
while removing the load-induced false kills. Caveats:

- The delivered signal changes (`SIGPROF`/`SIGVTALRM` instead of `SIGALRM`), and
  every scorer detects timeouts via `last_stop_signal() == SIGALRM`. So this
  ripples into all five scorers plus `sandbox.cc`'s startup-signal handling —
  mechanical, but it touches several files. (This pairs with the §7 item on
  centralizing the SIGALRM/"non-viable" penalty out of the individual scorers.)
- A pure CPU timer will not fire on a program *blocked* (not spinning) — e.g.
  hung in a blocking syscall during startup. The monitor already terminates
  evolved code at its first syscall (`kTerminateInEvolvedCode`), so that window
  is small, but for belt-and-suspenders one can run **both** timers
  simultaneously (they are independent): a tight `ITIMER_PROF` (e.g. 50 ms CPU)
  as the real bound plus a loose `ITIMER_REAL` (e.g. 500 ms wall) purely as a
  hang backstop, with a scorer treating either signal as a timeout.

Lower-effort interim option if the scorer ripple is undesirable now: keep the
wall-clock timer but expose `timeout_usec_` as a proper CLI flag (there is an
existing "use a command line flag" TODO) so it can be tuned per machine.

---

## 13. Additional findings (2026-09 review)

A further pass after the `Sandbox` extraction and the CPU-time timeout
landed. Items are grouped as: concrete defects (13.1), execution-environment
determinism and safety (13.2), throughput (13.3), a representation-level
observation about why the current recombination is destructive (13.4),
mutation operators (13.5), MNIST-specific scoring and input representation
(13.6), instrumentation for the "validity tax" question the README poses
(13.7), search-strategy ideas (13.8), and build/test/docs hygiene (13.9). The
roadmap in §11 is amended at the end (13.10).

### 13.1 Concrete defects

- **[DONE]** **`mutator_recombine_plain_elf_test` never compiles its test file.**
  In `mutator/BUILD` the `cc_test` listed `srcs = ["mutator_recombine_plain_elf.cc"]`
  (the library source) instead of `mutator_recombine_plain_elf_test.cc`. The
  binary linked `gtest_main` with zero test cases and reported
  `[  PASSED  ] 0 tests` — verified via `bazel test`. The two tests in
  `mutator_recombine_plain_elf_test.cc` (`Mutate`, `InitializeProgramToAllNops`)
  had therefore never run under Bazel. `srcs` now names the test file; both
  tests compile and pass unchanged (they had not bit-rotted).
- **[DONE]** **`mutator_composite_random_test.cc` names its suite `MutatorRecombineRandomTest`**
  (copy-paste from the recombine test). Harmless, but it made `--gtest_filter`
  and failure output misleading. The suite is now `MutatorCompositeRandomTest`;
  nothing referenced the old name.
- **[DONE]** **Input validation is `assert`-only and vanishes under `-c opt`.**
  Every check in `ScorerMnistDigits::LoadData`/`LoadSample` (file open, magic
  bytes, dimensions, read counts), the `ScorerGuessValue(-1)` guard, and the
  `phi_ >= 0 && mu_ >= phi_` check in `EvolverAdHoc`'s constructor were
  `assert()`s. Bazel's `-c opt` defines `NDEBUG`, so an optimized build silently
  proceeded with an unopened/truncated MNIST file (zeroed inputs, garbage
  labels) or an invalid `phi`.

  `VIAEVO_CHECK(condition, message)` (`@/home/baran/prjs/viaevo/util/check.h`)
  now provides an unconditional check that reports
  `file:line: Check failed: condition: message` on stderr and exits with
  `EXIT_FAILURE`. The message expression is evaluated only on failure, so it can
  be composed from runtime values (filenames, actual vs expected sizes) at no
  cost on the success path. Terminating in a single function keeps the §2.1
  migration to exceptions a one-line change. All the checks above were
  converted, as were the constructor-argument checks in `ScorerMock` and
  `ScorerMarkedMock` (whose `EXPECT_DEATH` tests failed under `-c opt` for the
  same reason). The only remaining `assert` is
  `assert(n == scores.size())` in `EvolverAdHoc::SelectParents`, a genuine
  internal invariant. Added `//util:check_test` plus death tests for a missing
  MNIST images/labels file.

  Fixing the `assert`s was necessary but not sufficient to make `-c opt` usable
  — see the next item.
- **[DONE]** **`-c opt` silently recompiled the template ELFs and changed the
  evolvable substrate.** The templates in `elfs/` were plain `cc_binary`
  targets, so they inherited the compilation mode of the rest of the project.
  Under `-c opt` this changed the very thing being evolved, and 16 of 24 tests
  failed. Three separate causes, all now pinned in `elfs/BUILD` via
  `TEMPLATE_COPTS`/`TEMPLATE_LINKOPTS`:
  - `-O2` folds away the `dummy` assignments that give the evolution something
    to repurpose; `main` shrank from 3,304 to 2,573 bytes. Pinned `-O0`.
  - Bazel links the PIC object in `fastbuild` but the non-PIC object in `opt`,
    and PIC global accesses are longer — roughly 700 bytes of `main` on its own.
    Pinned `-fPIC`.
  - `opt` adds `-ffunction-sections -fdata-sections` and links with
    `--gc-sections`, which drops the `inputs` global outright (nothing in `main`
    references it, by design). Every test that loads a template then died with
    `symbol inputs not found`, and `intermediate_small` segfaulted. Pinned
    `-fno-function-sections -fno-data-sections` and
    `-Wl,--no-gc-sections`, plus `-U_FORTIFY_SOURCE` to undo the
    `_FORTIFY_SOURCE=1` that `opt` defines.

  All six template ELFs are now byte-identical between `fastbuild` and `opt`
  (verified by hash), and the default build's output is unchanged, so existing
  runs remain reproducible. `bazel test //...` and `bazel test -c opt //...`
  both pass 25/25. This unblocks the `-c opt` recommendation in 13.3, which
  should be re-benchmarked now that the templates are held constant.
- **The ignored `PTRACE_GETREGS` failure path leaks a zombie.** When
  `Sandbox::MonitorElfProcess` hits the "No such process" case it `return`s
  from inside the `waitpid` loop without ever reaping the child, so the process
  (if it is still exiting) stays a zombie until the evolver exits, and
  `result->last_results` may be stale. At minimum, on `ESRCH` continue the
  `waitpid` loop until `WIFEXITED`/`WIFSIGNALED` so the child is always reaped,
  and count these occurrences the way `ResultsHaveMinSize` does so their
  frequency is visible. (See 13.2 for `PTRACE_O_EXITKILL`, which also narrows
  the set of possible causes.)
- **`ReadProcStatAddresses` is the last brittle piece of the monitor.** It
  parses `/proc/[pid]/stat` positionally (the code's own NOTE flags the
  signedness assumptions) and pairs the kernel's `start_data` with the
  `data_start` *symbol* to locate `results` — a coincidence of the current GCC/ld
  layout rather than a guarantee. Since the templates are PIEs, every segment is
  relocated by one load bias, so a single number suffices: read `AT_ENTRY` from
  `/proc/[pid]/auxv` (a fixed-layout binary file) at the post-exec stop and
  compute `bias = AT_ENTRY - ehdr.e_entry`; then `main = bias + st_value(main)`
  and `results = bias + st_value(results)`. This drops `start_code`,
  `start_data`, and the `data_start` symbol lookup in `elf_layout.cc`. Linking
  the templates `-no-pie` (13.2) makes the bias zero and removes the lookup
  entirely.

### 13.2 Execution-environment determinism and safety

The README attributes divergence between same-seed runs to "non-deterministic
programs". Two cheap, async-signal-safe calls in the `vfork` child (before the
seccomp filter, so nothing new needs allowlisting) remove the two most common
sources of that non-determinism:

- **Disable ASLR for the child:** `personality(ADDR_NO_RANDOMIZE)` before
  `execveat`. The templates are PIEs (`readelf -h` shows `DYN`) and are
  dynamically linked (`ldd` shows `libc.so.6`, `ld-linux`, and the vDSO), so
  today every execution places the text, data, stack, heap, `ld.so`, libc and
  vDSO at different addresses. Any evolved instruction that reads a pointer
  (argv/envp/auxv on the stack, the return address, a GOT entry, `%rsp` itself)
  and lets it influence `results` is non-deterministic across runs purely
  because of ASLR. The tests already assume fixed offsets; making addresses
  fixed makes *values* reproducible too.
- **Make `rdtsc` fault:** `prctl(PR_SET_TSC, PR_TSC_SIGSEGV)`. The `rdtsc`
  encoding is two bytes (`0f 31`) and is easily reached by a bit flip; a program
  that latches onto it gets a different value every run. With this setting the
  instruction raises `SIGSEGV` and the program is terminated like any other
  invalid one. (`rdrand`/`rdseed` cannot be disabled from user space; they
  remain a small residual source. The 16 random bytes at `AT_RANDOM` on the
  stack are another; they are harmless unless evolved code reads the stack.)
- **`PTRACE_O_EXITKILL`:** set it (via `PTRACE_SETOPTIONS`) at the first stop.
  If the evolver dies — and `myfail` calls `exit()` from a worker thread — every
  traced child is currently *detached and resumed*, i.e. evolved code keeps
  running outside the tracer's control (still under seccomp and the itimers,
  but no longer terminated at its first syscall). `EXITKILL` makes the kernel
  `SIGKILL` all tracees when the tracer exits. While there, consider
  `PTRACE_O_TRACEEXEC` so the post-exec stop is an unambiguous
  `PTRACE_EVENT_EXEC` rather than a generic `SIGTRAP`, and
  `PTRACE_O_TRACESYSGOOD` so syscall stops are distinguishable from a genuine
  `SIGTRAP` (e.g. an evolved `int3`).
- **Static, non-PIE, libc-free templates.** The template C files use no libc at
  all (globals plus inline asm and intrinsics), yet each execution pays for
  `ld.so`, libc relocation and initialization — which is where the entire
  seccomp allowlist beyond `execveat`/`exit_group` comes from (`brk`, `mmap`,
  `openat`, `pread64`, `mprotect`, `getrandom`, ...). Building the templates
  with `-static -no-pie -nostartfiles -fno-stack-protector` and a three-line
  `_start` (align the stack, `call main`, `exit_group`) gives: (a) a
  seccomp allowlist of two syscalls, (b) an `exec` that maps a single tiny
  segment pair instead of two shared objects (13.3), (c) fixed addresses without
  `personality`, (d) no libc code in the address space for evolved jumps to land
  in, and (e) no `/proc` parsing (13.1). Keep the `results[0] = 20` control and
  the `main` symbol; nothing in `elf_layout.cc` depends on dynamic linking. Note
  that Gentoo's GCC enables `-fstack-protector` by default and that the canary
  lives in TLS (`%fs:0x28`), which requires libc's TLS setup — hence the
  explicit `-fno-stack-protector` (the current `main`s happen not to reference
  the canary, verified via `objdump`, but a future template with local arrays
  would).

### 13.3 Throughput: where the time actually goes

Numbers from the logs in the repo root (8 cores):

| Run | Generations | Wall | User | Sys |
| --- | --: | --: | --: | --: |
| `simple_small_guess_474741_rs_4130` (10 evals/program) | 1,000 | 5m33s | 18m58s | 17m54s |

That is 2,000,000 executions in 333 s, ≈ 6,000 executions/s, ≈ 1.3 ms of CPU
per execution, and **sys ≈ user** — i.e. roughly half of all CPU is kernel time
in `execveat`, page-fault handling for `ld.so`/libc, and ptrace round trips,
not evolved code (whose intrinsic cost is microseconds). Consequences and
recommendations, in order of payoff per effort:

- **Cut the CPU timeout by an order of magnitude and expose it as a flag.** In
  `complex_large_digits_rs_13146.log`, single generations report up to 17,648
  `SIGPROF` timeouts out of 40,000 executions (44%), and several generations
  exceed 4,000. At the 50 ms CPU budget, one such generation burns ~880 CPU
  seconds *in timeouts alone* (~110 s wall on 8 cores) while the useful work in
  that generation is a few seconds. A legitimate template runs in well under a
  millisecond; a budget of ~5 ms (one or two scheduler ticks at
  `CONFIG_HZ=250`, the practical floor for an itimer) keeps the same safety
  posture and makes runaway-heavy generations ~10× cheaper. Report the timeout
  *fraction* per generation (13.7) so the effect is visible.
- **Do not evaluate deterministic tasks ten times.** `000_guess_value` and
  `001_copy_value` default to `evaluations_per_program = 10`, but for
  `guess_value` `ResetInputs()` is a no-op, so nine of ten executions of a
  deterministic program are wasted. Evaluate once, and re-execute only
  candidates that would become parents (or the champion) to confirm they are
  deterministic — a racing / successive-halving scheme: cheap screen on 1
  evaluation, full `evaluations_per_program` only for the top fraction. For MNIST
  the same idea applies with a small sample (e.g. 20 images) as the screen and
  the full 200 for finalists; most of a generation's 40,000 executions are spent
  on programs that a handful of samples already show are hopeless.
- **Flatten the evaluation loop.** `EvaluatePrograms` runs
  `evaluations_per_program` sequential rounds, each a `std::execution::par`
  `for_each` over 200 programs with a barrier between rounds — 200 barriers per
  MNIST generation, and every round waits for its slowest (timed-out) member.
  Pre-draw the generation's input schedule (a vector of `evaluations_per_program`
  input sets), then parallelize over all (program × evaluation) pairs at once.
  Pre-drawing also gives every program the same inputs (as now), makes the
  schedule serializable for checkpoints (§10.1), and is the natural place to
  add a fixed held-out set for champion validation (§12.3).
- **Build with `-c opt`** (now unblocked: the `assert`s and the template
  recompilation in 13.1 are fixed, and `bazel test -c opt //...` passes).
  The run scripts use the default `fastbuild`. Mutators copy the full `main`
  vector two or three times per offspring and the parent-side monitor is C++
  that benefits from optimization; measure before assuming, but there is no
  reason to run research workloads unoptimized. Add `--config=opt` to
  `.bazelrc`. Note that the template ELFs are deliberately pinned to `-O0`
  (and to PIC, no section GC) and so are unaffected by the switch — that is
  the point: the substrate must not change when the framework is optimized.
- **Zygote fork instead of `execveat` per evaluation.** The larger step beyond
  the worker pool in §8: exec each template *once* into a "zygote" that is
  stopped at the `main` breakpoint, then for every evaluation `fork()` the
  zygote (copy-on-write, no exec, no loader), write the offspring's `main`
  bytes and `inputs` into the fork with `process_vm_writev`/`PTRACE_POKETEXT`,
  and continue it under `PTRACE_SYSCALL`. Per-evaluation kernel cost drops
  from "exec + map two shared objects + relocate" to "fork a ~50 KB process +
  two `process_vm_writev` calls". The static-template change in 13.2 gets a
  large part of this win with far less code, so do that first and measure.

### 13.4 RIP-relative addressing makes byte-offset recombination destructive

This is the most important representation-level observation in this review.
`complex_large`'s `main` contains 366 `(%rip)`-relative memory operands and 41
relative jumps (`objdump`); the smaller templates are similar in proportion.
Both `MutatorRecombineRandom` and `MutatorRecombinePlainElf` copy a byte range
from offset `q` in the source into offset `p ≠ q` in the destination. Every
RIP-relative displacement and every relative jump inside the copied range is
now *off by `p − q` bytes*: a `mov inputs+0x40(%rip), %eax` copied 37 bytes
later reads `inputs+0x40−37`, which is a misaligned address 9 ints earlier —
still inside `.data` (the arrays are contiguous and large), so the instruction
does not fault, it just silently reads or writes the wrong variable. Likewise a
`jmp .+127` moved by a few bytes lands mid-instruction. In other words the
current crossover almost never transplants *semantics*; it transplants bytes
whose meaning depends on where they sit.

Because every program in the population descends from the same template and all
mutations preserve length, the population is *positionally aligned*: offset `k`
in one program corresponds to offset `k` in every other. That makes the fix
nearly free:

- **Homologous (same-offset) crossover:** copy bytes `[q, q+len)` of parent 2
  into `[q, q+len)` of parent 1. RIP-relative operands and relative jumps keep
  their meaning as long as the instruction stays at its offset. This is
  standard one-/two-point crossover on a fixed-length genome and is exactly
  the "homologous crossover" of Nordin et al. (§6.1) that the README cites but
  does not implement. Keep the non-homologous variant as a separate operator
  so the two can be compared.
- **Same for `MutatorRecombinePlainElf`:** re-inserting template bytes at their
  *original* offsets is a "repair" operator that restores valid, correctly
  addressed vocabulary; at a random offset it is mostly noise.
- **Displacement-aware point mutation:** a bit flip inside a 4-byte
  RIP-relative displacement moves the referenced address by a power of two
  bytes — most single flips produce a *misaligned* int address (bits 0–1) or
  jump to a different array (high bits). Flipping only bits ≥ 2 of a
  displacement (once instruction boundaries are known, §4.2 / 13.5) keeps the
  operand 4-byte aligned and turns "which global does this instruction touch"
  into a smooth knob. The same holds for the immediates in `complex_large`
  Section 6.

Expect this to change results materially for the copy/sum/MNIST tasks, where
"read the right input, write the right result" is precisely a matter of getting
displacements right.

### 13.5 Mutation operators that respect the byte stream

- **Instruction-boundary map.** All operators in §4.2, 13.4 and below need to
  know where instructions start. A length decoder for the current 3–7 KB
  `main` costs microseconds; use Zydis (MIT, C, a single Bazel `cc_library`)
  or Capstone, or a minimal x86-64 length-decoder (~300 lines, prefixes +
  opcode maps + ModRM/SIB + immediates). Decode from `main`'s start, stop at
  the first undecodable byte, and treat everything after as "already broken"
  (a useful statistic in itself, 13.7). Length statistics for the current
  templates show why this matters: `simple_small` is 79% one-byte `nop`s
  (1,459 of 1,834 instructions) with the rest almost all 3- or 7-byte;
  `complex_large` averages 3.06 bytes with 20% of instructions ≥ 7 bytes
  (VEX-prefixed AVX2 and `disp32` memory operands) — long encodings are exactly
  the ones a random flip turns into a frame shift.
- **Boundary-aligned recombination** (§4.4) becomes a two-line change once the
  map exists: snap `q` and `q+len` to instruction starts in both parents.
- **NOP-sled-consuming insert/delete.** No current operator changes the
  *number* of instructions; everything is an in-place overwrite. Add an
  operator that inserts `k` bytes (a whole instruction copied from a parent or
  the template) at an instruction boundary and deletes `k` `nop`s from the
  nearest sled after it (or the reverse), so all code *after* the sled keeps
  its offset and the RIP-relative operands there stay valid. The `NOPS`
  macro already provides the sleds; this operator is what they are for.
- **Whole-instruction substitution.** Replace one decoded instruction with a
  random instruction *of the same length* drawn from the template's own
  instruction multiset (the "vocabulary" of §4.1). Same-length substitution is
  the least disruptive way to change an opcode.
- **Rate control.** All mutators apply exactly one edit. Add a configurable
  per-offspring edit count (Poisson with mean ~1–3) and log which operator
  produced each parent-becoming offspring, so operator *effectiveness* can be
  measured and the composite's uniform weights (`MutatorCompositeRandom`)
  replaced by adaptive ones (e.g. multi-armed bandit over operators).
- **Drop division from the vocabulary, or guard it.** `complex_large` has four
  `idiv`s. Division raises `SIGFPE` on a zero divisor (and on `INT_MIN / −1`),
  and MNIST inputs are *mostly zero* (background pixels), so any evolved path
  that divides by an input dies on most samples. Division is "fragile
  vocabulary" with little benefit for the current tasks.

### 13.6 MNIST: scoring and input representation

- **The diversity term dominates correctness by four orders of magnitude.**
  `ScoreResultsHistory` awards up to 11.999 × 10¹⁵ for merely *emitting*
  distinct in-range values, while the correctness component of `Score` is at
  most 200 × 10⁹ per generation. Lexicographically, "outputs all ten digits at
  random" beats "outputs the right digit 50% of the time using five distinct
  values". The logs show exactly this: `complex_large_digits_rs_13144` climbs to
  11.995 × 10¹⁵ (diversity saturated) by generation ~3,000, with the
  correctness digits in the low part of the score still near chance. The
  mechanism was meant to fight the broken-clock problem, but as a *dominant*
  term it optimizes the wrong thing.
- **Replace it with a measure that rewards diversity only when it is
  informative.** Two principled options, both computable from the per-generation
  results history the evolver already collects:
  - *Balanced accuracy* (mean per-class recall). A constant output scores
    0.1; ten correct classes score 1.0; diversity is rewarded exactly to the
    extent it is correct.
  - *Mutual information* `I(prediction; label)` over the generation's
    (prediction, label) pairs, with a *post-hoc relabeling*: let evolution
    discover any mapping from images to output values, and fix the assignment
    from output values to digits in the scorer (argmax of the confusion matrix,
    or a Hungarian assignment). The evolved program then only needs to
    *separate* classes, not to know their names — a strictly easier problem —
    and the constant/random-output programs score 0 automatically. Report both
    the raw and the relabeled accuracy.
- **Give evolution a representation it can use.** `LoadSample` packs the 784
  pixel bytes four-per-`int` into 196 ints, so an evolved instruction reading
  `inputs[k]` sees four pixels smeared across one word; extracting one pixel
  needs a shift-and-mask sequence the mutators must assemble by chance
  (Section 1 of `complex_large` includes such sequences for this reason).
  Offer, as flags, (a) one int per pixel (784 ints fit `complex_large`'s 801),
  (b) 14×14 or 7×7 mean-pooled images (196 / 49 ints), and (c) binarized pixels
  (0/1). A 7×7 binary MNIST is a far smaller search problem and is the natural
  next rung after `sum_two`; the raw 28×28 result can remain the headline.
- **Binary and few-class stepping stones** (§5.3): 0-vs-1, then 0/1/2, with
  balanced sampling per class so the scorer's batch is not dominated by the
  majority class.
- **Use the test set that is already in the repo.** `t10k-*` files are present
  in `examples/100_mnist_digits/data` but `validate.cc` scores champions on the
  *training* images. Validate on `t10k`, and use a fixed training subset for
  the §12.3 champion check.

### 13.7 Instrument the "validity tax" directly

The README's central question is how much random machine-code edits cost in
invalid programs, yet the only per-generation diagnostics are best score, the
`rip` mode, and timeout counts. Add, per generation (to the machine-readable
record of §8 and the status line):

- **Termination-reason histogram:** `SIGSEGV`, `SIGILL`, `SIGFPE`, `SIGBUS`,
  `SIGTRAP` (evolved `int3`), `SIGPROF`/`SIGALRM`, and "syscall-stop" split by
  syscall number (`exit_group` from falling off the end of `main` vs. anything
  else). The fraction reaching `exit_group` *is* the viability rate.
- **Offspring viability by operator:** the same histogram keyed by which
  mutator created the offspring (13.5), plus "offspring became a parent" — the
  data needed to tune operator weights.
- **Decodable-prefix length** (13.5): how many bytes of `main` still decode
  from the start; its population distribution shows how fast code "erodes".
- **Executed-region trace for champions:** a diagnostic `ExecuteMode` using
  `PTRACE_SINGLESTEP` that records the sequence of `rip` offsets. This
  separates effective code from introns (Brameier & Banzhaf, §6.2), shows
  *how* an evolved `copy_value` program actually copies (worth a README
  figure), and enables intron-aware operators (mutate only executed bytes, or
  only unexecuted ones for neutral drift). Pair it with an `objdump`-style
  disassembly of the champion's executed path.
- **Population diversity:** mean pairwise Hamming distance of `main` (sampled),
  and the count of distinct genomes. Convergence to one `rip` mode with count
  >150 of 200, as seen in the logs, suggests diversity collapses well before
  the score stalls.
- **Per-execution instruction count** via `perf_event_open`
  (`PERF_COUNT_HW_INSTRUCTIONS`, enabled at the `main` breakpoint, read at the
  terminating stop). This is a deterministic cost measure independent of
  machine load, usable as a secondary objective (prefer shorter programs) and,
  with a sample period and `PERF_EVENT_IOC_REFRESH`, as a deterministic
  *instruction budget* replacing the time-based timeout for reproducible runs.

### 13.8 Search-strategy ideas specific to this setting

- **Island model on the available cores.** Each generation is embarrassingly
  parallel but generations are serialized; the `run_*_loop.sh` scripts run
  seeds *sequentially*. A trivial island model — N independent populations
  with different seeds, occasional migration of champions — uses the same
  budget and is known to help exactly the premature-convergence pattern in
  13.7. It composes with the §10.2 program library.
- **MAP-Elites with cheap, meaningful descriptors** (concretizing §6.3): the
  termination reason, the number of `results` slots changed, the last `rip`
  offset bucket, and the decodable-prefix length are all already observable per
  execution and make a natural behavior space. Keeping one elite per cell
  preserves the "changes results[5] but crashes" and "runs to exit but changes
  nothing" lineages that pure score-based selection discards.
- **Neutral-drift budget.** With tie-breaking fixed (§12.2), run the
  all-`nop` experiment again but *measure* neutral drift: how many silent bit
  flips accumulate in never-executed regions per generation. If drift is
  present but the all-`nop` start still stalls, the bottleneck is the missing
  vocabulary (§4.1), not selection.
- **Seed with a solved neighbor** (§10.2 in practice): start `double_value`
  from a `copy_value` champion and `sum_two` from a `double_value` champion;
  report generations-to-solution against a template start. This is the
  cheapest possible transfer-learning experiment and directly tests whether
  evolved programs are reusable building blocks.

### 13.9 Build, test and documentation hygiene

- **Undeclared system prerequisites.** `-lseccomp` and `-ltbb` are bare
  `linkopts`; the AVX2 template needs a CPU with AVX2 or it `SIGILL`s before
  evolution starts; `program_test` encodes the current GCC/glibc syscall
  sequence. Document the prerequisites (Linux x86-64, `libseccomp`, TBB, a
  ptrace-permitting environment — e.g. `kernel.yama.ptrace_scope` and container
  seccomp profiles) in the README, and make the AVX2 dependency a build/runtime
  check rather than a surprise. Longer term, fetch `libseccomp` and TBB as
  Bazel modules so the build is hermetic.
- **Stale paths in the example READMEs.** The evolved-ELF path is documented as
  `.../main.runfiles/__main__/`; with bzlmod the directory is `_main`
  (verified on this checkout). Better: write outputs to a `--output_dir`
  outside the runfiles tree (pairs with the `results/` directory of §8) so users
  never need the Bazel-internal path.
- **Missing READMEs.** `002_double_value` and `010_sum_two` have no README and
  are unlinked in the top-level table; the copy/guess READMEs also still show
  the pre-2023 results initializer `{10, 0, 0, ...}` in the quoted `--help`
  output.
- **Results provenance.** The README tables date from May 2023 and were
  produced by code that has since changed in ways that affect outcomes
  (φ selection and tie-breaking regressed and were reinstated, `int3` detection,
  CPU-time timeout, positive-score preference). Re-run the tables with the
  current code, and stamp each table with the commit hash and the exact flags
  (the §8 run record makes this automatic).
- **Repo root.** `google4db608c70141c63b.html` (a site-verification file) and
  ~130 `*.log` files sit in the root; the logs are ignored but still shipped
  around by every tool that lists the tree. Move them under `results/`.
- **Test brittleness to toolchain.** `program_test` asserts `last_syscall ==
  231` and exact stop counts; that is fine, but the static-template change in
  13.2 makes these assertions toolchain-independent and is another reason to
  do it. Add a test that a template's `main` decodes cleanly with the 13.5
  length decoder and that every `jmp .+N` lands on an instruction boundary
  (this was hand-verified with `objdump` for `complex_large` and has already
  been wrong once in the intermediate templates, per `git log`).
- **`gen_() % n` modulo bias** is negligible at these `n`, but if the RNG is
  ever replaced with `std::uniform_int_distribution`, note that `RandomMock`
  only intercepts `operator()` — the mock would stop controlling outcomes.

### 13.10 Roadmap amendments

Insert into §11 as follows:

- **Before item 3:** fix the `mutator_recombine_plain_elf_test` `srcs` typo and
  the `assert`-based validation (13.1); add `personality(ADDR_NO_RANDOMIZE)`,
  `PR_SET_TSC`, and `PTRACE_O_EXITKILL` to the child/monitor (13.2); lower the
  CPU timeout and expose it as a flag; evaluate deterministic tasks once (13.3).
  All are small and each removes a real source of noise or waste.
- **New item 3b (high leverage, low effort):** homologous crossover and
  same-offset template repair (13.4), then an instruction-length decoder and
  boundary-aligned / sled-consuming operators (13.5). This likely changes the
  headline numbers more than any infrastructure work.
- **Alongside item 4:** the termination-reason and per-operator viability
  instrumentation (13.7), so the "validity tax" is a measured quantity before
  and after 3b.
- **Before item 8 (MNIST QD work):** replace the diversity term with balanced
  accuracy or relabeled mutual information, and add the pooled/binarized input
  variants (13.6). Without this, MNIST results mostly measure the diversity hack.
- **Static, libc-free templates** (13.2) can be done at any point and simplify
  items 3, 6 and 12.4; do it before the zygote/worker-pool work so that work is
  measured against a cheap baseline.
