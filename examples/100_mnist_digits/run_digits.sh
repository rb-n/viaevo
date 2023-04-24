#!/bin/bash

# Based on https://tldp.org/LDP/abs/html/sha-bang.html.
E_WRONG_ARGS=85

if [ $# -ne 1 ]; then
  echo "Usage: $(basename $0) random_seed_value"
  exit $E_WRONG_ARGS
fi

RANDOM_SEED=$1
PREFIX="intermediate_medium_digits_rs_${RANDOM_SEED}"

echo "random seed: $RANDOM_SEED"
echo "output file prefix: $PREFIX"

(time bazel run //examples/100_mnist_digits:main -- --random_seed=$RANDOM_SEED --output_filename_prefix="${PREFIX}_") |& tee >(sed -e 's/.*\r\([^\n]\)/\1/' >$PREFIX.log)

exit
