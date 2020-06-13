#####################
# Find dependencies #
#####################


#
# Find OpenGL.
#
find_package(OpenGL REQUIRED)
# CMake 3.8 introduced import targets for OpenGL. For prior versions we need to add the imported target ourself.
if (NOT TARGET OpenGL::GL)
	add_library(OpenGL::GL IMPORTED INTERFACE)
	set_target_properties(OpenGL::GL PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${OPENGL_INCLUDE_DIR}")
	set_target_properties(OpenGL::GL PROPERTIES INTERFACE_LINK_LIBRARIES "${OPENGL_LIBRARIES}")
endif()
if (NOT TARGET OpenGL::GLU)
	# We leave the GLU library target empty since the GLU library and include path are already handled by
	# 'OPENGL_LIBRARIES' and 'OPENGL_INCLUDE_DIR'. We're only creating this because we need to provide a 'OpenGL::GLU' target.
	add_library(OpenGL::GLU IMPORTED INTERFACE)
endif()

#
# Find GLEW.
#
# CMake 3.1 introduced import targets for GLEW. Our minimum CMake requirement is higher so we don't need to add the imported target ourself.
find_package(GLEW REQUIRED)

#
# Find ZLIB.
#
# We use our own "FindZLIB.cmake" module so we can find contrib-built zlibwapi on Windows, which also defines the ZLIB::ZLIB imported target.
find_package(ZLIB REQUIRED)

#
# Find CGAL.
#
# According to this link, starting with CGAL 4.12 a call to 'find_package' is all that's needed
# (along with "target_link_libraries(... CGAL::CGAL)"), and so we no longer need to "include(${CGAL_USE_FILE})":
#   https://github.com/CGAL/cgal/wiki/How-to-use-CGAL-with-CMake-or-your-own-build-system
#
# This also seems to work with CGAL 4.7 (supported by Ubuntu Xenial 16.04).
# Although we don't specify a 4.7 minimum requirement since that results in a CMake error on Ubuntu Xenial.
#
# NOTE: We find CGAL before Boost because CGAL also finds Boost and ends up overriding
#       Boost variables (like 'Boost_LIBRARIES') if we find CGAL after.
find_package(CGAL REQUIRED)

#
# Find Python.
#
# And put the Python major/minor versions in variables GPLATES_PYTHON_VERSION_MAJOR/GPLATES_PYTHON_VERSION_MINOR
# since needed by Boost Python.
#
# CMake 3.12 and above support the Python2 and Python3 find modules, otherwise
# we fall back on the PythonLibs find module.
if (CMAKE_VERSION VERSION_LESS 3.12)
	# Note that we don't really need to find 'PythonInterp' but we do in case it makes 'PythonLibs' more robust.
	# In which case it should be called first.
	if (GPLATES_PYTHON_3)
		find_package(PythonInterp 3 EXACT)
		find_package(PythonLibs 3 EXACT REQUIRED)
	else()
		find_package(PythonInterp 2 EXACT)
		find_package(PythonLibs 2 EXACT REQUIRED)
	endif()
	# Get the Python major/minor versions.
	string(REGEX MATCHALL "[0-9]+" GPLATES_PYTHONLIBS_VERSION_MAJOR_MINOR ${PYTHONLIBS_VERSION_STRING})
	list(GET GPLATES_PYTHONLIBS_VERSION_MAJOR_MINOR 0 GPLATES_PYTHON_VERSION_MAJOR)
	list(GET GPLATES_PYTHONLIBS_VERSION_MAJOR_MINOR 1 GPLATES_PYTHON_VERSION_MINOR)
	unset(GPLATES_PYTHONLIBS_VERSION_MAJOR_MINOR)
else()
	# Note that we don't really need to specify the 'Interpreter' component but we do in case it makes 'Development' more robust.
	if (GPLATES_PYTHON_3)
		find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
		set(GPLATES_PYTHON_VERSION_MAJOR ${Python3_VERSION_MAJOR})
		set(GPLATES_PYTHON_VERSION_MINOR ${Python3_VERSION_MINOR})
	else()
		find_package(Python2 REQUIRED COMPONENTS Interpreter Development)
		set(GPLATES_PYTHON_VERSION_MAJOR ${Python2_VERSION_MAJOR})
		set(GPLATES_PYTHON_VERSION_MINOR ${Python2_VERSION_MINOR})
	endif()
endif()

#
# Find Boost.
#
# Use dynamic linking for all Boost libraries.
# This includes the optional unit test framework, since it's error prone to switch static linking off and on, and
# note that we set the compiler flag BOOST_TEST_DYN_LINK later below (required by code using unit test framework).
set(Boost_USE_STATIC_LIBS FALSE)
# First find the Boost library version (since the boost python component naming scheme changed in Boost version 1.67).
# We find the library version by finding Boost without any library components (this just finds the Boost headers).
find_package(Boost 1.35 REQUIRED)
# We've just found Boost, so check its version.
# First, note that CMake 3.15 and above switched Boost_VERSION from macro format (106700) to x.y.z format (1.67.0)
if (CMAKE_VERSION VERSION_LESS 3.15)
	set(Boost_VERSION_1_67 106700)
else()
	set(Boost_VERSION_1_67 1.67.0)
endif()
if (Boost_VERSION VERSION_LESS ${Boost_VERSION_1_67})
	# Boost versions prior to 1.67 use 'python' and 'python3' for Python 2 and 3 Boost python components respectively.
	if (GPLATES_PYTHON_VERSION_MAJOR EQUAL 3)
		set(GPLATES_BOOST_PYTHON_COMPONENT_NAME python3)
	else()
		set(GPLATES_BOOST_PYTHON_COMPONENT_NAME python)
	endif()
else()
	# Boost 1.67 and above use 2-digit Python version suffixes (eg, 'python37').
	# Match Boost Python with the Python version we found earlier.
	set(GPLATES_BOOST_PYTHON_COMPONENT_NAME python${GPLATES_PYTHON_VERSION_MAJOR}${GPLATES_PYTHON_VERSION_MINOR})
endif()
# Now find the Boost library components.
find_package(Boost 1.35 REQUIRED
	# These components are mandatory (including boost python)...
	COMPONENTS program_options thread system ${GPLATES_BOOST_PYTHON_COMPONENT_NAME}
	# Unit test framework is optional because we only need it for the GPlates unit-test executable and
	# it is possible to compile the GUI version of GPlates without it.
	# If it's found then 'Boost_UNIT_TEST_FRAMEWORK_FOUND' will be true...
	OPTIONAL_COMPONENTS unit_test_framework)
# Imported target name for boost python (for later).
# CMake 3.5 and above support imported targets for Boost.
set(GPLATES_BOOST_PYTHON_IMPORTED_TARGET Boost::${GPLATES_BOOST_PYTHON_COMPONENT_NAME})

#
# Find Qt5.
#
# CMake automatically runs Qt's moc, uic and rcc code generation tools.
# See https://doc.qt.io/qt-5/cmake-get-started.html
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
if(CMAKE_VERSION VERSION_LESS 3.7)
	set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()
find_package(Qt5 5.5 REQUIRED COMPONENTS Core Gui Network OpenGL Svg Widgets Xml XmlPatterns)

#
# Find Qwt.
#
# Note that we find this *after* finding Qt since we use the Qt include directory to help find Qwt.
# And we use our own "FindQwt.cmake" module, which also defines the Qwt::Qwt imported target.
#
find_package(Qwt REQUIRED)

#
# Find GDAL.
#
# CMake 3.14 introduced import targets for GDAL. For prior versions we need to add the imported target ourself.
#
# However we are still using our own custom "FindGDAL.cmake" (instead of using the one provided by CMake)
# since it has trouble finding "GDAL.h". And our own module defines the GDAL::GDAL imported target.
#
find_package(GDAL REQUIRED)

#
# Find PROJ.
#
# NOTE: Starting with Proj7 there is also the PROJ name (not just PROJ4).
#       And a future Proj release will deprecate the PROJ4 name
#       (with the PROJ4::proj eventually being retired in preference to PROJ::proj).
#       For now we'll just use PROJ4, but note that this can mean any proj version (eg, 4,5,6,7).
#
# First look for the CONFIG package (provided with the installed Proj library, as opposed to CMake itself).
find_package(PROJ4 CONFIG QUIET)
# If there was no CONFIG package then use our own "FindPROJ.cmake" (note that CMake does not provide one).
# Proj version 4 does not have a CONFIG package but more recent versions should. Proj 4.9 is still used by Ubuntu Xenial (16.04).
if (NOT PROJ4_FOUND)
  find_package(PROJ MODULE REQUIRED)  # Uses our own "FindPROJ.cmake".
  # Use same variables as the modern CONFIG package.
  set(PROJ4_INCLUDE_DIRS ${PROJ_INCLUDE_DIR})
  set(PROJ4_LIBRARIES ${PROJ_LIBRARY})
endif()


#################
# Initial setup #
#################


# Some useful utilities.
include (Utils)


# Get the Subversion revision details (except for public releases) at the root GPlates source directory.
#
# We use ${GPlates_WC_*} which evaluate to empty strings for public releases (or if Subversion package not found).
if (NOT GPLATES_PUBLIC_RELEASE)
	find_package(Subversion)
	if (SUBVERSION_FOUND)
		Subversion_WC_INFO(${GPlates_SOURCE_DIR} GPlates)
		# The branch name is the last part of the part of the working-copy URL.
		string(REGEX MATCH "[^/]*$" GPlates_WC_BRANCH_NAME ${GPlates_WC_URL})
	endif()
endif()

# Generate the version source file.
#
# This uses the ${GPlates_VERSION} set by project(), and ${GPlates_WC_*} queried from Subversion.
configure_file(${GPlates_SOURCE_DIR}/src/global/Version.cc.in ${GPlates_BINARY_DIR}/src/global/Version.cc @ONLY)

# Generate the license source file.
configure_file(${GPlates_SOURCE_DIR}/src/global/License.cc.in ${GPlates_BINARY_DIR}/src/global/License.cc @ONLY)

# Generate 'global/config.h' from 'global/config.h.in'.
include (Config_h)
configure_file(${GPlates_SOURCE_DIR}/src/global/config.h.in ${GPlates_BINARY_DIR}/src/global/config.h @ONLY)


##################
# Create targets #
##################


#
# Create a single static library containing all source code except the main source file(s) in current directory.
#
# Note that any target INTERFACE_* properties set on 'gplates-lib', via "target_*(gplates-lib PUBLIC ...)", are
# inherited by targets linking to 'gplates-lib', such as 'gplates' and 'gplates-unit-test'.
#
# For now just add the generated version and license source files.
# Below we'll add the remaining source files in 'add_subdirectory()' calls.
add_library(gplates-lib STATIC EXCLUDE_FROM_ALL
	${GPlates_BINARY_DIR}/src/global/Version.cc
	${GPlates_BINARY_DIR}/src/global/License.cc
	${GPlates_BINARY_DIR}/src/global/config.h)

#
# Add 'gplates' executable target (linked to gplates-lib).
#
add_executable(gplates EXCLUDE_FROM_ALL gplates_main.cc ScribeExportGPlates.cc)
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
if (Boost_UNIT_TEST_FRAMEWORK_FOUND)
	add_executable(gplates-unit-test EXCLUDE_FROM_ALL gplates_unit_test_main.cc ScribeExportGPlatesUnitTest.cc)
	target_link_libraries(gplates-unit-test PRIVATE gplates-lib)
else()
	# Only print status message if this is not a public release.
	# Because it'll only confuse users (because gplate-unit-test is not part of public release).
	if (NOT GPLATES_PUBLIC_RELEASE)
		message(STATUS "Warning: boost unit_test_framework not found so GPlates unit-test executable gplates-unit-test will not be available.")
	endif()
endif()

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
	${GPlates_BINARY_DIR}/src/global/Version.cc
	${GPlates_BINARY_DIR}/src/global/License.cc
	${GPlates_BINARY_DIR}/src/global/config.h)


###################
# Sub-directories #
###################


#
# Recurse into subdirectories.
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
#
# Note: This is also set in ConfigDefault.cmake (for CMake < 3.8) as the variable CMAKE_CXX_STANDARD.
#       So that code can be removed once our minimum CMake requirement is 3.8.
if (NOT CMAKE_VERSION VERSION_LESS 3.8)
	target_compile_features(gplates-lib PUBLIC cxx_std_11)
	target_compile_features(pygplates PUBLIC cxx_std_11)
endif()


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
		target_precompile_headers(gplates-lib PRIVATE ${GPlates_SOURCE_DIR}/src/gplates-lib_pch.h)
		target_precompile_headers(pygplates PRIVATE ${GPlates_SOURCE_DIR}/src/pygplates_pch.h)
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
target_include_directories(gplates-lib PUBLIC ${GPlates_SOURCE_DIR}/src)
target_include_directories(pygplates PUBLIC ${GPlates_SOURCE_DIR}/src)

# External library include directories are treated as system include directories.
# 'src/system-fixes' contains files copied from external libraries.
target_include_directories(gplates-lib SYSTEM PUBLIC ${GPlates_SOURCE_DIR}/src/system-fixes)
target_include_directories(pygplates SYSTEM PUBLIC ${GPlates_SOURCE_DIR}/src/system-fixes)


#
# General COMPILE_DEFINITIONS (target property).
#

# Add GPLATES_DEBUG preprocessor define to DEBUG and RELWITHDEBINFO configurations.
target_compile_definitions(gplates-lib PUBLIC $<$<CONFIG:DEBUG>:GPLATES_DEBUG> $<$<CONFIG:RELWITHDEBINFO>:GPLATES_DEBUG>)
target_compile_definitions(pygplates PUBLIC $<$<CONFIG:DEBUG>:GPLATES_DEBUG> $<$<CONFIG:RELWITHDEBINFO>:GPLATES_DEBUG>)
# The 64-bit C99 macro UINT64_C macro fails to compile on Visual Studio 2005 using boost 1.36.
# Boost 1.42 defines __STDC_CONSTANT_MACROS in <boost/cstdint.hpp> but prior to that the application
# is required to define it and it needs to be defined before any header inclusion to ensure it is defined
# before it is accessed (which means before pre-compiled headers). So we define it on the compiler command-line.
target_compile_definitions(gplates-lib PUBLIC __STDC_CONSTANT_MACROS)
target_compile_definitions(pygplates PUBLIC __STDC_CONSTANT_MACROS)
# Boost 1.58 introduced a breaking change in boost::variant that does compile-time with boost::get<U>(variant)
# to see if U is one of the variant types. However it seems to generate compile errors for references and boost::optional.
# So we'll default to using the old relaxed (run-time) method.
target_compile_definitions(gplates-lib PUBLIC BOOST_VARIANT_USE_RELAXED_GET_BY_DEFAULT)
target_compile_definitions(pygplates PUBLIC BOOST_VARIANT_USE_RELAXED_GET_BY_DEFAULT)
# Temporary avoidance of warning in Boost due to bug in version 1.69 caused by using deprecated "boost/pending/integer_log2.hpp".
# Apparently it wasn't fixed in 1.69 (only 1.70 and above).
target_compile_definitions(gplates-lib PUBLIC BOOST_ALLOW_DEPRECATED_HEADERS)
target_compile_definitions(pygplates PUBLIC BOOST_ALLOW_DEPRECATED_HEADERS)

if (CMAKE_CXX_COMPILER_ID MATCHES MSVC)
	# Disable Visual Studio deprecation warnings like:
	# "warning C4996: '_strcpy': This function or variable may be unsafe. Consider using _strcpy_s instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. ".
	target_compile_definitions(gplates-lib PUBLIC _CRT_SECURE_NO_DEPRECATE)
	target_compile_definitions(pygplates PUBLIC _CRT_SECURE_NO_DEPRECATE)
endif()

#
# General COMPILE_OPTIONS (target property).
#

if (CMAKE_CXX_COMPILER_ID MATCHES MSVC)
	# If we've been asked to do parallel builds in Visual Studio within a project.
	if (GPLATES_MSVC_PARALLEL_BUILD)
		target_compile_options(gplates-lib PUBLIC /MP)
		target_compile_options(pygplates PUBLIC /MP)
	endif()
	# If we've been asked to output a list of header files included by source files.
	if (GPLATES_MSVC_SHOW_INCLUDES)
		target_compile_options(gplates-lib PUBLIC /showIncludes)
		target_compile_options(pygplates PUBLIC /showIncludes)
	endif()
	
	# Increase pre-compiled header memory allocation limit to avoid compile error.
	# Error happens on 12-core Windows 8.1 machine (in Visual Studio 2005).
	target_compile_options(gplates-lib PUBLIC /Zm1000)
	target_compile_options(pygplates PUBLIC /Zm1000)
	# Some C++ object files (like "PyPropertyValues.obj") contain a lot of sections (enough to require increasing the limit).
	target_compile_options(gplates-lib PUBLIC /bigobj)
	target_compile_options(pygplates PUBLIC /bigobj)
	
	#
	# Warnings
	#
	if (GPLATES_PUBLIC_RELEASE)
		# Disable all warnings when releasing source code to non-developers.
		target_compile_options(gplates-lib PUBLIC /w)
		target_compile_options(pygplates PUBLIC /w)
	else()
		# Default warning level /W3 seems sufficient (/W4 generates informational warnings which are not necessary to write good code).
		# But avoid /WX - we don't want to treat all warnings as errors.
		target_compile_options(gplates-lib PUBLIC /W3)
		target_compile_options(pygplates PUBLIC /W3)
		
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
		target_compile_options(gplates-lib PUBLIC /wd4267)
		target_compile_options(pygplates PUBLIC /wd4267)
		# Disable warning C4503: 'identifier' : decorated name length exceeded, name was truncated
		#
		# Apparently a hash is applied to truncated names, so program correctness is unaffected.
		# However debugging and linking are possibly affected.
		# But this warning no longer occurs in Visual Studio 2017 and later compilers.
		target_compile_options(gplates-lib PUBLIC /wd4503)
		target_compile_options(pygplates PUBLIC /wd4503)
	endif()
endif()

# Compile options common to GNU and Clang compilers.
if (CMAKE_CXX_COMPILER_ID MATCHES GNU OR
	CMAKE_CXX_COMPILER_ID MATCHES Clang)
	#
	# Warnings
	#
	if (GPLATES_PUBLIC_RELEASE)
		# Disable all warnings when releasing source code to non-developers.
		target_compile_options(gplates-lib PUBLIC -w)
		target_compile_options(pygplates PUBLIC -w)
	else()
		# Compile warnings.
		set(_WARNINGS
				-Wall -Wcast-align -Wwrite-strings -Wfloat-equal
				-Wpointer-arith -Wshadow -Wnon-virtual-dtor
				-Woverloaded-virtual -Wold-style-cast)
		target_compile_options(gplates-lib PUBLIC ${_WARNINGS})
		target_compile_options(pygplates PUBLIC ${_WARNINGS})
		unset(_WARNINGS)

		# Disable some warnings.
		set(_DISABLE_WARNINGS
				-Wno-long-long -Wno-unused-parameter -Wno-unused-const-variable
				# Prefer to keep unused functions available...
				-Wno-unused-function
				# Keep unused local typedefs - used by some compilers but not others...
				-Wno-unused-local-typedefs)
		target_compile_options(gplates-lib PUBLIC ${_DISABLE_WARNINGS})
		target_compile_options(pygplates PUBLIC ${_DISABLE_WARNINGS})
		unset(_DISABLE_WARNINGS)
	endif()
endif()

# Compile options for GNU compiler only.
if (CMAKE_CXX_COMPILER_ID MATCHES GNU)
	target_compile_options(gplates-lib PUBLIC -fno-strict-aliasing)
	target_compile_options(pygplates PUBLIC -fno-strict-aliasing)

	#
	# Warnings
	#
	if (NOT GPLATES_PUBLIC_RELEASE)
		# Disable some warnings.
		set(_DISABLE_WARNINGS
				-Wno-clobbered -Wno-maybe-uninitialized)
		target_compile_options(gplates-lib PUBLIC ${_DISABLE_WARNINGS})
		target_compile_options(pygplates PUBLIC ${_DISABLE_WARNINGS})
		unset(_DISABLE_WARNINGS)
	endif()
endif()

# Compile options for Clang compiler only.
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
				-Qunused-arguments
				# We don't always use private data members...
				-Wno-unused-private-field)
		target_compile_options(gplates-lib PUBLIC ${_DISABLE_WARNINGS})
		target_compile_options(pygplates PUBLIC ${_DISABLE_WARNINGS})
		unset(_DISABLE_WARNINGS)
	endif()
endif()


#
# General LINK_OPTIONS (target_property).
#

# Note: CMake 3.13 introduced the 'target_link_options()' command and the LINK_OPTIONS target property.
#       For earlier CMake versions we'll use the less useful LINK_FLAGS target property (which must be
#       a single string instead of a list, has no INTERFACE_LINK_OPTIONS, doesn't support generator expressions, etc).

if (CMAKE_CXX_COMPILER_ID MATCHES MSVC)
	# Enable 4Gb of virtual address space instead of 2Gb (default for Windows).
	# This doubles addressable memory if GPlates is compiled as 32-bit but run on a 64-bit Windows OS.
	# On a 32-bit Windows OS this won't help because only 2Gb (by default) is accessible
	# (the 2-4Gb process address range is reserved for the system).
	#
	# Note: It seems not needed for pygplates (only the 'python.exe' that load the pygplates DLL).
	if (CMAKE_VERSION VERSION_LESS 3.13)
		get_target_property(_LINK_FLAGS gplates-lib LINK_FLAGS)
		set_target_properties(gplates-lib PROPERTIES LINK_FLAGS "${_LINK_FLAGS} /LARGEADDRESSAWARE")
		unset(_LINK_FLAGS)
	else()
		target_link_options(gplates-lib PRIVATE /LARGEADDRESSAWARE)
	endif()
endif()

if (APPLE)
	# 'bind_at_load' causes undefined symbols to be referenced at load/launch.
	if (CMAKE_VERSION VERSION_LESS 3.13)
		get_target_property(_LINK_FLAGS gplates-lib LINK_FLAGS)
		set_target_properties(gplates-lib PROPERTIES LINK_FLAGS "${_LINK_FLAGS} -bind_at_load")
		get_target_property(_LINK_FLAGS pygplates LINK_FLAGS)
		set_target_properties(pygplates PROPERTIES LINK_FLAGS "${_LINK_FLAGS} -bind_at_load")
		unset(_LINK_FLAGS)
	else()
		target_link_options(gplates-lib PRIVATE LINKER:-bind_at_load)
		target_link_options(pygplates PRIVATE LINKER:-bind_at_load)
	endif()
endif(APPLE)


########################
# Dependency libraries #
########################

#
# OpenGL (and GLEW) dependency.
#
target_link_libraries(gplates-lib PUBLIC OpenGL::GL OpenGL::GLU GLEW::GLEW)
target_link_libraries(pygplates PUBLIC OpenGL::GL OpenGL::GLU GLEW::GLEW)
# Define GL_SILENCE_DEPRECATION to avoid a bunch of OpenGL deprecation warnings.
# Currently happens when compiling on macOS mojave (10.14).
# We will eventually replace OpenGL with Vulkan, but not for a while since
# Apple are unlikely to 'remove' OpenGL in their drivers anytime soon.
#
# Set as usage requirements (INTERFACE) on the OpenGL imported library target.
#
# Note: Prior to CMake 3.11 none of the 'target_*()' commands could set INTERFACE_ properties on imported targets.
if (CMAKE_VERSION VERSION_LESS 3.11)
	# Using 'set_propery' since 'set_target_property' cannot append.
	set_property(TARGET OpenGL::GL APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS GL_SILENCE_DEPRECATION)
else()
	target_compile_definitions(OpenGL::GL INTERFACE GL_SILENCE_DEPRECATION)
endif()

#
# ZLIB dependency.
#
target_link_libraries(gplates-lib PUBLIC ZLIB::ZLIB)
target_link_libraries(pygplates PUBLIC ZLIB::ZLIB)

#
# Boost dependency.
#
# CMake 3.5 and above support imported targets for Boost.
target_link_libraries(gplates-lib PUBLIC Boost::boost Boost::program_options Boost::thread Boost::system ${GPLATES_BOOST_PYTHON_IMPORTED_TARGET})
target_link_libraries(pygplates PUBLIC Boost::boost Boost::program_options Boost::thread Boost::system ${GPLATES_BOOST_PYTHON_IMPORTED_TARGET})
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
target_link_libraries(gplates-lib PUBLIC Qt5::Core Qt5::Gui Qt5::Network Qt5::OpenGL Qt5::Svg Qt5::Widgets Qt5::Xml Qt5::XmlPatterns)
target_link_libraries(pygplates PUBLIC Qt5::Core Qt5::Gui Qt5::Network Qt5::OpenGL Qt5::Svg Qt5::Widgets Qt5::Xml Qt5::XmlPatterns)
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
target_include_directories(gplates-lib PUBLIC
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_BINARY_DIR}/gplates-lib_autogen/include
	${CMAKE_CURRENT_BINARY_DIR}/gplates-lib_autogen/include_$<CONFIG>)
# Define QT_NO_KEYWORDS so that Qt will not pollute namespaces.
# Must use "Q_SLOTS Q_EMIT Q_SIGNALS" instead of "slots emit signals".
#
# Set as usage requirements (INTERFACE) on the Qt imported QtCore library so that all consuming targets,
# including the other Qt imported libraries like QtGui as well as our targets, will inherit them.
#
# Note: Prior to CMake 3.11 none of the 'target_*()' commands could set INTERFACE_ properties on imported targets.
if (CMAKE_VERSION VERSION_LESS 3.11)
	# Using 'set_propery' since 'set_target_property' cannot append.
	set_property(TARGET Qt5::Core APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS QT_NO_KEYWORDS)
else()
	target_compile_definitions(Qt5::Core INTERFACE QT_NO_KEYWORDS)
endif()

#
# Qwt dependency.
#
target_link_libraries(gplates-lib PUBLIC Qwt::Qwt)
target_link_libraries(pygplates PUBLIC Qwt::Qwt)
# Define Qwt version keywords.
#
# Set as usage requirements (INTERFACE) on the OpenGL imported library target.
#
# Note: Prior to CMake 3.11 none of the 'target_*()' commands could set INTERFACE_ properties on imported targets.
if (CMAKE_VERSION VERSION_LESS 3.11)
	# Using 'set_propery' since 'set_target_property' cannot append.
	set_property(TARGET Qwt::Qwt APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS
		QWT_MAJOR_VERSION=${QWT_MAJOR_VERSION}
		QWT_MINOR_VERSION=${QWT_MINOR_VERSION}
		QWT_PATCH_VERSION=${QWT_PATCH_VERSION})
else()
	target_compile_definitions(Qwt::Qwt INTERFACE
		QWT_MAJOR_VERSION=${QWT_MAJOR_VERSION}
		QWT_MINOR_VERSION=${QWT_MINOR_VERSION}
		QWT_PATCH_VERSION=${QWT_PATCH_VERSION})
endif()

#
# CGAL dependency.
#
target_link_libraries(gplates-lib PUBLIC CGAL::CGAL)
target_link_libraries(pygplates PUBLIC CGAL::CGAL)

#
# GDAL dependency.
#
target_link_libraries(gplates-lib PUBLIC GDAL::GDAL)
target_link_libraries(pygplates PUBLIC GDAL::GDAL)

#
# PROJ dependency.
#
target_link_libraries(gplates-lib PUBLIC PROJ4::proj)
target_link_libraries(pygplates PUBLIC PROJ4::proj)

#
# Python dependency.
#
if (TARGET Python3::Python)
	# We used the Python3 find module.
	target_link_libraries(gplates-lib PUBLIC Python3::Python)
	# Note that the Python3_add_library() call above has already linked pygplates to Python3::Module.
elseif (TARGET Python2::Python)
	# We used the Python2 find module.
	target_link_libraries(gplates-lib PUBLIC Python2::Python)
	# Note that the Python2_add_library() call above has already linked pygplates to Python2::Module.
else()
	# We used the PythonLibs find module (which does not have imported targets).
	target_link_libraries(gplates-lib PUBLIC ${PYTHON_LIBRARIES})
	target_link_libraries(pygplates PUBLIC ${PYTHON_LIBRARIES})
	target_include_directories(gplates-lib SYSTEM PUBLIC ${PYTHON_INCLUDE_DIRS})
	target_include_directories(pygplates SYSTEM PUBLIC ${PYTHON_INCLUDE_DIRS})
endif()
# Python is embedded in the GPlates application (as opposed to pygplates which is a Python extension library).
target_compile_definitions(gplates-lib PUBLIC GPLATES_PYTHON_EMBEDDING)


###########
# Install #
###########


#
# Set the platform-dependent icon file.
#
if (APPLE)
	set(GPlates_ICON "${GPlates_SOURCE_DIR}/cmake/distribution/gplates_desktop_icon.icns")
elseif (MSVC)
	set(GPlates_ICON "${GPlates_SOURCE_DIR}/cmake/distribution/gplates_desktop_icon.rc")
endif()
if (GPlates_ICON)
	target_sources_util(gplates PRIVATE ${GPlates_ICON})
endif()

# NOTE: On MacOS X some libraries referenced by these executable targets do not have
# their full path specified in the executables themselves. When this happens
# the error "dyld: Library not loaded:" error is generated when trying to
# run the executable. An example is CGAL installed via Macports.
# GPLATES_FIX_MACOS_LIBRARY_PATHS_IN_EXECUTABLE_TARGET() creates a post-build command
# that looks at a built executable target and does these fixups.
GPLATES_FIX_MACOS_LIBRARY_PATHS_IN_EXECUTABLE_TARGET(gplates TRUE)
GPLATES_FIX_MACOS_LIBRARY_PATHS_IN_EXECUTABLE_TARGET(gplates-no-gui FALSE)
if (TARGET gplates-unit-test)
	GPLATES_FIX_MACOS_LIBRARY_PATHS_IN_EXECUTABLE_TARGET(gplates-unit-test FALSE)
endif()

# Referenced in "Package.cmake" so it knows which executable to package up.
set(GPLATES_MAIN_TARGET gplates)

if (WIN32)
	# Tell cmake to build the main target with as a GUI application (with a WinMain entry point).
	# This also tells Qt5::Core to link in its WinMain.
	set_target_properties(${GPLATES_MAIN_TARGET} PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

if (APPLE)
	# Copy icon file to 'Resources' directory inside application bundle.
	set_source_files_properties(${GPlates_ICON} PROPERTIES
		MACOSX_PACKAGE_LOCATION Resources)

	#
	# Tell cmake to build the main target as an application bundle.
	#
	set_target_properties(${GPLATES_MAIN_TARGET}
		PROPERTIES MACOSX_BUNDLE TRUE)
	# Specify our own Info.plist template file.
	# We add a couple of keys to ensure Qt 4.8.4 (and above) include support Mac Retina displays.
	set_target_properties(${GPLATES_MAIN_TARGET}
		PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${GPlates_SOURCE_DIR}/cmake/distribution/MacOSXBundleInfo.plist.in")
	set_target_properties(${GPLATES_MAIN_TARGET}
		PROPERTIES MACOSX_BUNDLE_INFO_STRING "${GPLATES_PACKAGE_DESCRIPTION_SUMMARY}")
	set_target_properties(${GPLATES_MAIN_TARGET}
		PROPERTIES MACOSX_BUNDLE_ICON_FILE gplates_desktop_icon.icns)
	set_target_properties(${GPLATES_MAIN_TARGET}
		PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "${GPLATES_VERSION_STRING}-${CMAKE_SYSTEM}")
	set_target_properties(${GPLATES_MAIN_TARGET}
		PROPERTIES MACOSX_BUNDLE_LONG_VERSION_STRING "${GPLATES_VERSION_STRING}")
	set_target_properties(${GPLATES_MAIN_TARGET}
		PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "${GPLATES_PACKAGE_NAME}")
	set_target_properties(${GPLATES_MAIN_TARGET}
		PROPERTIES MACOSX_BUNDLE_SHORT_VERSION_STRING "${GPlates_VERSION}")
	set_target_properties(${GPLATES_MAIN_TARGET}
		PROPERTIES MACOSX_BUNDLE_BUNDLE_VERSION "${GPlates_VERSION}")
	set_target_properties(${GPLATES_MAIN_TARGET}
		PROPERTIES MACOSX_BUNDLE_COPYRIGHT "${GPLATES_COPYRIGHT_STRING}")
endif()


include(GNUInstallDirs)
if (APPLE)
	install(TARGETS gplates
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
		CONFIGURATIONS release)
else()
	install(TARGETS gplates
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		CONFIGURATIONS release)
endif()

install(TARGETS pygplates
	DESTINATION lib/pygplates/revision${PYGPLATES_REVISION}/
	CONFIGURATIONS release)

# Include the package module which adds a 'package' target that packages up
# a standalone version of GPlates (with dependent libraries included) for distribution
# to another computer.
#
include(Package)

# Test whether Python embedding works.
# This must be run after the include and library paths have all been set.
include(TestPythonEmbedding)
