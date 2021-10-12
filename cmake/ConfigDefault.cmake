#
# Useful CMAKE variables.
#

# There are two configuration files:
#   1) "ConfigDefault.cmake" - is version controlled and used to add new default variables and set defaults for everyone.
#   2) "ConfigUser.cmake" - is not version controlled (currently listed in svn:ignore property) and used to override defaults on a per-user basis.
#
# NOTE: If you want to change CMake behaviour just for yourself then modify the "ConfigUser.cmake" file (not "ConfigDefault.cmake").
#

# The GPlates package name.
set(GPLATES_PACKAGE_NAME "GPlates")

# The GPlates package version.
set(GPLATES_PACKAGE_VERSION "0.9.4")

# The current GPlates version.
set(GPLATES_VERSION_STRING "${GPLATES_PACKAGE_NAME} ${GPLATES_PACKAGE_VERSION}")

# Set to 'true' if this is a source code release (to non-developers).
# Currently turns off warnings and any errors caused by them (because warnings are treated as errors).
set(GPLATES_SOURCE_RELEASE false)

# Pre-compiled headers are turned off by default as they are not implicitly supported by CMake.
# Developers of GPlates may want to turn them on in their 'ConfigUser.cmake' file to speed up build times.
set(GPLATES_USE_PCH false)

# Specify which source directories (relative to the 'doc/' directory) should be scanned by doxygen.
set(GPLATES_DOXYGEN_INPUT
    "../src/feature-visitors ../src/file-io ../src/model ../src/property-values ../src/utils")

# You can set the build configuration type as a command-line argument to 'cmake' using -DCMAKE_BUILD_TYPE:STRING=Debug for example.
# If no build configuration type was given as a command-line option to 'cmake' then a default cache entry is set here.
# A cache entry is what appears in the 'CMakeCache.txt' file that CMake generates - you can edit that file directly or use the CMake GUI to edit it.
# The user can then set this parameter via the CMake GUI before generating the native build system.
# NOTE: this is not needed for Visual Studio because it has multiple configurations in the IDE (and CMake includes them all).
# However Makefile generators can only have one build type (to have multiple build types you'll need multiple out-of-place builds - one for each build type).
#
# The following are some valid build configuration types:
# 1) Debug - no optimisation with debug info.
# 2) Release - release build optimised for speed.
# 3) RelWithDebInfo - release build optimised for speed with debug info.
# 4) MinSizeRel - release build optimised for size.
IF(NOT MSVC)
    IF(NOT CMAKE_BUILD_TYPE)
      SET(CMAKE_BUILD_TYPE Release CACHE STRING
          "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel ${extra_build_configurations}."
          FORCE)
    ENDIF(NOT CMAKE_BUILD_TYPE)
ENDIF(NOT MSVC)

# Turn this on if you want to...
#	Unix: see compiler commands echoed to console and messages about make entering and leaving directories.
#	VisualStudio: see compiler commands.
# Setting CMAKE_VERBOSE_MAKEFILE to 'true'...
#	Unix: puts 'VERBOSE=1' in the top Makefile.
#	VisualStudio: sets SuppressStartupBanner to FALSE.
# If CMAKE_VERBOSE_MAKEFILE is set to 'false' and you want to turn on verbosity temporarily you can...
#	Unix: type 'make VERBOSE=1'  on the command-line when building.
#	VisualStudio: change SuppressStartupBanner to 'no' in "project settings->configuration properties->*->general".
set(CMAKE_VERBOSE_MAKEFILE false)

# Order the include directories so that directories which are in the source or build tree always
# come before directories outside the project.
set(CMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE true)

# Set this to true if you don't want to rebuild the object files if the rules have changed,
# but not the actual source files or headers (e.g. if you changed some compiler switches) .
set(CMAKE_SKIP_RULE_DEPENDENCY false)

# Automatically add CMAKE_CURRENT_SOURCE_DIR and CMAKE_CURRENT_BINARY_DIR to the include directories in every processed CMakeLists.txt.
# It behaves as if you had used INCLUDE_DIRECTORIES in every CMakeLists.txt file or your project.
# The added directory paths are relative to the being-processed CMakeLists.txt, which is different in each directory.
set(CMAKE_INCLUDE_CURRENT_DIR false)

# This will cause CMake to not put in the rules that re-run CMake.
# Use this if you want to avoid Visual Studio asking to reload a project in the middle of a build,
# or Unix makefiles changing in middle of build, because a 'CMakeList.txt' file was changed.
set(CMAKE_SUPPRESS_REGENERATION false)
