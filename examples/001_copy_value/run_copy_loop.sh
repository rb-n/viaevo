#!/bin/bash

# Based on https://tldp.org/LDP/abs/html/sha-bang.html.
E_WRONG_ARGS=85

if [ $# -ne 2 ]; then
  echo "Usage: $(basename $0) num_value_copies_in_inputs number_of_iterations"
  exit $E_WRONG_ARGS
fi

NUM_COPIES=$1
ITERATIONS=$2

echo "num value copies in inputs: $NUM_COPIES iterations: $ITERATIONS"

for i in $(seq $ITERATIONS); do
  echo "iteration $i of $ITERATIONS"
  sh examples/001_copy_value/run_copy.sh $NUM_COPIES $i
done

exit
