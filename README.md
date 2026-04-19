# ringbuffer

[![C99](https://img.shields.io/badge/std-C99-blue.svg)](https://en.wikipedia.org/wiki/C99)
[![CMake](https://img.shields.io/badge/build-CMake%203.10%2B-blue.svg)](https://cmake.org/)
[![License](https://img.shields.io/badge/license-Copyright%202025%20Chris%20Schibi-lightgrey.svg)]()

A char-only circular (ring) buffer library written in C99, with no external dependencies. The library uses an **overwrite-on-full** policy — when the buffer is at capacity, the oldest element is silently dropped to make room for the newest. Includes a record-delimiter layer for structured streaming and CSV export.

---

## Architecture

```
ring_buffer.h / ring_buffer.c   Core library
main.c                          Interactive REPL demo
ring_buffer_tests.c             Plain-C test suite (no framework)
CMakeLists.txt                  Build and test configuration
```

### Core Library (`ring_buffer.{h,c}`)

The `RingBuffer` struct holds a heap-allocated `char *buffer`, `capacity`, `head` (write index), `tail` (read index), and `count`.

- **Write policy:** overwrite-on-full — `ring_buffer_write` silently advances `tail` when the buffer is full.
- **Resize:** `ring_buffer_resize` linearizes the buffer into a fresh allocation. If the new capacity is smaller than `count`, the oldest elements are truncated.
- **Records:** `ring_buffer_write_record` appends data followed by `RING_BUFFER_RECORD_SEP` (`,`), and `ring_buffer_dump_csv` converts delimiters to newlines on export.

### Demo App (`main.c`)

Interactive REPL accepting single-letter commands. Logs each operation to syslog (`LOG_USER` facility) and measures wall-clock time per operation via `gettimeofday`.

### Test Suite (`ring_buffer_tests.c`)

20 test functions compiled into a single binary. Uses a small `expect_*` helper set and a `tests_run` / `tests_failed` counter. No external test framework.

---

## Build & Test

```bash
# Configure (first time or after CMakeLists.txt changes)
cmake -S . -B build

# Build everything
cmake --build build

# Run tests
ctest --test-dir build

# Build and run tests with failure output (convenience target)
cmake --build build --target check

# Run the interactive demo app (capacity defaults to 100)
./build/ring_buffer_app [capacity]
```

To run a single test there is no per-test filtering — all tests are compiled into one binary (`./build/ring_buffer_tests`) and run sequentially. Add a focused `test_*` function in `ring_buffer_tests.c` and call it from `main` to isolate a case.

---

## Public API

```c
#define RING_BUFFER_RECORD_SEP ','
```

### Lifecycle

```c
int    ring_buffer_init(RingBuffer *rb, size_t capacity);
void   ring_buffer_free(RingBuffer *rb);
void   ring_buffer_clear(RingBuffer *rb);
int    ring_buffer_resize(RingBuffer *rb, size_t new_capacity);
```

### Single-Element I/O

```c
int    ring_buffer_write(RingBuffer *rb, char data);
int    ring_buffer_read(RingBuffer *rb);   // returns char or -1 if empty
int    ring_buffer_peek(RingBuffer *rb);   // non-consuming; returns char or -1
```

### Bulk I/O

```c
int    ring_buffer_bulk_write(RingBuffer *rb, const char *data, size_t len);
int    ring_buffer_bulk_read(RingBuffer *rb, char *dest, size_t len);
```

### Inspection

```c
size_t ring_buffer_size(RingBuffer *rb);
size_t ring_buffer_capacity(RingBuffer *rb);
size_t ring_buffer_available_space(RingBuffer *rb);
int    ring_buffer_is_empty(RingBuffer *rb);
int    ring_buffer_is_full(RingBuffer *rb);
double ring_buffer_usage_percent(RingBuffer *rb);
```

### Records & Export

```c
int    ring_buffer_write_record(RingBuffer *rb, const char *data, size_t len);
int    ring_buffer_dump_csv(RingBuffer *rb, const char *filename);
```

All functions return `-1` on error. Sizes and counts use `size_t`. Every public entry point is defensive against `NULL` pointers and zero capacities.

---

## Demo App Commands

| Command | Description |
|---------|-------------|
| `w <data>` | Bulk-write a string into the ring buffer |
| `r` | Read one character |
| `R <n>` | Bulk-read `n` characters |
| `k` | Peek at the next character without consuming it |
| `s` | Show current size |
| `a` | Show available space and usage percentage |
| `p` | Print all buffer contents |
| `d` | Dump buffer to a timestamped CSV file, then clear |
| `c` | Clear the ring buffer |
| `z <cap>` | Resize the ring buffer to a new capacity |
| `q` | Quit |

---

## Branch History — Feature Evolution

This repo was developed incrementally across four branches, each adding a clean layer on top of the last with no rework or rebase churn.

| Branch | Commit | What it added |
|--------|--------|---------------|
| `master` | `97bd80f` | Core library, REPL demo, initial unit tests |
| `improve-unit-tests` | `88596ac` | Removed redundant tests; added wraparound-resize coverage that exposed an edge case in the linearize path |
| `feature/csv-record-delimiter` | `58beb41` | Introduced `ring_buffer_write_record()` and `RING_BUFFER_RECORD_SEP` — a structured-data layer that treats the char stream as comma-delimited records without adding a separate data structure |
| `feature/csv-dump` | `625a86e` | Added `ring_buffer_dump_csv()`, timestamped filename generation, and the `d` REPL command — completing the pipeline from streaming write to durable file export |

The progression reflects a deliberate design strategy: keep the core buffer generic (any `char` stream), then layer meaning on top via the record separator convention, then layer persistence on top of that via CSV export. Each branch is independently reviewable.

---

## Suggested Future Features

The following are ranked roughly by implementation effort, smallest first.

**1. Configurable overflow policy**
Expose drop-newest vs. drop-oldest as an enum flag stored on `RingBuffer`. The existing overwrite-on-full behavior becomes `RING_BUFFER_DROP_OLDEST`; add `RING_BUFFER_DROP_NEWEST` (reject writes when full). Low-effort — one field, one branch in `ring_buffer_write`.

**2. Non-destructive iterator**
```c
int ring_buffer_foreach(RingBuffer *rb,
                        int (*fn)(char c, void *userdata),
                        void *userdata);
```
Traverses all elements in order without consuming them. Useful for inspection, filtering, and building alternative serializers beyond CSV.

**3. Thread safety**
Add a `ring_buffer_init_mt()` variant that embeds a `pthread_mutex_t` (or a C11 `mtx_t`). The lock-based approach is straightforward here because the buffer already centralizes all state. Consider a separate `ring_buffer_mt.h` to keep the non-threaded path zero-overhead.

**4. Numeric statistics**
```c
typedef struct { uint8_t min; uint8_t max; double mean; } RingBufferStats;
int ring_buffer_stats(RingBuffer *rb, RingBufferStats *out);
```
Treats each `char` as a `uint8_t` sample. Useful for telemetry buffers carrying sensor byte streams.

**5. Generic element size**
Generalize from `char` to `void *` + `size_t element_size`. This is the largest change — it touches the struct definition, all I/O functions, and the resize path — but it would turn the library into a general-purpose ring buffer for any POD type.

**6. Named-pipe / file-descriptor I/O**
```c
int ring_buffer_from_fd(RingBuffer *rb, int fd, size_t max_bytes);
int ring_buffer_to_fd(RingBuffer *rb, int fd);
```
Direct integration with pipes, sockets, and serial ports. Natural next step for embedded or IPC use cases.

**7. Memory-mapped persistent backing**
Replace the `malloc`'d buffer with an `mmap`'d file region. The ring buffer survives process restarts — the head/tail/count metadata would need to live in a small header at the start of the mapped file.

---

## Cross-Platform Portability

### Core library — already portable

`ring_buffer.c` depends only on `<stdlib.h>`, `<stdio.h>`, and `<string.h>`. It contains no inline assembly, no SIMD, no endian assumptions, and no fixed-width integer casts. It will compile unchanged with any C99 cross-compiler targeting x86, ARM, or RISC-V.

### Demo app (`main.c`) — four POSIX-specific dependencies

| Dependency | Issue | Portable replacement |
|------------|-------|----------------------|
| `<syslog.h>` / `openlog` / `syslog` / `closelog` | Linux/macOS only; absent on Windows and bare-metal RTOS | Wrap in `#ifdef __linux__` / `#elif defined(_WIN32)` using `OutputDebugStringA`; or substitute a single-file logger such as [log.c](https://github.com/rxi/log.c) |
| `gettimeofday()` | Deprecated in POSIX.1-2008; absent on Windows | Replace with C11 `timespec_get(&ts, TIME_UTC)` for maximum portability, or `clock_gettime(CLOCK_MONOTONIC)` on POSIX, `QueryPerformanceCounter` on Win32 |
| `setenv()` | Not available on Windows | Guard with `#ifndef _WIN32`; use `_putenv_s("TZ", "CST6CDT", 1)` on MSVC |
| `localtime()` | Thread-unsafe on some platforms | Replace with `localtime_r()` on POSIX; `localtime_s()` on MSVC |

### ISA cross-compilation (ARM / RISC-V)

CMake's toolchain file mechanism handles the rest. Example for RISC-V 64-bit Linux:

```bash
cmake -S . -B build-riscv64 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/riscv64-linux-gnu.cmake \
  -DCMAKE_C_COMPILER=riscv64-linux-gnu-gcc
cmake --build build-riscv64
```

A minimal `cmake/riscv64-linux-gnu.cmake` toolchain file:

```cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR riscv64)
set(CMAKE_C_COMPILER riscv64-linux-gnu-gcc)
set(CMAKE_FIND_ROOT_PATH /usr/riscv64-linux-gnu)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
```

The same pattern works for ARM (`arm-linux-gnueabihf-gcc`) and x86-32 (`i686-linux-gnu-gcc`).

### Recommended CMake enhancement

Rather than hardcoding `#ifdef` guards in source, let CMake probe at configure time:

```cmake
include(CheckIncludeFile)
include(CheckFunctionExists)

check_include_file(syslog.h HAVE_SYSLOG_H)
check_function_exists(gettimeofday HAVE_GETTIMEOFDAY)
check_function_exists(clock_gettime HAVE_CLOCK_GETTIME)

target_compile_definitions(ring_buffer_app PRIVATE
    $<$<BOOL:${HAVE_SYSLOG_H}>:HAVE_SYSLOG_H>
    $<$<BOOL:${HAVE_GETTIMEOFDAY}>:HAVE_GETTIMEOFDAY>
    $<$<BOOL:${HAVE_CLOCK_GETTIME}>:HAVE_CLOCK_GETTIME>
)
```

`main.c` then uses `#ifdef HAVE_SYSLOG_H` instead of hardcoded platform names, and the build system becomes the single source of truth about what the target platform supports.

---

## Contributing

- Language standard: C99 (`-std=c99`).
- Indentation: 4 spaces; braces on the same line as control statements.
- All public API functions must use the `ring_buffer_` prefix.
- Use `size_t` for sizes and counts; return `-1` (int) on error.
- Every public function entry point must be defensive against `NULL` pointers and zero capacities.
- Add new tests to `ring_buffer_tests.c`; keep each `test_*` function focused on one behavior.
- Run `cmake --build build --target check` before submitting changes.
- Use clear, imperative commit messages (e.g., `Add ring_buffer_foreach iterator`).
- PRs should include a concise summary, test results, and relevant usage notes.

---

*Copyright Chris Schibi 2025*
