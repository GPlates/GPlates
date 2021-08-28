#
# Useful CMAKE variables.
#
# Many of these variables are cache variables that can be configured using 'cmake -D <name>=<value> ...', or via ccmake or cmake-gui.
# This file doesn't need to be edited in those cases.
#


# A short description of the GPlates project (only a few words).
#
# CMake (>= 3.16) uses this as the first line of Debian package description and Debian doesn't want first word to be same name as package name (eg, 'gplates').
set(GPLATES_PACKAGE_DESCRIPTION_SUMMARY "Desktop software for the interactive visualisation of plate tectonics.")


# The GPlates package vendor.
set(GPLATES_PACKAGE_VENDOR "Earthbyte project")


# The GPlates package contact (Debian requires a name and email address - so use format 'FirstName LastName <EmailAddress>').
#
# NOTE: Leave it as the *empty* string here (so it doesn't get committed to source code control).
#       It is currently only needed when creating Debian packages (using cpack) where the developer should set it using 'cmake -D', or CMake GUI, or 'ccmake'.
set(GPLATES_PACKAGE_CONTACT "" CACHE STRING "Package contact/maintainer. Use format 'FirstName LastName <EmailAddress>'.")


# The GPlates copyright - string version to be used in a source file.
set(GPLATES_COPYRIGHT_STRING "")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING}Copyright (C) 2003-2021 The University of Sydney, Australia\\n")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING}Copyright (C) 2004-2021 California Institute of Technology\\n")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING}Copyright (C) 2007-2021 The Geological Survey of Norway\\n")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING}\\n")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING}The GPlates source code also contains code derived from:\\n")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING} * ReconTreeViewer (James Boyden)\\n")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING} * Boost intrusive_ptr (Peter Dimov)\\n")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING} * Loki ScopeGuard (Andrei Alexandrescu, Petru Marginean, Joshua Lehrer)\\n")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING} * Loki RefToValue (Richard Sposato, Peter Kummel)\\n")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING}\\n")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING}The GPlates source tree additionally contains icons from the GNOME desktop\\n")
set(GPLATES_COPYRIGHT_STRING "${GPLATES_COPYRIGHT_STRING}environment, the Inkscape vector graphics editor and the Tango icon library.")

# The GPlates copyright for html.
set(GPLATES_HTML_COPYRIGHT_STRING "")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}<html><body>\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}Copyright &copy; 2003-2021 The University of Sydney, Australia<br />\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}Copyright &copy; 2004-2021 California Institute of Technology<br />\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}Copyright &copy; 2007-2021 The Geological Survey of Norway<br />\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}<br />\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}The GPlates source code also contains code derived from: <ul>\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING} <li> ReconTreeViewer (James Boyden) </li>\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING} <li> Boost intrusive_ptr (Peter Dimov) </li>\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING} <li> Loki ScopeGuard (Andrei Alexandrescu, Petru Marginean, Joshua Lehrer) </li>\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING} <li> Loki RefToValue (Richard Sposato, Peter Kummel) </li>\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}</ul>\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}The GPlates source tree additionally contains icons from the GNOME desktop\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}environment, the Inkscape vector graphics editor and the Tango icon library.\\n")
set(GPLATES_HTML_COPYRIGHT_STRING "${GPLATES_HTML_COPYRIGHT_STRING}</body></html>\\n")


# GPLATES_VERSION_PRERELEASE - Pre-release version (empty for official public release).
#
# This should be:
# - empty for official public releases,
# - a number for development releases (eg, 1, 2, etc),
# - 'alpha' followed by '.' followed by a number for alpha releases (eg, alpha.1, alpha.2, etc),
# - 'beta' followed by '.' followed by a number for beta releases (eg, beta.1, beta.2, etc),
# - 'rc' followed by '.' followed by a number for release candidates (eg, rc.1, rc.2, etc).
#
# The reason for the above rules is they support the correct version ordering precedence for both Semantic Versioning and Debian versioning
# (even though Semantic and Debian versioning have slightly different precedence rules).
#
# Semantic version precedence separates identifiers between dots and compares each identifier.
# According to https://semver.org/spec/v2.0.0.html ...
# - digit-only identifiers are compared numerically,
# - identifiers with letters are compared lexically in ASCII order,
# - numeric identifiers have lower precedence than non-numeric identifiers.
# ...and so '1' < 'beta.1' because '1' < 'beta', and 'beta.1' < 'beta.2' because 'beta' == 'beta' but '1' < '2'.
#
# Debian version precedence separates identifiers into alternating non-digit and digit identifiers.
# According to https://www.debian.org/doc/debian-policy/ch-controlfields.html#version ...
# - find initial part consisting only of non-digits and compare lexically in ASCII order (modified so letters sort earlier than non-letters, and '~' earliest of all),
# - find next part consisting only of digits and compare numerically,
# - repeat the above two steps until a difference is found.
# ...and so '1' < 'beta.1' because '' < 'beta.', and 'beta.1' < 'beta.2' because 'beta.' == 'beta.' but '1' < '2'.
#
# For example:
# For Semantic Versioning: 2.3.0-1 < 2.3.0-alpha.1 < 2.3.0-beta.1 < 2.3.0-rc.1 < 2.3.0.
# For Debian versioning:   2.3.0~1 < 2.3.0~alpha.1 < 2.3.0~beta.1 < 2.3.0~rc.1 < 2.3.0.
set(GPLATES_VERSION_PRERELEASE "" CACHE STRING "Pre-release version suffix (eg, '1', 'alpha.1', 'beta.1', 'rc.1'). Empty means official public release.")
#
# Make sure pre-release contains only dot-separated alphanumeric identifiers.
if (GPLATES_VERSION_PRERELEASE)
	if (NOT GPLATES_VERSION_PRERELEASE MATCHES [[^[a-zA-Z0-9\.]+$]])
		message(FATAL_ERROR "GPLATES_VERSION_PRERELEASE should only contain dot-separated alphanumeric identifiers")
	endif()
endif()

# GPLATES_PACKAGE_VERSION      - Full package version (the actual version as dictated by Semantic Versioning).
# GPLATES_PACKAGE_VERSION_NAME - Full package version name for use in package filenames (or any string the user might see).
#
# Currently the only difference is GPLATES_PACKAGE_VERSION_NAME inserts 'dev' for development releases
# (but GPLATES_PACKAGE_VERSION does not, in order to maintain the correct version precendence, eg, '1' < 'alpha.1' as desired but 'dev.1' > 'alpha.1').
#
# For a public release both variables are equivalent to PROJECT_VERSION (as set by project() command in top-level CMakeLists.txt).
set(GPLATES_PACKAGE_VERSION ${PROJECT_VERSION})
set(GPLATES_PACKAGE_VERSION_NAME ${PROJECT_VERSION})
#
# For a pre-release append the pre-release version (using a hyphen for pre-releases as dictated by Semantic Versioning).
if (GPLATES_VERSION_PRERELEASE)
	set(GPLATES_PACKAGE_VERSION "${GPLATES_PACKAGE_VERSION}-${GPLATES_VERSION_PRERELEASE}")
	# A human-readable pre-release version name (unset/empty for official public releases).
	#
	# If a development release (ie, if pre-release version is just a number) then insert 'dev' into the version *name* to make it more obvious to users.
	# Note: We don't insert 'dev' into the version itself because that would give it a higher version ordering precedence than 'alpha' and 'beta' (since a < b < d).
	#       Keeping only the development number in the actual version works because digits have lower precedence than non-digits (according to Semantic and Debian versioning).
	if (GPLATES_VERSION_PRERELEASE MATCHES "^[0-9]+$")
		set(GPLATES_VERSION_PRERELEASE_NAME "dev${GPLATES_VERSION_PRERELEASE}")
	else()
		set(GPLATES_VERSION_PRERELEASE_NAME "${GPLATES_VERSION_PRERELEASE}")
	endif()
	set(GPLATES_PACKAGE_VERSION_NAME "${GPLATES_PACKAGE_VERSION_NAME}-${GPLATES_VERSION_PRERELEASE_NAME}")
endif()

# GPLATES_PUBLIC_RELEASE - Official public release.
#
# If GPLATES_VERSION_PRERELEASE is empty then GPLATES_PUBLIC_RELEASE is set to true to mark this as an official public release.
#
# Official public releases disable all warnings.
# Also defines a compiler flag GPLATES_PUBLIC_RELEASE (see 'src/global/config.h.in').
if (GPLATES_VERSION_PRERELEASE)
	set(GPLATES_PUBLIC_RELEASE false)
else()
	set(GPLATES_PUBLIC_RELEASE true)
endif()


# Whether to install GPlates (or pyGPlates) as a standalone bundle (by copying dependency libraries during installation).
#
# When this is true then we install code to fix up GPlates (or pyGPlates) for deployment to another machine
# (which mainly involves copying dependency libraries into the install location, which subsequently gets packaged).
# When this is false then we don't install dependencies, instead only installing the GPlates executable (or pyGPlates library) and a few non-dependency items.
if (WIN32 OR APPLE)
	# On Windows and Apple this is *enabled* by default since we typically distribute a self-contained package to users on those systems.
	# However this can be *disabled* for use cases such as creating a conda package (since conda manages dependency installation itself).
	set(_INSTALL_STANDALONE true)
else() # Linux
	# On Linux this is *disabled* by default since we rely on the Linux binary package manager to install dependencies on the user's system
	# (for example, we create a '.deb' package that only *lists* the dependencies, which are then installed on the target system if not already there).
	# However this can be *enabled* for use cases such as creating a standalone bundle for upload to a cloud service (where it is simply extracted).
	set(_INSTALL_STANDALONE false)
endif()
option(GPLATES_INSTALL_STANDALONE "Install GPlates (or pyGPlates) as a standalone bundle (copy dependency libraries into the installation)." ${_INSTALL_STANDALONE})
unset(_INSTALL_STANDALONE)


# Whether to install geodata (eg, in the binary installer) or not.
# By default this is false but should be enabled when packaging a public release.
#
# Developers may want to turn this on, using the cmake command-line or cmake GUI, even when not releasing a public build.
option(GPLATES_INSTALL_GEO_DATA "Install geodata (eg, in the binary installer)." false)

# The directory location of the geodata.
# The geodata is only included in the binary installer if 'GPLATES_INSTALL_GEO_DATA' is true.
# Paths must be full paths (eg, '~/geodata' is ok but '../geodata' is not).
set(GPLATES_INSTALL_GEO_DATA_DIR "" CACHE PATH "Location of geodata (use absolute path).")
#
# If we're installing geodata then make sure the source geodata directory has been specified, is an absolute path and exists.
if (GPLATES_INSTALL_GEO_DATA)
	if (NOT GPLATES_INSTALL_GEO_DATA_DIR)
		message(FATAL_ERROR "Please specify GPLATES_INSTALL_GEO_DATA_DIR when you enable GPLATES_INSTALL_GEO_DATA")
	endif()
	if (NOT IS_ABSOLUTE "${GPLATES_INSTALL_GEO_DATA_DIR}")
		message(FATAL_ERROR "GPLATES_INSTALL_GEO_DATA_DIR should be an absolute path (not a relative path)")
	endif()
	if (NOT EXISTS "${GPLATES_INSTALL_GEO_DATA_DIR}")
		message(FATAL_ERROR "GPLATES_INSTALL_GEO_DATA_DIR does not exist: ${GPLATES_INSTALL_GEO_DATA_DIR}")
	endif()
	file(TO_CMAKE_PATH ${GPLATES_INSTALL_GEO_DATA_DIR} GPLATES_INSTALL_GEO_DATA_DIR) # Convert '\' to '/' in paths.
endif()


# The macOS code signing identity used to sign installed/packaged GPlates application bundle with a Developer ID certificate.
#
# NOTE: Leave it as the *empty* string here (so it doesn't get committed to source code control).
#       Also it is not needed for local builds (ie, when running GPlates/pyGPlates on build machine), it's only needed when deploying to other machines.
#       When deploying, the developer is responsible for setting it to their Developer ID (eg, using 'cmake -D', or CMake GUI, or 'ccmake').
#       To create a Developer ID certificate the developer first needs to create an Apple developer account and pay a yearly fee.
#       This can be done as an individual or as a company (the latter requiring a company ID such as a company number).
#       After that's all done and a Developer ID certificate has been created, it should typically be installed into the Keychain.
#       It should have a name like "Developer ID Application: <ID>" thus allowing GPlates/pyGPlates to be configured with (for example):
#
#           cmake -D GPLATES_APPLE_CODE_SIGN_IDENTITY:STRING="Developer ID Application: <ID>" -S <source-dir> -B <build-dir>
#
#       Once a GPlates/pyGPlates package has been created for deployment (using 'cpack') the final step is to get Apple to notarize it (see 'Install.cmake' for details).
#       Only then will Apple's security checks pass when users run/install the package on their machines.
if (APPLE)
	set(GPLATES_APPLE_CODE_SIGN_IDENTITY "" CACHE STRING "Apple code signing identity.")
endif()


# We compile with Python 3 (by default).
#
# However developers can choose to compile with Python 2 instead.
option(GPLATES_PYTHON_3 "Compile with Python 3 (not Python 2)." true)


# Whether to enable GPlates custom CPU profiling functionality.
#
# Is false by default. However note that the custom build type 'ProfileGplates' effectively
# overrides by adding 'GPLATES_PROFILE_CODE' directly as a compiler flag. Other build types
# set it indirectly via '#cmakedefine GPLATES_PROFILE_CODE' in 'config.h.in' using the
# same-named CMake variable (ie, the option below).
#
# Usually it's easiest to just select the 'ProfileGplates' build type (note that enabling/disabling
# the option below then has no effect). However, being a custom build type, that sometimes creates problems
# (eg, the CGAL dependency does not always play nicely with custom build types). In this case you can choose
# the builtin 'Release' build type (for example) and enable this option to achieve the same affect.
option(GPLATES_PROFILE_CODE "Enable GPlates custom CPU profiling functionality." false)


# Pre-compiled headers are turned off by default.
#
# Developers may want to turn this on using the cmake command-line or cmake GUI.
#
# Only CMake 3.16 and above support pre-compiled headers natively.
if (COMMAND target_precompile_headers)
	option(GPLATES_USE_PRECOMPILED_HEADERS "Use pre-compiled headers to speed up build times." false)
endif()

if (MSVC)
	# When using Visual Studio this shows included headers (used by 'list_external_includes.py').
	# This disables pre-compiled headers (regardless of value of 'GPLATES_USE_PRECOMPILED_HEADERS').
	#
	# Developers may want to turn this on using the cmake command-line or cmake GUI.
	option(GPLATES_MSVC_SHOW_INCLUDES "Show included headers. Used when configuring pre-compiled headers." false)
	mark_as_advanced(GPLATES_MSVC_SHOW_INCLUDES)
	# Disable pre-compiled headers if showing include headers.
	# The only reason to show include headers is to use 'list_external_includes.py' script to generates pch header.
	if (GPLATES_MSVC_SHOW_INCLUDES)
		# Note: This sets the non-cache variable ('option' above sets the cache variable of same name).
		#       The non-cache variable will get precedence when subsequently accessed.
		#       It's also important to set this *after* 'option' since, prior to CMake 3.21, whenever a cache variable is added
		#       (eg, on the first run if not yet present in "CMakeCache.txt") the normal variable is removed.
		if (DEFINED GPLATES_USE_PRECOMPILED_HEADERS)
			set(GPLATES_USE_PRECOMPILED_HEADERS false)
		endif()
	endif()

	# If Visual Studio then enable parallel builds within a project.
	#
	# To also enable parallel project builds set
	# Tools->Options->Programs and Solutions->Build and Run->maximum number of parallel project builds to
	# the number of cores on your CPU.
	#
	# This is on by default otherwise compilation will take a long time.
	option(GPLATES_MSVC_PARALLEL_BUILD "Enable parallel builds within each Visual Studio project." true)
endif()


# Specify which source directories (relative to the 'doc/' directory) should be scanned by doxygen.
set(GPLATES_DOXYGEN_INPUT
    "../src/feature-visitors ../src/file-io ../src/model ../src/property-values ../src/utils")


# The location of the GPlates executable is placed here when it is built (but not installed).
# Note that this is different from the "RUNTIME DESTINATION" in the "install" command which specifies
# the suffix path of where the installed executable goes (versus the built executable).
#
# Set default location of built (not installed) executables on all platforms and DLLs on Windows.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${GPlates_BINARY_DIR}/bin")
# Set default location for built (not installed) shared libraries on non-Windows platforms.
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${GPlates_BINARY_DIR}/bin")

# Set the minimum C++ language standard to C++11.
#
# std::auto_ptr was deprecated in C++11 (and removed in C++17), so we now use std::unique_ptr introduced in C++11.
# Also GDAL 2.3 and above require C++11.
# And CGAL 5.x requires C++14, and this requirement is transitively passed to us via the CGAL::CGAL target
# where CMake chooses the stricter requirement between our C++11 and CGAL's C++14 (which is C++14).
#
# Also note that this avoids the problem of compile errors, when C++14 features are used (eg, by CGAL 5),
# due to forcing C++11 by specifying '-std=c++11' directly on the compiler command-line.
#
# Note: This is also set using 'target_compile_features()' in src/CMakeLists.txt (for CMake >= 3.8).
#       So this can be removed once our minimum CMake requirement is 3.8.
if (CMAKE_VERSION VERSION_LESS 3.8)
	set(CMAKE_CXX_STANDARD 11)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

# Disable compiler-specific extensions.
# Eg, for C++11 on GNU compilers we want '-std=c++11' instead of '-std=g++11'.
set(CMAKE_CXX_EXTENSIONS OFF)

# Order the include directories so that directories which are in the source or build tree always
# come before directories outside the project.
set(CMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE true)
