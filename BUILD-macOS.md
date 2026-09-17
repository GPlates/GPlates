# Building GPlates and pyGPlates on macOS

This is a guide to building **GPlates** (the desktop application) or **pyGPlates** (the Python
library) from source on macOS, using [conda](https://docs.conda.io/) to install the dependencies.

> **Note:** This source tree builds *either* GPlates *or* pyGPlates (not both in a single build).
> By default GPlates is built. To build pyGPlates instead, set the CMake variable
> `GPLATES_BUILD_GPLATES` to `FALSE` (this is handled automatically by the `pip` build below).

## Contents

1. [Prerequisites](#prerequisites)
2. [Install the dependencies with conda](#install-the-dependencies-with-conda)
3. [Build GPlates](#build-gplates)
4. [Build pyGPlates](#build-pygplates)
5. [Run the tests](#run-the-tests)
   - [GPlates](#gplates)
   - [pyGPlates](#pygplates)
   - [Reading the result](#reading-the-result)
6. [Code intelligence (clangd)](#code-intelligence-clangd)

## Prerequisites

- **Xcode command-line tools** (they provide the macOS SDK and system frameworks):

  ```bash
  xcode-select --install
  ```

- **Miniconda** (or Anaconda / Miniforge). Download and install
  [Miniconda](https://docs.conda.io/en/latest/miniconda.html), then open a terminal to run the
  commands below.

## Install the dependencies with conda

From the root source directory (the one containing `env.macOS.yml`), create and activate an
environment containing the dependencies:

```bash
conda env create -n gplates -f env.macOS.yml
conda activate gplates
```

Alternatively, create the environment directly:

```bash
conda create -n gplates -c conda-forge cmake ninja cxx-compiler python numpy "libblas=*=*openblas" qt6-main qwt libboost-devel libboost-python-devel libgdal proj cgal-cpp gmp mpfr glew zlib gtest
conda activate gplates
```

> **Qt version:** the environment installs **Qt6** (recommended). To build against Qt5 instead,
> replace `qt6-main` with `"qt-main<6"` and `qwt` with `"qwt<=6.2.0"`. Qt5 remains supported for now
> but will be removed in a future release.

Keep this environment activated for all the build commands below. While it is activated, CMake
auto-detects the conda environment: it adds the environment to the dependency search path and prefers
conda's libraries over similarly-named macOS system frameworks. So you do **not** need to pass
`-DCMAKE_PREFIX_PATH=$CONDA_PREFIX` or `-DCMAKE_FIND_FRAMEWORK=LAST`.

## Build GPlates

The commands below build *in place*, into a `build-gplates` sub-directory (this directory is ignored
by git). Run them from the root source directory.

### Configure

```bash
cmake -S . -B build-gplates -G Ninja -DCMAKE_BUILD_TYPE=Release
```

### Compile

```bash
cmake --build build-gplates
```

This produces the application bundle `gplates.app` under `build-gplates/bin` (it appears as
`gplates` in Finder). You can run it from the command line with
`build-gplates/bin/gplates.app/Contents/MacOS/gplates`.

To check the build, run the unit tests — see [Run the tests](#run-the-tests).

### Install (standalone)

```bash
cmake --install build-gplates --prefix <install_dir>
```

On macOS, GPlates is installed as a **standalone bundle**: the dependency libraries are copied inside
`gplates.app`, so it runs without an activated conda environment.

> **Standalone numpy / BLAS note:** the embedded Python interpreter's numpy needs a BLAS/LAPACK
> backend. Use **OpenBLAS** (the `"libblas=*=*openblas"` package above), *not* MKL — OpenBLAS is
> bundled into the app automatically (along with its numpy extension modules' other native
> dependencies), whereas MKL cannot be bundled reliably.

> **Note (conda Python):** conda's Python is *not* packaged as a `Python.framework`. The standalone
> build handles this: the Python standard library is bundled under
> `gplates.app/Contents/Resources/lib/pythonX.Y` and the embedded interpreter is given its home
> explicitly (a framework Python, eg MacPorts, is bundled under `Contents/Frameworks` and locates its
> home via dyld instead). A non-framework stdlib must live under `Contents/Resources`, not
> `Contents/Frameworks` — macOS bundle code-signing rejects a plain directory tree placed in the
> reserved `Contents/Frameworks`. No extra steps are required.

## Build pyGPlates

pyGPlates is built and installed into the active Python (conda) environment with `pip`:

```bash
python -m pip install .
```

A `pygplates` package should then be importable in the environment (`python -m pip list` shows
`pygplates`).

> **Developers** who prefer to build the `pygplates` target directly (rather than via `pip`) can
> configure a *separate* build tree into a `build-pygplates` sub-directory, with
> `-DGPLATES_BUILD_GPLATES=FALSE`:
>
> ```bash
> cmake -S . -B build-pygplates -G Ninja -DCMAKE_BUILD_TYPE=Release -DGPLATES_BUILD_GPLATES=FALSE
> ```
>
> then `cmake --build build-pygplates`. Using a separate build directory (rather than reusing
> `build-gplates`) avoids mixing up which tree was configured for which target. This tree is also
> what the pyGPlates tests run from — see [Run the tests](#run-the-tests).

> Python binary wheels can also be built for distribution to other computers — see
> `pygplates/wheel/README.md`.

## Run the tests

Both products have a test suite, run with
[CTest](https://cmake.org/cmake/help/latest/manual/ctest.1.html) from the build tree, which is the
quickest way to confirm that a from-source build works. Two rules apply to every test run:

- **Configure Release, and pass `-C Release` to `ctest`.** Several tests exercise error paths that
  throw in a Release build but abort in a Debug build, so a Debug test run is unsupported. The
  pyGPlates tests are registered for Release (and MinSizeRel) only, and `-C Release` is what selects
  them: `ctest` does not infer the configuration from the build tree, so without it they are
  silently skipped — even on the single-configuration Ninja tree used above (see
  [Reading the result](#reading-the-result)). The GPlates tests carry no such restriction; a Debug
  tree is left without any of them instead (see [GPlates](#gplates) below).
- **Run `ctest` from the activated conda environment** you built in, so that the test executables
  find the dependency libraries.

### GPlates

The unit tests are a separate executable, `gplates-unit-test`, which a plain `cmake --build` does
not build. Build it by name, then run the tests:

```bash
cmake --build build-gplates --target gplates-unit-test
ctest --test-dir build-gplates -C Release --output-on-failure
```

Each test case is registered with CTest individually, so `ctest -R <pattern>` runs a subset and
`ctest -j <N>` runs them in parallel.

> The `gplates-unit-test` target only exists if CMake found GoogleTest when the tree was configured
> (the `gtest` conda package, in the dependency list above). Without it there are no GPlates tests
> at all: the configure output says `Warning: GoogleTest (gtest) not found so GPlates unit-test
> executable gplates-unit-test will not be available` (the message is suppressed when building a
> release version), and the build command above fails with an unknown target. Install the package
> and re-configure.

> A **Debug** tree registers no GPlates tests, whatever `-C` value you pass — re-configure it as
> Release.

### pyGPlates

The pyGPlates tests run against a **build tree**, not against the package that `pip install .`
installs (`pip` builds in a fresh temporary directory). So they need the separate `build-pygplates`
tree from the Developers note under [Build pyGPlates](#build-pygplates), configured with
`-DGPLATES_BUILD_GPLATES=FALSE`. Build it, then run the tests:

```bash
cmake --build build-pygplates
ctest --test-dir build-pygplates -C Release --output-on-failure
```

This runs the Python test suite (`pygplates-test`) against the module just built, plus three checks
that the built module is what the repository says it should be: `pygplates-stub-test` (the
committed type stub `pygplates/stub/__init__.pyi` still matches the module), and
`pygplates-source-closure-test` and `pygplates-linkage-test` (the module was compiled from, and
links against, only what it should).

### Reading the result

One test, `version-resolver-test`, belongs to neither product and is registered by every tree for
every configuration (it checks the version resolver's own logic, and needs nothing built). So if
`ctest` reports **only that test** — `100% tests passed, 0 tests failed out of 1` — nothing else
was selected, and the run says nothing about your build:

- **pyGPlates tree:** `-C Release` was omitted.
- **GPlates tree:** the tree was configured Debug, or GoogleTest was not found when it was
  configured (see the notes under [GPlates](#gplates)).

A GPlates tree whose `gplates-unit-test` target has *not been built* is louder: `ctest` reports a
single *failing* placeholder test, `gplates-unit-test_NOT_BUILT`. Build the target and run `ctest`
again.

## Code intelligence (clangd)

Editors and IDEs that use [clangd](https://clangd.llvm.org/) (including the Claude Code `clangd-lsp`
plugin) provide code navigation, diagnostics and completion. clangd needs a `compile_commands.json`
compilation database, which the Ninja build generates automatically:
`CMAKE_EXPORT_COMPILE_COMMANDS` is enabled by default (override with
`-DCMAKE_EXPORT_COMPILE_COMMANDS=OFF`). Configuring a build tree *also* writes a `.clangd` file at the
repo root that points clangd at that tree, so once you have configured `build-gplates` (above) there
is nothing else to set up.

- **clangd binary:** the Xcode command-line tools already provide `clangd` (`/usr/bin/clangd`). For a
  newer version, `brew install llvm` and add `"$(brew --prefix llvm)/bin"` to your `PATH`. Either way
  it ends up on `PATH` system-wide, which is what the `clangd-lsp` plugin needs - it launches `clangd`
  from `PATH`, with no conda environment necessarily activated.
- **Which build tree:** `.clangd` is generated (from the committed `.clangd.in`) every time you
  configure, and points at the tree you configured *most recently* - so it follows
  `GPLATES_BUILD_GPLATES` by itself. Configure `build-pygplates` with `-DGPLATES_BUILD_GPLATES=FALSE`
  and clangd switches to the pyGPlates compile commands; re-configure `build-gplates` to switch back.
  Re-configuring an already-configured tree is enough (`cmake -S . -B build-pygplates`, a second or
  two), and the generated file names the target it was written for, so its header tells you which one
  is currently active. Only build trees *inside* the source directory are used, so a `pip install .`
  (or `conda build`) - which configures into a temporary directory - never clobbers your setting.
- The database lives inside the (git-ignored) build directory and is refreshed on each
  configure/build. The path recorded in `.clangd` is relative, so it works in any git worktree.
- `.clangd` is git-ignored because it is generated. To hand-maintain your own instead, configure with
  `-DGPLATES_WRITE_CLANGD_CONFIG=FALSE` and CMake will leave the file alone.
