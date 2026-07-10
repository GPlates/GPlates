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
conda create -n gplates -c conda-forge cmake ninja make cxx-compiler python numpy "libblas=*=*openblas" qt6-main qwt libboost-devel libboost-python-devel libgdal proj cgal-cpp gmp mpfr glew zlib
conda activate gplates
```

> **Qt version:** the conda environment installs **Qt6** (recommended). To build against Qt5 instead,
> replace `qt6-main` with `"qt-main<6"` and `qwt` with `"qwt<=6.2.0"`. Qt5 remains supported for now
> but will be removed in a future release.

Keep this environment activated for the build commands below, and pass `-DCMAKE_PREFIX_PATH=$CONDA_PREFIX`
when configuring (shown below).

### Option B: Ubuntu system packages

The dependencies are also available as Ubuntu packages. The minimum supported release is
**Ubuntu 22.04 (Jammy)**; the commands below have been written for 22.04 and later. This route uses
**Qt5** (Ubuntu does not currently package Qwt for Qt6); use the conda option above for a Qt6 build.

```bash
sudo apt-get update
sudo apt-get install \
    cmake ninja-build g++ \
    libgl1-mesa-dev libglu1-mesa-dev libglew-dev \
    python3-dev python3-numpy \
    libboost-dev libboost-python-dev libboost-thread-dev libboost-program-options-dev libboost-test-dev \
    libqt5opengl5-dev libqt5svg5-dev libqt5xmlpatterns5-dev libqwt-qt5-dev \
    libgdal-dev libcgal-dev libproj-dev zlib1g-dev
```

You can check whether a particular package is installed with `dpkg-query -l <package>` (a leading
`ii` means installed). Similar packages are available on other distributions (Fedora, Debian, etc.).

When configuring with the system packages, omit `CMAKE_PREFIX_PATH` (CMake finds the packages in the
standard system locations).

## Build GPlates

The commands below build *in place*, into a `build-gplates` sub-directory (this directory is ignored
by git). Run them from the root source directory.

### Configure

Using conda dependencies:

```bash
cmake -S . -B build-gplates -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$CONDA_PREFIX
```

Using Ubuntu system packages (no `CMAKE_PREFIX_PATH`):

```bash
cmake -S . -B build-gplates -G Ninja -DCMAKE_BUILD_TYPE=Release
```

> You can ignore the CMake warning that `CGAL_DATA_DIR cannot be deduced`.

### Compile

```bash
cmake --build build-gplates
```

This produces the `gplates` executable under `build-gplates/bin`.

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
cmake -S . -B build-gplates -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$CONDA_PREFIX \
    -DGPLATES_INSTALL_STANDALONE=TRUE
```

then compile and install as above. This copies the conda dependency libraries alongside the installed
`gplates` executable, so it runs without an activated conda environment.

> **Standalone numpy / BLAS note:** the embedded Python interpreter's numpy needs a BLAS/LAPACK
> backend. Use **OpenBLAS** (the `"libblas=*=*openblas"` package above), *not* MKL. OpenBLAS is a
> single self-contained shared library that is bundled automatically; MKL's runtime is very large and
> loads its compute kernels dynamically, so it cannot be bundled reliably.

## Build pyGPlates

pyGPlates is built and installed into the active Python environment with `pip`. From the root source
directory:

```bash
# conda dependencies:
python -m pip install . -C cmake.define.CMAKE_PREFIX_PATH=$CONDA_PREFIX

# Ubuntu system packages (no CMAKE_PREFIX_PATH needed):
python -m pip install .
```

A `pygplates` package should then be importable in the environment (`python -m pip list` shows
`pygplates`).

> **Developers** who prefer to build the `pygplates` target directly (rather than via `pip`) can
> configure a *separate* build tree into a `build-pygplates` sub-directory, with
> `-DGPLATES_BUILD_GPLATES=FALSE`:
>
> ```bash
> # conda dependencies:
> cmake -S . -B build-pygplates -G Ninja -DCMAKE_BUILD_TYPE=Release -DGPLATES_BUILD_GPLATES=FALSE \
>     -DCMAKE_PREFIX_PATH=$CONDA_PREFIX
>
> # Ubuntu system packages (no CMAKE_PREFIX_PATH needed):
> cmake -S . -B build-pygplates -G Ninja -DCMAKE_BUILD_TYPE=Release -DGPLATES_BUILD_GPLATES=FALSE
> ```
>
> then `cmake --build build-pygplates`. Using a separate build directory (rather than reusing
> `build-gplates`) avoids mixing up which tree was configured for which target.

> Python binary wheels can also be built for distribution to other computers — see
> `pygplates/wheel/README.md`. These are manylinux wheels compatible with a broad range of Linux
> distributions.
