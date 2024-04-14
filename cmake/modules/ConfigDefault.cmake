#
# Useful CMake variables.
#
# Many of these variables are cache variables that can be configured using 'cmake -D <name>=<value> ...', or via ccmake or cmake-gui.
# This file doesn't need to be edited in those cases.
#


# A short description of the GPlates (or pyGPlates) project (only a few words).
#
# CMake (>= 3.16) uses this as the first line of Debian package description and Debian doesn't want first word to be same name as package name ('GPlates' or 'PyGPlates').
if (GPLATES_BUILD_GPLATES) # GPlates ...
	set(GPLATES_PACKAGE_DESCRIPTION_SUMMARY "Desktop software for the interactive visualisation of plate tectonics.")
else() # pyGPlates ...
	set(GPLATES_PACKAGE_DESCRIPTION_SUMMARY "Python library for fine-grained access to GPlates functionality.")
endif()


# A longer description of the GPlates (or pyGPlates) project.
if (GPLATES_BUILD_GPLATES) # GPlates ...

	set(GPLATES_PACKAGE_DESCRIPTION [[
GPlates is a plate-tectonics program. Manipulate reconstructions of geological and paleo-geographic features through geological time. Interactively visualize vector, raster and volume data.

Documentation is available at https://www.gplates.org/docs

GPlates can:
- handle and visualise data in a variety of geometries and formats, including raster data
- link plate kinematics to geodynamic models
- serve as an interactive client in a grid-computing network
- facilitate the production of high-quality paleo-geographic maps

Features of GPlates
-------------------

Feature Data IO - Load/save geological, geographic and tectonic feature data.

Data Visualization - Visualize vector/raster data on a globe or in one of the map projections.

Reconstruction Tools - Modify reconstructions graphically.

Cookie-cutting - Assign reconstruction/rotation poles to feature data by cookie-cutting with plate polygons.

3D Scalar Data - Visualize sub-surface 3D scalar fields.

Surface Velocities - Calculate surface velocities in topological plate polygons and deforming meshes.

Reconstruct Data - Reconstruct/rotate feature data (vector and raster data).

Export Data - Export reconstructed data as a time-sequence of exported files.

Edit Feature - Query and edit feature properties and geometries.

Deforming - Track crustal extension/contraction inside deforming regions.
]])

else() # pyGPlates ...

	set(GPLATES_PACKAGE_DESCRIPTION [[
PyGPlates is the GPlates Python library enabling fine-grained access to GPlates functionality.

Documentation is available at https://www.gplates.org/docs/pygplates/index.html
]])

endif()


# The GPlates (or pyGPlates) package vendor.
set(GPLATES_PACKAGE_VENDOR "Earthbyte project")


# The GPlates (or pyGPlates) package contact (Debian requires a name and email address - so use format 'FirstName LastName <EmailAddress>').
#
# NOTE: Leave it as the *empty* string here (so it doesn't get committed to source code control).
#       It is currently only needed when creating Debian packages (using cpack) where the developer should set it using 'cmake -D', or CMake GUI, or 'ccmake'.
set(GPLATES_PACKAGE_CONTACT "" CACHE STRING "Package contact/maintainer. Use format 'FirstName LastName <EmailAddress>'.")


# The GPlates (or pyGPlates) package license.
set(GPLATES_PACKAGE_LICENSE [[
Copyright (C) 2003-2024 The University of Sydney, Australia.
Copyright (C) 2007-2024 The Geological Survey of Norway.
Copyright (C) 2004-2024 California Institute of Technology.
This is free software. You may redistribute copies of it under the terms of
the GNU General Public License version 2 <http://www.gnu.org/licenses/gpl.html>.
There is NO WARRANTY, to the extent permitted by law.
]])


# The GPlates copyright - string version to be used in a source file.
set(GPLATES_COPYRIGHT_STRING [[
Copyright (C) 2003-2024 The University of Sydney, Australia
Copyright (C) 2004-2024 California Institute of Technology
Copyright (C) 2007-2024 The Geological Survey of Norway

The GPlates source code also contains code derived from:
 * ReconTreeViewer (James Boyden)
 * Boost intrusive_ptr (Peter Dimov)
 * Loki ScopeGuard (Andrei Alexandrescu, Petru Marginean, Joshua Lehrer)
 * Loki RefToValue (Richard Sposato, Peter Kummel)

The GPlates source tree additionally contains icons from the GNOME desktop
environment, the Inkscape vector graphics editor and the Tango icon library.
]])

# The GPlates copyright for html.
set(GPLATES_HTML_COPYRIGHT_STRING [[
<html><body>
Copyright &copy; 2003-2024 The University of Sydney, Australia<br />
Copyright &copy; 2004-2024 California Institute of Technology<br />
Copyright &copy; 2007-2024 The Geological Survey of Norway<br />
<br />

The GPlates source code also contains code derived from: <ul>
 <li> ReconTreeViewer (James Boyden) </li>
 <li> Boost intrusive_ptr (Peter Dimov) </li>
 <li> Loki ScopeGuard (Andrei Alexandrescu, Petru Marginean, Joshua Lehrer) </li>
 <li> Loki RefToValue (Richard Sposato, Peter Kummel) </li>
</ul>

The GPlates source tree additionally contains icons from the GNOME desktop
environment, the Inkscape vector graphics editor and the Tango icon library.
</body></html>
]])

# The pyGPlates copyright - string version to be used in Python API documentation.
# We don't include the word 'Copyright' since we're using Sphinx for documentation and it prepends it to our copyright string.
set(PYGPLATES_DOCS_COPYRIGHT_STRING [[
(C) 2003-2024 The University of Sydney, Australia
(C) 2004-2024 California Institute of Technology
(C) 2007-2024 The Geological Survey of Norway
]])


# GPLATES_PUBLIC_RELEASE - Official public release (GPlates or pyGPlates depending on GPLATES_BUILD_GPLATES).
#
# Official public releases disable all warnings.
# Also defines a compiler flag GPLATES_PUBLIC_RELEASE (see 'src/global/config.h.in').
#
if (GPLATES_BUILD_GPLATES) # GPlates ...
	# First remove cache variable (eg, leftover if switching from a pyGPlates build to GPlates by enabling GPLATES_BUILD_GPLATES).
	unset(GPLATES_PUBLIC_RELEASE CACHE)
	# If GPLATES_VERSION_PRERELEASE_SUFFIX is empty then it's an offical public GPlates release (eg, 2.3.0).
	if (GPLATES_VERSION_PRERELEASE_SUFFIX)
		set(GPLATES_PUBLIC_RELEASE false)
	else()
		set(GPLATES_PUBLIC_RELEASE true)
	endif()
else() # pyGPlates ...
	# Currently the pyGPlates major version is zero and the minor version increments each time the API is changed
	# (including internal releases) and so the pre-release suffix is typically left empty, so we can't really use
	# the presence of the pre-release suffix to distinguish public releases (like we do with GPlates).
	# So we just default to false and rely on the developer setting it to true for official releases
	# (eg, using 'cmake -D GPLATES_PUBLIC_RELEASE:BOOL=TRUE ...', or via ccmake or cmake-gui).
	option(GPLATES_PUBLIC_RELEASE "PyGPlates official public release." false)
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


# Only GPlates has option to install geodata (we don't distribute it with pyGPlates).
#
if (GPLATES_BUILD_GPLATES) # GPlates ...
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
else() # pyGPlates ...
	# Remove cache variables (eg, leftover if switching from a GPlates build to pyGPlates by disabling GPLATES_BUILD_GPLATES).
	unset(GPLATES_INSTALL_GEO_DATA CACHE)
	unset(GPLATES_INSTALL_GEO_DATA_DIR CACHE)
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
if (COMMAND target_precompile_headers)
	option(GPLATES_USE_PRECOMPILED_HEADERS "Use pre-compiled headers to speed up build times." false)
endif()

if (MSVC)
	# When using Visual Studio this shows included headers (used by 'list_external_includes.py').
	# This disables pre-compiled headers (regardless of value of 'GPLATES_USE_PRECOMPILED_HEADERS').
	set(GPLATES_MSVC_SHOW_INCLUDES false)
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
	#
	# Allow user to specify the number of parallel build processes (defaults to zero which indicates uses all available CPUs).
	set(GPLATES_MSVC_PARALLEL_BUILD_PROCESSES 0 CACHE STRING "Number of parallel build processes (if GPLATES_MSVC_PARALLEL_BUILD enabled). Set to zero for max.")
endif()


# Specify which source directories (relative to the 'doc/' directory) should be scanned by doxygen.
set(GPLATES_DOXYGEN_INPUT
    "../src/feature-visitors ../src/file-io ../src/model ../src/property-values ../src/utils")


# The location of the GPlates executable is placed here when it is built (but not installed).
# Note that this is different from the "RUNTIME DESTINATION" in the "install" command which specifies
# the suffix path of where the installed executable goes (versus the built executable).
#
# Set default location of built (not installed) executables on all platforms and DLLs on Windows.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
# Set default location for built (not installed) shared libraries on non-Windows platforms.
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

# Order the include directories so that directories which are in the source or build tree always
# come before directories outside the project.
set(CMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE true)
