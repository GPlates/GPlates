#
# Useful CMake variables.
#
# Many of these variables are cache variables that can be configured using 'cmake -D <name>=<value> ...', or via ccmake or cmake-gui.
# This file doesn't need to be edited in those cases.
#


#
# A longer description of the GPlates (or pyGPlates) project.
#
# Note: The short description is now in the 'project()' command in the root 'CMakeLists.txt' file.
#
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
Copyright (C) 2003-2026 The University of Sydney, Australia.
Copyright (C) 2007-2026 The Geological Survey of Norway.
Copyright (C) 2004-2026 California Institute of Technology.
This is free software. You may redistribute copies of it under the terms of
the GNU General Public License version 2 <http://www.gnu.org/licenses/gpl.html>.
There is NO WARRANTY, to the extent permitted by law.
]])


# The GPlates copyright - string version to be used in a source file.
set(GPLATES_COPYRIGHT_STRING [[
Copyright (C) 2003-2026 The University of Sydney, Australia
Copyright (C) 2004-2026 California Institute of Technology
Copyright (C) 2007-2026 The Geological Survey of Norway

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
Copyright &copy; 2003-2026 The University of Sydney, Australia<br />
Copyright &copy; 2004-2026 California Institute of Technology<br />
Copyright &copy; 2007-2026 The Geological Survey of Norway<br />
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
(C) 2003-2026 The University of Sydney, Australia
(C) 2004-2026 California Institute of Technology
(C) 2007-2026 The Geological Survey of Norway
]])


# Detect if this build is part of a conda build (eg, a "conda build ..." command).
#
# Note: Conda builds use scikit-build-core (because conda relies on 'pip install') which in turn defines the SKBUILD CMake variable.
#       A side note: Building using scikit-build-core happens when building wheels with pip (eg, 'pip wheel ...' or 'pip install ...').
#       However there are cases where we'd like to distinguish between scikit-build-core builds that are conda and non-conda.
#       An example is cross-compiling using conda where it uses the PYTHON environment variable to find the target platform Python (not build platform).
#       Maybe that'll also be required non-conda cross-compiles but we've not encountered them yet (and they don't set the PYTHON environment variable).
if (DEFINED ENV{CONDA_BUILD} AND ("$ENV{CONDA_BUILD}" EQUAL 1))
    set(GPLATES_CONDA_BUILD TRUE)
endif()


# GPLATES_PUBLIC_RELEASE - Official public release (GPlates or pyGPlates depending on GPLATES_BUILD_GPLATES).
#
# Official public releases disable all warnings.
# Also defines a compiler flag GPLATES_PUBLIC_RELEASE (see 'src/global/config.h.in').
#
# First remove cache variable (eg, leftover from older versions where a pyGPlates build would create it as a cache variable).
unset(GPLATES_PUBLIC_RELEASE CACHE)
if (GPLATES_BUILD_GPLATES) # GPlates ...
	# If GPLATES_VERSION_PRERELEASE_SUFFIX is empty then it's an offical public GPlates release (eg, 2.3.0).
	if (GPLATES_VERSION_PRERELEASE_SUFFIX)
		set(GPLATES_PUBLIC_RELEASE false)
	else()
		set(GPLATES_PUBLIC_RELEASE true)
	endif()
else() # pyGPlates ...
	# If PYGPLATES_VERSION_RELEASE_SUFFIX is empty then it's an offical public pyGPlates release (eg, 1.0.0).
	if (PYGPLATES_VERSION_RELEASE_SUFFIX)
		set(GPLATES_PUBLIC_RELEASE false)
	else()
		set(GPLATES_PUBLIC_RELEASE true)
	endif()
endif()


# GPLATES_INSTALL_STANDALONE - Whether to install GPlates (or pyGPlates) as a standalone bundle (by copying dependency libraries during installation).
#
# When this is true then we install code to fix up GPlates (or pyGPlates) for deployment to another machine
# (which mainly involves copying dependency libraries into the install location, which subsequently gets packaged).
# When this is false then we don't install dependencies, instead only installing the GPlates executable (or pyGPlates library) and a few non-dependency items.
#
if (SKBUILD)
	# We're building using scikit-build-core. This happens when building pyGPlates wheels with pip (eg, 'pip wheel ...' or 'pip install ...').
	# And conda also builds using scikit-build-core (because conda relies on 'pip install').
	if (GPLATES_CONDA_BUILD)
		# Conda does NOT need a standalone installation since conda manages binary shared library dependencies itself.
		set(_INSTALL_STANDALONE false)
	else()
		# But a regular Python 'pip install ...' DOES need a standalone installation
		# (since it does not manage binary shared libraries dependencies, only Python dependencies).
		set(_INSTALL_STANDALONE true)
	endif()
else()
	if (GPLATES_BUILD_GPLATES)  # GPlates ...
		# Use reasonable defaults based on the platform.
		if (WIN32 OR APPLE)
			# On Windows and Apple this is *enabled* by default since we typically distribute a self-contained package to users on those systems.
			set(_INSTALL_STANDALONE true)
		else() # Linux
			# On Linux this is *disabled* by default since we rely on the Linux binary package manager to install dependencies on the user's system
			# (for example, we create a '.deb' package that only *lists* the dependencies, which are then installed on the target system if not already there).
			# However this can be *enabled* for use cases such as creating a standalone bundle for upload to a cloud service (where it is simply extracted).
			set(_INSTALL_STANDALONE false)
		endif()
	else() # pyGPlates ...
		# We're NOT building using scikit-build-core (ie, not running 'pip wheel ...' or 'pip install ...' or 'conda install ...').
		# Which means the user is probably doing a manual CMake build/install (eg, "cmake ." followed by "cmake --build ." and "cmake --install ."),
		# or running CPack to create a zip file (which uses "cmake --install ...").
		# So we'll default to a standalone installation to ensure all dependency libraries are included.
		set(_INSTALL_STANDALONE true)
	endif()
endif()
# Make GPLATES_INSTALL_STANDALONE a cache variable, using the "option()" command, so that the user can change it (eg, via command-line, ccmake or cmake-gui).
option(GPLATES_INSTALL_STANDALONE "Install GPlates (or pyGPlates) as a standalone bundle." ${_INSTALL_STANDALONE})
unset(_INSTALL_STANDALONE)
if (GPLATES_INSTALL_STANDALONE)
	# We're installing standalone, so install shared library dependencies (unless specifically requested not to).
	#
	# An example where we explicitly request not to install dependency libraries is when creating a Python wheel for pyGPlates that will be
	# post-processed using auditwheel(manylinux)/delocate(macOS)/delvewheel(Windows) which handles copying dependency libraries into the wheel.
	option(GPLATES_INSTALL_STANDALONE_SHARED_LIBRARY_DEPENDENCIES "Copy dependency libraries into the GPlates (or pyGPlates) standalone bundle" true)
	mark_as_advanced(GPLATES_INSTALL_STANDALONE_SHARED_LIBRARY_DEPENDENCIES)
else()
	# We're not installing standalone, so remove option to install shared library dependencies.
	unset(GPLATES_INSTALL_STANDALONE_SHARED_LIBRARY_DEPENDENCIES CACHE)
endif()


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
# It is worth doing: a full pyGPlates build measured ~286s with pre-compiled headers off and
# ~150s with them on (1.9x) on a 16-core machine with MSVC 14.44, Ninja and 'Release'.
#
# Note: Our CI builds must keep this off - they use a compiler cache (sccache), which cannot
#       cache pre-compiled header translation units.
if (COMMAND target_precompile_headers)
	option(GPLATES_USE_PRECOMPILED_HEADERS "Use pre-compiled headers to speed up build times." false)
endif()

if (MSVC)
	# When using Visual Studio this shows included headers (used by 'list_external_includes.py').
	# This disables pre-compiled headers (regardless of value of 'GPLATES_USE_PRECOMPILED_HEADERS').
	#
	# Note: This is an 'option' (ie, a cache variable) so that it can be enabled with
	#       '-DGPLATES_MSVC_SHOW_INCLUDES=TRUE' on the cmake command-line. A plain 'set()' here would
	#       create a normal variable that shadows the cache variable, silently ignoring the command-line.
	option(GPLATES_MSVC_SHOW_INCLUDES "Show each included header (used by 'list_external_includes.py' to generate the pch headers)." false)
	# Disable pre-compiled headers if showing include headers.
	# The only reason to show include headers is to use 'list_external_includes.py' script to generates pch header.
	if (GPLATES_MSVC_SHOW_INCLUDES)
		# Note: This sets the non-cache variable 'GPLATES_USE_PRECOMPILED_HEADERS'
		#       (the 'option(GPLATES_USE_PRECOMPILED_HEADERS ...)' above sets the cache variable of same name).
		#       The non-cache variable will get precedence when subsequently accessed.
		#       It's also important to set this *after* 'option' since, prior to CMake 3.21, whenever a cache variable is added
		#       (eg, on the first run if not yet present in "CMakeCache.txt") the normal variable is removed.
		if (DEFINED GPLATES_USE_PRECOMPILED_HEADERS)
			set(GPLATES_USE_PRECOMPILED_HEADERS false)
		endif()
	endif()

	# If Visual Studio then enable parallel builds WITHIN a project.
	#
	# Note: To ALSO enable parallel project builds set
	#       Tools->Options->Projects and Solutions->Build and Run->maximum number of parallel project builds to
	#       the number of cores on your CPU.
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
