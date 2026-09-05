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
| `versions.sh` (this directory) | The dependency version pins - the single source of truth shared by all three platforms. |
| `manylinux_2_28.dockerfile` (this directory) | Docker image with the pyGPlates dependency libraries, that the Linux wheels are built inside. |
| `build_macos_deps.sh` (this directory) | Builds the pyGPlates dependency libraries on macOS (run automatically by cibuildwheel, once per machine; cached in CI). |
| `build_windows_deps.sh` (this directory) | The same on Windows. |
| `vcpkg.json` (this directory) | The subset of the Windows dependency libraries that vcpkg supplies, and the vcpkg baseline that pins their versions. |
| `msvc_env.sh` (this directory) | Puts the MSVC toolchain into the environment: the two Windows scripts above source it (they build with `b2` and `nmake`, which cannot find Visual Studio for themselves), and the CI job runs it to set up the environment Ninja compiles in. |
| `build_boost_python.sh` (this directory) | Builds Boost.Python against each wheel's Python version, on macOS and Windows (run automatically by cibuildwheel before each wheel build). |
| `check_wheel_contents.py` (this directory) | Checks each repaired wheel carries the data PROJ and GDAL read at run time (run automatically by cibuildwheel, after the test suite). |
| `.github/workflows/build-wheel-images.yml` | Builds the Docker images (natively, per architecture) and pushes them to GHCR. |
| `.github/workflows/build-wheels.yml` | CI builds of the sdist and the wheels (full matrix on release tags and manual dispatch; two-Python smoke test on pull requests touching the wheel machinery). On a release tag it also publishes to PyPI - see [Publishing a release](#publishing-a-release). |

## Building a wheel locally

Run cibuildwheel from the root source directory, selecting a single build with `--only`
(otherwise it builds every Python version). For example:

```
pipx run cibuildwheel==4.2.* --only cp313-manylinux_x86_64
```

...or `--only cp313-macosx_arm64` on Apple Silicon (`cp313-macosx_x86_64` on Intel), or
`--only cp313-win_amd64` on Windows. The repaired (and tested) wheel ends up in the `wheelhouse`
sub-directory of the root source directory.

On Linux, cibuildwheel runs the build inside Docker (so Docker must be installed) using the
dependency image described below - it pulls the image from GHCR automatically, or uses your
locally built copy if you have one (see the next section).

On macOS, the first run builds the dependency libraries into `~/pygplates-wheel-deps` (about
an hour - later runs reuse them; see the macOS section below). Xcode command line tools,
`python3` and `cmake` must be installed.

On Windows, the first run builds them into `C:\pygplates-wheel-deps` (see the Windows section
below), and Visual Studio 2022 - or its Build Tools - with the C++ x64 toolset, Git for Windows
(which provides the `bash` those scripts are written for), Python and CMake must be installed.
Start it from an **"x64 Native Tools Command Prompt for VS 2022"**: the wheels are built with
Ninja, which compiles with whatever `cl` the environment provides, and neither cibuildwheel nor
scikit-build-core sets that environment up (see "Ninja and the compiler cache" below).

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

## The Windows dependency libraries

Windows wheels are `win_amd64` wheels. As on macOS there is no dependency image - cibuildwheel
runs `build_windows_deps.sh` (its `before-all` hook) once per machine, which installs the
dependency libraries into a single prefix, `C:\pygplates-wheel-deps` (a short path at the root of
the drive, because the dependency builds nest deeply enough to run into the 260-character Windows
path limit):

- Qt comes as the official binaries, downloaded with aqtinstall, as on macOS. The debug symbols
  that the official MSVC packages carry - most of their size - are deleted afterwards. The debug
  libraries themselves stay, unused: Qt's CMake package declares a Debug configuration for every
  imported target and CMake checks that each file it names is there.
- GLEW, zlib, PROJ, GDAL, GMP and MPFR come from [vcpkg](https://vcpkg.io) (see `vcpkg.json`).
  PROJ's and GDAL's run-time data - the coordinate reference system database and GDAL's driver
  data, which the build copies into the wheel - are named explicitly in `pyproject.toml`, since
  vcpkg installs a port's tools apart from its libraries and `projinfo`, which the build asks
  where PROJ's data is, would otherwise answer for a directory layout that is not this one.
  GMP and MPFR are the reason vcpkg is here at all: they have no MSVC build system. PROJ and GDAL
  come along with them because they are a deep stack that vcpkg already knows how to build on
  Windows. These versions are pinned by the vcpkg baseline commit in `vcpkg.json` rather than by
  `versions.sh` - but the `versions.sh` pins for GLEW, PROJ and GDAL are kept equal to what the
  baseline supplies, so every platform's wheels ship the same versions of the libraries a user
  can feel (CRS handling, datum grids, driver behaviour). Bump the baseline and those three pins
  together, as one act, with vcpkg setting the cadence; `build_windows_deps.sh` compares what
  vcpkg installed against `versions.sh` and fails if they have drifted apart.
- Qwt, Boost and CGAL are built from source against the `versions.sh` pins, as on the other
  platforms. Qwt is built as a *static* library, because a Qwt DLL would require everything that
  includes its headers to be compiled with `-DQWT_DLL`.
- Boost.Python is per-Python-version, so - as on macOS - cibuildwheel runs
  `build_boost_python.sh` (its `before-build` hook) with each build's Python on PATH, reusing the
  Boost source tree left in the prefix.

Neither Boost's `b2` nor Qwt's `qmake`/`nmake` is a CMake build, so neither can locate Visual
Studio for itself. Both scripts therefore source `msvc_env.sh`, which runs `vcvarsall.bat` in a
cmd shell and imports the environment it produces. That is what lets them run under cibuildwheel,
whose hooks run in a plain cmd shell, instead of requiring an "x64 Native Tools Command Prompt".

In CI the prefix is cached exactly as on macOS. The CI job does not run from a developer command
prompt either - it imports the same environment with `msvc_env.sh` before running cibuildwheel,
which is the one thing a local build has to arrange for itself.

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

### `OpenGL_GL_PREFERENCE=LEGACY` (Linux) - history, and why the image still sets it

> **No longer set for the module itself.** `pygplates.so` links only `Qt6Core` (its sources
> are the include closure of the pyGPlates API — see
> `cmake/pygplates_source_closure.py` — and the last Gui uses, `gui/Colour`'s `QColor`
> conversions and `file-io/RgbaRasterReader`'s `QImage`, moved to the GPlates side). Nothing on
> its link line reaches OpenGL, so there is no GL family to choose and `pyproject.toml` no longer
> sets this variable; `cmake/check_linkage.py` fails the build if any GL library reappears.
> Dropping the module's `Qt6Gui` also dropped `Qt6Network`, `Qt6Xml`, `Qt6DBus`, fontconfig,
> freetype, libpng, libxkbcommon, libbz2 and libuuid from the vendored libraries - ten of the
> twenty-six, about 37 MB of the unpacked wheel.
>
> The variable did have to stay for one intermediate period: after the module stopped compiling
> any OpenGL of its own but while it still linked `Qt6Gui`, whose CMake imported target
> propagates Qt's own OpenGL dependency onto everything linking it. Removing it then, as a
> supposed no-op, produced exactly the `import pygplates` segmentation fault described at the
> end of this section. The rest of this section is kept because the docker image builds Qt and
> GLEW with `OpenGL_GL_PREFERENCE=LEGACY` for the same reason (`manylinux_2_28.dockerfile`),
> and that reasoning still stands for `libQt6Gui`/`libGLEW` until the image builds a Core-only
> Qt.

We set the CMake variable `OpenGL_GL_PREFERENCE` to `LEGACY` (instead of the default `GLVND`).
This caused pyGPlates to prefer to use the `libGL` LEGACY dependency (instead of the default
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

### Checking the wheel, not just the code

`test-command` runs the pyGPlates test suite and then `check_wheel_contents.py`, which confirms the
wheel carries PROJ's coordinate reference system database and GDAL's driver data.

The suite cannot stand in for that check, because it passes either way. `cmake/modules/Install.cmake`
locates both data directories while configuring, and used to report failure as a CMake warning and
carry on - so the Windows wheels were built, repaired, tested and reported green for as long as the
Windows job existed while shipping neither, with PROJ printing `Cannot find proj.db` to stderr and
coordinate reference systems quietly failing to resolve.

Those warnings are now errors, raised when the bundle is *installed* rather than when it is
configured - the same way the code signing and Qt version checks at the top of that file work. That
stops a broken bundle being produced without stopping a build: `GPLATES_INSTALL_STANDALONE` is on by
default for Windows and macOS GPlates builds too, and someone who only wants to compile and run from
their build tree should not have to satisfy it. Building a wheel runs `cmake --install`, so the
wheels are covered either way.

That much covers the causes the build can see. `check_wheel_contents.py` covers the rest of the
path, since the data still has to survive installation and the repair step to reach the user.

### Ninja and the compiler cache (Windows)

pyGPlates is built with Ninja on every platform, Windows included (`CMAKE_GENERATOR` in
`[tool.cibuildwheel.windows.environment]`). The Visual Studio generator would locate the compiler
by itself, which is convenient, but it ignores `CMAKE_<LANG>_COMPILER_LAUNCHER` - so there would
be no compiler cache, and each of the six Python versions would pay a full compile of about an
hour.

What Ninja costs is that it compiles with whatever `cl` the environment provides, and neither
cibuildwheel nor scikit-build-core arranges one (scikit-build-core sets the MSVC environment up
only for "Visual Studio" generators). So a local wheel build must be started from an "x64 Native
Tools Command Prompt for VS 2022", and the CI job imports the same environment with `msvc_env.sh`
before running cibuildwheel. The dependency scripts are unaffected either way - they source
`msvc_env.sh` themselves, because as cibuildwheel hooks they cannot set up the environment of the
process that runs them.

Ninja also means CMake has to *find* a compiler, rather than being told one by the generator - and
what it finds on a machine with Visual Studio installed is the LLVM that Visual Studio bundles, in
`VC/Tools/Llvm`. So the wheel would be built with clang against a Qt, Boost and vcpkg stack built
with MSVC. `CMAKE_C_COMPILER`/`CMAKE_CXX_COMPILER` in the `config-settings` therefore name `cl`.

The cache earns it. Only about a hundred of the thousand-odd sources include Python's own
headers, so the second and later Python versions reuse around 90% of the first's object files -
measured on Linux and macOS, where the same arrangement cut the second wheel from 33 to 14
minutes. Windows is therefore one job per platform like everything else, rather than a
dependency job followed by one wheel job per Python version.

For that reuse to happen at all, every Python version must build in the *same* directory: sccache
keys on preprocessor output, which names the path of every header included, so a per-version
build directory makes each source that includes a generated header (`config.h`, Qt's moc and ui
output) hash differently for no reason. Hence `build-dir` in the cibuildwheel `config-settings`,
together with the `before-build` that empties it - the emptying is what keeps a shared directory
from also being an incremental build across Python versions.

### `delvewheel --add-path` (Windows)

Windows has no rpath: a DLL records only the *name* of each DLL it needs, which is then searched
for in a list of directories. So delvewheel cannot follow the pyGPlates module's own dependency
chain to the dependency libraries the way delocate does on macOS, and `repair-wheel-command` in
`pyproject.toml` names the directories they are in (Boost's, Qt's and vcpkg's) instead.

Windows is also where Boost.Python *does* link a Python library (`python313.lib` and friends):
an extension module resolves its Python symbols at link time here, rather than leaving them to
the interpreter at load time as on macOS and Linux. That is expected - delvewheel excludes the
Python DLL from what it vendors, so the wheel still runs against the interpreter's own runtime.

## Updating Python versions

Wheels are built for the [currently supported Python versions](https://devguide.python.org/versions/)
that NumPy ships wheels for (we build against the NumPy C API, so a Python version without
NumPy wheels is not usable anyway). To add or remove a version:

1. Update the `build` list in `[tool.cibuildwheel]` in `pyproject.toml`
   (and `requires-python`/classifiers in `[project]` if the floor changed).
2. Update `PYTHON_VERSIONS` in `versions.sh` to match, and rebuild the Linux images
   (a push of the change rebuilds them automatically). The `sdist` job fails if the two lists
   disagree, so this cannot be forgotten quietly.
   (macOS and Windows need no equivalent step: `build_boost_python.sh` builds Boost.Python for
   whatever Python version each wheel build brings.)

## Publishing a release

Releasing to [PyPI](https://pypi.org/p/pygplates) is part of `build-wheels.yml`: pushing a
release tag builds the full matrix and then publishes it, with two safeguards between building
and the real index. Nothing holds an API token - the publish jobs authenticate with
[Trusted Publishing](https://docs.pypi.org/trusted-publishers/), where the index trusts short-lived
OIDC tokens GitHub mints for this specific workflow file.

The flow, end to end:

1. Set the release version in `cmake/modules/Version.cmake` (`PYGPLATES_PEP440_VERSION`) and
   commit. For a first pass at a release, use an `rc` version (eg, `1.1.0rc1`): pip ignores
   release candidates by default, so it exercises this whole pipeline - the tag check, the
   rehearsal, the approval gate, a real PyPI upload - with low stakes.
2. Tag that commit `PyGPlates-<version>` (exactly the version string - the run fails in its
   first minute if the two disagree, or if the version is a `.dev` one) and push the tag.
3. The run builds the sdist and every wheel (about 2.5 hours warm, 4.5 cold), then uploads the
   sdist plus one platform's wheels to [TestPyPI](https://test.pypi.org/p/pygplates) - a
   rehearsal that catches anything the index itself would reject (metadata, most of all)
   before the real upload.
4. The run then sits at **waiting** until a maintainer approves the `pypi` deployment (repo
   page -> the run -> "Review deployments"). This is the moment to eyeball the TestPyPI
   project page. Approval waits expire after 30 days.
5. On approval the whole matrix - one sdist, one wheel per Python version per platform, counted
   before upload - goes to PyPI, each file with a [PEP 740](https://peps.python.org/pep-0740/)
   attestation.

Recovery paths, should something fail:

- **A build failed**: "Re-run failed jobs" - only the failed platform runs again (from its
  dependency cache), and the publish jobs then start as if the run had been green all along.
  They upload what the run's artifacts hold, so this works within the artifact retention
  window - 90 days.
- **An upload failed partway**: also "Re-run failed jobs". `skip-existing` on the publish steps
  skips the files that made it and uploads the rest - without that, the index's refusal to
  accept a filename twice would wedge the release permanently.
- **Something is wrong with the release itself**: don't approve; cancel the run, fix, bump the
  version, re-tag. PyPI reserves every uploaded filename forever (deleting a release does not
  free its names), which is why a suspect release should be stopped *before* approval.

Housekeeping to be aware of:

- PyPI's default project quota is 10 GiB, and a full release is ~1.2 GiB - about seven releases'
  worth. File a [size-limit increase](https://docs.pypi.org/project-management/storage-limits/)
  before it becomes urgent, and delete superseded `rc` releases (deletion does free quota - it's
  only the *filenames* that stay reserved).
- TestPyPI has the same quota and rehearsals accrete there too; delete old ones now and then.

### One-time setup (already done - recorded for when it must be redone)

The trusted-publisher registrations bind to this repository **and the workflow filename**, so
renaming `build-wheels.yml` (or moving the repository) silently breaks publishing until the
registrations are updated to match. The order below matters: a workflow run referencing a
deployment environment that does not exist *auto-creates it unprotected*, so the environments
must exist - with their protection rules - before the first tag push.

1. Create the `pypi` [environment](https://docs.github.com/en/actions/how-tos/deploy/configure-and-manage-deployments/manage-environments)
   in the repository settings: required reviewers = the release maintainer(s); deployment
   branches/tags restricted to tags matching `PyGPlates-*`. Leave "prevent self-review" off -
   a lone maintainer must be able to approve their own release.
2. Create the `testpypi` environment (no reviewers - the rehearsal is what runs unattended).
3. On **TestPyPI** (separate account from PyPI, 2FA required): pygplates doesn't exist there,
   so register a [pending publisher](https://docs.pypi.org/trusted-publishers/creating-a-project-through-oidc/):
   owner `GPlates`, repository `GPlates`, workflow `build-wheels.yml`, environment `testpypi`.
   (A pending publisher doesn't reserve the name, so do this close to first use; it becomes a
   normal publisher on the first upload.)
4. On **PyPI**, in the pygplates project -> Settings -> Publishing, add the trusted publisher:
   owner `GPlates`, repository `GPlates`, workflow `build-wheels.yml`, environment `pypi`.
5. After the first successful trusted publish, revoke any remaining pygplates API tokens.
