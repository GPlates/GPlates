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
