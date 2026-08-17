---
name: build-pygplates
description: Configure, build and test pyGPlates. Use when asked to build pyGPlates, rebuild after C++/Python API changes, or run the pyGPlates tests (pygplates-test, pygplates-stub-test).
---

# Build and test pyGPlates

Build directory is `build-pygplates/` unless the user names another one.

## 1. Check the environment

The conda environment named `gplates` must be active — check `CONDA_PREFIX`. If it is not,
tell the user to activate it rather than trying to activate it yourself in a non-interactive
shell (the activation does not survive):

```
conda activate gplates
```

**On Windows this must be an Anaconda Prompt (cmd) or a Developer PowerShell for VS 2022.**
In plain PowerShell, `conda activate` runs the `vs2022_win-64` vcvars script in a nested `cmd`
and the compiler environment never propagates back, so CMake fails with
"No CMAKE_C_COMPILER could be found". If you see that error, this is the cause.

If the environment does not exist yet: `conda env create -n gplates -f env.Windows.yml`
(or `env.macOS.yml` / `env.Linux.yml`).

## 2. Configure

```
cmake -S . -B build-pygplates -G Ninja -DCMAKE_BUILD_TYPE=Release -DGPLATES_BUILD_GPLATES=FALSE
```

- `-DGPLATES_BUILD_GPLATES=FALSE` is **required**. The option defaults to `TRUE`, so omitting it
  silently builds the GPlates desktop application instead.
- Do not add `-DCMAKE_PREFIX_PATH` or `-DBoost_ROOT`; the root `CMakeLists.txt` finds the conda
  environment itself.
- Confirm the configure output contains `Detected active conda environment`. If it does not, the
  environment was not active — stop and fix that before building.
- Skip configure if `build-pygplates/CMakeCache.txt` already exists and its
  `GPLATES_BUILD_GPLATES` entry is `FALSE`.

## 3. Build

```
cmake --build build-pygplates
```

## 4. Test

```
ctest --test-dir build-pygplates -C Release --output-on-failure
```

**`-C Release` is mandatory.** The tests are registered `CONFIGURATIONS Release MinSizeRel`, so
without it CTest matches nothing and exits successfully having run zero tests. If the output says
"No tests were found" or reports 0 tests, treat that as a failure of the command, not a pass.

Two tests should run: `pygplates-test` and `pygplates-stub-test`.

## 5. Report

Report the real outcome, including the test counts. If `pygplates-stub-test` fails, the committed
`__init__.pyi` is stale relative to the docstrings — its failure output prints the exact
regeneration command; surface that to the user rather than guessing at a fix.
