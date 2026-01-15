#!/usr/bin/env sh
set -eu

./generate_tests.rb
cc -std=c99 -Wall -Wextra -Werror whenwords.c tests.c -lm -o tests
./tests
