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
   - [Switching between conda environments](#switching-between-conda-environments)
   - [Using PowerShell](#using-powershell)
   - [Using the Visual Studio IDE](#using-the-visual-studio-ide)
6. [Code intelligence (clangd)](#code-intelligence-clangd)

## Prerequisites

- **Visual Studio 2022** (the free Community edition is fine), or the standalone *Build Tools for
  Visual Studio 2022*, with the **Desktop development with C++** workload installed. GPlates requires
  the MSVC compiler.

  > You don't need to set the compiler up yourself — activating the conda environment below does that
  > for you (via the `vs2022_win-64` package). Visual Studio itself must already be installed, though.

- **Miniconda** (or Anaconda / Miniforge). Download and install
  [Miniconda](https://docs.conda.io/en/latest/miniconda.html).

Then open the **Anaconda Prompt** from the Start menu to run the commands below.

> Prefer PowerShell? See [Choosing a terminal](#choosing-a-terminal) in the Developers section below.

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
> replace `qt6-main` with `"qt-main<6"` and `qwt` with `"qwt<=6.2.0"`. Qt5 also works with Visual
> Studio 2019, if you'd rather use that — install `vs2019_win-64` instead of `vs2022_win-64` (Qt6
> requires Visual Studio 2022). Qt5 remains supported for now but will be removed in a future release.

Keep this environment activated for all the build commands below. While it is activated, CMake
auto-detects the conda environment and adds both `%CONDA_PREFIX%` and its `%CONDA_PREFIX%\Library`
sub-directory to the dependency search path, so you do **not** need to pass `-DCMAKE_PREFIX_PATH`.

> **Note:** In a Command Prompt (including the Anaconda Prompt), `conda deactivate` does not restore
> `PATH` — the `vs2022_win-64` compiler activation leaves the environment's directories on it. This
> doesn't affect building, but it matters if you switch to another environment in the same shell (a
> Qt5 one, say): see [Switching between conda environments](#switching-between-conda-environments).

## Build GPlates

The commands below build *in place*, into a `build-gplates` sub-directory (this directory is ignored
by git). Run them from the root source directory.

### Configure

```bat
cmake -S . -B build-gplates -G Ninja -DCMAKE_BUILD_TYPE=Release
```

The activated conda environment is auto-detected (see above), so neither `-DCMAKE_PREFIX_PATH` nor
`-DBoost_ROOT` is needed — Boost, like the other dependencies, is found in the conda environment. This
holds even if you still have a `BOOST_ROOT` *environment* variable left over from a from-source build:
the auto-detection points CMake's Boost search at the conda environment, so it no longer interferes
with conda builds.

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
python -m pip install .
```

A `pygplates` package should then be importable in the environment (`python -m pip list` shows
`pygplates`).

> **Developers** who prefer to build the `pygplates` target directly (rather than via `pip`) can
> configure a *separate* build tree into a `build-pygplates` sub-directory, with
> `-DGPLATES_BUILD_GPLATES=FALSE`:
>
> ```bat
> cmake -S . -B build-pygplates -G Ninja -DCMAKE_BUILD_TYPE=Release -DGPLATES_BUILD_GPLATES=FALSE
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

| Terminal | Ninja build (needs MSVC on `PATH`) | `devenv` available | `conda deactivate` restores `PATH` |
|---|---|---|---|
| Anaconda Prompt (cmd) — *the documented default* | ✅ conda `vs2022_win-64` sets up MSVC | ✅ | ❌ |
| Plain Command Prompt | ✅ conda `vs2022_win-64` sets up MSVC | ✅ | ❌ |
| Plain PowerShell / Anaconda PowerShell Prompt | ❌ conda can't propagate the MSVC env | ❌ | ✅ |
| Developer PowerShell for VS 2022 | ✅ VS sets up MSVC | ✅ | ✅ |
| Developer Command Prompt for VS 2022 | ✅ VS sets up MSVC | ✅ | ❌ |

In short: use the **Anaconda Prompt**, or — if you prefer PowerShell — **"Developer PowerShell for VS
2022"** (see [Using PowerShell](#using-powershell)). A *plain* PowerShell can't configure a Ninja build
at all. In every working terminal, activate the conda environment (`conda activate gplates`) so the
dependency libraries are on `PATH`; outside the Anaconda Prompt this first needs conda hooked into the
shell (`conda init cmd.exe` or `conda init powershell`).

The last column only matters if you switch between conda environments in one shell — see
[Switching between conda environments](#switching-between-conda-environments).

### Switching between conda environments

If you keep more than one environment — a Qt6 `gplates` and a Qt5 one, say — **open a fresh
terminal for each**, rather than `conda deactivate` / `conda activate` in the same shell. In any
Command Prompt the two environments' directories end up mixed on `PATH`, and the one activated
*first* wins.

The cause is the `vs2022_win-64` package. Its activation script
(`etc\conda\activate.d\vs2022_compiler_vars.bat`) prepends `%CONDA_PREFIX%`,
`%CONDA_PREFIX%\Library\bin` etc. to `PATH` after running `vcvarsall.bat`, and ships no deactivation
script, so conda — which only undoes the `PATH` entries it added itself — leaves them behind on
`conda deactivate`, and `conda activate <other>` then prepends the other environment's directories in
front of the leftovers. Compiling is unaffected (CMake locates the dependencies through
`CONDA_PREFIX`, which conda does update), but anything that *runs* from that shell resolves
same-named DLLs through `PATH`: a Qt5 `gplates.exe` or `ctest` run with a Qt6 environment's
`Library\bin` still ahead on `PATH` picks up that environment's `qwt.dll` (Qwt 6.3 instead of 6.2) and
dies at startup with "The procedure entry point ??0QwtPointSeriesData@@ ... could not be located".
The symptom looks like a broken build; it is a stale shell.

PowerShell terminals are immune, because conda runs only `.ps1` activation scripts there and the
package ships only the `.bat` — which is also why a plain PowerShell never sees the compiler. Even
so, one terminal per environment is the simplest rule and is what these instructions assume. This is a
Windows-only quirk of the `vs2022_win-64` activation script; macOS and Linux are not affected.

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
on this page work unchanged in PowerShell. If you do add your own multi-line flags, note two
PowerShell-specific differences: use a backtick `` ` `` instead of `^` to continue a command onto the
next line, and `$env:CONDA_PREFIX` instead of `%CONDA_PREFIX%` to refer to the activated environment
(PowerShell won't error on `%CONDA_PREFIX%` — it passes the literal text through unexpanded).

Since "Developer PowerShell for VS 2022" already puts `devenv` on `PATH`, the [Using the Visual Studio
IDE](#using-the-visual-studio-ide) commands below work unchanged in this shell too.

### Using the Visual Studio IDE

The Ninja build above does not create a Visual Studio solution. If you want to work in the Visual
Studio IDE, configure a *separate* build tree with the Visual Studio generator:

```bat
cmake -S . -B build-gplates-vs -G "Visual Studio 17 2022" -A x64
```

This produces `GPlates.sln` (or `PyGPlates.sln` when configured with `-DGPLATES_BUILD_GPLATES=FALSE`)
under `build-gplates-vs`. Open it **from an activated conda environment** so that Visual Studio finds
the correct dependency DLLs (via the environment's `PATH`) when it runs GPlates:

```bat
devenv build-gplates-vs\GPlates.sln
```

> `devenv` is on `PATH` in the Anaconda Prompt and in "Developer PowerShell for VS 2022", but *not* in
> a plain PowerShell (see [Choosing a terminal](#choosing-a-terminal)).

## Code intelligence (clangd)

Editors and IDEs that use [clangd](https://clangd.llvm.org/) (including the Claude Code `clangd-lsp`
plugin) provide code navigation, diagnostics and completion. clangd needs a `compile_commands.json`
compilation database, which the **Ninja** build generates automatically:
`CMAKE_EXPORT_COMPILE_COMMANDS` is enabled by default (override with
`-DCMAKE_EXPORT_COMPILE_COMMANDS=OFF`). Configuring a build tree *also* writes a `.clangd` file at the
repo root that points clangd at that tree, so once you have configured `build-gplates` (above) there
is nothing else to set up. clangd understands the MSVC (`cl.exe`) command lines that CMake records,
and locates the Visual Studio and Windows SDK headers itself (it does not need a developer command
prompt).

- **clangd binary:** `winget install LLVM.clangd` installs the standalone clangd (a ~50 MB download)
  and adds it to your `PATH` - restart your terminal, and your editor, afterwards. Use
  `winget install LLVM.LLVM` instead if you also want the rest of the LLVM tools (clang-tidy,
  clang-format); it is a much larger download. Prefer either of these to installing clangd into the
  conda environment: the `clangd-lsp` plugin launches `clangd` from `PATH`, so a binary that lives in
  the `gplates` environment is only found when that environment happens to be activated.
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

> **Visual Studio generator caveat:** the optional `build-gplates-vs` tree
> ([Using the Visual Studio IDE](#using-the-visual-studio-ide)) uses the "Visual Studio 17 2022"
> generator, which does **not** emit `compile_commands.json` - the flag above only affects the Ninja
> (and Makefile) generators. Configuring it therefore leaves `.clangd` pointing at your last Ninja
> tree rather than at a tree with no database. Keep a Ninja `build-gplates` configured for clangd, and
> use Visual Studio's own IntelliSense inside the IDE.
