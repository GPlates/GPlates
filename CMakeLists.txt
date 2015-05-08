project(GPlates)

cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)

# Where to find our CMake modules (this variable is visible in subdirectories).
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules/")

# Include configuration options (default options and options overridden by user).
include (ConfigCMake)

#
# CMake version 2.6 introduces policies to tell CMake to behave like a certain version ( > 2.4 ).
#
include (Policies_26)

#
# Recurse into source code directory.
#
add_subdirectory(src)

#
# Recurse into documentation directory.
#
add_subdirectory(doc)
