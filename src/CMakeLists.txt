########################################################################
#                                                                      #
# This file creates targets for building, installing and packaging.    #
#                                                                      #
########################################################################


#####################
# Find dependencies #
#####################


###############
# Find OpenGL #
#
# Note: CMake 3.8 introduced import targets for OpenGL.
#       So since our min CMake requirement exceeds this we don't need to add imported targets ourself.
find_package(OpenGL REQUIRED)

#############
# Find GLEW #
#
# CMake 3.1 introduced import targets for GLEW. Our minimum CMake requirement is higher so we don't need to add the imported target ourself.
find_package(GLEW REQUIRED)

#############
# Find ZLIB #
#
# We use our own "FindZLIB.cmake" module so we can find contrib-built zlibwapi on Windows, which also defines the ZLIB::ZLIB imported target.
find_package(ZLIB REQUIRED)

#############
# Find CGAL #
#
# According to this link, starting with CGAL 4.12 a call to 'find_package' is all that's needed
# (along with "target_link_libraries(... CGAL::CGAL)"), and so we no longer need to "include(${CGAL_USE_FILE})":
#   https://github.com/CGAL/cgal/wiki/How-to-use-CGAL-with-CMake-or-your-own-build-system
#
# However we don't specify a required version because currently (eg, with CGAL 4.14.3) we get the error message:
#   'Could not find a configuration file for package "CGAL" that is compatible with requested version "4.12"'.
#
# NOTE: We find CGAL before Boost because CGAL also finds Boost and ends up overriding
#       Boost variables (like 'Boost_LIBRARIES') if we find CGAL after.
find_package(CGAL REQUIRED)

###############
# Find Python #
#
# And put the Python major/minor versions in variables GPLATES_PYTHON_VERSION_MAJOR/GPLATES_PYTHON_VERSION_MINOR
# since needed by Boost Python.
#
if (GPLATES_PYTHON_3)
	set(_PYTHON_MAJOR_VER 3)
else()
	set(_PYTHON_MAJOR_VER 2)
endif()

# Note that we don't really need to specify the 'Interpreter' component but we do in case it makes 'Development' more robust.
# We also specify the optional component NumPy to get access to the numpy C-API header include directories.
find_package(Python${_PYTHON_MAJOR_VER} REQUIRED COMPONENTS Interpreter Development OPTIONAL_COMPONENTS NumPy)

# Get the Python major/minor versions.
set(GPLATES_PYTHON_VERSION_MAJOR ${Python${_PYTHON_MAJOR_VER}_VERSION_MAJOR})
set(GPLATES_PYTHON_VERSION_MINOR ${Python${_PYTHON_MAJOR_VER}_VERSION_MINOR})

# Get the Python interpreter executable.
set(GPLATES_PYTHON_EXECUTABLE ${Python${GPLATES_PYTHON_VERSION_MAJOR}_EXECUTABLE})

# Get the Python standard library location.
#
# Note: find_package(Python2) and find_package(Python3) have the Python2_STDLIB and Python3_STDLIB variables.
set(GPLATES_PYTHON_STDLIB_DIR ${Python${GPLATES_PYTHON_VERSION_MAJOR}_STDLIB})

# Make sure Python interpreter exists before we use it.
if (NOT EXISTS ${GPLATES_PYTHON_EXECUTABLE})
	message(FATAL_ERROR "Python interpreter was not found")
endif()

# Get the Python prefix directory (GPLATES_PYTHON_PREFIX_DIR).
execute_process(COMMAND ${GPLATES_PYTHON_EXECUTABLE} "-c" "from __future__ import print_function; import sys; print(sys.prefix);"
	RESULT_VARIABLE _PYTHON_PREFIX_RESULT
	OUTPUT_VARIABLE GPLATES_PYTHON_PREFIX_DIR
	ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
if (_PYTHON_PREFIX_RESULT)
	message(FATAL_ERROR "Unable to find Python prefix location")
endif()

# Convert '\' to '/' in paths.
file(TO_CMAKE_PATH ${GPLATES_PYTHON_EXECUTABLE} GPLATES_PYTHON_EXECUTABLE)
file(TO_CMAKE_PATH ${GPLATES_PYTHON_STDLIB_DIR} GPLATES_PYTHON_STDLIB_DIR)
file(TO_CMAKE_PATH ${GPLATES_PYTHON_PREFIX_DIR} GPLATES_PYTHON_PREFIX_DIR)

#
# Find the Python NumPy include directories (and store in GPLATES_Python_NumPy_INCLUDE_DIRS if found).
#
# We called find_package(Python2) or find_package(Python3) with optional NumPy component.
# If NumPy was not found then numpy hasn't been installed into Python.
if (Python${GPLATES_PYTHON_VERSION_MAJOR}_NumPy_FOUND)
	set(GPLATES_Python_NumPy_INCLUDE_DIRS ${Python${GPLATES_PYTHON_VERSION_MAJOR}_NumPy_INCLUDE_DIRS})
else()
	# Warn that numpy was not found.
	message(WARNING [[
	Unable to 'import numpy':
		You will not be able to pass NumPy int/float scalars as arguments to pyGPlates functions.
		To enable this functionality please install NumPy and then re-run CMake (to enable use of NumPy C-API).]])
endif()

##############
# Find Boost #
#
# Use dynamic linking for all Boost libraries.
# This includes the optional unit test framework, since it's error prone to switch static linking off and on, and
# note that we set the compiler flag BOOST_TEST_DYN_LINK later below (required by code using unit test framework).
set(Boost_USE_STATIC_LIBS FALSE)
# Starting with version 1.70 Boost provides a CMake package configuration file so that CMake does not have to update
# its builtin FindBoost module for every Boost release. For backward compatibility CMake still provides FindBoost but
# it will use a Boost CMake configuration file if it finds one. However, on Windows, the Boost config file gives the error
# "No suitable build variant has been found" (if you compile GPlates/pyGPlates with a different version of Visual Studio
# than was used to compile Boost). So we'll disable the search for the Boost config file for now, but just on Windows.
# This also means that, on Windows, the CMake version should be high enough that it supports the Boost version.
#
# TODO: This will need to be removed when FindBoost is eventually deprecated/removed.
if (WIN32)
	set(Boost_NO_BOOST_CMAKE ON)
endif()

# First find the Boost library version (since the boost python component naming scheme changed in Boost version 1.67).
# We find the library version by finding Boost without any library components (this just finds the Boost headers).
find_package(Boost 1.35 REQUIRED)

# We've just found Boost, so check its version.
if (Boost_VERSION VERSION_LESS 1.67.0)
	# Boost versions prior to 1.67 use 'python' and 'python3' for Python 2 and 3 Boost python components respectively.
	if (GPLATES_PYTHON_VERSION_MAJOR EQUAL 3)
		set(GPLATES_BOOST_PYTHON_COMPONENT_NAME python3)
		set(GPLATES_BOOST_PYTHON_NUMPY_COMPONENT_NAME numpy3)
	else()
		set(GPLATES_BOOST_PYTHON_COMPONENT_NAME python)
		set(GPLATES_BOOST_PYTHON_NUMPY_COMPONENT_NAME numpy)
	endif()
else()
	# Boost 1.67 and above use 2-digit Python version suffixes (eg, 'python37' and 'numpy37').
	# Match Boost Python with the Python version we found earlier.
	set(GPLATES_BOOST_PYTHON_COMPONENT_NAME python${GPLATES_PYTHON_VERSION_MAJOR}${GPLATES_PYTHON_VERSION_MINOR})
	set(GPLATES_BOOST_PYTHON_NUMPY_COMPONENT_NAME numpy${GPLATES_PYTHON_VERSION_MAJOR}${GPLATES_PYTHON_VERSION_MINOR})
endif()

# Now find the Boost library components.
find_package(Boost 1.35 REQUIRED
		# These components are required (including boost python)...
		COMPONENTS program_options thread system ${GPLATES_BOOST_PYTHON_COMPONENT_NAME}
		# These components are optional...
		#
		# Unit test framework is optional because we only need it for the GPlates unit-test executable and
		# it is possible to compile the GUI version of GPlates without it.
		#
		# Boost.Python.Numpy is optional because we actually haven't started using it yet and
		# it requires Boost >= 1.63 (which is above our current minimum Boost requirement).
		OPTIONAL_COMPONENTS unit_test_framework ${GPLATES_BOOST_PYTHON_NUMPY_COMPONENT_NAME})

############
# Find Qt5 #
#
# CMake automatically runs Qt's moc, uic and rcc code generation tools.
# See https://doc.qt.io/qt-5/cmake-get-started.html
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
# Require Qt 5.6 for the Qt::AA_EnableHighDpiScaling attribute (Bionic, our minimum supported Ubuntu, uses Qt 5.9).
find_package(Qt5 5.6 REQUIRED COMPONENTS Core Gui Network OpenGL Svg Widgets Xml XmlPatterns)

############
# Find Qwt #
#
# Note that we find this *after* finding Qt since we use the Qt include directory to help find Qwt.
# And we use our own "FindQwt.cmake" module, which also defines the Qwt::Qwt imported target.
#
find_package(Qwt REQUIRED)

#############
# Find GDAL #
#
find_package(GDAL 2.0 REQUIRED)

#############
# Find PROJ #
#
find_package(PROJ REQUIRED)


#################
# Initial setup #
#################


# Some useful utilities.
include (Utils)


# Generate the version source file.
configure_file(${PROJECT_SOURCE_DIR}/src/global/Version.cc.in ${PROJECT_BINARY_DIR}/src/global/Version.cc @ONLY)

# Generate the license source file.
configure_file(${PROJECT_SOURCE_DIR}/src/global/License.cc.in ${PROJECT_BINARY_DIR}/src/global/License.cc @ONLY)

# Generate 'global/config.h' from 'global/config.h.in'.
include (Config_h)
configure_file(${PROJECT_SOURCE_DIR}/src/global/config.h.in ${PROJECT_BINARY_DIR}/src/global/config.h @ONLY)


##################
# Create targets #
##################


if (GPLATES_BUILD_GPLATES)

	# The target to add source files (via target_sources()) and other target properties (eg, via target_link_libraries()).
	# This is the gplates library, which then propagates target properties to those depending on it (such as target 'gplates').
	set(SOURCE_TARGET gplates-lib)
	# The build target (to install and package).
	set(BUILD_TARGET gplates)

	#
	# Create a single static library containing all source code except the main source file(s) in current directory.
	#
	# Note that any target INTERFACE_* properties set on 'gplates-lib', via "target_*(gplates-lib PUBLIC ...)", are
	# inherited by targets linking to 'gplates-lib', such as 'gplates' and 'gplates-unit-test'.
	#
	# For now just add the generated version and license source files.
	# Below we'll add the remaining source files in 'add_subdirectory()' calls.
	add_library(gplates-lib STATIC EXCLUDE_FROM_ALL
		${PROJECT_BINARY_DIR}/src/global/Version.cc
		${PROJECT_BINARY_DIR}/src/global/License.cc
		${PROJECT_BINARY_DIR}/src/global/config.h)

	#
	# Add 'gplates' executable target (linked to gplates-lib).
	#
	add_executable(gplates gplates_main.cc ScribeExportGPlates.cc)
	target_link_libraries(gplates PRIVATE gplates-lib)

	#
	# Add 'gplates-no-gui' executable target (linked to gplates-lib).
	#
	add_executable(gplates-no-gui EXCLUDE_FROM_ALL gplates_demo_no_gui_main.cc ScribeExportGPlatesDemoNoGui.cc)
	target_link_libraries(gplates-no-gui PRIVATE gplates-lib)

	#
	# Add 'gplates-unit-test' executable (linked to gplates-lib).
	#
	# It's only added if we have a Boost unit test framework.
	# It will be populated by source files from the 'unit-test/' sub-directory.
	if (TARGET Boost::unit_test_framework)
		add_executable(gplates-unit-test EXCLUDE_FROM_ALL gplates_unit_test_main.cc ScribeExportGPlatesUnitTest.cc)
		target_link_libraries(gplates-unit-test PRIVATE gplates-lib)
	else()
		# Only print status message if this is not a public release.
		# Because it'll only confuse users (because gplate-unit-test is not part of public release).
		if (NOT GPLATES_PUBLIC_RELEASE)
			message(STATUS "Warning: boost unit_test_framework not found so GPlates unit-test executable gplates-unit-test will not be available.")
		endif()
	endif()

else()  # pygplates ...

	# The target to add source files (via target_sources()) and other target properties (eg, via target_link_libraries()).
	set(SOURCE_TARGET pygplates)
	# The build target (to install and package).
	set(BUILD_TARGET pygplates)
	
	#
	# Add the 'pygplates' Python extension module.
	#
	# Note this is the external Python library that is not embedded inside GPlates.
	if (TARGET Python3::Python)
		# We used the Python3 find module.
		Python3_add_library(pygplates MODULE ScribeExportPyGPlates.cc)
	elseif (TARGET Python2::Python)
		# We used the Python2 find module.
		Python2_add_library(pygplates MODULE ScribeExportPyGPlates.cc)
	else()
		# We used the PythonLibs find module (which does not have a Python_add_library wrapper).
		add_library(pygplates MODULE ScribeExportPyGPlates.cc)
		# Emulate Python_add_library wrapper (which sets pygplates prefix/suffix).
		set_target_properties(pygplates PROPERTIES PREFIX "")
		if (WIN32)
			set_target_properties(pygplates PROPERTIES SUFFIX ".pyd")
		endif()
	endif()
	# All sources files compiled in pygplates need to be position independent and hence compiled differently than 'gplates-lib'.
	# This is why it's not linked to 'gplates-lib' and why source files get added to both 'pygplates' and 'gplates-lib'.
	#
	# It's also why any target INTERFACE_* properties set on 'gplates-lib', via "target_*(gplates-lib PUBLIC ...)", are *not*
	# inherited by target 'pygplates' (because 'pygplates' does not link to 'gplates-lib'), and hence also need to be set on
	# 'pygplates' via "target_*(pygplates ...)".
	#
	# Note that module libraries default to position-independent code, but we'll set it anyway.
	set_target_properties(pygplates PROPERTIES POSITION_INDEPENDENT_CODE ON)
	target_sources_util(pygplates PRIVATE
		${PROJECT_BINARY_DIR}/src/global/Version.cc
		${PROJECT_BINARY_DIR}/src/global/License.cc
		${PROJECT_BINARY_DIR}/src/global/config.h)

endif()


##########################
# Source sub-directories #
##########################


#
# Recurse into source sub-directories.
#

# Specify source sub-directories.
set(source_sub_directories 
	api
	app-logic
	canvas-tools
	cli
	data-mining
	feature-visitors
	file-io
	global
	gui
	maths
	model
	opengl
	presentation
	property-values
	qt-resources
	qt-widgets
	scribe
	unit-test
	utils
	view-operations)

# Specify the default grouping of source files (for display in Visual Studio and XCode IDEs).
# This will apply to the top-level source directories.
#
# Note: The last regular expression that matches a source file applies.
#       So we need to specify the default/general source groups first.
source_group(sources REGULAR_EXPRESSION "\\.(cc|cpp|cxx)$")
source_group(headers REGULAR_EXPRESSION "\\.hh?$")
source_group(ui REGULAR_EXPRESSION "\\.ui$")
source_group(qrc REGULAR_EXPRESSION "\\.qrc$")
source_group(rc REGULAR_EXPRESSION "\\.rc$")

foreach(sub_dir ${source_sub_directories})
	# Traverse into sub-directory "CMakeLists.txt".
	add_subdirectory(${sub_dir})
	
	# Specify specific grouping of source files specific to the current sub-directory (for display in Visual Studio and XCode IDEs).
	#
	# Here we give each sub-directory its own folder group.
	# And under each sub-directory folder we have folders for each source file category (".cc", ".h", ".ui", ".qrc").
	#
	# Note: The groups defined by 'source_group' are scoped in the directory where it is called.
	#       So, since we're matching subdirectories in the regular expressions, we cannot call 'source_group'
	#       in the sub-directory "CMakeLists.txt" files.
	source_group(${sub_dir}\\sources REGULAR_EXPRESSION "/${sub_dir}/[a-zA-Z0-9_-]+\\.(cc|cpp|cxx)$")
	source_group(${sub_dir}\\headers REGULAR_EXPRESSION "/${sub_dir}/[a-zA-Z0-9_-]+\\.hh?$")
	source_group(${sub_dir}\\ui REGULAR_EXPRESSION "/${sub_dir}/[a-zA-Z0-9_-]+\\.ui$")
	source_group(${sub_dir}\\qrc REGULAR_EXPRESSION "/${sub_dir}/[a-zA-Z0-9_-]+\\.qrc$")
	source_group(${sub_dir}\\rc REGULAR_EXPRESSION "/${sub_dir}/[a-zA-Z0-9_-]+\\.rc$")
endforeach()

# Enable the location of each target to be specified using the FOLDER target property (for Visual Studio and XCode).
# This also enables the default placement of some targets created by CMake under a folder called "CMakePredefinedTargets".
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if (NOT GPLATES_BUILD_GPLATES)
	# Traverse pygplates sub-directory.
	add_subdirectory(pygplates)
endif()


#############################
# General target properties #
#############################


#
# General COMPILE_FEATURES (target property).
#

# Set the minimum C++ language standard to C++11.
#
# std::auto_ptr was deprecated in C++11 (and removed in C++17), so we now use std::unique_ptr introduced in C++11.
# Also GDAL 2.3 and above require C++11.
# And CGAL 5.x requires C++14, and this requirement is transitively passed to us via the CGAL::CGAL target
# where CMake chooses the stricter requirement between our C++11 and CGAL's C++14 (which is C++14).
#
# Also note that this avoids the problem of compile errors, when C++14 features are used (eg, by CGAL 5),
# due to forcing C++11 by specifying '-std=c++11' directly on the compiler command-line.
target_compile_features(${SOURCE_TARGET} PUBLIC cxx_std_11)
#
# We also want to disable compiler-specific extensions (eg, for C++11 on GNU compilers we want '-std=c++11' instead of '-std=g++11').
# However CMake policy CMP0128 is a bit confusing about how to do this using 'CMAKE_CXX_EXTENSIONS'.
# It seems to indicate that, prior to CMake 3.22, 'CMAKE_CXX_EXTENSIONS' is ignored unless 'CMAKE_CXX_STANDARD' is also set.
# So we'll go ahead and also include the old pre-CMake-3.8 approach to setting C++11 via target properties.
# Apparently CMake will use the stronger requirement of cxx_std_11 and CXX_STANDARD=11 (which is just C++11 anyway).
set_target_properties(${SOURCE_TARGET} PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)


#
# General PRECOMPILE_HEADERS (target property).
#

# Use pre-compiled headers for targets 'gplates-lib' and 'pygplates'.
#
# These targets have the largest amount of source code.
# Each target has its own pre-compiled "_pch.h" header that currently lists only external headers.
# Some internal headers that are used a lot but infrequently changed (such as utils) could be added also.
if (GPLATES_USE_PRECOMPILED_HEADERS)
	# Pre-compiled headers are supported natively in CMake 3.16 and above.
	if (COMMAND target_precompile_headers)
		target_precompile_headers(${SOURCE_TARGET} PRIVATE ${PROJECT_SOURCE_DIR}/src/${SOURCE_TARGET}_pch.h)
	endif()

	# Exclude specific source files from including pre-compiled header.
	#
	# NOTE: Source file properties are visible only to targets added in the same directory.
	#       So unfortunately we must set them here (we cannot set them in the source sub-directories).
	#
	set(_EXCLUDE_PCH_SOURCE_FILES
			# HellingerThread.cc redefines BOOST_PYTHON_MAX_ARITY, so Boost headers (eg, pre-compiled header) cannot be included before that...
			qt-widgets/HellingerThread.cc)
	foreach(_SOURCE_FILE IN LISTS _EXCLUDE_PCH_SOURCE_FILES)
		set_source_files_properties(${_SOURCE_FILE} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
	endforeach()
	unset(_EXCLUDE_PCH_SOURCE_FILES)
endif()


#
# General INCLUDE_DIRECTORIES (target property).
#

# A lot of "#include" statements are relative to the 'src' directory.
target_include_directories(${SOURCE_TARGET} PUBLIC ${PROJECT_SOURCE_DIR}/src)
# The 'src' directory exists in both the CMake source and binary trees (which, while same for in-place builds,
# are different for out-of-place builds). This is so generated source files (like 'global/config.h') can be found.
target_include_directories(${SOURCE_TARGET} PUBLIC ${PROJECT_BINARY_DIR}/src)

# External library include directories are treated as system include directories.
# 'src/system-fixes' contains files copied from external libraries.
target_include_directories(${SOURCE_TARGET} SYSTEM PUBLIC ${PROJECT_SOURCE_DIR}/src/system-fixes)


#
# General COMPILE_DEFINITIONS (target property).
#

# Add GPLATES_DEBUG preprocessor define to DEBUG and RELWITHDEBINFO configurations.
target_compile_definitions(${SOURCE_TARGET} PUBLIC $<$<CONFIG:DEBUG>:GPLATES_DEBUG> $<$<CONFIG:RELWITHDEBINFO>:GPLATES_DEBUG>)
# The 64-bit C99 macro UINT64_C macro fails to compile on Visual Studio 2005 using boost 1.36.
# Boost 1.42 defines __STDC_CONSTANT_MACROS in <boost/cstdint.hpp> but prior to that the application
# is required to define it and it needs to be defined before any header inclusion to ensure it is defined
# before it is accessed (which means before pre-compiled headers). So we define it on the compiler command-line.
target_compile_definitions(${SOURCE_TARGET} PUBLIC __STDC_CONSTANT_MACROS)
# Boost 1.58 introduced a breaking change in boost::variant that does compile-time with boost::get<U>(variant)
# to see if U is one of the variant types. However it seems to generate compile errors for references and boost::optional.
# So we'll default to using the old relaxed (run-time) method.
target_compile_definitions(${SOURCE_TARGET} PUBLIC BOOST_VARIANT_USE_RELAXED_GET_BY_DEFAULT)
# Temporary avoidance of warning in Boost due to bug in version 1.69 caused by using deprecated "boost/pending/integer_log2.hpp".
# Apparently it wasn't fixed in 1.69 (only 1.70 and above).
target_compile_definitions(${SOURCE_TARGET} PUBLIC BOOST_ALLOW_DEPRECATED_HEADERS)

#
# COMPILE_DEFINITIONS (target property) for MSVC compiler only.
#
if (CMAKE_CXX_COMPILER_ID MATCHES MSVC)
	# Disable Visual Studio deprecation warnings like:
	# "warning C4996: '_strcpy': This function or variable may be unsafe. Consider using _strcpy_s instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. ".
	target_compile_definitions(${SOURCE_TARGET} PUBLIC _CRT_SECURE_NO_DEPRECATE)
	# Disable deprecation warnings about renamed POSIX functions (eg, 'dup' renamed to '_dup' but we want to use 'dup' across all platforms).
	target_compile_definitions(${SOURCE_TARGET} PUBLIC _CRT_NONSTDC_NO_WARNINGS)
endif()

#
# General COMPILE_OPTIONS (target property).
#

#
# COMPILE_OPTIONS (target property) for MSVC compiler only.
#
if (CMAKE_CXX_COMPILER_ID MATCHES MSVC)
	# If we've been asked to do parallel builds in Visual Studio within a project.
	if (GPLATES_MSVC_PARALLEL_BUILD)
		if (GPLATES_MSVC_PARALLEL_BUILD_PROCESSES)
			# User specified the number of parallel build processes.
			target_compile_options(${SOURCE_TARGET} PUBLIC /MP${GPLATES_MSVC_PARALLEL_BUILD_PROCESSES})
		else()
			# Use all available CPUs.
			target_compile_options(${SOURCE_TARGET} PUBLIC /MP)
		endif()
	endif()
	# If we've been asked to output a list of header files included by source files.
	if (GPLATES_MSVC_SHOW_INCLUDES)
		target_compile_options(${SOURCE_TARGET} PUBLIC /showIncludes)
	endif()
	
	# Increase pre-compiled header memory allocation limit to avoid compile error.
	# Error happens on 12-core Windows 8.1 machine (in Visual Studio 2005).
	target_compile_options(${SOURCE_TARGET} PUBLIC /Zm1000)
	# Some C++ object files (like "PyPropertyValues.obj") contain a lot of sections (enough to require increasing the limit).
	target_compile_options(${SOURCE_TARGET} PUBLIC /bigobj)
	
	#
	# Warnings
	#
	if (GPLATES_PUBLIC_RELEASE)
		# Disable all warnings when releasing source code to non-developers.
		target_compile_options(${SOURCE_TARGET} PUBLIC /W0)
	else()
		# Default warning level /W3 seems sufficient (/W4 generates informational warnings which are not necessary to write good code).
		# But avoid /WX - we don't want to treat all warnings as errors.
		target_compile_options(${SOURCE_TARGET} PUBLIC /W3)
		
		# Disable warning C4267: 'var' : conversion from 'size_t' to 'type', possible loss of data
		#
		# When compiling with Visual Studio in 64-bit mode this warning shows up in a very large number of places.
		# Mostly because std::vector<Type>::size(), etc, return 'size_t' (which is 64 bits) and we store the result
		# in an 'unsigned int' loop counter (which is 32 bits). However, in pretty much all cases we do not
		# need more than 32 bits. We could explicitly tell the compiler this by using 'static_cast<unsigned int>()'
		# (or use size_t for the loop counter) but it just becomes far too cumbersome to change this everywhere.
		#
		# Also this warning is not produced by gcc, even with "-Wall" and "-Wextra" turned on (just checked this with gcc 8.3).
		# The only way to enable this warning with gcc is with "-Wconversion" (and then use "-W-no-float-conversion" to
		# disable the extra 'double->float' warnings also created by this).
		#
		# So we'll disable it for Visual Studio also.
		target_compile_options(${SOURCE_TARGET} PUBLIC /wd4267)
		# Disable warning C4503: 'identifier' : decorated name length exceeded, name was truncated
		#
		# Apparently a hash is applied to truncated names, so program correctness is unaffected.
		# However debugging and linking are possibly affected.
		# But this warning no longer occurs in Visual Studio 2017 and later compilers.
		target_compile_options(${SOURCE_TARGET} PUBLIC /wd4503)
	endif()
endif()

#
# COMPILE_OPTIONS (target property) common to GNU and Clang compilers.
#
if (CMAKE_CXX_COMPILER_ID MATCHES GNU OR
	CMAKE_CXX_COMPILER_ID MATCHES Clang)
	#
	# Warnings
	#
	if (GPLATES_PUBLIC_RELEASE)
		# Disable all warnings when releasing source code to non-developers.
		target_compile_options(${SOURCE_TARGET} PUBLIC -w)
	else()
		# Compile warnings.
		set(_WARNINGS
				-Wall -Wcast-align -Wwrite-strings -Wfloat-equal
				-Wpointer-arith -Wshadow -Wnon-virtual-dtor
				-Woverloaded-virtual -Wold-style-cast)
		target_compile_options(${SOURCE_TARGET} PUBLIC ${_WARNINGS})
		unset(_WARNINGS)

		# Disable some warnings.
		set(_DISABLE_WARNINGS
				-Wno-long-long -Wno-unused-parameter -Wno-unused-const-variable
				# Prefer to keep unused functions available...
				-Wno-unused-function
				# Keep unused local typedefs - used by some compilers but not others...
				-Wno-unused-local-typedefs)
		target_compile_options(${SOURCE_TARGET} PUBLIC ${_DISABLE_WARNINGS})
		unset(_DISABLE_WARNINGS)
	endif()
endif()

#
# COMPILE_OPTIONS (target property) for GNU compiler only.
#
if (CMAKE_CXX_COMPILER_ID MATCHES GNU)
	target_compile_options(${SOURCE_TARGET} PUBLIC -fno-strict-aliasing)

	#
	# Warnings
	#
	if (NOT GPLATES_PUBLIC_RELEASE)
		# Disable some warnings.
		set(_DISABLE_WARNINGS
				-Wno-clobbered -Wno-maybe-uninitialized)

		# Disable '-Wuninitialized' for g++ >= 12 and Boost 1.80.
		#
		# G++ 12 started treating some '-Wmaybe-uninitialized' warnings as '-Wuninitialized'
		# warnings creating a problem for boost::function in Boost 1.80 (but Boost fixed it
		# for 1.81 by disabling same warning: https://github.com/boostorg/function/pull/43)...
		if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 12)
			if (Boost_VERSION VERSION_EQUAL 1.80.0)
				list(APPEND _DISABLE_WARNINGS -Wno-uninitialized)
			endif()
		endif()

		target_compile_options(${SOURCE_TARGET} PUBLIC ${_DISABLE_WARNINGS})
		unset(_DISABLE_WARNINGS)
	endif()
endif()

#
# COMPILE_OPTIONS (target property) for Clang compiler only.
#
if (CMAKE_CXX_COMPILER_ID MATCHES Clang)
	#
	# Warnings
	#
	if (NOT GPLATES_PUBLIC_RELEASE)
		# Disable some warnings.
		set(_DISABLE_WARNINGS
				# Suppress redeclared-class-member warnings from boost (from boost 1.47 at least), related to BOOST_BIMAP...
				-Wno-redeclared-class-member
				# Suppress warnings about command-line arguments unused by Clang...
				-Qunused-arguments -Wno-unknown-warning-option
				# We don't always use private data members...
				-Wno-unused-private-field
				# We have a lot of boost-python 'bp::list' returned as 'bp::object', for example, which we won't std::move...
				-Wno-return-std-move)
		target_compile_options(${SOURCE_TARGET} PUBLIC ${_DISABLE_WARNINGS})
		unset(_DISABLE_WARNINGS)
	endif()
endif()


#
# General LINK_OPTIONS (target_property).
#
# Note: CMake 3.13 introduced the 'target_link_options()' command and the LINK_OPTIONS target property.

#
# LINK_OPTIONS (target_property) for MSVC compiler only.
#
if (CMAKE_CXX_COMPILER_ID MATCHES MSVC)
	if (GPLATES_BUILD_GPLATES)
		# Enable 4Gb of virtual address space instead of 2Gb (default for Windows).
		# This doubles addressable memory if GPlates is compiled as 32-bit but run on a 64-bit Windows OS.
		# On a 32-bit Windows OS this won't help because only 2Gb (by default) is accessible
		# (the 2-4Gb process address range is reserved for the system).
		#
		# Note: It seems not needed for pygplates (only the 'python.exe' that load the pygplates DLL).
		target_link_options(gplates-lib PRIVATE /LARGEADDRESSAWARE)
	endif()
endif()

#
# LINK_OPTIONS (target_property) for Apple compilers only.
#
if (APPLE)
	# 'bind_at_load' causes undefined symbols to be referenced at load/launch.
	target_link_options(${SOURCE_TARGET} PRIVATE LINKER:-bind_at_load)
endif(APPLE)


#
# Application (bundle, resources, etc) target properties.
#

if (GPLATES_BUILD_GPLATES)
	# Set the platform-dependent icon file.
	if (APPLE)
		set(GPlates_ICON "${PROJECT_SOURCE_DIR}/cmake/distribution/gplates_desktop_icon.icns")
	elseif (MSVC)
		set(GPlates_ICON "${PROJECT_SOURCE_DIR}/cmake/distribution/gplates_desktop_icon.rc")
	endif()
	if (GPlates_ICON)
		target_sources_util(gplates PRIVATE ${GPlates_ICON})
	endif()

	if (APPLE)
		# Copy icon file to 'Resources' directory inside application bundle.
		set_source_files_properties(${GPlates_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

		set_target_properties(gplates PROPERTIES
				# Tell cmake to build gplates as an application bundle.
				MACOSX_BUNDLE TRUE

				# Specify our own Info.plist template file.
				# We add a couple of keys to ensure Qt includes support Mac Retina displays.
				MACOSX_BUNDLE_INFO_PLIST "${PROJECT_SOURCE_DIR}/cmake/distribution/MacOSXBundleInfo.plist.in"

				MACOSX_BUNDLE_INFO_STRING "${GPLATES_PACKAGE_DESCRIPTION_SUMMARY}"
				MACOSX_BUNDLE_ICON_FILE gplates_desktop_icon.icns
				MACOSX_BUNDLE_GUI_IDENTIFIER "GPlates ${GPLATES_VERSION_PRERELEASE_USER}-${CMAKE_SYSTEM}"
				MACOSX_BUNDLE_LONG_VERSION_STRING "GPlates ${GPLATES_VERSION_PRERELEASE_USER}"
				MACOSX_BUNDLE_BUNDLE_NAME "GPlates"
				MACOSX_BUNDLE_SHORT_VERSION_STRING "${GPLATES_VERSION}"
				MACOSX_BUNDLE_BUNDLE_VERSION "${GPLATES_VERSION_PRERELEASE}"
				MACOSX_BUNDLE_COPYRIGHT "${GPLATES_COPYRIGHT_STRING}")
	endif()

	if (WIN32)
		# UPDATE: Don't build GPlates as a GUI application on Windows (with a WinMain entry point).
		#
		# If GPlates is built as a GUI application (on Windows) then it detaches from the parent console and
		# all output to stdout and stderr are lost. We actually capture the stdout/stderr output in GPlates
		# (eg, generated from dependency libraries) and output them to the GPlates log window and log file,
		# but there's nothing to capture unless GPlates is built as a console application. In fact a simple
		# 'print()' statement in the embedded Python interpreter would have triggered an error.
		if (FALSE)
			set_target_properties(gplates PROPERTIES WIN32_EXECUTABLE TRUE)  # This also tells Qt5::Core to link in its WinMain.
		endif()
	endif()
endif()


########################
# Dependency libraries #
########################

#
# OpenGL (and GLEW) dependency.
#
target_link_libraries(${SOURCE_TARGET} PUBLIC OpenGL::GL OpenGL::GLU GLEW::GLEW)
# Define GL_SILENCE_DEPRECATION to avoid a bunch of OpenGL deprecation warnings.
# Currently happens when compiling on macOS mojave (10.14).
# We will eventually replace OpenGL with Vulkan, but not for a while since
# Apple are unlikely to 'remove' OpenGL in their drivers anytime soon.
#
# Set as usage requirements (INTERFACE) on the OpenGL imported library target.
#
# Note: Prior to CMake 3.11 none of the 'target_*()' commands could set INTERFACE_ properties on imported targets.
if (CMAKE_VERSION VERSION_LESS 3.11)
	# Using 'set_property' since 'set_target_property' cannot append.
	set_property(TARGET OpenGL::GL APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS GL_SILENCE_DEPRECATION)
else()
	target_compile_definitions(OpenGL::GL INTERFACE GL_SILENCE_DEPRECATION)
endif()

#
# ZLIB dependency.
#
target_link_libraries(${SOURCE_TARGET} PUBLIC ZLIB::ZLIB)

#
# Boost dependency.
#
# Add required Boost components.
target_link_libraries(${SOURCE_TARGET} PUBLIC Boost::boost Boost::program_options Boost::thread Boost::system Boost::${GPLATES_BOOST_PYTHON_COMPONENT_NAME})
# Disable Boost auto-linking.
# This solves the issue whereby, for example, Boost is compiled with Visual Studio 2015 but we're compiling GPlates/pyGPlates
# with Visual Studio 2019 (which causes auto-linking to look for Boost libs with 'vc142' in their name) and hence cannot find
# the Boost libs with 'vc140' in their name (built with the 2015 compiler).
target_link_libraries(${SOURCE_TARGET} PUBLIC Boost::disable_autolinking)
# Add optional Boost.Python.Numpy component (if found).
if (TARGET Boost::${GPLATES_BOOST_PYTHON_NUMPY_COMPONENT_NAME})
	target_link_libraries(${SOURCE_TARGET} PUBLIC Boost::${GPLATES_BOOST_PYTHON_NUMPY_COMPONENT_NAME})
endif()
if (TARGET gplates-unit-test)
	# Link to the Boost unit test framework.
	target_link_libraries(gplates-unit-test PRIVATE Boost::unit_test_framework)
	# Boost unit test framework is now dynamically linked (Boost_USE_STATIC_LIBS is OFF for all boost components) so
	# we define the BOOST_TEST_DYN_LINK compiler flag here to avoid having to define it in every unit test source file...
	target_compile_definitions(gplates-unit-test PRIVATE BOOST_TEST_DYN_LINK)
endif()

#
# Qt5 dependency.
#
target_link_libraries(${SOURCE_TARGET} PUBLIC Qt5::Core Qt5::Gui Qt5::Network Qt5::OpenGL Qt5::Svg Qt5::Widgets Qt5::Xml Qt5::XmlPatterns)
# The directory where Qt AUTOIUC auto-generated files ('ui_*.h') get written is automatically added to the
# INCLUDE_DIRECTORIES target property of the 'gplates-lib' and 'pygplates' targets, but not INTERFACE_INCLUDE_DIRECTORIES.
# This is fine for 'pygplates' (because nothing links to it), but it means the include directories are not propagated to
# any executables linking to 'gplates-lib' (such as 'gplates').
#
# Hence we get compile errors when sources (added to any targets linked to 'gplates-lib') try to include 'ui_*.h' files.
#
# The workaround involves explicitly adding these include paths as PUBLIC (which is same as PRIVATE plus INTERFACE).
#
# Note that CMake 3.8 and above no longer place the generated 'ui_*.h' files in ${CMAKE_CURRENT_BINARY_DIR}.
# Instead they go in '${CMAKE_CURRENT_BINARY_DIR}/<target-name>_autogen/include' for single-configuration generators, and
# in '${CMAKE_CURRENT_BINARY_DIR}/<target-name>_autogen/include_$<CONFIG>' for multi-configuration generators.
# So to cover all CMake versions we add all three include paths.
target_include_directories(${SOURCE_TARGET} PUBLIC
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_BINARY_DIR}/${SOURCE_TARGET}_autogen/include
	${CMAKE_CURRENT_BINARY_DIR}/${SOURCE_TARGET}_autogen/include_$<CONFIG>)
# Define QT_NO_KEYWORDS so that Qt will not pollute namespaces.
# Must use "Q_SLOTS Q_EMIT Q_SIGNALS" instead of "slots emit signals".
#
# Set as usage requirements (INTERFACE) on the Qt imported QtCore library so that all consuming targets,
# including the other Qt imported libraries like QtGui as well as our targets, will inherit them.
target_compile_definitions(Qt5::Core INTERFACE QT_NO_KEYWORDS)

#
# Qwt dependency.
#
target_link_libraries(${SOURCE_TARGET} PUBLIC Qwt::Qwt)
# Define Qwt version keywords.
#
# Set as usage requirements (INTERFACE) on the Qwt imported library target.
target_compile_definitions(Qwt::Qwt INTERFACE
	QWT_MAJOR_VERSION=${QWT_MAJOR_VERSION}
	QWT_MINOR_VERSION=${QWT_MINOR_VERSION}
	QWT_PATCH_VERSION=${QWT_PATCH_VERSION})

#
# CGAL dependency.
#
target_link_libraries(${SOURCE_TARGET} PUBLIC CGAL::CGAL)
# It appears "auxiliary/gmp/include" in base CGAL installation does not always get included by
# CGALConfig.cmake (found using 'find_package(CGAL)' above). Noticed this with CMake 4.13.2 when
# installing CGAL with a CMAKE_INSTALL_PREFIX different than base installation directory.
target_include_directories(${SOURCE_TARGET} SYSTEM PUBLIC ${GMP_INCLUDE_DIR} ${MPFR_INCLUDE_DIR})

#
# GDAL dependency.
#
target_link_libraries(${SOURCE_TARGET} PUBLIC GDAL::GDAL)

#
# PROJ dependency.
#
target_link_libraries(${SOURCE_TARGET} PUBLIC PROJ::proj)

#
# Python dependency.
#
if (TARGET Python3::Python)
	if (GPLATES_BUILD_GPLATES)
		# We used the Python3 find module.
		target_link_libraries(gplates-lib PUBLIC Python3::Python)
	else()
		# Note that the Python3_add_library() call above has already linked pygplates to Python3::Module.
	endif()
else()  # TARGET Python2::Python
	if (GPLATES_BUILD_GPLATES)
		# We used the Python2 find module.
		target_link_libraries(gplates-lib PUBLIC Python2::Python)
	else()
		# Note that the Python2_add_library() call above has already linked pygplates to Python2::Module.
	endif()
endif()
# Add the NumPy include directories if found.
if (GPLATES_Python_NumPy_INCLUDE_DIRS)
	target_include_directories(${SOURCE_TARGET} SYSTEM PUBLIC ${GPLATES_Python_NumPy_INCLUDE_DIRS})
endif()
if (GPLATES_BUILD_GPLATES)
	# Python is embedded in the GPlates application (as opposed to pygplates which is a Python extension library).
	target_compile_definitions(gplates-lib PUBLIC GPLATES_PYTHON_EMBEDDING)
endif()


# Test whether Python embedding works.
# This must be run after the include and library paths have all been set.
include(TestPythonEmbedding)

#
# Include code to *install* the ${BUILD_TARGET} target (either 'gplates' or 'pygplates').
#
# This is done in the same directory scope that the targets were created in since this is required by CMake <= 3.12
# (once CMake 3.13 or above is our minimum requirement then this will no longer be necessary).
# Note that, while 'include()' pulls in content from another directory, it pulls it into the *current* directory scope.
#
include(Install)

#
# Include code to *package* the ${BUILD_TARGET} target (either 'gplates' or 'pygplates').
#
# This should be included after code that installs the targets since packaging first installs to a staging area.
#
include(Package)
