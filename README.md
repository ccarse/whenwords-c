# whenwords (C)

Human-friendly time formatting and parsing in C. Implements the whenwords v0.1.0 specification.

## What’s included

- `whenwords.c` / `whenwords.h`: library implementation
- `tests.yaml`: spec-driven tests
- `generate_tests.rb`: generates `tests.c` from `tests.yaml`
- `run_tests.sh`: build + run tests
- `usage.md`: API documentation and examples

## Build & test

```sh
./run_tests.sh
```

Or manually:

```sh
./generate_tests.rb
cc -std=c99 -Wall -Wextra -Werror whenwords.c tests.c -lm -o tests
./tests
```

## Using the library

Add `whenwords.c` and `whenwords.h` to your project and compile them with your sources. See `usage.md` for a quick start and full API reference.
