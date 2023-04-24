#!/bin/bash

# Based on https://tldp.org/LDP/abs/html/sha-bang.html.
E_WRONG_ARGS=85

if [ $# -ne 1 ]; then
  echo "Usage: $(basename $0) number_of_iterations"
  exit $E_WRONG_ARGS
fi

ITERATIONS=$1

echo "iterations: $ITERATIONS"

for i in $(seq $ITERATIONS); do
  echo "iteration $i of $ITERATIONS"
  sh examples/100_mnist_digits/run_digits.sh $VALUE_TO_GUESS $i
done

exit
