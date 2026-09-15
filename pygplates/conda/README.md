# Building the conda package

pyGPlates is distributed on conda-forge as well as PyPI. This directory holds the conda recipe. It
is built here as it stands, and it is copied by hand into
[conda-forge/pygplates-feedstock](https://github.com/conda-forge/pygplates-feedstock) at each
release - the only route to conda-forge, since no CI here pushes to the feedstock. The differences
between those two uses are most of what this README records.

The pieces fit together like this:

| Piece | What it does |
|---|---|
| `meta.yaml` (this directory) | The recipe: version, source, build/host/run dependencies and the test block. Copied to the feedstock with the version and source block swapped for the PyPI form - see [Releasing](#releasing). |
| `build.sh` / `bld.bat` (this directory) | The build scripts, one per platform. Both just run `pip install` and let scikit-build-core drive CMake. Copied to the feedstock as they are. |
| `conda_build_config.yaml` (this directory) | **Local builds only** - the variants conda-forge re-renders for itself. Never copied. |
| The root `pyproject.toml` | The scikit-build-core configuration that actually builds the module. Its `[tool.cibuildwheel]` half is for the wheels and is not used here. |
| `cmake/check_linkage.py`, `pygplates/test/test.py` | Run by the recipe's test block, from the sdist. Both are shared with the wheel builds. |

Wheels are a separate pipeline with its own README: `pygplates/wheel/README.md`.

## Building the recipe locally

conda-build must be installed in the `base` environment (`conda install -n base conda-build`), and
`conda build` is run from the root of the source tree. No environment needs to be activated -
conda-build creates its own build and host environments from the recipe:

```
conda build pygplates/conda --python 3.13
```

Without `--python` it builds every interpreter in `conda_build_config.yaml`, one package each.

Two things to know before the first run:

- **Commit your changes first.** `source:` is `git_url: ../../`, which clones the checkout - so
  conda-build sees committed files only, exactly the file set the sdist ships to the feedstock.
  Uncommitted work is silently not built. (`path: ../../`, the obvious alternative, copies the
  *entire* working tree including every in-source `build*/` directory - tens of gigabytes.)
- **Export the version before building.** `meta.yaml` reads `PYGPLATES_PEP440_VERSION` from the
  environment, and fails the render if it is unset (rather than building a package called
  version "None"). conda-build renders the recipe with Jinja, which cannot run the resolver
  itself, so:

  ```
  export PYGPLATES_PEP440_VERSION=$(cmake -P cmake/modules/VersionFromGit.cmake pygplates)
  ```

  Only this one: it names the *package*, and conda-build clones this repository (tags and
  all) so the build resolves both product versions from git for itself. The feedstock form
  has no repository, and reads both from the `cmake/modules/VersionRecorded.cmake` that
  `cmake/version.py` writes into the sdist.

  `build.sh`/`bld.bat` then re-export conda-build's own `PKG_VERSION` into the build, so the
  module's version equals the recipe version by construction. That is also what makes the
  feedstock form work untouched: it builds from a PyPI sdist with no repository, and the
  literal version in the feedstock recipe becomes the module's version with no git involved.

The package lands in `<conda-root>/conda-bld/<subdir>/pygplates-<version>-<build string>.conda`,
and conda-build prints the path. To try it:

```
conda install -c local pygplates
```

To check what the recipe *renders* to without building it - a minute rather than an hour, and
enough to catch a Jinja or selector mistake, a wrong pin, or a dependency that should not be there:

```
conda render pygplates/conda --python 3.13
```

Two things about that output are expected and not worth chasing:

- `WARNING: Number of parsed outputs does not match detected raw metadata blocks` - conda-build
  26.5 emits this even with the recipe's Jinja and comments stripped out. The recipe has a single
  output and no `outputs:` section.
- Rendering another platform (`--variants "{target_platform: linux-64}"`) does not give a
  faithful Linux render. The selectors in `conda_build_config.yaml` are evaluated against the
  *build* platform, so from Windows you get Windows' `c_stdlib` and compilers with a Linux target -
  which then cannot solve. Build in Docker instead (below).

On Windows, conda-build activates Visual Studio 2022 itself, so a plain `cmd` or PowerShell with
`base` on `PATH` is all that is needed - but Visual Studio 2022 must be installed, and Git for
Windows must be on `PATH` for the `git_url` clone.

On Linux the recipe can be built without touching the host machine at all - and this is how to
check the Linux dependency list from a Windows or macOS machine:

```
docker run --rm -e CPU_COUNT=6 -v /path/to/gplates:/src:ro condaforge/miniforge3 \
    bash -c "git config --global --add safe.directory '*' && \
             conda install -y conda-build git && \
             conda build /src/pygplates/conda --python 3.13"
```

`CPU_COUNT` is what `build.sh` passes to CMake as `CMAKE_BUILD_PARALLEL_LEVEL`, and conda-build
takes it from the environment if it is set. Cap it. Left alone it becomes the container's CPU
count, and pyGPlates' Boost and CGAL template instantiation needs on the order of a gigabyte or two
per compiler process - so on a large machine the build is killed part-way through with nothing but
`error waiting for container: unexpected EOF` to explain itself. The `safe.directory` line is
there because the mounted repository is owned by someone other than the container's root, and git
refuses to clone from it otherwise.

## What conda-build needs that conda-forge supplies for itself

`conda_build_config.yaml` exists only because a local `conda build` has no conda-forge pinning
feedstock behind it. The feedstock re-renders its own variants from
[conda-forge-pinning](https://github.com/conda-forge/conda-forge-pinning-feedstock), so this file
is never copied there. Each key prevents a specific failure:

- **`numpy: 2`** - conda-build's built-in default is NumPy 1.26 for Python >= 3.12. The
  `numpy>=2.0` build requirement in `pyproject.toml` does not save you, because conda-build runs
  pip *without build isolation*: the host environment's NumPy is the one used. A module built
  against 1.x then refuses to import under the NumPy 2 that the run dependency resolves to, so the
  package fails its own test block - after the whole build has been paid for.
- **`c_stdlib` / `c_stdlib_version`** - without them `{{ stdlib("c") }}` renders to a dependency on
  a nonexistent `c_win-64` (or `c_linux-64`) and the build fails to solve. With them, `meta.yaml`
  can carry the `stdlib` line uncommented and identical to the feedstock's, rather than having a
  commented-out line that someone has to remember to enable.
- **`c_compiler` / `cxx_compiler`** - Windows only. conda-build defaults to vs2017 there: its
  default table has a vs2022 entry, but the lookup clamps every Python >= 3.5 to the 3.5 row, so
  that entry is unreachable. A local build would then ask for a toolchain that is probably not
  installed, and would not be the compiler the released package is built with. Linux and macOS are
  left alone - conda-build's `gcc`/`gxx` and `clang`/`clangxx` already match conda-forge. That is
  possible because a key whose values are all excluded by selectors is dropped, leaving
  conda-build's own default in place.
- **`python`** - the interpreters to build. conda-forge's `python_min` is currently 3.11.

The values track conda-forge's pinning as of September 2026; refresh them when it moves.

## The conda-forge feedstock

The feedstock's `recipe/` holds `meta.yaml`, `build.sh`, `bld.bat` and a `patches/` directory. The
three files are copied from here; the rest of the feedstock is generated by `conda smithy rerender`
from `conda-forge.yml` and the global pinning.

What the feedstock does for itself:

- **Versions and pins.** The autotick bot raises pull requests for the version, the sha256, the
  build number and the migrated pins. Do not hand-maintain those in this copy.
- **Variants.** Python, NumPy, compilers, `c_stdlib` and the rest, re-rendered from
  conda-forge-pinning.
- **Patches.** `source:` may carry patches against the released sdist for problems fixed upstream
  after the release. The current two (a GDAL 3.13 compile error and a Boost 1.89 CMake error) are
  fixes already in this tree, and are dropped at the next release.

What is worth knowing about its configuration (`conda-forge.yml`), because it constrains this
recipe:

- **`error_overlinking: true`** - a library the module links but the recipe does not name in `run`
  is an error, not a warning. This is why the `run:` section names things that host `run_exports`
  would supply anyway: each is absorbed by the pinned run_export of the same name, so it costs
  nothing, and the failure it prevents happens on someone else's machine. `zlib` is the exception
  and is left out: the module links libz/zlib1, which ships in `libzlib`, so host `zlib`'s
  run_export already pins the right package and naming `zlib` in `run` would only add a second,
  unpinned dependency on the development package.
- **Cross builds.** `linux-aarch64`, `linux-ppc64le` and `osx-arm64` are each built on a different
  architecture from the one they target - `conda-forge.yml` maps `osx_arm64` to `osx_64` and the two
  Linux targets to `linux_64` - and tested `native_and_emulated`. That is what the `python` /
  `cross-python_{{ target_platform }}` / `numpy` entries in `build:` are for; see
  [Cross builds run the host interpreter](#cross-builds-run-the-host-interpreter), which is where
  those entries turn out to be load-bearing.

Pre-releases go to the feedstock's `rc` branch, not `main`, and are published under the
`pygplates_rc` label so that `conda install pygplates` does not pick them up. Use it for anything
where the feedstock build is the first real test of a change.

## Why the recipe is the way it is

These are the reasons behind the non-obvious parts of the recipe, each learned from a failure.

### Cross builds run the host interpreter

Half the platforms conda-forge publishes are cross builds, and on those the build still has to
*run* "the host Python": `build.sh` invokes `$PYTHON -m pip install`, and `src/CMakeLists.txt` asks
that same interpreter for the standard library location, for `sys.prefix`, and for the NumPy include
directory. Each of the three is a `FATAL_ERROR` if it fails - the NumPy one only when building
pyGPlates, which is all this recipe ever does.

It has to be the host interpreter rather than the one `find_package(Python3)` turns up, because the
paths taken from it decide where the module is installed. Picking up the build environment's Python
installs into the build prefix and the package then comes out empty - conda-build says exactly that
("Empty package; python present in build and host deps...") - which is why `src/CMakeLists.txt`
uses `$ENV{PYTHON}` under `CONDA_BUILD`; the comment there records the error in full.

What makes that work on a cross build is `cross-python_{{ target_platform }}` in `build:`, and it is
load-bearing rather than boilerplate. Its activation script *replaces* `$PREFIX/bin/python` - the
target-architecture binary - with a build-architecture shim that execs a `crossenv` wrapper built
against the host's `_sysconfigdata_*.py`. So `$PYTHON` runs as a native process while reporting the
host prefix's paths, which is the combination the build needs, and nothing foreign is executed.
`python` and `numpy` in `build:` are what that wrapper imports: the crossenv puts the *build*
prefix's site-packages on the path, since the host's compiled extensions cannot be loaded, so
`import numpy; numpy.get_include()` resolves to build-prefix headers - architecture-independent, and
the same version as the host's because one variant pins both.

Emulation does appear, but in the test phase rather than the build: that activation script skips
itself when `CONDA_BUILD_STATE` is `TEST`, which is where conda-forge's `native_and_emulated`
setting takes over.

One CMake detail belongs here rather than in `src/CMakeLists.txt` alone, because these three
platforms are the only place it applies: on them, and only on them, the `Interpreter` and
`Development` components of Python are searched in two *separate* `find_package()` calls. Policy
CMP0190 (CMake 4.1) makes asking for both in one call a hard error whenever `CMAKE_CROSSCOMPILING`
is true and `CMAKE_CROSSCOMPILING_EMULATOR` is not set - which is exactly what conda-forge's
compiler activation arranges - so a combined call fails to *configure* on every cross build.
pyGPlates 1.0.0 shipped only because its `cmake_minimum_required` range stopped at 3.31, before the
policy existed. Every other platform keeps the combined call, because that is what makes CMake look
the headers and library up *from* the interpreter and so keeps the two consistent - a guarantee a
cross build cannot have anyway. CMP0190 is also why NumPy is asked for by running the interpreter
rather than as a `find_package()` component: on CMake 4.1 that component implies both of the others,
and the error comes straight back.

### Qt Core only - no Qt Gui, GLEW, Qwt, OpenGL or X11

The pygplates module links Qt Core and no other Qt module (`src/CMakeLists.txt`). GLEW, Qwt and
OpenGL are found and linked only when `GPLATES_BUILD_GPLATES` is on, which for this recipe it never
is. Every GL, GLU, EGL, X11, `libselinux` and Qwt entry that this recipe used to carry - seventeen
of them in `build:` alone - was dead weight, pulling packages into the build that the module cannot
reference.

The guard is the test block: `check_linkage.py --installed` fails if a Qt Gui, Qt Widgets, Qt Svg,
Qwt, GLEW or OpenGL library shows up among the built module's direct dependencies. If a change
reintroduces one, the package fails to build rather than shipping a module that needs a display
server. On Linux the module's direct dependencies are exactly ten: `libQt6Core`, `libgdal`,
`libboost_python`, `libgmp`, `libz`, and the five that come with the C and C++ runtimes.

What that does *not* mean is a small environment. `qt6-main` is the whole of Qt in one package, so
installing pyGPlates from conda-forge still pulls Qt's own X11 and OpenGL dependencies in as conda
packages, and they appear in the build environment here too. That is a packaging constraint, not a
linkage one - conda-forge has no Qt Core-only package to depend on - and it is the one place where
the conda package cannot follow the wheels, which vendor `libQt6Core` alone.

### `scikit-build-core >=0.11.1,<2` in `host`

conda-build runs pip without build isolation, so the backend that builds the module is the one in
the host environment - this pin, not the `[build-system] requires` bound in `pyproject.toml`. The
two must be kept in sync, and `pyproject.toml` is where the reasoning for the bounds lives. Note
which way the enforcement runs: a floor raised there fails loudly here through `minimum-version`,
but an upper bound there is not enforced in a conda build at all. If the cap is not carried across,
the scikit-build-core major that completes the deprecated-table removal is picked up the day it
releases.

### NumPy 2 in `host`, and `numpy` in `run`

pyGPlates uses NumPy only for scalar support, but `src/CMakeLists.txt` requires it: if
`numpy.get_include()` fails, or the directory it names holds no `numpy/arrayobject.h`, configure
stops with a `FATAL_ERROR`. It used to warn and carry on, which was the worse outcome by far - the
module built, imported and passed its tests, and only rejected a `numpy.float64` argument later, in
someone else's script.

`numpy` therefore stays in `host`: it is the copy a native build compiles against, and the one whose
run export pins the runtime bound. Be clear about how far the new error reaches, though - on the
cross builds the probe is answered by the *build* prefix's numpy (see [Cross builds run the host
interpreter](#cross-builds-run-the-host-interpreter)), so dropping `numpy` from `host` would still
configure and build there. On those three platforms `host` is about the pin, not the probe.

The version matters as much as the presence: see `numpy: 2` above. Building against NumPy 2 gives a
`>=1.2x,<3` run export, so no `pin_compatible` is needed - but `numpy` is still named explicitly in
`run` because the module imports it at run time.

### `cgal-cpp` is host-only; `gmp` and `mpfr` are there for it

CGAL is header-only and has no `run_exports`, so it belongs in `host` and nowhere else. GMP and
MPFR are never found or linked directly by the build - they reach the compile line only through
CGAL's `CGAL::CGAL` target - but their own `run_exports` cover whatever ends up needed at run time.

### `libgdal`, not `libgdal-core`

`libgdal` is the metapackage; its `run_exports` bring in the plugin drivers (netCDF among them)
that `libgdal-core` alone does not. pyGPlates reads formats that live in those drivers, so the
smaller package would build and test green and fail on a user's file.

### No `CMAKE_PREFIX_PATH` or `Boost_ROOT` in the build scripts

`conda build` sets `CONDA_BUILD=1`, and the root `CMakeLists.txt` detects that: it prepends
`$PREFIX` (plus `Library` under it on Windows) to `CMAKE_PREFIX_PATH`, points `Boost_ROOT` there,
and on macOS sets `CMAKE_FIND_FRAMEWORK=LAST` so conda's libraries win over system frameworks.
`ConfigDefault.cmake` also forces `GPLATES_INSTALL_STANDALONE` off for conda builds, so nothing is
bundled into the package. The feedstock's current copies of these scripts (superseded at the next
release) still pass some of this explicitly. That is redundant at best; at worst it is how a build
ends up with a Boost outside conda - the one an inherited `BOOST_ROOT` or `PATH` pointed at.

### NMake, single CPU, on Windows

`bld.bat` replaces the Visual Studio generator that conda-build's vs2022 activation selects with
`NMake Makefiles`, and sets `GPLATES_MSVC_PARALLEL_BUILD=FALSE`. Both are for memory: the parallel
Visual Studio build died with `C1060: compiler is out of heap space` on conda-forge's 7 GB Windows
runners. NMake compiles one translation unit at a time, which is slow and fits.

Before any of that, `bld.bat` makes sure vcvars has run, because NMake - unlike the Visual
Studio generator - cannot locate the toolchain by itself. The call is guarded by
`if not defined VSCMD_VER` rather than made unconditionally, as the feedstock's current script
does. conda-build's vs2022 activation already runs `vcvars64.bat`, and running vcvars a second
time appends the whole toolchain to `PATH` again: on a machine whose `PATH` is already long that
overflows cmd's 8191-character limit and the script dies having printed only `The input line is
too long`, with no indication of what failed. There is no real fallback to keep behind that guard,
either: `VSINSTALLDIR` is set by the same activation script that sets `VSCMD_VER`, so when the guard
fires there is nothing left to call. The block says that and exits, rather than expanding an empty
variable into a relative path and failing with `The system cannot find the path specified`.

The obvious improvement is Ninja, which could parallelise within the memory budget: add
`ninja  # [win]` to `build:` and set `CMAKE_C_COMPILER`/`CMAKE_CXX_COMPILER` to `cl` (Ninja makes
CMake search `PATH`, which on a machine with Visual Studio finds the bundled LLVM instead - the
same trap `pyproject.toml` documents for the wheels). It is untested on the feedstock's runners, so
prove it on the `rc` branch before it reaches a release.

## Testing

The test block runs against the *installed* package, in a fresh environment built from the `run`
dependencies - so it tests what a user gets, not what the build tree holds.

- `python -X faulthandler pygplates/test/test.py` - the pyGPlates test suite, the same one the
  wheels run (`test-command` in `pyproject.toml`). `-X faulthandler` prints a Python stack when the
  native module crashes, instead of a bare exit code. The suite fails unless at least 400
  tests ran (`MINIMUM_EXPECTED_TESTS`), which catches a suite that silently collected nothing. It
  writes temporary files into its own `fixtures/` directory - fine here, because `source_files`
  are copied into a writable directory before the commands run.
- `python cmake/check_linkage.py --installed` - the linkage guard described above. It imports
  `pygplates.pygplates` to find the module, then lists its direct dependencies with the platform's
  native tool. On Linux that is `readelf`, which is why `binutils` is a test requirement:
  conda-forge's package installs an unprefixed `readelf` into the environment. On macOS `otool`
  comes with the system.

The linkage check is skipped on Windows, where the tool is `dumpbin`, which is not on `PATH` in a
conda test environment. The script fails loudly rather than silently passing when its lister is
missing, so it cannot simply be run and ignored. Windows linkage is covered by the wheel CI, which
runs the same check inside an MSVC environment.

## Releasing

The conda-forge package is built from the PyPI sdist, so **release to PyPI first** - see
"Publishing a release" in `pygplates/wheel/README.md`. Then, in a checkout of the feedstock:

1. Copy `meta.yaml`, `build.sh` and `bld.bat` from this directory into `recipe/`. Do **not** copy
   `conda_build_config.yaml` or this README.
2. In `meta.yaml`, swap both halves of the local form for the feedstock form; the file marks them
   "FEEDSTOCK form, half one" and "half two". Half one replaces the `load_file_regex` block with a
   literal `set version` line and **must stay above `package:`** - Jinja runs `set` statements in
   document order, so a version set below its use renders blank and the recipe reaches conda-forge
   with no version, while the `url:` below it still interpolates and hides the mistake. Half two
   replaces `source: git_url:` with the PyPI `url` and `sha256`. Uncomment by removing the leading
   `# `.
3. Fill in the sha256 of the sdist on PyPI (the file's page lists it; `openssl dgst -sha256` on a
   download does too).
4. Set `build: number: 0`. It is only bumped for a rebuild of an unchanged version.
5. Drop any `patches/` whose fixes the new release contains, along with their `source: patches:`
   entries. They were made against the *previous* sdist, and a patch that no longer applies fails
   the build.
6. `conda smithy rerender` (or let the bot do it in the pull request).
7. Open the pull request against `main`, or against the `rc` branch for a pre-release.

The feedstock build is the first time the recipe runs across every platform, Python version and
cross target at once. Build the recipe locally on at least
Windows and Linux before opening that pull request; the Windows build script in particular has no
other coverage.

## References

- conda-forge's [knowledge base](https://conda-forge.org/docs/maintainer/knowledge_base/) -
  `run_exports`, `stdlib("c")`, cross-compiling, overlinking and pre-release labels.
- conda-build's [variant configuration](https://docs.conda.io/projects/conda-build/en/stable/resources/variants.html)
  and [defining metadata](https://docs.conda.io/projects/conda-build/en/stable/resources/define-metadata.html)
  (`load_file_regex` and the Jinja context).
- [scikit-build-core configuration](https://scikit-build-core.readthedocs.io/en/latest/configuration/index.html) -
  the `config-settings` the build scripts pass.
- The feedstock: [conda-forge/pygplates-feedstock](https://github.com/conda-forge/pygplates-feedstock).
- [conda-forge/qt-main-feedstock](https://github.com/conda-forge/qt-main-feedstock) - what
  `qt6-main` is and what it exports.
