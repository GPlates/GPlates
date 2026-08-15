# The dependency version pins for the pyGPlates wheel builds - the single source of truth
# shared by the Linux image ('manylinux_2_28.dockerfile' copies this file in and sources it
# in each build step) and the macOS and Windows dependency builds ('build_macos_deps.sh' and
# 'build_windows_deps.sh' source it directly). Bumping a version here rebuilds that dependency
# everywhere: a push rebuilds the Linux images (see '.github/workflows/build-wheel-images.yml'),
# and the macOS and Windows deps cache keys include this file's hash (see
# '.github/workflows/build-wheels.yml').
#
# Not everything is pinned here: the libraries vcpkg supplies to the Windows build are pinned
# by the vcpkg baseline in 'vcpkg.json' instead. For the libraries both systems pin - GLEW,
# PROJ and GDAL - the versions here are kept equal to what that baseline supplies, so all three
# platforms' wheels ship the same geospatial behaviour. Bump the baseline and these three
# together, as one act, with vcpkg setting the cadence (it is the one of the two whose available
# versions cannot be picked freely); 'build_windows_deps.sh' fails if they drift apart.
#
# Keep this file to plain NAME=VALUE assignments and comments - it is sourced by /bin/sh in
# the Docker build steps.

# The Python versions to build Boost.Python for, in the Linux image - which is the only thing
# that reads this. Must match the 'build' list in '[tool.cibuildwheel]' in 'pyproject.toml' (the
# authoritative list of wheel Python versions); the sdist job in
# '.github/workflows/build-wheels.yml' fails if the two ever disagree. macOS and Windows need no
# such list: 'build_boost_python.sh' builds Boost.Python for whatever Python version each wheel
# build brings.
PYTHON_VERSIONS="3.9 3.10 3.11 3.12 3.13 3.14"

QT_VERSION=6.7.3
GLEW_VERSION=2.3.1
QWT_VERSION=6.3.0
BOOST_VERSION=1.91.0
PROJ_VERSION=9.8.1
GDAL_VERSION=3.12.4
CGAL_VERSION=6.0.3
SCCACHE_VERSION=0.17.0

# macOS only (the Linux image uses EL8's gmp-devel/mpfr-devel packages, and Windows takes
# both from vcpkg - neither has an MSVC build system).
GMP_VERSION=6.3.0
MPFR_VERSION=4.2.1

# Linux only (macOS uses the SDK's SQLite). The year is part of the sqlite.org download path.
SQLITE3_VERSION=3460000
SQLITE3_YEAR=2024
