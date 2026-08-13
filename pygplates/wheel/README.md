# Building Wheels

PyGPlates wheels are built with [cibuildwheel](https://cibuildwheel.pypa.io/) - both in CI and
locally. Cibuildwheel drives the whole per-wheel pipeline: build pyGPlates for each Python
version, repair the wheel (copy the dependency shared libraries into it and give them unique
names - `auditwheel` on Linux, `delocate` on macOS, `delvewheel` on Windows) and run the
pyGPlates test suite against the repaired wheel.

The pieces fit together like this:

| Piece | What it does |
|---|---|
| `[tool.cibuildwheel]` in the root `pyproject.toml` | The wheel configuration: Python versions, dependency images, hooks, CMake defines, test command. |
| `versions.sh` (this directory) | The dependency version pins - the single source of truth shared by the Linux image and the macOS dependency build. |
| `manylinux_2_28.dockerfile` (this directory) | Docker image with the pyGPlates dependency libraries, that the Linux wheels are built inside. |
| `build_macos_deps.sh` (this directory) | Builds the pyGPlates dependency libraries on macOS (run automatically by cibuildwheel, once per machine; cached in CI). |
| `build_boost_python.sh` (this directory) | Builds Boost.Python against each wheel's Python version (run automatically by cibuildwheel before each wheel build). |
| `.github/workflows/build-wheel-images.yml` | Builds the Docker images (natively, per architecture) and pushes them to GHCR. |
| `.github/workflows/build-wheels.yml` | CI builds of the sdist and the wheels (full matrix on release tags and manual dispatch; single-Python smoke test on pull requests touching the wheel machinery). |

> [!NOTE]
> Windows is not migrated to cibuildwheel yet - see the legacy section at the end.

## Building a wheel locally

Run cibuildwheel from the root source directory, selecting a single build with `--only`
(otherwise it builds every Python version). For example:

```
pipx run cibuildwheel==4.2.* --only cp313-manylinux_x86_64
```

...or `--only cp313-macosx_arm64` on Apple Silicon (`cp313-macosx_x86_64` on Intel). The
repaired (and tested) wheel ends up in the `wheelhouse` sub-directory of the root source
directory.

On Linux, cibuildwheel runs the build inside Docker (so Docker must be installed) using the
dependency image described below - it pulls the image from GHCR automatically, or uses your
locally built copy if you have one (see the next section).

On macOS, the first run builds the dependency libraries into `~/pygplates-wheel-deps` (about
an hour - later runs reuse them; see the macOS section below). Xcode command line tools,
`python3` and `cmake` must be installed.

## The Linux dependency image

Linux wheels are `manylinux_2_28` wheels (AlmaLinux 8, glibc 2.28+), which work on every
non-EOL mainstream distribution. `manylinux_2_28.dockerfile` extends the official
`quay.io/pypa/manylinux_2_28` image with the pyGPlates dependency libraries. Qt is built from
source in the image because EL8 has no Qt6 packages; Boost.Python is built once per supported
Python version against the interpreters the base image provides in `/opt/python`.

There is one image per architecture, both built by `.github/workflows/build-wheel-images.yml`:

- `ghcr.io/gplates/pygplates-manylinux_2_28_x86_64`
- `ghcr.io/gplates/pygplates-manylinux_2_28_aarch64`

Each push is tagged `latest` (what wheel builds pull, as configured in
`[tool.cibuildwheel.linux]` in `pyproject.toml`) and with the commit SHA it was built from
(so older images remain pullable, eg, to rebuild an old release or bisect a dependency
problem).

### Rebuilding the image

The images only need rebuilding when a dependency changes (ie, when the dockerfile or the
version pins in `versions.sh` change). In CI that happens automatically: the workflow triggers
on any push to the `pygplates` branch that touches either file (and can also be dispatched
manually).

To build the image locally instead (eg, to test a dockerfile change before pushing):

```
docker build -t ghcr.io/gplates/pygplates-manylinux_2_28_x86_64:latest -f manylinux_2_28.dockerfile .
```

...from this directory (add `--build-arg ARCH=aarch64` and adjust the tag when building on an
arm64 machine, eg, Apple Silicon). Using the GHCR name as the local tag means a subsequent
local `cibuildwheel` run uses your local image rather than pulling from GHCR.

> [!NOTE]
> The GHCR packages must be *public* for unauthenticated pulls (eg, local cibuildwheel runs).
> This is a one-time manual step after the first push of a new package:
> GitHub organization -> Packages -> package settings -> Change visibility.
> (The CI wheel builds log in with `GITHUB_TOKEN`, so they work either way.)

## The macOS dependency libraries

macOS wheels are thin (single-architecture) `macosx_11_0_arm64` and `macosx_11_0_x86_64`
wheels, built natively on each architecture. There is no dependency image on macOS - instead
cibuildwheel runs `build_macos_deps.sh` (its `before-all` hook) once per machine, which
installs the dependency libraries into a single prefix, `~/pygplates-wheel-deps`:

- Qt comes as the official binaries (downloaded with [aqtinstall](https://github.com/miurahr/aqtinstall)),
  with each framework library thinned from universal to the build architecture - halving what
  delocate later vendors into the wheels.
- Qwt, GLEW, Boost, PROJ, GDAL, GMP, MPFR and CGAL are built from source (macOS has no
  system package manager, and Homebrew binaries must not leak into the wheels - see below).
  The versions come from `versions.sh`, shared with the Linux image - so the platforms
  cannot drift apart.
- Boost.Python is the exception: it must match each wheel's Python version, so cibuildwheel
  runs `build_boost_python.sh` (its `before-build` hook) with each build's Python on PATH,
  reusing the Boost source tree that `build_macos_deps.sh` leaves in the prefix. Each version
  is built only once (about a minute) and then reused.

In CI the whole prefix is cached (`.github/workflows/build-wheels.yml`), keyed on the hash of
`versions.sh` and the two scripts plus the deployment target - so bumping a dependency version
rebuilds the cache automatically, and runs with an up-to-date cache spend about a minute
restoring it instead of about an hour building dependencies. `build_macos_deps.sh` is
idempotent (each dependency is stamped once installed), so a partial cache saved by a failed
run is completed by the next run, not rebuilt from scratch.

## Why the wheels are configured the way they are

These are the reasons behind the non-obvious `[tool.cibuildwheel]` settings in
`pyproject.toml` (learned the hard way with the pre-cibuildwheel wheel scripts).

### `GPLATES_INSTALL_STANDALONE_SHARED_LIBRARY_DEPENDENCIES=FALSE`

We set the CMake variable `GPLATES_INSTALL_STANDALONE_SHARED_LIBRARY_DEPENDENCIES` to `FALSE`
since we don't want to install shared library dependencies into the wheel - they will get
installed (copied into the wheel) when `auditwheel` (or `delocate`/`delvewheel`) is
subsequently run to repair our wheel. Note that this variable is only used if
`GPLATES_INSTALL_STANDALONE` is `TRUE`, which it is by default when building using
scikit-build-core (eg, `pip wheel ...`) outside of conda.

### `OpenGL_GL_PREFERENCE=LEGACY` (Linux)

We set the CMake variable `OpenGL_GL_PREFERENCE` to `LEGACY` (instead of the default `GLVND`).
This causes pyGPlates to prefer to use the `libGL` LEGACY dependency (instead of the default
`libOpenGL` GLVND dependency). The `libGL` library is whitelisted by auditwheel (meaning it
will not be copied into the wheel repaired by auditwheel). This is presumably because it is
available by default on all Linux distributions. Whereas `libOpenGL` is NOT whitelisted
(presumably because it is NOT available by default on all Linux distros) and hence would need
to be copied into the wheel (if it was used). However copying into the wheel is problematic if
`libOpenGL` itself needs to come from the end machine (eg, if it's NOT hardware-independent -
see <https://github.com/pypa/auditwheel/issues/241>). Alternatively, if `libOpenGL` actually
is hardware-independent and we copy it into the wheel then the end machine might still need to
have the `libglvnd` package installed (which is not the case for all Linux distros by
default - see <https://github.com/linuxdeploy/linuxdeploy/issues/152#issuecomment-830975582>).
So we prefer to link to `libGL` instead (which should be available by default on all Linux
distros).

Besides pyGPlates, Qt is the other library that uses `libGL`. And they made an effort to not
use `libOpenGL` for the same reasons (ie, it's not installed by default on all Linux distros).
See <https://bugreports.qt.io/browse/QTBUG-89754>.

Previously we linked to `libOpenGL` (because we didn't set `OpenGL_GL_PREFERENCE` to `LEGACY`)
and so it was copied into the wheel (because it's not whitelisted by auditwheel). It, in turn,
links to `libGLdispatch` and so that was also copied into the wheel. That caused a
segmentation fault during `import pygplates` because there were two copies of `libGLdispatch`
being referenced. One was copied into the wheel (due to being a dependency of `libOpenGL` that
was referenced by pyGPlates). The other was referenced by `libGL` (via Qt) and hence was not
copied into the wheel (since `libGL` is whitelisted). The segmentation fault was most likely
because, according to <https://github.com/NVIDIA/libglvnd>:

> "since all OpenGL functions are dispatched through the same table in libGLdispatch,
> it doesn't matter which library is used to find the entrypoint"

...where by "it doesn't matter which library is used to find the entrypoint" they mean
`libOpenGL` and `libGL` (not `libGLdispatch`). So having Qt reference
`/usr/lib64/libGLdispatch.so.0` (via `libGL`) and pyGPlates reference the `libGLdispatch`
copied into the wheel (via `libOpenGL`) would result in *two* dispatch tables (instead of one
central table). And this is likely what caused the segmentation fault.

The same two-dispatch-tables segfault can be reintroduced through *any* dependency that links
the GLVND libraries, not just pyGPlates itself. It resurfaced twice while moving the image to
manylinux_2_28 (reproduced with gdb - the crash is a null jump in `__glDispatchInit`):

- EL8's `glew-devel` package links `libGLX`/`libOpenGL`/`libGLdispatch` (the CentOS 7 package
  linked plain `libGL`) - so the image builds GLEW from source against `libGL` instead.
- Qt 6 links `libEGL` (which also drags in `libGLdispatch`) when EGL headers are present at
  configure time - so the image configures Qt with `FEATURE_egl=OFF` (desktop OpenGL on X11
  goes through GLX and does not need EGL).

The image's final sanity-check layer fails the build if `libQt6Gui` or `libGLEW` links any
GLVND library, so a dependency change that reintroduces one is caught at image-build time.

### `CMAKE_INSTALL_RPATH` (macOS)

The Qt frameworks and the CMake-built dependencies (PROJ, GDAL) have `@rpath/...` install
names, and delocate resolves those through the rpaths recorded in the binaries that reference
them. But CMake *strips* the build rpaths from the pygplates module when it installs it (and
the pyGPlates build system doesn't set an install rpath of its own) - leaving delocate unable
to resolve any `@rpath/...` dependency. So `pyproject.toml` passes `CMAKE_INSTALL_RPATH`
(pointing at the deps prefix's `lib` and Qt `lib` directories) to the pyGPlates build, and
`build_macos_deps.sh` does the equivalent for the dependencies' own dependencies (PROJ's rpath
in GDAL, Qt's in Qwt). The rpaths only need to be valid on the *build* machine: delocate
follows them to find the libraries, copies the libraries into the wheel, and rewrites every
reference to `@loader_path/...`.

### No Homebrew, no libpython (macOS)

The GitHub macOS runners carry a large Homebrew installation, and any dependency configured
against it would get Homebrew libraries vendored into the wheels - libraries built for the
runner's macOS version (much newer than `MACOSX_DEPLOYMENT_TARGET`) that can change ABI
whenever the runner image updates. So the source builds only look in the deps prefix and the
macOS SDK (GDAL's other optional dependencies are explicitly disabled - it uses its internal
copies), and `build_macos_deps.sh` ends with a sanity check that fails the run if anything
links a Homebrew path.

Similarly, `libboost_python` must not link `libpython`: delocate would vendor a second Python
runtime into the wheel. `build_boost_python.sh` gives b2 no Python library to link against -
Boost.Python's Python symbols resolve at import time (`-undefined dynamic_lookup`, standard
practice for anything loaded into a Python process) - and fails if the built library links a
Python runtime anyway.

## Updating Python versions

Wheels are built for the [currently supported Python versions](https://devguide.python.org/versions/)
that NumPy ships wheels for (we build against the NumPy C API, so a Python version without
NumPy wheels is not usable anyway). To add or remove a version:

1. Update the `build` list in `[tool.cibuildwheel]` in `pyproject.toml`
   (and `requires-python`/classifiers in `[project]` if the floor changed).
2. Update `PYTHON_VERSIONS` in `versions.sh` to match, and rebuild the Linux images
   (a push of the change rebuilds them automatically).
   (macOS needs no equivalent step: `build_boost_python.sh` builds Boost.Python for whatever
   Python versions the `build` list asks for.)

## Building wheels on Windows (legacy - not yet migrated to cibuildwheel)

The `build_windows_wheels.bat` batch file builds, tests and copies wheels into the
`wheelhouse` sub-directory of the root source directory, for each Python version listed in the
batch file (accessed via the `py -<version>` launcher). It will be replaced by a cibuildwheel
configuration in a follow-up.

```
cmd /c build_windows_wheels.bat
```
