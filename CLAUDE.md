# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Test Commands

```bash
# Configure (first time or after CMakeLists.txt changes)
cmake -S . -B build

# Build everything
cmake --build build

# Run tests
ctest --test-dir build

# Build and run tests with failure output (convenience target)
cmake --build build --target check

# Run the interactive demo app
./build/ring_buffer_app [capacity]   # capacity defaults to 100
```

To run a single test there is no per-test filtering — all tests are compiled into one binary (`./build/ring_buffer_tests`) and run sequentially. Add a focused `test_*` function in `ring_buffer_tests.c` and call it from `main` to isolate a case.

## Architecture

The project is a C99 char-only ring buffer library with an interactive demo and a plain-C test suite.

**Core library** (`ring_buffer.h` / `ring_buffer.c`):
- `RingBuffer` struct holds a heap-allocated `char *buffer`, `capacity`, `head` (write index), `tail` (read index), and `count`.
- Write policy: **overwrite on full** — when full, `ring_buffer_write` silently drops the oldest element by advancing `tail`.
- `ring_buffer_resize` linearizes the buffer into a fresh allocation, truncating oldest elements if the new capacity is smaller than `count`.

**Demo app** (`main.c`):
- Interactive REPL accepting single-letter commands (`w`, `r`, `R`, `k`, `s`, `a`, `p`, `c`, `z`, `q`).
- Logs to syslog (`LOG_USER` facility) and measures wall-clock time per operation via `gettimeofday`.

**Tests** (`ring_buffer_tests.c`):
- No test framework dependency — uses a small `expect_*` helper set and a `tests_run`/`tests_failed` counter.
- Each `test_*` function is a standalone void; all are called sequentially from `main`.

## Coding Conventions

- Language: C99 (`-std=c99` via `CMAKE_C_STANDARD`).
- Indentation: 4 spaces, braces on the same line as control statements.
- All public API functions are prefixed `ring_buffer_`.
- Sizes/counts use `size_t`; error returns use `-1` (int).
- Defensive NULL and zero-capacity checks on every public function entry point.
