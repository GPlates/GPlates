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
   - [Choosing a terminal](#choosing-a-terminal)
   - [Using the Visual Studio IDE](#using-the-visual-studio-ide)
   - [Using PowerShell](#using-powershell)
6. [Code intelligence (clangd)](#code-intelligence-clangd)

## Prerequisites

- **Visual Studio 2022** (the free Community edition is fine), or the standalone *Build Tools for
  Visual Studio 2022*, with the **Desktop development with C++** workload installed. GPlates requires
  the MSVC compiler. In the **Anaconda Prompt** (Command Prompt), the conda environment below activates
  this compiler for you (via the `vs2022_win-64` package); from PowerShell you use a Visual Studio
  developer shell instead (see [Choosing a terminal](#choosing-a-terminal)). Either way, Visual Studio
  itself must already be installed.

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
> affect the build itself, but if you need a clean shell afterward, just close and reopen the terminal.

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

### Choosing a terminal

The build commands on this page assume the **Anaconda Prompt** (Command Prompt) — the default below.
Which terminal you use matters on Windows, because the MSVC compiler is activated differently in each:

| Terminal | Ninja build (needs MSVC on `PATH`) | `devenv` available |
|---|---|---|
| Anaconda Prompt (cmd) — *the documented default* | ✅ conda `vs2022_win-64` sets up MSVC | ✅ |
| Plain PowerShell / Anaconda PowerShell Prompt | ❌ conda can't propagate the MSVC env | ❌ |
| Developer PowerShell for VS 2022 | ✅ VS sets up MSVC (+ conda deps via `activate`) | ✅ |
| Developer Command Prompt for VS 2022 | ✅ VS sets up MSVC (+ conda deps) | ✅ |

In short: use the **Anaconda Prompt**, or — if you prefer PowerShell — **"Developer PowerShell for VS
2022"** (see [Using PowerShell](#using-powershell)). A *plain* PowerShell can't configure a Ninja build
at all. In every working terminal, activate the conda environment (`conda activate gplates`) so the
dependency libraries are on `PATH`.

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

> `devenv` is on `PATH` in the Anaconda Prompt and in "Developer PowerShell for VS 2022", but *not* in
> a plain PowerShell (see [Choosing a terminal](#choosing-a-terminal)).

### Using PowerShell

The commands on this page are written for the Anaconda Prompt (Command Prompt). If you'd rather
build from PowerShell, open **"Developer PowerShell for VS 2022"** from the Start menu, then activate
the conda environment inside it:

```powershell
conda activate gplates
```

> **Why the Developer shell, and not a plain PowerShell** (see the table in [Choosing a
> terminal](#choosing-a-terminal)): the `vs2022_win-64` package sets up the MSVC compiler by running the
> equivalent of `vcvarsall.bat`. In Command Prompt that runs directly and works, but in PowerShell
> `conda activate` runs it in a nested `cmd.exe` subprocess whose environment changes never make it back
> — so a plain PowerShell (including "Anaconda Powershell Prompt") can't find the compiler and CMake
> fails with "No CMAKE_C_COMPILER could be found". "Developer PowerShell for VS 2022" sets up the
> compiler itself, independent of conda; `conda activate gplates` on top of that still adds the conda
> dependency paths correctly, since — unlike the compiler — most conda packages (Qt, GDAL, PROJ, etc.)
> activate the same way in every shell.

This gives you both the MSVC compiler and the conda dependencies in the same shell. The build commands
are otherwise the same, with two PowerShell-specific changes: use a backtick `` ` `` instead of `^` to
continue a command onto the next line, and `$env:CONDA_PREFIX` instead of `%CONDA_PREFIX%` to refer to
the activated environment. For example, the GPlates configure step becomes:

```powershell
cmake -S . -B build-gplates -G Ninja -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_PREFIX_PATH="$env:CONDA_PREFIX;$env:CONDA_PREFIX\Library" `
    -DBoost_ROOT="$env:CONDA_PREFIX\Library"
```

> **Careful:** `%CONDA_PREFIX%` is Command Prompt syntax. PowerShell won't error on it — it just
> passes the literal text through unexpanded — so CMake silently fails to find the environment instead
> of giving an obvious error.

Since "Developer PowerShell for VS 2022" already puts `devenv` on `PATH`, the [Using the Visual Studio
IDE](#using-the-visual-studio-ide) commands above work unchanged in this shell too.

## Code intelligence (clangd)

Editors and IDEs that use [clangd](https://clangd.llvm.org/) (including the Claude Code `clangd-lsp`
plugin) provide code navigation, diagnostics and completion. clangd needs a `compile_commands.json`
compilation database, which the **Ninja** build generates automatically:
`CMAKE_EXPORT_COMPILE_COMMANDS` is enabled by default (override with
`-DCMAKE_EXPORT_COMPILE_COMMANDS=OFF`). The committed `.clangd` at the repo root points clangd at the
`build-gplates` tree, so once you have configured the Ninja `build-gplates` (above) there is nothing
else to set up. clangd understands the MSVC (`cl.exe`) command lines that CMake records.

- **clangd binary:** install LLVM — `winget install LLVM.LLVM` (then add its `bin` to `PATH`) or
  download from the [LLVM releases](https://github.com/llvm/llvm-project/releases). Or from
  conda-forge: `conda install clangdev`.
- The database lives inside the (git-ignored) `build-gplates` directory and is refreshed on each
  configure/build. It is portable across git worktrees (the `.clangd` path is relative).
- To edit against the pyGPlates tree instead, change `CompilationDatabase` in `.clangd` to
  `build-pygplates`.

> **Visual Studio generator caveat:** the optional `build-gplates-vs` tree
> ([Using the Visual Studio IDE](#using-the-visual-studio-ide)) uses the "Visual Studio 17 2022"
> generator, which does **not** emit `compile_commands.json` — the flag above only affects the Ninja
> (and Makefile) generators. That is fine: keep a Ninja `build-gplates` configured for clangd, and use
> Visual Studio's own IntelliSense inside the IDE.
