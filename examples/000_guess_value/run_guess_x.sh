#!/bin/bash

E_WRONG_ARGS=85

if [ $# -ne 2 ]; then
  echo "Usage: $(basename $0) value_to_guess random_seed_value"
  # `basename $0` is the script's filename.
  exit $E_WRONG_ARGS
fi

VALUE_TO_GUESS=$1
RANDOM_SEED=$2
PREFIX="simple_small_guess_${VALUE_TO_GUESS}_rs_${RANDOM_SEED}"

echo "value to guess: $VALUE_TO_GUESS random seed: $RANDOM_SEED"
echo "output file prefix: $PREFIX"

(time bazel run //examples/000_guess_value:main -- --value_to_guess=$VALUE_TO_GUESS --random_seed=$RANDOM_SEED --output_filename_prefix="${PREFIX}_") |& tee >(sed -e 's/.*\r\([^\n]\)/\1/' >$PREFIX.log)

exit
