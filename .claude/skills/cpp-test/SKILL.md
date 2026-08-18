---
name: cpp-test
description: Conventions for adding or modifying C++ unit tests in GPlates (GoogleTest, src/unit-test/). Use when asked to write a C++ test, add coverage for C++ code, or review an existing test.
---

# Adding a C++ test

Authoritative reference: @doc-cpp/design/testing/README.md. This skill covers the mechanics and
the rules that are easy to violate.

## Mechanics

1. Create or extend a file in `src/unit-test/` using `TEST()` / `TEST_F()`.
2. Add the file to the `srcs` list in `src/unit-test/CMakeLists.txt`.
3. Rebuild `gplates-unit-test` — per-case discovery via `gtest_discover_tests()` is automatic:

```
cmake --build build-gplates --target gplates-unit-test
ctest --test-dir build-gplates -C Release --output-on-failure
```

GoogleTest comes from the conda environment (`gtest` in the root `env.*.yml`). It is optional in
CMake — without it the test target simply does not exist — but CI builds the target explicitly,
so treat it as mandatory.

## Rules

- **GoogleTest only.** It is the framework for all new C++ tests, replacing Boost.Test. Do not
  mix frameworks within an executable.
- **Release only.** `GPlatesGlobal::Assert` calls `std::abort()` in Debug rather than throwing,
  which kills any test exercising an error path. C++ tests are not discovered in Debug build
  trees at all, so always run `ctest -C Release`. A run that reports zero tests is a broken
  invocation, not a pass.
- **Headless.** The test `main()` defaults `QT_QPA_PLATFORM` to `offscreen` and provides a
  `QApplication`, so `QSettings`-dependent code (`ApplicationState`, `UserPreferences`) and
  widget-level code *are* testable — but nothing may require a real display, a GPU, or user
  interaction.
- **Working-directory independent, and never write into the source tree.** Read input data from
  `src/unit-test/data/` via the `GPLATES_UNIT_TEST_DATA_DIR` compile definition (an absolute path
  baked in by CMake), e.g. `GPLATES_UNIT_TEST_DATA_DIR "/my_input.gpml"`. Write any output to a
  `QTemporaryDir`. **A test run must leave `git status` clean** — check this before reporting done.
- **Diagnosable from CI logs alone.** Failure detail goes to stdout/stderr, never only to a log
  file. Use `SCOPED_TRACE` to disambiguate assertions inside loops or shared helpers.
- **CTest stays the only top-level runner.** Do not add a second one (e.g. pytest wrapping ctest);
  other runners nest *under* CTest, as `pygplates-test` does.

## Scope guidance

The historical suite went stale because no `QCoreApplication` existed, making app-adjacent code
untestable. That constraint is gone (see `src/gplates_unit_test_main.cc`), so the thinnest
coverage areas — file-io first, then app-logic — no longer have a structural excuse.
