# [Viaevo](../../README.md) > example 100_mnist_digits

This example evolves programs that perform Handwritten digit recognition using the [MNIST database of handwritten digits](http://yann.lecun.com/exdb/mnist/). The training (and test) data are not a part of this repo and need to be [downloaded separately](examples/100_mnist_digits/data/README.md). The image binary data are placed in `inputs` global variable and the result (the value of the digit) is expected in the `results[1]` global variable. The default starting program for the evolution is //elfs:intermediate_medium ([intermediate_medium.c](../../elfs/intermediate_medium.c)).

> **_WARNING:_** The evolution produces invalid executables. To protect your system, ***always run these programs in a sandbox!*** When using bazel on Linux, this is achieved by default via `build --spawn_strategy=linux-sandbox` in [.bazelrc](../../.bazelrc). There are other safeguards in place (such as restricting system calls the evolved programs' processes can make and terminating the processes on any signal or system call once the evolved code is reached), but the evolved programs may still exhibit unwanted behavior.

## Evolution

The [main.cc](main.cc) program drives the evolution. [ScorerMnistDigits](scorer_mnist_digits.h) is used to scores based on the `results[1]` variable if it changes. If not, small scores are "awarded" for changes in values of `results[2]` to `results[10]` (assuming changes here are better than no changes at all). [ScorerMnistDigits](scorer_mnist_digits.h) also scores results history (results of multiple executions of the same program with different inputs) to "promote" different results in the range from `0` to `9`.

### Usage:

```
$ bazel run //examples/100_mnist_digits:main -- --help
Starting local Bazel server and connecting to it...
INFO: Analyzed target //examples/100_mnist_digits:main (60 packages loaded, 626 targets configured).
INFO: Found 1 target...
Target //examples/100_mnist_digits:main up-to-date:
  bazel-bin/examples/100_mnist_digits/main
INFO: Elapsed time: 11.477s, Critical Path: 3.08s
INFO: 3 processes: 1 internal, 2 linux-sandbox.
INFO: Build completed successfully, 3 total actions
INFO: Build completed successfully, 3 total actions
main: This program evolves ELFs to recognize digits from the MNIST database (http://yann.lecun.com/exdb/mnist/) and place the value in the results[1] global variable of the evolved program.

WARNING: The evolution produces invalid executables. To protect your system, always run this program in a sandbox!

Sample usage via the bazel build system (with 'build --spawn_strategy=linux-sandbox' in .bazelrc):

bazel run //examples/100_mnist_digits:main -- --random_seed=2 --output_filename_prefix=digits_rs_2_

  Flags from examples/100_mnist_digits/main.cc:
    --elf_filename (filename of the ELF executable to be used as the starting
      template for evolution (NOTE: results are expected to be initialized to
      {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}));
      default: "elfs/intermediate_medium";
    --evaluations_per_program (number of evaluations to be performed on each
      program in each generation (scores are accumulated across evaluations));
      default: 200;
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
      results across evaluations within a generation); default: true;

Try --helpfull to get a list of all flags or --help=substring shows help for
flags which include specified substring in either in the name, or description or
path.
```

### Example run:

> **_NOTE:_** Repeated evolutions with the same random seed may diverge if the evolved programs compute results non-deterministically.

```
$ bazel run //examples/100_mnist_digits:main -- --random_seed=5 --output_filename_prefix=digits_rs_5_
INFO: Analyzed target //examples/100_mnist_digits:main (0 packages loaded, 0 targets configured).
INFO: Found 1 target...
Target //examples/100_mnist_digits:main up-to-date:
  bazel-bin/examples/100_mnist_digits/main
INFO: Elapsed time: 0.254s, Critical Path: 0.00s
INFO: 1 process: 1 internal.
INFO: Build completed successfully, 1 total action
INFO: Build completed successfully, 1 total action
# elf_filename: elfs/intermediate_medium
# mu: 60
# phi: 10
# lambda: 140
# evaluations_per_program: 200
# max_generations: 10000
# score_results_history: true
# output_filename_prefix: digits_rs_5_
# random_seed: 5
# initialize_programs_to_all_nops: false
G:        4 | best score: 200 (overall: 200/11,999,200,000,000,000)  | rip distinct: 87 top: 1,056 count: 12
            | best last results: 52 -1 -1 -1 -1 -1 -1,300,280,952 -1 -1 -1 -1 
G:       19 | best score: 400 (overall: 400/11,999,200,000,000,000)  | rip distinct: 15 top: 71 count: 75
            | best last results: 52 -1 -1 -1 -1 -1 -1,223,799,880 -1,223,799,880 -1 -1 -1 
G:       28 | best score: 1,869,000,000,000,200 (overall: 1,869,000,000,000,200/11,999,200,000,000,000)  | rip distinct: 8 top: 87 count: 189
            | best last results: 1,238,320,901 21,852 -1 -1 -1 -1 -1 -1 -1 -1 -1 
G:       31 | best score: 1,870,000,000,000,200 (overall: 1,870,000,000,000,200/11,999,200,000,000,000)  | rip distinct: 7 top: 87 count: 150
            | best last results: 1,285,449,477 22,029 -1 -1 -1 -1 -1 -1 -1 -1 -1 
G:       33 | best score: 1,874,000,000,000,200 (overall: 1,874,000,000,000,200/11,999,200,000,000,000)  | rip distinct: 9 top: 11 count: 175
            | best last results: 1,483,749,125 22,012 -1 -1 -1 -1 -1 -1 -1 -1 -1 
G:       35 | best score: 1,876,000,000,000,200 (overall: 1,876,000,000,000,200/11,999,200,000,000,000)  | rip distinct: 6 top: 11 count: 195
            | best last results: 672,130,821 21,938 -1 -1 -1 -1 -1 -1 -1 -1 -1 
G:       71 | best score: 1,881,000,000,000,200 (overall: 1,881,000,000,000,200/11,999,200,000,000,000)  | rip distinct: 3 top: 11 count: 183
            | best last results: -406,092,027 21,952 -1 -1 -1 -1 -1 -1 -1 -1 -1
...
```

### Shell scripts

Scripts [run_digits.sh](run_digits.sh) and [run_digits_loop.sh](run_digits_loop.sh) can be used for systematic evolution trials. These scripts also save the stdout and stderr outputs into log files in the current directory.

## Validation

The [validate.cc](validate.cc) program evaluates the evolved programs.

Evolved programs are saved in the `bazel-out/k8-fastbuild/bin/examples/100_mnist_digits/main.runfiles/__main__/` directory during evolution.

```
$ ls -1 bazel-out/k8-fastbuild/bin/examples/100_mnist_digits/main.runfiles/__main__/
digits_rs_5_gen_19_best_program.elf
digits_rs_5_gen_28_best_program.elf
digits_rs_5_gen_31_best_program.elf
digits_rs_5_gen_33_best_program.elf
digits_rs_5_gen_35_best_program.elf
digits_rs_5_gen_4_best_program.elf
digits_rs_5_gen_71_best_program.elf
elfs
examples
...
```

Copy selected evolved programs to `examples/100_mnist_digits/evolved_elfs`.

```
$ cp bazel-out/k8-fastbuild/bin/examples/100_mnist_digits/main.runfiles/__main__/digits_rs_5_gen_71_best_program.elf examples/100_mnist_digits/evolved_elfs/ -v
'bazel-out/k8-fastbuild/bin/examples/100_mnist_digits/main.runfiles/__main__/digits_rs_5_gen_71_best_program.elf' -> 'examples/100_mnist_digits/evolved_elfs/digits_rs_5_gen_71_best_program.elf'
```

### Usage:

```
$ bazel run //examples/100_mnist_digits:validate -- --help
INFO: Analyzed target //examples/100_mnist_digits:validate (0 packages loaded, 0 targets configured).
INFO: Found 1 target...
Target //examples/100_mnist_digits:validate up-to-date:
  bazel-bin/examples/100_mnist_digits/validate
INFO: Elapsed time: 0.153s, Critical Path: 0.00s
INFO: 1 process: 1 internal.
INFO: Build completed successfully, 1 total action
INFO: Build completed successfully, 1 total action
validate: This program evaluates ELF programs that were evoloved to recognize handwritten digits and place the digit value in the results[1] global variable.
WARNING: The evolution produces invalid executables, always run this program in a sandbox!
Sample usage via the bazel build system (with 'build --spawn_strategy=linux-sandbox' in .bazelrc):
bazel run //examples/100_mnist_digits:validate -- --elf_filename=best_program.elf

  Flags from examples/100_mnist_digits/validate.cc:
    --elf_filename (filename of the evolved ELF program to validate);
      default: "examples/100_mnist_digits/evolved_elfs/no_9_labels_intermediate_medium_digits_rs_1_gen_2606_best_program.elf";
    --num_evaluations (number of evaluations to be performed on the evolved ELF
      program); default: 1000;
    --random_seed (random seed for the validation (values to copy a selected at
      random)); default: 1;

Try --helpfull to get a list of all flags or --help=substring shows help for
flags which include specified substring in either in the name, or description or
path.
```

### Example validation:

> **_NOTE:_** Do not forget to copy the evolved program(s) to `examples/001_copy_value/evolved_elfs` as outlined above.

```
$ bazel run //examples/100_mnist_digits:validate -- --elf_filename=examples/100_mnist_digits/evolved_elfs/no_9_labels_intermediate_medium_digits_rs_1_gen_2606_best_program.elf --num_evaluations=100 --random_seed=0
INFO: Analyzed target //examples/100_mnist_digits:validate (0 packages loaded, 0 targets configured).
INFO: Found 1 target...
Target //examples/100_mnist_digits:validate up-to-date:
  bazel-bin/examples/100_mnist_digits/validate
INFO: Elapsed time: 2.890s, Critical Path: 2.50s
INFO: 3 processes: 1 internal, 2 linux-sandbox.
INFO: Build completed successfully, 3 total actions
INFO: Running command line: bazel-bin/examples/100_mnist_digits/validate '--elf_filename=examples/100_mnist_digits/evolved_elfs/no_9_labels_intermediate_medium_digits_rs_1_gen_2606_best_program.elf' '--num_evaluatINFO: Build completed successfully, 3 total actions
# elf_filename: examples/100_mnist_digits/evolved_elfs/no_9_labels_intermediate_medium_digits_rs_1_gen_2606_best_program.elf
# random_seed: 0
# num_evaluations: 100
        expected  result[1] |      score  | results                   | signals
---           9           0 |     1000000 | 822083583 0 989855744 0 -16777216 16777215 0 -16777216 16777215 5046272 -16777216  | 11 9
---           5           0 |     1000000 | -1124073473 0 -956301312 0 -16777216 16777215 0 -16777216 16777215 0 -16777216  | 7 9
---           4           8 |     1000000 | 352321535 8 335544320 -16777208 -1 -1 16777215 -16777216 16777215 0 -16777216  | 11 9
---           4           0 |     1000000 | 1157627903 0 1325400064 0 -16777216 16777215 0 -16777216 -1728053249 16514557 -16777216  | 11 9
OK!           1           1 |  1000000000 | -50331649 1 -67108864 -16777215 -1 -1 16777215 -16777216 -1073741825 16777215 -16777216  | 8 9
---           1           5 |     1000000 | -1459617793 5 -1476395008 -16777211 -1 -1 16777215 -16777216 -1090519041 2946814 -16777216  | 11 9
---           4           1 |     1000000 | -50331649 1 -67108864 -16777215 -1 -1 16777215 -16777216 16777215 14555648 -16777216  | 8 9
---           5           9 |     1000000 | 1090519039 9 1073741824 -16777207 -1 -1 16777215 -16777216 16777215 6823936 -16777216  | 11 9
---           2           0 |     1000000 | 486539263 0 469762048 -16777216 -1 -1 16777215 -16777216 -33554433 16645629 -16777216  | 11 9
---           0           1 |     1000000 | -50331649 1 -67108864 -16777215 -1 -1 16777215 -16777216 16777215 0 -16777216  | 8 9
---           7           1 |     1000000 | -385875969 1 -402653184 -16777215 -1 -1 16777215 -16777216 16777215 0 -16777216  | 11 9
---           5           0 |     1000000 | 822083583 0 989855744 0 -16777216 16777215 0 -16777216 -33554433 4526 -16777216  | 11 9
---           8           0 |     1000000 | 1157627903 0 1325400064 0 -16777216 16777215 0 -16777216 1191182335 0 -16777216  | 11 9
---           9           1 |     1000000 | -50331649 1 -67108864 -16777215 -1 -1 16777215 -16777216 251658239 0 -16777216  | 11 9
---           2           0 |     1000000 | 822083583 0 989855744 0 -16777216 16777215 0 -16777216 486539263 16630812 -16777216  | 11 9
OK!           1           1 |  1000000000 | -50331649 1 -67108864 -16777215 -1 -1 16777215 -16777216 16777215 16660224 -16777216  | 8 9
---           9           1 |     1000000 | -50331649 1 -67108864 -16777215 -1 -1 16777215 -16777216 16777215 4979766 -16777216  | 8 9
---           3           1 |     1000000 | -50331649 1 -67108864 -16777215 -1 -1 16777215 -16777216 16777215 0 -16777216  | 8 9
OK!           1           1 |  1000000000 | -50331649 1 -67108864 -16777215 -1 -1 16777215 -16777216 -2097152001 7077373 -16777216  | 8 9
---           9           0 |     1000000 | 1157627903 0 1325400064 0 -16777216 16777215 0 -16777216 -33554433 49661 -16777216  | 11 9
OK!           3           3 |  1000000000 | -251658241 3 -268435456 -16777213 -1 -1 16777215 -16777216 16777215 0 -16777216  | 11 9
OK!           0           0 |  1000000000 | 486539263 0 469762048 -16777216 -1 -1 16777215 -16777216 16777215 8925978 -16777216  | 11 9
---           1           0 |     1000000 | 1493172223 0 1660944384 0 -16777216 16777215 0 -16777216 16777215 16657920 -16777216  | 11 9
OK!           1           1 |  1000000000 | -50331649 1 -67108864 -16777215 -1 -1 16777215 -16777216 16777215 16616704 -16777216  | 8 9
OK!           0           0 |  1000000000 | 1157627903 0 1325400064 0 -16777216 16777215 0 -16777216 201326591 0 -16777216  | 11 9
...
...
...
---           7           8 |     1000000 | -251658241 8 -268435456 -16777208 -1 -1 16777215 -16777216 16777215 0 -16777216  | 11 9
OK!           8           8 |  1000000000 | -922746881 8 -939524096 -16777208 -1 -1 16777215 -16777216 -33554433 2798334 -16777216  | 11 9
---           7           1 |     1000000 | 1224736767 1 1207959552 -16777215 -1 -1 16777215 -16777216 16777215 0 -16777216  | 11 9
SUCCESS RATE: 27.00%
```