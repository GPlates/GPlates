# Modern CMake requires version 3.
#
# Ubuntu Xenial (16.04 LTS) provides CMake 3.5.1, so we'll make this the minimum requirement.
# Also CMake 3.5 introduced imported targets for the Boost library.
#
# Note: There's a "..." between min and max versions which means we need to use the minimum version
#       but are safe to use with policies (see 'cmake_policy' command) up to the maximum version.
#       Specifying a maximum version means we don't need 'cmake_policy' calls for running CMake versions
#       in that [min, max] range. Note that CMake version prior to 3.12 apparently ignore the "...<max>" part.
cmake_minimum_required(VERSION 3.5...3.15 FATAL_ERROR)

# The GPlates version goes here.
project(GPlates VERSION 2.2.0 LANGUAGES C CXX)

# Where to find our CMake modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake/modules)

#
# Include configuration options (default options and options overridden by user).
#
include (ConfigDefault)

#
# Add new custom build configurations (ProfileGprof and ProfileGplates) to
# existing Debug/Release/RelWithDebInfo/MinSizeRel configurations.
#
include (CustomBuildConfigs)

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
