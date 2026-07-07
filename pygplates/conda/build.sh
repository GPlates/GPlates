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
# Note that CMAKE_FIND_FRAMEWORK (macOS) is set to LAST to avoid finding frameworks
#      (like Python and Qwt) outside the conda environment (it seems conda doesn't use frameworks).
CMAKE_BUILD_PARALLEL_LEVEL=$CPU_COUNT $PYTHON -m pip install -vv \
      -C "cmake.define.CMAKE_PREFIX_PATH=$PREFIX" \
      -C cmake.define.CMAKE_FIND_FRAMEWORK=LAST \
      "$SRC_DIR"
