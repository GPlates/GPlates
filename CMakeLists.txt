project(GPlates)

cmake_minimum_required(VERSION 2.4.0)

# Where to find our CMake modules (this variable is visible in subdirectories).
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules/")

# Include configuration options (default options and options overridden by user).
include (ConfigCMake)

#
# CMake version 2.6 introduces policies to tell CMake to behave like a certain version ( > 2.4 ).
# NOTE: we don't include if CMake version is less than 2.6 because we know 2.4.6 has trouble parsing "if (POLICY...)".
#
if ("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" STRGREATER "2.5")
    include (Policies_26)
endif ("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" STRGREATER "2.5")

# Include compiler options for the C++ programming language.
include (CompileOptionsCPlusPlus)

#
# Recurse into source code directory.
#
add_subdirectory(src)

#
# Recurse into documentation directory.
#
add_subdirectory(doc)
