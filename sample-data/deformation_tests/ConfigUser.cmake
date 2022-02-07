# 
# This is a branch and machine type (Mac) specific of a 'ConfigUser.cmake' file.
# Copy to branch root directory's cmake/ dir to use, for example: 
#
# deformation-2014-oct-17/cmake/ConfigUser.cmake
#
# Use this file to override variables in 'ConfigDefault.cmake' on a per-user basis.
# This file is version controlled in a unit-test or sample data directory
# This file is NOT version controlled in the [BRANCH]/cmake/ directory.

# Pre-compiled headers are turned off by default.
# Developers of GPlates may want to turn them on here to speed up build times.
set(GPLATES_USE_PCH false)

# When using Visual Studio this shows included headers (used by 'list_external_includes.py').
# This disables pre-compiled headers (regardless of value of 'GPLATES_USE_PCH').
set(GPLATES_SHOW_INCLUDES false)

# If Visual Studio 2005 then enable parallel builds within a project.
#
# To also enable parallel project builds set
# “Tools->Options->Programs and Solutions->Build and Run->maximum number of parallel project builds” to
# the number of cores on your CPU.
set(GPLATES_MSVC80_PARALLEL_BUILD false)


# Extra files/directories that should be included with the binary installer.
# Paths must be full paths (eg, '~/sample-data' is ok but '../sample-data' is not).
# NOTE: each file/directory listed should have its own double quotes as this is a list variable.
#
set(GPLATES_BINARY_INSTALL_EXTRAS "/Users/mturner/work/repos/gplates/deformation-2014-oct-17/sample-data/deformation_tests/")
#
