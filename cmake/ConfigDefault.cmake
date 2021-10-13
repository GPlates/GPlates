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

# The GPlates package vendor.
set(GPLATES_PACKAGE_VENDOR "Earthbyte project")

# A short description of the GPlates project (only a few words).
set(GPLATES_PACKAGE_DESCRIPTION_SUMMARY "GPlates is desktop software for the interactive visualisation of plate-tectonics.")

# The GPlates package version.
set(GPLATES_PACKAGE_VERSION_MAJOR "1")
set(GPLATES_PACKAGE_VERSION_MINOR "1")
set(GPLATES_PACKAGE_VERSION_PATCH "1")

# The GPlates package version.
set(GPLATES_PACKAGE_VERSION "${GPLATES_PACKAGE_VERSION_MAJOR}.${GPLATES_PACKAGE_VERSION_MINOR}.${GPLATES_PACKAGE_VERSION_PATCH}")

# The current GPlates version.
set(GPLATES_VERSION_STRING "${GPLATES_PACKAGE_NAME} ${GPLATES_PACKAGE_VERSION}")

# The GPlates copyright - string version to be used in a source file.
set(GPLATES_COPYRIGHT_STRING "")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING}Copyright (C) 2003-2011 The University of Sydney, Australia\\n")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING}Copyright (C) 2004-2011 California Institute of Technology\\n")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING}Copyright (C) 2007-2011 The Geological Survey of Norway\\n")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING}\\n")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING}The GPlates source code also contains code derived from:\\n")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING} * ReconTreeViewer (James Boyden)\\n")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING} * Boost intrusive_ptr (Peter Dimov)\\n")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING} * Boost fpclassify (John Maddock)\\n")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING} * Loki ScopeGuard (Andrei Alexandrescu, Petru Marginean, Joshua Lehrer)\\n")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING} * Loki RefToValue (Richard Sposato, Peter Kummel)\\n")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING}\\n")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING}The GPlates source tree additionally contains icons from the GNOME desktop\\n")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING}environment, the Inkscape vector graphics editor and the Tango icon library.")

# The GPlates copyright for html.
set(GPLATES_HTML_COPYRIGHT_STRING "")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}<html><body>\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}Copyright &copy; 2003-2011 The University of Sydney, Australia<br />\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}Copyright &copy; 2004-2011 California Institute of Technology<br />\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}Copyright &copy; 2007-2011 The Geological Survey of Norway<br />\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}<br />\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}The GPlates source code also contains code derived from: <ul>\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING} <li> ReconTreeViewer (James Boyden) </li>\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING} <li> Boost intrusive_ptr (Peter Dimov) </li>\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING} <li> Boost fpclassify (John Maddock) </li>\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING} <li> Loki ScopeGuard (Andrei Alexandrescu, Petru Marginean, Joshua Lehrer) </li>\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING} <li> Loki RefToValue (Richard Sposato, Peter Kummel) </li>\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}</ul>\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}The GPlates source tree additionally contains icons from the GNOME desktop\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}environment, the Inkscape vector graphics editor and the Tango icon library.\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}</body></html>\\n")

# The subversion revision of the GPlates source code.
# This is manually set when making GPlates *public* releases.
# However, when making internal releases or just an ordinary developer build, leave it
# empty; if it is empty, the revision number is automatically populated for you on build.
set(GPLATES_SOURCE_CODE_CONTROL_VERSION_STRING "11903")

# List the Qt plugins used by GPlates.
# This is needed for packaging standalone versions of GPlates for a binary installer.
# NOTE: each plugin listed should have its own double quotes as these are list variables.
# The paths listed should be relative to the Qt 'plugins' directory which can be found
# by typing 'qmake -query QT_INSTALL_PLUGINS' at the command/shell prompt.
# Each platform has a different name for the library(s).
# The binary packagers for Mac OS X and Windows will copy these plugins
# into a standalone location to be included in the installer and, in the case of Mac OS X,
# will fixup the references to point to libraries inside the app bundle.
# Note: not really required for Linux since the binary installer will use the
# Debian/RPM package manager to install Qt (and its plugins) on the target machine.
set(GPLATES_QT_PLUGINS_MACOSX 
	"imageformats/libqjpeg.dylib"
	"imageformats/libqgif.dylib"
	"imageformats/libqico.dylib"
	"imageformats/libqmng.dylib"
	"imageformats/libqsvg.dylib"
	"imageformats/libqtiff.dylib"
	)
set(GPLATES_QT_PLUGINS_WIN32 
	"imageformats/qjpeg4.dll"
	)
set(GPLATES_QT_PLUGINS_LINUX 
	"imageformats/libqjpeg.so"
	)

# Extra files/directories that should be included with the binary installer.
# Paths must be full paths (eg, '~/sample-data' is ok but '../sample-data' is not).
# NOTE: each file/directory listed should have its own double quotes as this is a list variable.
# NOTE: it's better to override this in 'cmake/ConfigUser.cmake' instead of changing it here
# to make sure this change does not inadvertently get checked into source code control.
set(GPLATES_BINARY_INSTALL_EXTRAS "")

# Set to 'true' if this is a public code release (to non-developers).
# Currently turns off warnings and any errors caused by them (because warnings are treated as errors).
# And also defines a compiler flag GPLATES_PUBLIC_RELEASE.
set(GPLATES_PUBLIC_RELEASE true)

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

# The following is from http://mail.kde.org/pipermail/kde-buildsystem/2008-November/005112.html...
#
# "The way to identify whether a generator is multi-configuration is to
# check whether CMAKE_CONFIGURATION_TYPES is set.  The VS/XCode generators
# set it (and ignore CMAKE_BUILD_TYPE).  The Makefile generators do not
# set it (and use CMAKE_BUILD_TYPE).  If CMAKE_CONFIGURATION_TYPES is not
# already set, don't set it."
#
IF(NOT CMAKE_CONFIGURATION_TYPES)
    IF(NOT CMAKE_BUILD_TYPE)
      # Should we set build type to RelWithDebInfo for developers and
      # to Release for general public (ie when GPLATES_SOURCE_RELEASE is true) ?
      # Currently it's Release for both.
      SET(CMAKE_BUILD_TYPE Release CACHE STRING
          "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel ${extra_build_configurations}."
          FORCE)
    ENDIF(NOT CMAKE_BUILD_TYPE)
ENDIF(NOT CMAKE_CONFIGURATION_TYPES)

# The location of the GPlates executable is placed here when it is built (but not installed).
# Note that this is different from the "RUNTIME DESTINATION" in the "install" command which specifies
# the suffix path of where the installed executable goes (versus the built executable).
# This variable is not advised for cmake 2.6 and above since it doesn't provide as fine grained control
# as the new RUNTIME_OUTPUT_DIRECTORY, ARCHIVE_OUTPUT_DIRECTORY and LIBRARY_OUTPUT_DIRECTORY but it's
# fine for our use (a single executable and no libraries).
SET(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")

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

# Disable Python for release 1.1
set(GPLATES_NO_PYTHON true)

