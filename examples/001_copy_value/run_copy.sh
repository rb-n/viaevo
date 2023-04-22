#!/bin/bash

# Based on https://tldp.org/LDP/abs/html/sha-bang.html.
E_WRONG_ARGS=85

if [ $# -ne 2 ]; then
  echo "Usage: $(basename $0) num_value_copies_in_inputs random_seed_value"
  exit $E_WRONG_ARGS
fi

NUM_COPIES=$1
RANDOM_SEED=$2
PREFIX="simple_small_copy_n${NUM_COPIES}_rs_${RANDOM_SEED}"

echo "num value copies in inputs: $NUM_COPIES random seed: $RANDOM_SEED"
echo "output file prefix: $PREFIX"

(time bazel run //examples/001_copy_value:main -- --num_value_copies_in_inputs=$NUM_COPIES --random_seed=$RANDOM_SEED --output_filename_prefix="${PREFIX}_") |& tee >(sed -e 's/.*\r\([^\n]\)/\1/' >$PREFIX.log)

exit
