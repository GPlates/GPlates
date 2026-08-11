# Building Wheels

PyGPlates wheels are built with [cibuildwheel](https://cibuildwheel.pypa.io/) - both in CI and
locally. Cibuildwheel drives the whole per-wheel pipeline: build pyGPlates for each Python
version, repair the wheel (copy the dependency shared libraries into it and give them unique
names - `auditwheel` on Linux, `delocate` on macOS, `delvewheel` on Windows) and run the
pyGPlates test suite against the repaired wheel.

The pieces fit together like this:

| Piece | What it does |
|---|---|
| `[tool.cibuildwheel]` in the root `pyproject.toml` | The wheel configuration: Python versions, dependency images, CMake defines, test command. |
| `manylinux_2_28.dockerfile` (this directory) | Docker image with the pyGPlates dependency libraries, that the Linux wheels are built inside. |
| `.github/workflows/build-wheel-images.yml` | Builds the Docker images (natively, per architecture) and pushes them to GHCR. |
| `.github/workflows/build-wheels.yml` | CI builds of the sdist and the wheels (full matrix on release tags and manual dispatch; single-Python smoke test on pull requests touching the wheel machinery). |

> [!NOTE]
> macOS and Windows are not migrated to cibuildwheel yet - see the legacy sections at the end.

## Building a wheel locally

Run cibuildwheel from the root source directory, selecting a single build with `--only`
(otherwise it builds every Python version). For example:

```
pipx run cibuildwheel==4.2.* --only cp313-manylinux_x86_64
```

The repaired (and tested) wheel ends up in the `wheelhouse` sub-directory of the root source
directory.

On Linux, cibuildwheel runs the build inside Docker (so Docker must be installed) using the
dependency image described below - it pulls the image from GHCR automatically, or uses your
locally built copy if you have one (see the next section).

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

The images only need rebuilding when a dependency changes (ie, when the dockerfile changes).
In CI that happens automatically: the workflow triggers on any push to the `pygplates` branch
that touches the dockerfile (and can also be dispatched manually).

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

## Updating Python versions

Wheels are built for the [currently supported Python versions](https://devguide.python.org/versions/)
that NumPy ships wheels for (we build against the NumPy C API, so a Python version without
NumPy wheels is not usable anyway). To add or remove a version:

1. Update the `build` list in `[tool.cibuildwheel]` in `pyproject.toml`
   (and `requires-python`/classifiers in `[project]` if the floor changed).
2. Update the Boost.Python versions in `manylinux_2_28.dockerfile` to match, and rebuild the
   Linux images (a push of the dockerfile change rebuilds them automatically).

## Building wheels on macOS (legacy - not yet migrated to cibuildwheel)

The `build_macos_wheels.sh` script builds, tests and copies wheels into the `wheelhouse`
sub-directory of the root source directory, for each Python version listed in the script. It
predates the conda-first dependency setup (it assumes MacPorts dependencies, run as root) and
will be replaced by a cibuildwheel configuration in a follow-up.

```
sudo -H MACOSX_DEPLOYMENT_TARGET=11.0 ./build_macos_wheels.sh
```

## Building wheels on Windows (legacy - not yet migrated to cibuildwheel)

The `build_windows_wheels.bat` batch file builds, tests and copies wheels into the
`wheelhouse` sub-directory of the root source directory, for each Python version listed in the
batch file (accessed via the `py -<version>` launcher). It will be replaced by a cibuildwheel
configuration in a follow-up.

```
cmd /c build_windows_wheels.bat
```
