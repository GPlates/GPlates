# Building GPlates and pyGPlates on Linux

This is a guide to building **GPlates** (the desktop application) or **pyGPlates** (the Python
library) from source on Linux. The dependencies can be installed with [conda](https://docs.conda.io/)
(recommended, and consistent with the other platforms) or, on Ubuntu, with the system package
manager.

> **Note:** This source tree builds *either* GPlates *or* pyGPlates (not both in a single build).
> By default GPlates is built. To build pyGPlates instead, set the CMake variable
> `GPLATES_BUILD_GPLATES` to `FALSE` (this is handled automatically by the `pip` build below).

## Contents

1. [Install the dependencies](#install-the-dependencies)
   - [Option A: conda (recommended)](#option-a-conda-recommended)
   - [Option B: Ubuntu system packages](#option-b-ubuntu-system-packages)
2. [Build GPlates](#build-gplates)
3. [Build pyGPlates](#build-pygplates)
4. [Run the tests](#run-the-tests)
   - [GPlates](#gplates)
   - [pyGPlates](#pygplates)
   - [Reading the result](#reading-the-result)
5. [Code intelligence (clangd)](#code-intelligence-clangd)

## Install the dependencies

### Option A: conda (recommended)

Install [Miniconda](https://docs.conda.io/en/latest/miniconda.html) (or Anaconda / Miniforge). Then,
from the root source directory (the one containing `env.Linux.yml`), create and activate an
environment containing the dependencies:

```bash
conda env create -n gplates -f env.Linux.yml
conda activate gplates
```

Alternatively, create the environment directly (see `env.Linux.yml` for the OpenGL/X11 development
packages that are also required):

```bash
conda create -n gplates -c conda-forge cmake ninja make cxx-compiler patchelf python numpy "libblas=*=*openblas" qt6-main qwt libboost-devel libboost-python-devel libgdal proj cgal-cpp gmp mpfr glew zlib gtest
conda activate gplates
```

> **Qt version:** the conda environment installs **Qt6** (recommended). To build against Qt5 instead,
> replace `qt6-main` with `"qt-main<6"` and `qwt` with `"qwt<=6.2.0"`. Qt5 remains supported for now
> but will be removed in a future release.

Keep this environment activated for the build commands below. While it is activated, CMake auto-detects
the conda environment and adds it to the dependency search path, so you do **not** need to pass
`-DCMAKE_PREFIX_PATH=$CONDA_PREFIX`.

### Option B: Ubuntu system packages

The dependencies are also available as Ubuntu packages. The minimum supported release is
**Ubuntu 22.04 (Jammy)**; the commands below have been written for 22.04 and later. This route uses
**Qt5** (Ubuntu does not currently package Qwt for Qt6); use the conda option above for a Qt6 build.
Qt5 remains supported for now but will be removed in a future release, at which point this apt route
will need Qt6 packages.

```bash
sudo apt-get update
sudo apt-get install \
    cmake ninja-build g++ patchelf \
    libgl1-mesa-dev libglu1-mesa-dev libglew-dev \
    python3-dev python3-numpy python3-pip \
    libboost-dev libboost-python-dev libboost-thread-dev libboost-program-options-dev \
    libqt5opengl5-dev libqt5svg5-dev libqwt-qt5-dev \
    libgdal-dev libcgal-dev libproj-dev proj-bin zlib1g-dev \
    libgtest-dev
```

You can check whether a particular package is installed with `dpkg-query -l <package>` (a leading
`ii` means installed). Similar packages are available on other distributions (Fedora, Debian, etc.).

When configuring with the system packages, omit `CMAKE_PREFIX_PATH` (CMake finds the packages in the
standard system locations).

## Build GPlates

The commands below build *in place*, into a `build-gplates` sub-directory (this directory is ignored
by git). Run them from the root source directory.

### Configure

```bash
cmake -S . -B build-gplates -G Ninja -DCMAKE_BUILD_TYPE=Release
```

> The same command works for both dependency routes: an activated conda environment is auto-detected
> (its prefix is added to CMake's search path), and Ubuntu system packages are found in the standard
> system locations.

> You can ignore the CMake warning that `CGAL_DATA_DIR cannot be deduced`.

> **WSL note (Ubuntu system packages):** WSL inherits the Windows `PATH`, so CMake can pick up a Windows
> dependency build (e.g. under `/mnt/c/SDK/...`) instead of the Ubuntu system libraries — usually seen at
> [Compile](#compile) as a `DSO missing from command line` linker error naming a `/mnt/c/...` library.
> Prevent it by disabling Windows `PATH` inheritance: add this to `/etc/wsl.conf`, then run
> `wsl --shutdown` (from Windows) and reopen the shell.
>
> ```ini
> [interop]
> appendWindowsPath = false
> ```

### Compile

```bash
cmake --build build-gplates
```

This produces the `gplates` executable under `build-gplates/bin`.

To check the build, run the unit tests — see [Run the tests](#run-the-tests).

> If the link step fails with `DSO missing from command line` referencing a `/mnt/c/...` library, see the
> WSL note under [Configure](#configure).

### Install

By default on Linux, unlike Windows and macOS, GPlates is **not** installed as a self-contained
bundle — it is installed into a prefix and uses the dependency libraries already on the system (or in
the activated conda environment).

Install into the default `/usr/local` prefix (requires root):

```bash
sudo cmake --install build-gplates
```

Or install into a location you own by setting the prefix (no root required):

```bash
cmake --install build-gplates --prefix $HOME/usr
```

> IMPORTANT: If you installed the dependencies with conda, the installed `gplates` links against the
> conda libraries — it will *not* run as-is outside that environment. Either:
> - run it from the activated environment (`conda activate gplates`), or
> - build it as a standalone bundle instead (see immediately below), so it runs without conda activated.

If you'd rather not activate conda every time you run GPlates, you can instead build a **standalone**
bundle (as on Windows and macOS) by adding `-DGPLATES_INSTALL_STANDALONE=TRUE` to the configure step:

```bash
cmake -S . -B build-gplates -G Ninja -DCMAKE_BUILD_TYPE=Release -DGPLATES_INSTALL_STANDALONE=TRUE
```

then compile and install as above. This copies the conda dependency libraries alongside the installed
`gplates` executable, so it runs without an activated conda environment.

> **Standalone bundles are for the conda route only.** With the [Ubuntu system packages](#option-b-ubuntu-system-packages)
> the dependencies are always present system-wide, so a plain install already runs anywhere on the
> machine. Bundling also can't fully work here: Debian/Ubuntu split Python packages across two locations
> (numpy lives in `dist-packages`, outside the standard library the bundle copies), whereas conda keeps
> everything under one prefix. So to package GPlates for another machine (e.g. with CPack), use conda.

> **Standalone numpy / BLAS note:** the embedded Python interpreter's numpy needs a BLAS/LAPACK
> backend. Use **OpenBLAS** (the `"libblas=*=*openblas"` package above), *not* MKL. OpenBLAS is a
> single self-contained shared library that is bundled automatically; MKL's runtime is very large and
> loads its compute kernels dynamically, so it cannot be bundled reliably.

## Build pyGPlates

pyGPlates is built and installed into the active Python environment with `pip`. From the root source
directory:

```bash
python3 -m pip install .
```

> This works for both dependency routes: an activated conda environment is auto-detected, and Ubuntu
> system packages are found in the standard system locations.

> `pip install .` copies pyGPlates' dependency libraries into the installed package and sets their
> RPATHs so they find each other there, which needs `patchelf` (included in both dependency lists
> above). Without it the configure step stops with "Unable to find 'patchelf' command".

A `pygplates` package should then be importable in the environment (`python3 -m pip list` shows
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
> `pygplates/wheel/README.md`. These are manylinux wheels compatible with a broad range of Linux
> distributions.

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
  find the dependency libraries. (With the Ubuntu system packages there is nothing to activate.)

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
> (the `gtest` conda package, or `libgtest-dev` on Ubuntu — both are in the dependency lists
> above). Without it there are no GPlates tests at all: the configure output says `Warning:
> GoogleTest (gtest) not found so GPlates unit-test executable gplates-unit-test will not be
> available` (the message is suppressed when building a release version), and the build command
> above fails with an unknown target. Install the package and re-configure.

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

- **clangd binary:** install it via your package manager - Ubuntu/Debian `sudo apt install clangd`,
  Fedora `sudo dnf install clang-tools-extra`, Arch `sudo pacman -S clang`. Prefer this to installing
  clangd into the conda environment: the `clangd-lsp` plugin launches `clangd` from `PATH`, so a
  binary that lives in the `gplates` environment is only found when that environment happens to be
  activated.
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
