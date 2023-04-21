# Viaevo example 000_guess_value

This example evolves programs that assign a specified value to the `results[1]` global variable. The default starting program for the evolution is //elfs:simple_small ([simple_small.c](../../elfs/simple_small.c)).

> **_WARNING:_** The evolution produces invalid executables. To protect your system, ***always run these programs in a sandbox!*** When using bazel on Linux, this is achieved by default via `build --spawn_strategy=linux-sandbox` in [.bazelrc](../../.bazelrc). There are other safeguards in place (such as restricting system calls the evolved programs' processes can make and terminating the processes on any signal or system call once the evolved code is reached), but the evolved programs may still exhibit unwanted behavior.

## Evolution

The [main.cc](main.cc) program drives the evolution. If the value of results[1] changes after the execution of the evolved program, [ScorerGuessValue](scorer_guess_value.h) scores every matching bit to that of the expected value to be "guessed". If results[1] does not change, small scores are "awarded" for changes in values of results[2] to results[10].

### Usage:

```
$ bazel run //examples/000_guess_value:main -- --help
INFO: Analyzed target //examples/000_guess_value:main (0 packages loaded, 0 targets configured).
INFO: Found 1 target...
Target //examples/000_guess_value:main up-to-date:
  bazel-bin/examples/000_guess_value/main
INFO: Elapsed time: 1.847s, Critical Path: 1.74s
INFO: 3 processes: 1 internal, 2 linux-sandbox.
INFO: Build completed successfully, 3 total actions
INFO: Build completed successfully, 3 total actions
main: This program evolves ELFs to 'guess' a value and place the value in the results[1] global variable of the evolved program.

WARNING: The evolution produces invalid executables. To protect your system, always run this program in a sandbox!

Sample usage via the bazel build system (with 'build --spawn_strategy=linux-sandbox' in .bazelrc):

bazel run //examples/000_guess_value:main -- --value_to_guess=1009

  Flags from examples/000_guess_value/main.cc:
    --elf_filename (filename of the ELF executable to be used as the starting
      template for evolution (NOTE: results are expected to be initialized to
      {10, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3})); default: "elfs/simple_small";
    --evaluations_per_program (number of evaluations to be performed on each
      program in each generation (scores are accumulated across evaluations));
      default: 10;
    --initialize_programs_to_all_nops (set all instructions in the evolvable
      code of the template ELF executable to nop prior to starting the
      evolution); default: false;
    --lambda (number of offspring to generate from mu parents in each generation
      (the size of the population in each generation is mu + lambda));
      default: 140;
    --max_generations (maximum number of generations for the evolution);
      default: 10000;
    --mu (number of parents selected in each generation to generate lambda
      offspring (the size of the population in each generation is mu + lambda));
      default: 60;
    --output_filename_prefix (prefix to prepend to output file names (e.g. for
      saved evolved elfs)); default: "";
    --phi (number of parents selected randomly in each generation as opposed to
      (mu - phi) parents that are selected based on their scores (mu >= phi));
      default: 10;
    --random_seed (random seed for the evolution (NOTE: repeated evolutions with
      the same random seed may diverge if the evolved programs compute results
      non-deterministically)); default: 1;
    --score_results_history (track and score the set of evolved program's
      results across evaluations within a generation (not applicable for
      000_guess_value)); default: false;
    --value_to_guess (value to be 'guessed' by the evolved program and placed
      into the results[1] global variable); default: 42;

Try --helpfull to get a list of all flags or --help=substring shows help for
flags which include specified substring in either in the name, or description or
path.
```

### Example run:

> **_NOTE:_** Repeated evolutions with the same random seed may diverge if the evolved programs compute results non-deterministically.

```
$ bazel run //examples/000_guess_value:main -- --value_to_guess=42 --random_seed=2 --output_filename_prefix="guess_42_rs_2_"
INFO: Analyzed target //examples/000_guess_value:main (0 packages loaded, 0 targets configured).
INFO: Found 1 target...
Target //examples/000_guess_value:main up-to-date:
  bazel-bin/examples/000_guess_value/main
INFO: Elapsed time: 0.084s, Critical Path: 0.00s
INFO: 1 process: 1 internal.
INFO: Build completed successfully, 1 total action
INFO: Running command line: bazel-bin/examples/000_guess_value/main '--value_to_guess=42' '--random_seed=2' '--output_filename_prefixINFO: Build completed successfully, 1 total action
# value_to_guess: 42
# elf_filename: elfs/simple_small
# mu: 60
# phi: 10
# lambda: 140
# evaluations_per_program: 10
# max_generations: 10000
# score_results_history: false
# output_filename_prefix: guess_42_rs_2_
# random_seed: 2
# initialize_programs_to_all_nops: false
G:        1 | best score: 10 (overall: 10/520)  | rip distinct: 195 top: 3,298 count: 4
            | best last results: 3 0 0 0 0 3 3 3 3 3 3 
G:        2 | best score: 20 (overall: 20/520)  | rip distinct: 144 top: 3,298 count: 13
            | best last results: 20 0 0 0 0 0 67 3 3 393,219 3 
G:        3 | best score: 60 (overall: 60/520)  | rip distinct: 120 top: 3,298 count: 17
            | best last results: 3 0 0 0 0 3 0 0 0 0 0 
G:        7 | best score: 80 (overall: 80/520)  | rip distinct: 107 top: 3,305 count: 24
            | best last results: 3 0 -1,879,048,192 9,474,192 0 3 0 0 0 0 0 
G:       11 | best score: 470 (overall: 470/520)  | rip distinct: 149 top: 2,809 count: 13
            | best last results: 3 1,207,959,552 0 0 0 3 3 3 3 3 3 
G:       21 | best score: 480 (overall: 480/520)  | rip distinct: 29 top: 2,167 count: 157
            | best last results: 3 134,217,728 0 0 0 3 3 3 3 3 3 
G:       23 | best score: 490 (overall: 490/520)  | rip distinct: 41 top: 2,167 count: 143
            | best last results: 20 3 0 0 0 3 3 3 3 3 3 
G:      116 | best score: 520 (overall: 520/520)  | rip distinct: 55 top: 2,171 count: 52
            | best last results: 20 42 0 0 0 42 3 3 3 3 3 
DONE! :)
```

### Shell scripts

Scripts [run_guess_x.sh](run_guess_x.sh) and [run_guess_x_loop.sh](run_guess_x_loop.sh) can be used for systematic evolution trials. These scripts also save the stdout and stderr outputs into log files in the current directory.

## Validation

The [validate.cc](validate.cc) program evaluates the evolved programs.

Evolved programs are saved in the `bazel-out/k8-fastbuild/bin/examples/000_guess_value/main.runfiles/__main__/` directory during evolution.

```
$ ls -1 bazel-out/k8-fastbuild/bin/examples/000_guess_value/main.runfiles/__main__/
elfs
examples
guess_42_rs_2_best_program.elf
guess_42_rs_2_gen_116_best_program.elf
guess_42_rs_2_gen_11_best_program.elf
guess_42_rs_2_gen_1_best_program.elf
guess_42_rs_2_gen_21_best_program.elf
guess_42_rs_2_gen_23_best_program.elf
guess_42_rs_2_gen_2_best_program.elf
guess_42_rs_2_gen_3_best_program.elf
guess_42_rs_2_gen_7_best_program.elf
```

Copy selected evolved programs to `examples/000_guess_value/evolved_elfs`.

```
$ cp bazel-out/k8-fastbuild/bin/examples/000_guess_value/main.runfiles/__main__/guess_42_rs_2_best_program.elf examples/000_guess_value/evolved_elfs/ -v
'bazel-out/k8-fastbuild/bin/examples/000_guess_value/main.runfiles/__main__/guess_42_rs_2_best_program.elf' -> 'examples/000_guess_value/evolved_elfs/guess_42_rs_2_best_program.elf'
```

### Usage:

```
$ bazel run //examples/000_guess_value:validate -- --help
INFO: Analyzed target //examples/000_guess_value:validate (1 packages loaded, 8 targets configured).
INFO: Found 1 target...
Target //examples/000_guess_value:validate up-to-date:
  bazel-bin/examples/000_guess_value/validate
INFO: Elapsed time: 0.122s, Critical Path: 0.02s
INFO: 3 processes: 3 internal.
INFO: Build completed successfully, 3 total actions
INFO: Build completed successfully, 3 total actions
validate: This program evaluates ELF programs that were evoloved to 'guess' a value and place the value in the results[1] global variable.
NOTE: The evolution produces invalid executables, always run this program in a sandbox!
Sample usage via the bazel build system (with 'build --spawn_strategy=linux-sandbox' in .bazelrc):
bazel run //examples/000_guess_value:validate -- --value_to_guess=1009 --elf_filename=best_program.elf

  Flags from examples/000_guess_value/validate.cc:
    --elf_filename (filename of the evolved ELF program to validate);
      default: "examples/000_guess_value/evolved_elfs/best_program.elf";
    --num_evaluations (number of evaluations to be performed on the evolved ELF
      program); default: 20;
    --value_to_guess (the correct value expected in the results[1] global
      variable after the execution of the evolved program); default: 42;

Try --helpfull to get a list of all flags or --help=substring shows help for
flags which include specified substring in either in the name, or description or
path.
```

### Example validation:

> **_NOTE:_** Do not forget to copy the evolved program(s) to `examples/000_guess_value/evolved_elfs` as outlined above.

```
$ bazel run //examples/000_guess_value:validate -- --value_to_guess=42 --num_evaluations=20 --elf_filename=examples/000_guess_value/evolved_elfs/guess_42_rs_2_best_program.elf 
INFO: Analyzed target //examples/000_guess_value:validate (0 packages loaded, 0 targets configured).
INFO: Found 1 target...
Target //examples/000_guess_value:validate up-to-date:
  bazel-bin/examples/000_guess_value/validate
INFO: Elapsed time: 1.191s, Critical Path: 1.10s
INFO: 3 processes: 1 internal, 2 linux-sandbox.
INFO: Build completed successfully, 3 total actions
INFO: Running command line: bazel-bin/examples/000_guess_value/validate '--value_to_guess=42' '--num_evaluations=20' '--elf_filename=INFO: Build completed successfully, 3 total actions
# value_to_guess: 42
# elf_filename: examples/000_guess_value/evolved_elfs/guess_42_rs_2_best_program.elf
# num_evaluations: 20
        expected  result[1] | score | results                   | signals
OK!          42          42 |    52 | 20 42 0 0 0 42 3 3 3 3 3  | 11 9
OK!          42          42 |    52 | 20 42 0 0 0 42 3 3 3 3 3  | 11 9
OK!          42          42 |    52 | 20 42 0 0 0 42 3 3 3 3 3  | 11 9
OK!          42          42 |    52 | 20 42 0 0 0 42 3 3 3 3 3  | 11 9
OK!          42          42 |    52 | 20 42 0 0 0 42 3 3 3 3 3  | 11 9
OK!          42          42 |    52 | 20 42 0 0 0 42 3 3 3 3 3  | 11 9
OK!          42          42 |    52 | 20 42 0 0 0 42 3 3 3 3 3  | 11 9
OK!          42          42 |    52 | 20 42 0 0 0 42 3 3 3 3 3  | 11 9
OK!          42          42 |    52 | 20 42 0 0 0 42 3 3 3 3 3  | 11 9
OK!          42          42 |    52 | 20 42 0 0 0 42 3 3 3 3 3  | 11 9
OK!          42          42 |    52 | 20 42 0 0 0 42 3 3 3 3 3  | 11 9
OK!          42          42 |    52 | 20 42 0 0 0 42 3 3 3 3 3  | 11 9
OK!          42          42 |    52 | 20 42 0 0 0 42 3 3 3 3 3  | 11 9
OK!          42          42 |    52 | 20 42 0 0 0 42 3 3 3 3 3  | 11 9
OK!          42          42 |    52 | 20 42 0 0 0 42 3 3 3 3 3  | 11 9
OK!          42          42 |    52 | 20 42 0 0 0 42 3 3 3 3 3  | 11 9
OK!          42          42 |    52 | 20 42 0 0 0 42 3 3 3 3 3  | 11 9
OK!          42          42 |    52 | 20 42 0 0 0 42 3 3 3 3 3  | 11 9
OK!          42          42 |    52 | 20 42 0 0 0 42 3 3 3 3 3  | 11 9
OK!          42          42 |    52 | 20 42 0 0 0 42 3 3 3 3 3  | 11 9
SUCCESS RATE: 100.00%
```