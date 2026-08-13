# The dependency version pins for the pyGPlates wheel builds - the single source of truth
# shared by the Linux image ('manylinux_2_28.dockerfile' copies this file in and sources it
# in each build step) and the macOS dependency build ('build_macos_deps.sh' sources it
# directly). Bumping a version here rebuilds that dependency everywhere: a push rebuilds the
# Linux images (see '.github/workflows/build-wheel-images.yml'), and the macOS deps cache key
# includes this file's hash (see '.github/workflows/build-wheels.yml').
#
# Keep this file to plain NAME=VALUE assignments and comments - it is sourced by /bin/sh in
# the Docker build steps.

# The Python versions to build Boost.Python for. Must match the 'build' list in
# '[tool.cibuildwheel]' in 'pyproject.toml' (the authoritative list of wheel Python versions).
# Only the Linux image consumes this: on macOS (and later Windows) 'build_boost_python.sh'
# builds Boost.Python for whatever Python version each wheel build brings.
PYTHON_VERSIONS="3.9 3.10 3.11 3.12 3.13 3.14"

QT_VERSION=6.7.3
GLEW_VERSION=2.2.0
QWT_VERSION=6.3.0
BOOST_VERSION=1.91.0
PROJ_VERSION=9.4.0
GDAL_VERSION=3.8.5
CGAL_VERSION=6.0.3
SCCACHE_VERSION=0.17.0

# macOS only (the Linux image uses EL8's gmp-devel/mpfr-devel packages).
GMP_VERSION=6.3.0
MPFR_VERSION=4.2.1

# Linux only (macOS uses the SDK's SQLite). The year is part of the sqlite.org download path.
SQLITE3_VERSION=3460000
SQLITE3_YEAR=2024
