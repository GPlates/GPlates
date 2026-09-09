#!/bin/bash
set -ex

if [[ "$target_platform" == "linux-ppc64le" ]]; then
  # Avoid error 'relocation truncated to fit: R_PPC64_REL24'.
  export CFLAGS="$(echo ${CFLAGS} | sed 's/-fno-plt//g') -fplt"
  export CXXFLAGS="$(echo ${CXXFLAGS} | sed 's/-fno-plt//g') -fplt"
fi

# Pin the version to the recipe version, rather than letting CMake count it from git.
#
# 'conda build' works from a copy of the source, and the conda-forge feedstock builds from a PyPI
# sdist with no repository at all, so what git would say here is not necessarily what the package
# is called. Exporting PKG_VERSION makes the module's version equal the recipe version by
# construction - see 'cmake/modules/VersionFromGit.cmake' for the resolution order.
export PYGPLATES_PEP440_VERSION="$PKG_VERSION"

# Build and install pyGPlates.
#
# Pip uses the scikit-build-core build backend to compile/install pyGPlates using CMake (see pyproject.toml).
#
# No CMAKE_PREFIX_PATH / Boost_ROOT / CMAKE_FIND_FRAMEWORK defines are needed here. 'conda build' sets
# CONDA_BUILD=1, and the root CMakeLists.txt then detects the conda 'host' environment from PREFIX
# (rather than CONDA_PREFIX, which points at the build environment): it prepends $PREFIX to
# CMAKE_PREFIX_PATH, points Boost_ROOT there, and on macOS sets CMAKE_FIND_FRAMEWORK=LAST so conda
# libraries are preferred over system frameworks. ConfigDefault.cmake also forces
# GPLATES_INSTALL_STANDALONE off for conda builds, so nothing gets bundled into the package.
CMAKE_BUILD_PARALLEL_LEVEL=$CPU_COUNT $PYTHON -m pip install -vv "$SRC_DIR"
