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
conda create -n gplates -c conda-forge cmake ninja cxx-compiler python numpy "libblas=*=*openblas" qt6-main qwt libboost-devel libboost-python-devel libgdal proj cgal-cpp gmp mpfr glew zlib
conda activate gplates
```

> **Qt version:** the environment installs **Qt6** (recommended). To build against Qt5 instead,
> replace `qt6-main` with `"qt-main<6"` and `qwt` with `"qwt<=6.2.0"`. Qt5 remains supported for now
> but will be removed in a future release.

Keep this environment activated for all the build commands below.

## Build GPlates

The commands below build *in place*, into a `build-gplates` sub-directory (this directory is ignored
by git). Run them from the root source directory.

### Configure

```bash
cmake -S . -B build-gplates -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH=$CONDA_PREFIX \
    -DCMAKE_FIND_FRAMEWORK=LAST
```

`CMAKE_FIND_FRAMEWORK=LAST` tells CMake to prefer the conda libraries over any similarly-named macOS
system frameworks.

### Compile

```bash
cmake --build build-gplates
```

This produces the application bundle `gplates.app` under `build-gplates/bin` (it appears as
`gplates` in Finder). You can run it from the command line with
`build-gplates/bin/gplates.app/Contents/MacOS/gplates`.

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

> **Note (conda Python):** installing a standalone bundle requires a build in which the macOS
> Python-framework requirement has been relaxed — conda's Python is *not* packaged as a
> `Python.framework`. If `cmake --install` reports that Python must be a framework, that relaxation
> still needs to be applied (see `cmake/modules/Install.cmake`).

## Build pyGPlates

pyGPlates is built and installed into the active Python (conda) environment with `pip`:

```bash
python -m pip install . \
    -C cmake.define.CMAKE_PREFIX_PATH=$CONDA_PREFIX \
    -C cmake.define.CMAKE_FIND_FRAMEWORK=LAST
```

A `pygplates` package should then be importable in the environment (`python -m pip list` shows
`pygplates`).

> **Developers** who prefer to build the `pygplates` target directly (rather than via `pip`) can
> configure a *separate* build tree into a `build-pygplates` sub-directory, with
> `-DGPLATES_BUILD_GPLATES=FALSE`:
>
> ```bash
> cmake -S . -B build-pygplates -G Ninja -DCMAKE_BUILD_TYPE=Release -DGPLATES_BUILD_GPLATES=FALSE \
>     -DCMAKE_PREFIX_PATH=$CONDA_PREFIX \
>     -DCMAKE_FIND_FRAMEWORK=LAST
> ```
>
> then `cmake --build build-pygplates`. Using a separate build directory (rather than reusing
> `build-gplates`) avoids mixing up which tree was configured for which target.

> Python binary wheels can also be built for distribution to other computers — see
> `pygplates/wheel/README.md`.
