# Modern CMake requires CMake version 3.
#
# Ubuntu Jammy (22.04 LTS) provides CMake 3.22, so we'll make this the minimum requirement.
#
# Note: There's a "..." between min and max versions which means we need to use the minimum version
#       but are safe to use with policies (see 'cmake_policy' command) up to the maximum version.
#       Specifying a maximum version means we don't need 'cmake_policy' calls for running CMake versions
#       in that [min, max] range.
cmake_minimum_required(VERSION 3.22...4.4 FATAL_ERROR)

# Where to find our CMake modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake/modules)

#
# Include versioning.
#
include (Version)

# Generate a 'compile_commands.json' compilation database by default, for clangd / IDE code
# intelligence (see the committed '.clangd' at the repo root). Honoured by the Ninja and Makefile
# generators (all our documented builds use Ninja); silently ignored by the Visual Studio and Xcode
# generators. Override with '-DCMAKE_EXPORT_COMPILE_COMMANDS=OFF' if not wanted.
#
# This must precede 'project()' below: 'project()' creates the CMAKE_EXPORT_COMPILE_COMMANDS cache
# entry itself, so a 'set(... CACHE ...)' default placed after it would be a silent no-op. Setting it
# before 'project()' makes the default apply, while a user's '-D' still wins (that populates the cache
# before this file is processed, so the 'set()' here becomes the no-op instead).
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Generate compile_commands.json (for clangd/IDE tooling).")

#
# Make an active conda environment discoverable to CMake's dependency search.
#
# When building inside an activated conda environment - whether a manual "cmake ..."/"pip install ..." build
# or an automated "conda build ..." - the environment root is exposed to us as an environment variable.
# Conda activation does NOT set CMAKE_PREFIX_PATH, so without help CMake won't look in the conda environment
# for dependencies (Qt, Qwt, Boost, PROJ, GDAL, ...) and can silently fall back to system/MacPorts/Homebrew
# libraries (eg, picking a Qt5 'qwt.framework' for a conda Qt6 build).
#
# Which variable holds the right root depends on the build:
#  - An automated "conda build ..." recipe installs the target/host dependencies under the 'host' prefix,
#    exposed as PREFIX, while CONDA_PREFIX points at the separate build (activation) environment. We detect
#    this case exactly as ConfigDefault.cmake does (CONDA_BUILD=1 -> GPLATES_CONDA_BUILD) and use PREFIX.
#  - A plain activated environment (manual "cmake ..."/"pip install ...") uses CONDA_PREFIX.
#
# Handling it here (rather than requiring the user to pass '-DCMAKE_PREFIX_PATH=$CONDA_PREFIX' etc) makes it
# work for every entry point with no extra flags. It is especially important for 'pip install ...':
# scikit-build-core builds the CMake command line itself (populating CMAKE_PREFIX_PATH from site-packages and
# overriding any CMAKE_PREFIX_PATH passed via 'cmake.define'), but these *environment* variables still reach
# the CMake process.
#
# This must precede 'project()':
#  - On macOS, CMAKE_FIND_FRAMEWORK only reliably affects the later find_library()/find_path() calls when set
#    before the project languages are configured (matching how a command-line '-DCMAKE_FIND_FRAMEWORK=LAST'
#    behaves); setting it afterwards is silently too late.
#  - We therefore test CMAKE_HOST_WIN32/CMAKE_HOST_APPLE (available at startup) rather than WIN32/APPLE, which
#    are not set until 'project()'.
if (DEFINED ENV{CONDA_BUILD} AND ("$ENV{CONDA_BUILD}" EQUAL 1))
    # A "conda build ..." recipe: use the 'host' prefix where the target dependencies are installed.
    set(_GPLATES_CONDA_PREFIX "$ENV{PREFIX}")
elseif (DEFINED ENV{CONDA_PREFIX})
    set(_GPLATES_CONDA_PREFIX "$ENV{CONDA_PREFIX}")
endif()

if (_GPLATES_CONDA_PREFIX)
    if (CMAKE_HOST_WIN32)
        # Conda installs most Windows libraries under a 'Library' sub-directory of the environment.
        set(CMAKE_PREFIX_PATH "${_GPLATES_CONDA_PREFIX}" "${_GPLATES_CONDA_PREFIX}/Library" ${CMAKE_PREFIX_PATH})
    else()
        set(CMAKE_PREFIX_PATH "${_GPLATES_CONDA_PREFIX}" ${CMAKE_PREFIX_PATH})
    endif()

    # Point CMake's Boost search at the conda environment. find_package()'s <Package>_ROOT variable is
    # consulted before CMAKE_PREFIX_PATH and, importantly, before the BOOST_ROOT *environment* variable
    # (honoured under policy CMP0074, NEW for our CMake >= 3.22). Users who built Boost from source
    # (rather than using conda) often still have a BOOST_ROOT env var pointing at that old location;
    # setting the CMake variable here makes conda's Boost win without requiring them to unset the env var
    # (they may still occasionally build without conda, where the env var is wanted). On Windows conda
    # puts Boost under the 'Library' sub-directory; elsewhere it's directly under the prefix.
    # Guarded so an explicit '-DBoost_ROOT=...' on the command line still overrides this.
    if (NOT DEFINED Boost_ROOT)
        if (CMAKE_HOST_WIN32)
            set(Boost_ROOT "${_GPLATES_CONDA_PREFIX}/Library")
        else()
            set(Boost_ROOT "${_GPLATES_CONDA_PREFIX}")
        endif()
    endif()

    if (CMAKE_HOST_APPLE)
        # Prefer conda's libraries over similarly-named frameworks elsewhere on the system (eg, a
        # MacPorts/Homebrew Qt5 'qwt.framework' that would otherwise be chosen for a conda Qt6 build).
        # Set WITHOUT 'FORCE' so a user-supplied '-DCMAKE_FIND_FRAMEWORK=...' still wins (that populates the
        # cache before this file is processed, making this a no-op).
        set(CMAKE_FIND_FRAMEWORK LAST CACHE STRING "Search macOS frameworks last, so conda (and other non-framework) libraries are preferred.")
    endif()

    message(STATUS "Detected active conda environment - prepended '${_GPLATES_CONDA_PREFIX}' to CMAKE_PREFIX_PATH (and set Boost_ROOT to it)")
    unset(_GPLATES_CONDA_PREFIX)
endif()

#
# Build either GPlates or pyGPlates.
#
# We don't build both because CPack debian packages do not allow a separate version for each component package.
#
# NOTE: For versioning we always use MAJOR.MINOR.PATCH (to be compatible with Semantic Versioning 2.0 - https://semver.org/spec/v2.0.0.html)
#       CMake always allows an extra TWEAK component but we don't use it.
if (GPLATES_BUILD_GPLATES)
    project(GPlates
            VERSION ${GPLATES_VERSION_MAJOR}.${GPLATES_VERSION_MINOR}.${GPLATES_VERSION_PATCH}
            DESCRIPTION "Desktop software for the interactive visualisation of plate tectonics."
            LANGUAGES C CXX)
else()
    project(PyGPlates
            VERSION ${PYGPLATES_VERSION_MAJOR}.${PYGPLATES_VERSION_MINOR}.${PYGPLATES_VERSION_PATCH}
            DESCRIPTION "Python library for fine-grained access to GPlates functionality."
            LANGUAGES C CXX)
endif()

# Enable testing of GPlates (the 'gplates-unit-test' executable) or pyGPlates (the Python test suite),
# either via running 'ctest', or 'make test', or building 'RUN_TESTS' in Visual Studio.
enable_testing()
# Print test output when test fails - when running tests with 'make test' or building 'RUN_TESTS' (in Visual Studio).
list(APPEND CMAKE_CTEST_ARGUMENTS "--output-on-failure")  # Ignored if CMake < 3.17

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
# GPlates or pyGPlates specific sub-directories.
#
if (GPLATES_BUILD_GPLATES)
    #
    # Recurse into C++ documentation sub-directory.
    #
    add_subdirectory(doc-cpp)
else()
    #
    # Recurse into the sub-directory for testing/installing the pyGPlates Python module.
    #
    add_subdirectory(pygplates)

    #
    # Recurse into the sub-directory for generating the Python API documentation.
    #
    add_subdirectory(doc-python-api)
endif()
