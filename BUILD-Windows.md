# Building GPlates and pyGPlates on Windows

This is a guide to building **GPlates** (the desktop application) or **pyGPlates** (the Python
library) from source on Windows, using [conda](https://docs.conda.io/) to install the dependencies.

> **Note:** This source tree builds *either* GPlates *or* pyGPlates (not both in a single build).
> By default GPlates is built. To build pyGPlates instead, set the CMake variable
> `GPLATES_BUILD_GPLATES` to `FALSE` (this is handled automatically by the `pip` build below).

## Contents

1. [Prerequisites](#prerequisites)
2. [Install the dependencies with conda](#install-the-dependencies-with-conda)
3. [Build GPlates](#build-gplates)
4. [Build pyGPlates](#build-pygplates)
5. [Developers](#developers)
   - [Using the Visual Studio IDE](#using-the-visual-studio-ide)
   - [Using PowerShell](#using-powershell)

## Prerequisites

- **Visual Studio 2022** (the free Community edition is fine), or the standalone *Build Tools for
  Visual Studio 2022*, with the **Desktop development with C++** workload installed. GPlates requires
  the MSVC compiler. The conda environment below activates this compiler for you (via the
  `vs2022_win-64` package) — but Visual Studio itself must already be installed.

  > Visual Studio 2019 (version 16) also works; if you use it, install `vs2019_win-64` instead of
  > `vs2022_win-64` in the environment file.

- **Miniconda** (or Anaconda / Miniforge). Download and install
  [Miniconda](https://docs.conda.io/en/latest/miniconda.html), then open the **Anaconda Prompt** from
  the Start menu to run the commands below.

  > Prefer PowerShell? See [Using PowerShell](#using-powershell) in the Developers section below.

## Install the dependencies with conda

From the root source directory (the one containing `env.Windows.yml`), create and activate an
environment containing the dependencies:

```bat
conda env create -n gplates -f env.Windows.yml
conda activate gplates
```

Alternatively, create the environment directly:

```bat
conda create -n gplates -c conda-forge cmake ninja vs2022_win-64 python numpy "libblas=*=*openblas" qt6-main qwt libboost-devel libboost-python-devel libgdal proj cgal-cpp gmp mpfr glew zlib
conda activate gplates
```

> **Qt version:** the environment installs **Qt6** (recommended). To build against Qt5 instead,
> replace `qt6-main` with `"qt-main<6"` and `qwt` with `"qwt<=6.2.0"`. Qt5 remains supported for now
> but will be removed in a future release.

Keep this environment activated for all the build commands below.

> **Note:** In a plain Command Prompt / Anaconda Prompt, `conda deactivate` may not fully restore
> `PATH` afterward (a known interaction with the `vs2022_win-64` compiler activation) — this doesn't
> affect the build itself, but if you need a clean shell afterward, close and reopen the terminal, or
> use PowerShell instead (see [Using PowerShell](#using-powershell)), where `conda deactivate` restores
> `PATH` correctly.

## Build GPlates

The commands below build *in place*, into a `build-gplates` sub-directory (this directory is ignored
by git). Run them from the root source directory.

### Configure

```bat
cmake -S . -B build-gplates -G Ninja -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_PREFIX_PATH="%CONDA_PREFIX%;%CONDA_PREFIX%\Library" ^
    -DBoost_ROOT="%CONDA_PREFIX%\Library"
```

`%CONDA_PREFIX%` points at the activated environment; conda places most Windows libraries under its
`Library` sub-directory, which is why both paths are on `CMAKE_PREFIX_PATH`.

### Compile

```bat
cmake --build build-gplates
```

This produces `gplates.exe` under `build-gplates`.

### Install (standalone)

```bat
cmake --install build-gplates --prefix <install_dir>
```

On Windows, GPlates is installed as a **standalone application**: the dependency libraries are copied
alongside `gplates.exe`, so it runs without an activated conda environment (you can double-click it).

> **Standalone numpy / BLAS note:** the embedded Python interpreter's numpy needs a BLAS/LAPACK
> backend. Use **OpenBLAS** (the `"libblas=*=*openblas"` package above), *not* MKL. OpenBLAS is a
> single self-contained DLL that is bundled automatically; MKL's runtime is very large and loads its
> compute kernels dynamically, so it cannot be bundled reliably (numpy would then fail to import from
> the standalone `gplates.exe`).

## Build pyGPlates

pyGPlates is built and installed into the active Python (conda) environment with `pip`:

```bat
python -m pip install . ^
    -C cmake.define.CMAKE_PREFIX_PATH="%CONDA_PREFIX%;%CONDA_PREFIX%\Library" ^
    -C cmake.define.Boost_ROOT="%CONDA_PREFIX%\Library"
```

A `pygplates` package should then be importable in the environment (`python -m pip list` shows
`pygplates`).

> **Developers** who prefer to build the `pygplates` target directly (rather than via `pip`) can
> configure a *separate* build tree into a `build-pygplates` sub-directory, with
> `-DGPLATES_BUILD_GPLATES=FALSE`:
>
> ```bat
> cmake -S . -B build-pygplates -G Ninja -DCMAKE_BUILD_TYPE=Release -DGPLATES_BUILD_GPLATES=FALSE ^
>     -DCMAKE_PREFIX_PATH="%CONDA_PREFIX%;%CONDA_PREFIX%\Library" ^
>     -DBoost_ROOT="%CONDA_PREFIX%\Library"
> ```
>
> then `cmake --build build-pygplates`. Using a separate build directory (rather than reusing
> `build-gplates`) avoids mixing up which tree was configured for which target.

> Python binary wheels can also be built for distribution to other computers — see
> `pygplates/wheel/README.md`.

## Developers

### Using the Visual Studio IDE

The Ninja build above does not create a Visual Studio solution. If you want to work in the Visual
Studio IDE, configure a *separate* build tree with the Visual Studio generator:

```bat
cmake -S . -B build-gplates-vs -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_PREFIX_PATH="%CONDA_PREFIX%;%CONDA_PREFIX%\Library" ^
    -DBoost_ROOT="%CONDA_PREFIX%\Library"
```

This produces `GPlates.sln` (or `PyGPlates.sln` when configured with `-DGPLATES_BUILD_GPLATES=FALSE`)
under `build-gplates-vs`. Open it **from an activated conda environment** so that Visual Studio finds
the correct dependency DLLs (via the environment's `PATH`) when it runs GPlates:

```bat
devenv build-gplates-vs\GPlates.sln
```

### Using PowerShell

The commands on this page are written for the Anaconda Prompt (Command Prompt). If you'd rather
build from PowerShell, open **"Anaconda Powershell Prompt"** from the Start menu, then activate the
conda environment inside it:

```powershell
conda activate gplates
```

This gives you both the MSVC compiler and the conda dependencies in the same shell.

> There's no need for a Visual-Studio-specific shell (e.g. "Developer PowerShell for VS 2022"). That
> shell exists to run `vcvarsall.bat`, which sets up the compiler environment (`PATH`, plus `INCLUDE` and
> `LIB` so the compiler and linker can find system headers and libraries). The `vs2022_win-64` package
> activated above does this itself as part of `conda activate gplates` — that's why it's in the
> environment (see Prerequisites) — so a separate Developer shell would just repeat it. The Visual Studio
> generator / `devenv` path (see [Using the Visual Studio IDE](#using-the-visual-studio-ide) above)
> doesn't need any of this in the first place: Visual Studio resolves its own compiler toolset
> internally, regardless of `PATH`; the conda environment there is only needed for the dependency DLLs,
> so `devenv` can run/debug `gplates.exe`.

The build commands are otherwise the same, with two PowerShell-specific changes: use a backtick `` ` ``
instead of `^` to continue a command onto the next line, and `$env:CONDA_PREFIX` instead of `%CONDA_PREFIX%`
to refer to the activated environment. For example, the GPlates configure step becomes:

```powershell
cmake -S . -B build-gplates -G Ninja -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_PREFIX_PATH="$env:CONDA_PREFIX;$env:CONDA_PREFIX\Library" `
    -DBoost_ROOT="$env:CONDA_PREFIX\Library"
```

> **Careful:** `%CONDA_PREFIX%` is Command Prompt syntax. PowerShell won't error on it — it just
> passes the literal text through unexpanded — so CMake silently fails to find the environment instead
> of giving an obvious error.
