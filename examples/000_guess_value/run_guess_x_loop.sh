#!/bin/bash

E_WRONG_ARGS=85

if [ $# -ne 2 ]; then
  echo "Usage: $(basename $0) value_to_guess number_of_iterations"
  # `basename $0` is the script's filename.
  exit $E_WRONG_ARGS
fi

VALUE_TO_GUESS=$1
ITERATIONS=$2

echo "value to guess: $VALUE_TO_GUESS iterations: $ITERATIONS"

for i in $(seq $ITERATIONS); do
  sh examples/000_guess_value/run_guess_x.sh $VALUE_TO_GUESS $i
done

exit
