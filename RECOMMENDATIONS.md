# Viaevo — Recommendations for the Future

A structured review of the `viaevo` project (genetic programming via random
changes at the machine-code level). The goal is to flag antipatterns, surface
missed opportunities, and propose concrete directions. Observations reference
the current code where relevant.

The sections are independent and roughly ordered as: bugs to fix first, then
C++ design, the SIGUSR idea, template-program evolvability, benchmark problems,
prior art, scoring, infrastructure/reproducibility, documentation, operability
features (checkpointing, program libraries, interactive control), and a
prioritized roadmap.

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

- `ElfImage` — owns the `memfd`, knows symbol offsets, `GetElfCode/SetElfCode/
  GetElfInputs/SetElfInputs/SaveElf`. Pure data + ELF knowledge, trivially
  testable without forking.
- **[DONE]** `ElfLayout`/`SymbolData` resolver — the `InitializeElfSymbolData`
  logic (~140 lines) is now a standalone free function `ResolveElfSymbolData(int
  fd)` with the `SymbolData` struct in `@/home/baran/prjs/viaevo/program/elf_layout.h`
  / `elf_layout.cc`, unit-tested in `program/elf_layout_test.cc`. `Program` now
  calls it from `Create`. (It still terminates on malformed input; the
  recoverable-error split is tracked in §2.1.)
- `Sandbox`/`Runner` — owns seccomp policy, fork/exec, ptrace loop, timeout. The
  allowed-syscall list and the alarm timer belong here, not interleaved in
  `Program`.

This also removes the awkward "default constructor for mocking" TODO
(`@/home/baran/prjs/viaevo/program/program.h:29-31`): if execution lives behind
an interface (`ProgramRunner`), you mock the interface, not subclass a concrete
class with a public default ctor.

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
`mutator_recombine_random.cc:20-29`.

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
