#!/bin/bash
set -ex

if [[ "$target_platform" == "linux-ppc64le" ]]; then
  # Avoid error 'relocation truncated to fit: R_PPC64_REL24'.
  export CFLAGS="$(echo ${CFLAGS} | sed 's/-fno-plt//g') -fplt"
  export CXXFLAGS="$(echo ${CXXFLAGS} | sed 's/-fno-plt//g') -fplt"
fi

# Build and install pyGPlates.
#
# Pip uses the scikit-build-core build backend to compile/install pyGPlates using CMake (see pyproject.toml).
#
# CMake auto-detects the active conda environment (it prepends $CONDA_PREFIX to CMAKE_PREFIX_PATH and, on
# macOS, sets CMAKE_FIND_FRAMEWORK=LAST so conda libraries are preferred over system frameworks), so no
# '-C cmake.define.CMAKE_PREFIX_PATH'/'CMAKE_FIND_FRAMEWORK' flags are needed here.
CMAKE_BUILD_PARALLEL_LEVEL=$CPU_COUNT $PYTHON -m pip install -vv "$SRC_DIR"
