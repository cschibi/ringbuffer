# Repository Guidelines

## Project Structure & Module Organization
- `main.c` contains a small demo/entry point for the ring buffer.
- `ring_buffer.c` and `ring_buffer.h` implement the core library.
- `ring_buffer_tests.c` holds the unit tests.
- `CMakeLists.txt` defines the build and test targets.
- `build/` is the out-of-tree build directory (generated).

## Build, Test, and Development Commands
- Configure: `cmake -S . -B build` creates build files in `build/`.
- Build: `cmake --build build` compiles `ring_buffer_app` and tests.
- Run app: `./build/ring_buffer_app` executes the demo binary.
- Run tests: `ctest --test-dir build` runs the CTest suite.
- Convenience test target: `cmake --build build --target check` builds and runs tests with failure output.

## Coding Style & Naming Conventions
- Language standard is C99 (see `CMakeLists.txt`).
- Indentation uses 4 spaces; keep braces on the same line as control statements.
- Use `ring_buffer_` prefix for public functions (e.g., `ring_buffer_init`).
- Prefer `size_t` for sizes and counts; return `-1` on error as in existing APIs.
- No formatter/linter is configured; keep changes consistent with current style.

## Testing Guidelines
- Tests are plain C compiled into `ring_buffer_tests` and run via CTest.
- Add new tests to `ring_buffer_tests.c` and keep them focused on one behavior.
- Run `ctest --test-dir build` locally before submitting changes.

## Commit & Pull Request Guidelines
- Git history is not available in this workspace, so no commit convention is enforced here.
- Use clear, imperative commit messages (e.g., “Fix ring buffer wraparound”).
- PRs should include: a concise summary, test results, and any relevant usage notes.

## Security & Configuration Tips
- Avoid introducing dynamic allocations without clear ownership rules.
- Keep APIs defensive against `NULL` pointers and zero capacities, consistent with current checks.
