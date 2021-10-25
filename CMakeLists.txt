# Modern CMake requires version 3.
#
# Ubuntu Xenial (16.04 LTS) provides CMake 3.5.1, so we'll make this the minimum requirement.
# Also CMake 3.5 introduced imported targets for the Boost library.
#
# Note: There's a "..." between min and max versions which means we need to use the minimum version
#       but are safe to use with policies (see 'cmake_policy' command) up to the maximum version.
#       Specifying a maximum version means we don't need 'cmake_policy' calls for running CMake versions
#       in that [min, max] range. Note that CMake version prior to 3.12 apparently ignore the "...<max>" part.
cmake_minimum_required(VERSION 3.5...3.21 FATAL_ERROR)

# Set the minimum macOS version we support.
# We select 10.13 because Qt5 currently supports 10.13+.
# Also, as of 10.9, Apple has removed support for libstdc++ (the default prior to 10.9) in favour of libc++ (C++11).
# Note that this needs to be set before project() and must be a cache variable (avoids project() overwriting it).
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13 CACHE STRING "Minimum macOS deployment version")

# The GPlates version goes here.
#
# NOTE: Always use MAJOR.MINOR.PATCH (to be compatible with Semantic Versioning 2.0 - https://semver.org/spec/v2.0.0.html)
#       CMake always allows an extra TWEAK component but we don't use it.
#       We do however support pre-releases (see GPLATES_VERSION_PRERELEASE in 'ConfigDefault.cmake').
project(GPlates VERSION 2.3.0 LANGUAGES C CXX)

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
# Recurse into source code sub-directory.
#
add_subdirectory(src)

#
# Recurse into documentation sub-directory.
#
add_subdirectory(doc)
