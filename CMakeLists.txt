project(GPlates)

# CMake 2.8.8 introduced CMAKE_CXX_COMPILER_VERSION.
cmake_minimum_required(VERSION 2.8.8 FATAL_ERROR)

# Where to find our CMake modules (this variable is visible in subdirectories).
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules/")

# Include configuration options (default options and options overridden by user).
include (ConfigCMake)

#
# CMake policies give the choice of OLD (backwards compatible) or NEW (non-backwards compatible) behaviour
# when a version of CMake newer than specified in 'cmake_minimum_required()' above is used.
# CMake states that ultimately all policies should specify NEW with OLD being a temporary solution.
#
include (Policies)

#
# Recurse into source code directory.
#
add_subdirectory(src)

#
# Recurse into C++ documentation directory.
#
add_subdirectory(doc-cpp)

#
# Recurse into python API documentation directory.
#
# NOTE: We add this subdirectory after 'src' because the latter defines '${EXECUTABLE_OUTPUT_PATH}'
# which we use to tell Sphinx (HTML document generator) where to find the 'pygplates' module
# (because Sphinx imports 'pygplates' to search its docstrings).
# Also we need to add a dependency to the 'pygplates' target so that it gets built first.
#
add_subdirectory(doc-python-api)
