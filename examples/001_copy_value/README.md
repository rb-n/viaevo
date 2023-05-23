# [Viaevo](../../README.md) > example 001_copy_value

This example evolves programs that copy the value from `inputs[0]` (or any other element in inputs if the value is present in this array in multiple copies) to the `results[1]` global variable. The default starting program for the evolution is //elfs:simple_small ([simple_small.c](../../elfs/simple_small.c)).

> **_WARNING:_** The evolution produces invalid executables. To protect your system, ***always run these programs in a sandbox!*** When using bazel on Linux, this is achieved by default via `build --spawn_strategy=linux-sandbox` in [.bazelrc](../../.bazelrc). There are other safeguards in place (such as restricting system calls the evolved programs' processes can make and terminating the processes on any signal or system call once the evolved code is reached), but the evolved programs may still exhibit unwanted behavior.

## Evolution

The [main.cc](main.cc) program drives the evolution. If the value of results[1] changes after the execution of the evolved program, [ScorerCopyValue](scorer_copy_value.h) scores every matching bit to that of the expected value to be copied. If results[1] does not change, small scores are "awarded" for changes in values of results[2] to results[10].

### Usage:

```
$ bazel run //examples/001_copy_value:main -- --help
INFO: Analyzed target //examples/001_copy_value:main (0 packages loaded, 2 targets configured).
INFO: Found 1 target...
Target //examples/001_copy_value:main up-to-date:
  bazel-bin/examples/001_copy_value/main
INFO: Elapsed time: 0.134s, Critical Path: 0.01s
INFO: 1 process: 1 internal.
INFO: Build completed successfully, 1 total action
INFO: Build completed successfully, 1 total action
main: This program evolves ELFs to copy a value from inputs[0] (or other element in inputs if multiple copies are present there) to the results[1] global variable of the evolved program.

WARNING: The evolution produces invalid executables. To protect your system, always run this program in a sandbox!

Sample usage via the bazel build system (with 'build --spawn_strategy=linux-sandbox' in .bazelrc):

bazel run //examples/001_copy_value:main

  Flags from examples/001_copy_value/main.cc:
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
    --num_value_copies_in_inputs (number of copies of the value to be copied in
      inputs (the first n items of inputs are filled with this value assuming
      more copies are more likely to lead to an evolved program copying this
      value to results[1])); default: 10;
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
      000_copy_value)); default: false;

Try --helpfull to get a list of all flags or --help=substring shows help for
flags which include specified substring in either in the name, or description or
path.
```

### Example run:

> **_NOTE:_** Repeated evolutions with the same random seed may diverge if the evolved programs compute results non-deterministically.

```
$ bazel run //examples/001_copy_value:main -- --num_value_copies_in_inputs=10 --random_seed=2 --output_filename_prefix="copy_n10_rs_2_"
INFO: Analyzed target //examples/001_copy_value:main (0 packages loaded, 0 targets configured).
INFO: Found 1 target...
Target //examples/001_copy_value:main up-to-date:
  bazel-bin/examples/001_copy_value/main
INFO: Elapsed time: 0.106s, Critical Path: 0.00s
INFO: 1 process: 1 internal.
INFO: Build completed successfully, 1 total action
INFO: Running command line: bazel-bin/examples/001_copy_value/main '--num_value_copies_in_inputs=10' '--random_seed=2' '--output_fileINFO: Build completed successfully, 1 total action
# elf_filename: elfs/simple_small
# mu: 60
# phi: 10
# lambda: 140
# evaluations_per_program: 10
# max_generations: 10000
# score_results_history: false
# output_filename_prefix: copy_n10_rs_2_
# random_seed: 2
# initialize_programs_to_all_nops: false
# num_value_copies_in_inputs: 10
G:        1 | best score: 10 (overall: 10/1,120)  | rip distinct: 190 top: 3,305 count: 4
            | best last results: 20 0 0 0 0 0 3 3 3 3 0 
G:        4 | best score: 361 (overall: 361/1,120)  | rip distinct: 114 top: 3,305 count: 15
            | best last results: 10 2 0 0 0 -1,869,574,000 3 3 3 3 3 
G:        7 | best score: 369 (overall: 369/1,120)  | rip distinct: 83 top: 467 count: 70
            | best last results: 10 2 0 0 0 -1,869,574,000 3 3 3 3 3 
G:        9 | best score: 956 (overall: 956/1,120)  | rip distinct: 109 top: 1,259 count: 23
            | best last results: 10 2 0 0 0 -1,869,574,000 3 16,515,075 3 -1,533,411,325 -1,533,399,907 
G:       10 | best score: 986 (overall: 986/1,120)  | rip distinct: 100 top: 1,385 count: 15
            | best last results: 10 2 0 0 0 -1,869,574,000 3 16,515,075 3 204,996,611 205,030,144 
G:       46 | best score: 991 (overall: 991/1,120)  | rip distinct: 62 top: 2,260 count: 34
            | best last results: 10 2 0 0 0 -1,869,574,000 3 16,515,075 3 1,257,308,163 1,257,361,667 
G:       65 | best score: 1,120 (overall: 1,120/1,120)  | rip distinct: 55 top: 1,903 count: 41
            | best last results: 0 1,573,338,765 -1,869,574,000 -1,869,574,000 -1,869,574,000 -1,869,574,000 3 3 3 3 3 
DONE! :)
```

### Shell scripts

Scripts [run_copy.sh](run_copy.sh) and [run_copy_loop.sh](run_copy_loop.sh) can be used for systematic evolution trials. These scripts also save the stdout and stderr outputs into log files in the current directory.

## Validation

The [validate.cc](validate.cc) program evaluates the evolved programs.

Evolved programs are saved in the `bazel-out/k8-fastbuild/bin/examples/001_copy_value/main.runfiles/__main__/` directory during evolution.

```
$ ls -1 bazel-out/k8-fastbuild/bin/examples/001_copy_value/main.runfiles/__main__/
copy_n10_rs_2_best_program.elf
copy_n10_rs_2_gen_10_best_program.elf
copy_n10_rs_2_gen_1_best_program.elf
copy_n10_rs_2_gen_46_best_program.elf
copy_n10_rs_2_gen_4_best_program.elf
copy_n10_rs_2_gen_65_best_program.elf
copy_n10_rs_2_gen_7_best_program.elf
copy_n10_rs_2_gen_9_best_program.elf
...
```

Copy selected evolved programs to `examples/001_copy_value/evolved_elfs`.

```
$ cp bazel-out/k8-fastbuild/bin/examples/001_copy_value/main.runfiles/__main__/copy_n10_rs_2_best_program.elf examples/001_copy_value/evolved_elfs/ -v
'bazel-out/k8-fastbuild/bin/examples/001_copy_value/main.runfiles/__main__/copy_n10_rs_2_best_program.elf' -> 'examples/001_copy_value/evolved_elfs/copy_n10_rs_2_best_program.elf'
```

### Usage:

```
$ bazel run //examples/001_copy_value:validate -- --help
INFO: Analyzed target //examples/001_copy_value:validate (0 packages loaded, 0 targets configured).
INFO: Found 1 target...
Target //examples/001_copy_value:validate up-to-date:
  bazel-bin/examples/001_copy_value/validate
INFO: Elapsed time: 2.191s, Critical Path: 2.08s
INFO: 3 processes: 1 internal, 2 linux-sandbox.
INFO: Build completed successfully, 3 total actions
INFO: Build completed successfully, 3 total actions
validate: This program evaluates ELF programs that were evoloved to copy a value from inputs and place the value in the results[1] global variable.
WARNING: The evolution produces invalid executables, always run this program in a sandbox!
Sample usage via the bazel build system (with 'build --spawn_strategy=linux-sandbox' in .bazelrc):
bazel run //examples/001_copy_value:validate -- --num_value_copies_in_inputs=10 --elf_filename=best_program.elf

  Flags from examples/001_copy_value/validate.cc:
    --elf_filename (filename of the evolved ELF program to validate);
      default: "examples/001_copy_value/evolved_elfs/best_program.elf";
    --num_evaluations (number of evaluations to be performed on the evolved ELF
      program); default: 20;
    --num_value_copies_in_inputs (number of copies of the value to be copied in
      inputs (the first n items of inputs are filled with this value, use the
      same value here as in the evolution)); default: 10;
    --random_seed (random seed for the validation (values to copy a selected at
      random)); default: 1;

Try --helpfull to get a list of all flags or --help=substring shows help for
flags which include specified substring in either in the name, or description or
path.
```

### Example validation:

> **_NOTE:_** Do not forget to copy the evolved program(s) to `examples/001_copy_value/evolved_elfs` as outlined above.

```
$ bazel run //examples/001_copy_value:validate -- --elf_filename=examples/001_copy_value/evolved_elfs/copy_n10_rs_2_best_program.elf --num_value_copies_in_inputs=10
INFO: Analyzed target //examples/001_copy_value:validate (0 packages loaded, 0 targets configured).
INFO: Found 1 target...
Target //examples/001_copy_value:validate up-to-date:
  bazel-bin/examples/001_copy_value/validate
INFO: Elapsed time: 0.090s, Critical Path: 0.00s
INFO: 1 process: 1 internal.
INFO: Build completed successfully, 1 total action
INFO: Running command line: bazel-bin/examples/001_copy_value/validate '--elf_filename=examples/001_copy_value/evolved_elfs/copy_n10_INFO: Build completed successfully, 1 total action
# elf_filename: examples/001_copy_value/evolved_elfs/copy_n10_rs_2_best_program.elf
# random_seed: 1
# num_value_copies_in_inputs: 10
# num_evaluations: 20
        expected  result[1] | score | results                   | signals
OK!   -12091157   -12091157 |   112 | 0 -12091157 -1869574000 -1869574000 -1869574000 -1869574000 3 3 3 3 3  | 11 9
OK! -1201197172 -1201197172 |   112 | 0 -1201197172 -1869574000 -1869574000 -1869574000 -1869574000 3 3 3 3 3  | 11 9
OK!  -289663928  -289663928 |   112 | 0 -289663928 -1869574000 -1869574000 -1869574000 -1869574000 3 3 3 3 3  | 11 9
OK!      491263      491263 |   112 | 0 491263 -1869574000 -1869574000 -1869574000 -1869574000 3 3 3 3 3  | 11 9
OK!   550290313   550290313 |   112 | 0 550290313 -1869574000 -1869574000 -1869574000 -1869574000 3 3 3 3 3  | 11 9
OK!  1298508491  1298508491 |   112 | 0 1298508491 -1869574000 -1869574000 -1869574000 -1869574000 3 3 3 3 3  | 11 9
OK!    -4120955    -4120955 |   112 | 0 -4120955 -1869574000 -1869574000 -1869574000 -1869574000 3 3 3 3 3  | 11 9
OK!   630311759   630311759 |   112 | 0 630311759 -1869574000 -1869574000 -1869574000 -1869574000 3 3 3 3 3  | 11 9
OK!  1013994432  1013994432 |   112 | 0 1013994432 -1869574000 -1869574000 -1869574000 -1869574000 3 3 3 3 3  | 11 9
OK!   396591248   396591248 |   112 | 0 396591248 -1869574000 -1869574000 -1869574000 -1869574000 3 3 3 3 3  | 11 9
OK!  1703301249  1703301249 |   112 | 0 1703301249 -1869574000 -1869574000 -1869574000 -1869574000 3 3 3 3 3  | 11 9
OK!   799981516   799981516 |   112 | 0 799981516 -1869574000 -1869574000 -1869574000 -1869574000 3 3 3 3 3  | 11 9
OK!  1666063943  1666063943 |   112 | 0 1666063943 -1869574000 -1869574000 -1869574000 -1869574000 3 3 3 3 3  | 11 9
OK!  1484172013  1484172013 |   112 | 0 1484172013 -1869574000 -1869574000 -1869574000 -1869574000 3 3 3 3 3  | 11 9
OK! -1418429956 -1418429956 |   112 | 0 -1418429956 -1869574000 -1869574000 -1869574000 -1869574000 3 3 3 3 3  | 11 9
OK!  1704103302  1704103302 |   112 | 0 1704103302 -1869574000 -1869574000 -1869574000 -1869574000 3 3 3 3 3  | 11 9
OK!  -276857575  -276857575 |   112 | 0 -276857575 -1869574000 -1869574000 -1869574000 -1869574000 3 3 3 3 3  | 11 9
OK! -1980767054 -1980767054 |   112 | 0 -1980767054 -1869574000 -1869574000 -1869574000 -1869574000 3 3 3 3 3  | 11 9
OK!  -660089580  -660089580 |   112 | 0 -660089580 -1869574000 -1869574000 -1869574000 -1869574000 3 3 3 3 3  | 11 9
OK!  1800426750  1800426750 |   112 | 0 1800426750 -1869574000 -1869574000 -1869574000 -1869574000 3 3 3 3 3  | 11 9
SUCCESS RATE: 100.00%
```