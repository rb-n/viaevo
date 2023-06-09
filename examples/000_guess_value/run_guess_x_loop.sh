#!/bin/bash

# Based on https://tldp.org/LDP/abs/html/sha-bang.html.
E_WRONG_ARGS=85

if [ $# -ne 2 ]; then
  echo "Usage: $(basename $0) value_to_guess number_of_iterations"
  exit $E_WRONG_ARGS
fi

VALUE_TO_GUESS=$1
ITERATIONS=$2

echo "value to guess: $VALUE_TO_GUESS iterations: $ITERATIONS"

for i in $(seq $ITERATIONS); do
  echo "iteration $i of $ITERATIONS"
  sh examples/000_guess_value/run_guess_x.sh $VALUE_TO_GUESS $i
done

exit
