---
name: build-gplates
description: Configure, build and test the GPlates desktop application. Use when asked to build GPlates, rebuild after C++ changes, or run the GPlates unit tests (gplates-unit-test).
---

# Build and test GPlates

Build directory is `build-gplates/` unless the user names another one. This builds the Qt desktop
application, not the pyGPlates module — use `/build-pygplates` for that.

## 1. Check the environment

The conda environment named `gplates` must be active — check `CONDA_PREFIX`. If it is not, tell
the user to activate it rather than trying to activate it yourself in a non-interactive shell:

```
conda activate gplates
```

**On Windows this must be an Anaconda Prompt (cmd) or a Developer PowerShell for VS 2022.**
In plain PowerShell, `conda activate` runs the `vs2022_win-64` vcvars script in a nested `cmd`
and the compiler environment never propagates back, so CMake fails with
"No CMAKE_C_COMPILER could be found".

## 2. Configure

```
cmake -S . -B build-gplates -G Ninja -DCMAKE_BUILD_TYPE=Release -DGPLATES_BUILD_GPLATES=TRUE
```

`GPLATES_BUILD_GPLATES` defaults to `TRUE`, but pass it explicitly — this repo builds both
products and the flag makes the intent unambiguous in the shell history and in `CMakeCache.txt`.

Do not add `-DCMAKE_PREFIX_PATH` or `-DBoost_ROOT`. Confirm the configure output contains
`Detected active conda environment`; if it does not, the environment was not active.

Skip configure only if `build-gplates/CMakeCache.txt` already has **both**
`GPLATES_BUILD_GPLATES=TRUE` and `CMAKE_BUILD_TYPE=Release`. A Debug cache must be
reconfigured — see step 4.

## 3. Build

The unit-test binary is `EXCLUDE_FROM_ALL`, so a plain `cmake --build` will not produce it. Name
both targets when you intend to run tests:

```
cmake --build build-gplates --target gplates gplates-unit-test
```

For an application-only build, `cmake --build build-gplates` is fine.

## 4. Test

```
ctest --test-dir build-gplates -C Release --output-on-failure
```

**The build tree must have been configured `Release`.** Unlike the pyGPlates tests, the GPlates
tests are registered by `gtest_discover_tests()`, which cannot attach `CONFIGURATIONS`; they are
skipped at *configure* time instead, by `if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")` in
`src/CMakeLists.txt`. So a Debug tree has no registered tests and **no `-C` value will reveal
any** — reconfigure as Release.

Pass `-C Release` regardless: it is required for the multi-config generators (Visual Studio,
Xcode), and harmless on a single-config Release tree.

If the run reports 0 tests, check `CMAKE_BUILD_TYPE` in `build-gplates/CMakeCache.txt` before
anything else — do not simply retry the command.

Tests are GoogleTest cases and run headless. On failure, CI uploads
`build-gplates/Testing/Temporary/`; locally, read that directory for the detailed log.

## 5. Report

Report the actual test counts and any failures with their output.
