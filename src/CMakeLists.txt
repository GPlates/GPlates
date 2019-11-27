# Find our required libraries.
set(required_libs "Qt4" "OpenGL" "GLEW" "GDAL" "GZIP" "PROJ" "CGAL" "Qwt")
foreach(lib ${required_libs})
	find_package(${lib} REQUIRED)
endforeach(lib)

# The following Boost libraries are both required and dynamically linked.
set(Boost_USE_STATIC_LIBS FALSE)
set(boost_components program_options thread system)

if(NOT GPLATES_NO_PYTHON)
	# Boost 1.67 and later require that boost python components have a Python version suffix, e.g. ``python27``.
	# But first we need to determine the boost library version.
	# To do this we do the simplest boost library find which is just to find the headers.
	find_package("Boost" 1.34.1)
	if (NOT Boost_FOUND)
		message(FATAL_ERROR "Boost headers not found - please make sure Boost library is installed.")
	endif ()
	if(NOT Boost_VERSION VERSION_LESS 106700)
		if(NOT GPLATES_PYTHON_3)
			set(boost_components ${boost_components} python27)
		else(NOT GPLATES_PYTHON_3)
			set(boost_components ${boost_components} python3)
		endif(NOT GPLATES_PYTHON_3)
	else()
		if(NOT GPLATES_PYTHON_3)
			set(boost_components ${boost_components} python)
		else(NOT GPLATES_PYTHON_3)
			set(boost_components ${boost_components} python3)
		endif(NOT GPLATES_PYTHON_3)
	endif()
endif(NOT GPLATES_NO_PYTHON)

find_package("Boost" 1.34.1 REQUIRED ${boost_components})

if(NOT GPLATES_NO_PYTHON)
	find_package("PythonLibs" REQUIRED)
endif(NOT GPLATES_NO_PYTHON)

# Check that each of the above libraries was found, because even though it is
# required, if Boost is found but not the libraries, the CMake scripts will
# happily continue on their merry way.
foreach(boost_component ${boost_components})
	string(TOUPPER ${boost_component} boost_component_uppercase)
	if(NOT Boost_${boost_component_uppercase}_FOUND)
		message(FATAL_ERROR "Boost ${boost_component} library not found - please make sure it is installed.")
	endif(NOT Boost_${boost_component_uppercase}_FOUND)
endforeach(boost_component)


# The Boost unit test framework is optional, because we only need it for the
# unit-test version of GPlates and it is possible to compile the GUI version
# of GPlates without it, and it is statically linked, because it contains the
# main() function.
set(Boost_USE_STATIC_LIBS TRUE)
find_package("Boost" 1.34.1 COMPONENTS unit_test_framework)

# However we do need to find the boost include directory to locate the headers.
if (NOT Boost_INCLUDE_DIR)
	message(FATAL_ERROR "Boost include path not found - please make sure boost is installed.")
endif (NOT Boost_INCLUDE_DIR)

if (Boost_UNIT_TEST_FRAMEWORK_FOUND)
    set(ADD_UNIT_TEST_TARGET TRUE)
else (Boost_UNIT_TEST_FRAMEWORK_FOUND)
	# Only print status message if this is not a public release.
	# Because it'll only confuse users (because gplate-unit-test is not part of public release).
	if (NOT GPLATES_PUBLIC_RELEASE)
		message(STATUS "Warning: boost unit_test_framework not found so GPlates unit-test executable gplates-unit-test will not be available.")
	endif (NOT GPLATES_PUBLIC_RELEASE)
endif (Boost_UNIT_TEST_FRAMEWORK_FOUND)

find_package("GMP")

# Mac OS 10.4 produced alot of "error: template with C linkage" errors when using -isystem for 'src/system-fixes' directory.
# Mac OS 10.5 was fine though - could just be the g++ compiler version (was 4.0.0 on OS 10.4 and 4.0.1 on OS 10.5).
# Just disable on all Macs for now.
# Moved this from "CompileOptionsCPlusPlus.cmake" to here because it is used in the following included cmake scripts.
# TODO: Sort out why "-isystem" seems to fail on some compilers (this is what the "SYSTEM" cmake variable produces for gcc).
if (NOT APPLE)
	set(SYSTEM_INCLUDE_FLAG SYSTEM)
endif (NOT APPLE)

if ( NOT WIN32 )
	# CGAL expects that the build type is "Release" or "Debug".
	# If it is set to "profilegprof" or "profilegplates", we temporarily set the
	# build type to "Debug" and "Release", respectively, and set it back after
	# CGAL has finished setting itself up.
	# Note that "profilegprof" and "profilegplates" cannot be used on Windows.
	set(CMAKE_BUILD_TYPE_BEFORE_CGAL ${CMAKE_BUILD_TYPE})
	if (CMAKE_BUILD_TYPE STREQUAL "profilegprof")
		# "profilegprof" build type is closest to a "Debug" build.
		set(CMAKE_BUILD_TYPE "Debug")
	endif (CMAKE_BUILD_TYPE STREQUAL "profilegprof")
	if (CMAKE_BUILD_TYPE STREQUAL "profilegplates")
		# "profilegplates" build type is closest to a "Release" build.
		set(CMAKE_BUILD_TYPE "Release")
	endif (CMAKE_BUILD_TYPE STREQUAL "profilegplates")
endif ( NOT WIN32 )

# The following is necessary otherwise CGAL is going to set the built type to
# "Release" when running CMake for the first time, even if we set the build
# type to be "Debug".
set(CGAL_BUILD_TYPE_INIT ${CMAKE_BUILD_TYPE})

# Include CGAL.
include(${CGAL_USE_FILE})
if ( NOT WIN32 )
	# The system-provided CGAL CMake scripts do something silly: they insert
	# -g and -O<something> into CMAKE_CXX_FLAGS, which means that even if you
	# want a Release build, you get debug symbols, and the optimisation level
	# might conflict with the optimisation level you want for the particular
	# type of build (Release/Debug).
	# The two following lines scrub out the -g and -O<something> from
	# CMAKE_CXX_FLAGS but leave the good stuff (e.g. -frounding-math).
	if (CMAKE_CXX_FLAGS)
		string(REGEX REPLACE "-g" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
		string(REGEX REPLACE "-O." "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
	endif (CMAKE_CXX_FLAGS)

	# Valgrind doesn't emulate the FPU properly, so CGAL will complain about
	# incorrect rounding when run under Valgrind (this is even if we compile
	# with -frounding-math).
	# If you intend to run GPlates under Valgrind, set the CMake variable
	# GPlates_CGAL_DISABLE_ROUNDING_MATH_CHECK (type BOOL) to true.
	if ( GPlates_CGAL_DISABLE_ROUNDING_MATH_CHECK )
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCGAL_DISABLE_ROUNDING_MATH_CHECK")
	endif ( GPlates_CGAL_DISABLE_ROUNDING_MATH_CHECK )
endif ( NOT WIN32 )


if ( NOT WIN32 )
	# Restore CMAKE_BUILD_TYPE before we kludged it for CGAL (see above).
	set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE_BEFORE_CGAL})
	message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
endif ( NOT WIN32 )

# Tell Qt that we use QtXML.
# NOTE: subdirectory 'CMakeList.txt' files will also see our QT_USE_* variables.
SET(QT_USE_QTXML 1)
SET(QT_USE_QTXMLPATTERNS 1)
SET(QT_USE_QTNETWORK 1)
SET(QT_USE_QTOPENGL 1)
SET(QT_USE_QTSVG 1)
include(${QT_USE_FILE})

#Define QT_NO_KEYWORDS so that Qt will not pollute namespaces.
#Must use "Q_SLOTS Q_EMIT Q_SIGNALS" instead of "slots emit signals"
ADD_DEFINITIONS(-DQT_NO_KEYWORDS)

# Include compiler options for the C++ programming language.
# NOTE: We do this after all dependency packages have been found because we are going to modify the
# compiler/linker flags without overriding them (by appending to them).
# Previously this was included before the packages but now that we use CGAL (which sets the compiler flags
# to be the same as when the CGAL library was built) it's easiest if we just append our own flags after
# CGAL has overridden the flags.
include (CompileOptionsCPlusPlus)

# Some useful utilities.
include (Util)

#
# Generate 'global/Constants.cc' file from 'global/Constants.cc.in'.
#
# To change the current GPlates version string edit the "ConfigDefault.cmake" file.
#
include (GPlatesVersion)

# Source files.
set(GPlates_SRCS
	gplates_main.cc ScribeExportGPlates.cc)

set(GPlates_no_gui_SRCS
	gplates_demo_no_gui_main.cc ScribeExportGPlatesDemoNoGui.cc)

if (ADD_UNIT_TEST_TARGET)
    set(GPlates_unit_test_SRCS
	    gplates_unit_test_main.cc ScribeExportGPlatesUnitTest.cc)
endif (ADD_UNIT_TEST_TARGET)

# N.B.  The order in which some of these subdirectories are
# specified is significant.  For example, model must come before
# feature-visitors.  (This stops weird "undefined reference to 
# vtable for blah" error messages occurring.)
#
# NOTE: 'opengl' is an app library since it can be used to reconstruct floating-point
# rasters using an off-screen context (which is an application logic level operation).
set(gui_LIBS
	qt-widgets qt-resources)
set(presentation_LIBS
	view-operations presentation gui canvas-tools)
set(app_LIBS
	app-logic file-io feature-visitors opengl property-values model data-mining api)
set(util_LIBS
	maths scribe utils global)

set(cli_LIBS
	cli)

if (ADD_UNIT_TEST_TARGET)
set(unit_test_LIBS
    unit-test)
endif (ADD_UNIT_TEST_TARGET)

# Libraries used by the GUI version of GPlates.
# NOTE: The command-line version of GPlates is no longer a separate executable to the GUI version of GPlates.
set(GPlates_LIBS
	${cli_LIBS} ${gui_LIBS} ${presentation_LIBS} ${app_LIBS} ${util_LIBS})

# Libraries used by the GPlates unit test application.
# This currently includes all GPlates internal libraries
# in case any of them need to be tested.
if (ADD_UNIT_TEST_TARGET)
	set(GPlates_unit_test_LIBS
		${unit_test_LIBS} ${GPlates_LIBS})
endif (ADD_UNIT_TEST_TARGET)

# Visual Studio and Borland compilers use autolinking for the Boost libraries
# (where pragma in header file brings in the library).
# So instead of specifying the library in target_link_libraries() we
# specify the location of library directories.
if (WIN32)
	# Specify directories in which the linker will look for libraries.
	# This command will apply only to targets created after it is called.
	if (Boost_LIBRARY_DIRS)
		link_directories(${Boost_LIBRARY_DIRS})
	endif (Boost_LIBRARY_DIRS)
endif (WIN32)

# Add any include directories of generated header files here (eg, *Ui.h headers).
set(generated_headers_INCLUDE_DIRS
	${GPlates_BINARY_DIR}/src/qt-widgets	# Autogenerated source headers are written here (eg, *Ui.h files).
	${GPlates_BINARY_DIR}/src/)

# The 'include_directories' command doesn't propagate to subdirectories but any variable we set does.
# We'll use this variable in subdirectories to add these includes directories (avoids duplicating changes to subdirectories).
# External include directories:
# NOTE: Qt and CGAL includes are taken care of by including QT_USE_FILE and
# CGAL_USE_FILE respectively.
set(external_INCLUDE_DIRS
	${GPlates_SOURCE_DIR}/src/system-fixes	# Contains files copied from external libraries
	${OPENGL_INCLUDE_DIR}
	${GLEW_INCLUDE_DIR}
	${GDAL_INCLUDE_DIR}
	${GDAL_OGR_INCLUDE_DIR}
	${GDAL_PORT_INCLUDE_DIR}
	${GDAL_FRMTS_INCLUDE_DIR}
	${Boost_INCLUDE_DIR}
        ${PROJ_INCLUDE_DIR}
        ${QWT_INCLUDE_DIR})
if(NOT GPLATES_NO_PYTHON)
	set(external_INCLUDE_DIRS
		${external_INCLUDE_DIRS}
		${PYTHON_INCLUDE_PATH})
endif(NOT GPLATES_NO_PYTHON)
	
# Internal include directories:
set(internal_INCLUDE_DIRS
	${GPlates_SOURCE_DIR}/src				# A lot of "#include" statements reference from this directory.
    ${generated_headers_INCLUDE_DIRS})
# NOTE: if you're doing an in-place build then SOURCE and BINARY directories are the same and
# generated source files will be mixed in with your versioned source code.
	
# All the GPlates subdirectories.
set(ALL_GPlates_src_subdirs
	${gui_LIBS} ${presentation_LIBS} ${cli_LIBS} ${app_LIBS} ${util_LIBS} ${unit_test_LIBS} )

#
# Generate 'global/config.h' file from 'global/config.h.in' and add HAVE_CONFIG_H compiler definition so GPlates will look for it.
#
# This should be included after the above include and find_package calls and before the add_subdirectory calls because
# it uses the GDAL and Boost packages and it sets the HAVE_CONFIG_H compiler flag needed by source code in the subdirectories.
#
include (Config_h)

# Recurse into subdirectories - CMake continues processing this file when it returns.
# NOTE: any variables set after 'add_subdirectory' will not be visible in the subdirectory 'CMakeList.txt' files.
foreach(dir ${ALL_GPlates_src_subdirs})
	add_subdirectory(${dir})
endforeach(dir)

#
# Compile and run the gplates-extract-svn-info executable, which generates SubversionInfo.cc,
# a file that contains the parsed output of the 'svnversion' and 'svn info' commands.
#
# There is an optional CMake variable called GPLATES_SOURCE_CODE_CONTROL_VERSION_STRING
# that, if set, will cause the value of that variable to be used as the source
# control version instead of any value read from `svnversion` and "" to be
# used as the branch name. This optional variable is useful when making public
# releases from a branch (e.g. the 0.9.9 branch) or an exported source tree. The
# CMake type of the GPLATES_SOURCE_CODE_CONTROL_VERSION_STRING variable is STRING; it
# may be set to non-numeric string values.
#
add_executable(gplates-extract-svn-info
	gplates_extract_svn_info_main.cc)
GPLATES_FIX_MACOS_LIBRARY_PATHS_IN_EXECUTABLE_TARGET(gplates-extract-svn-info FALSE)
target_link_libraries(gplates-extract-svn-info ${QT_LIBRARIES})
set(GPlates_EXTRACT_SVN_INFO_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/global/SubversionInfo.cc)
if (WIN32)
	set (GPlates_EXTRACT_SVN_INFO_PATH gplates-extract-svn-info)
else (WIN32)
	set (GPlates_EXTRACT_SVN_INFO_PATH ${EXECUTABLE_OUTPUT_PATH}/gplates-extract-svn-info)
endif (WIN32)
add_custom_target(
	run-gplates-extract-svn-info ALL ${GPlates_EXTRACT_SVN_INFO_PATH} ${CMAKE_SOURCE_DIR} ${GPlates_EXTRACT_SVN_INFO_OUTPUT_PATH} ${GPLATES_SOURCE_CODE_CONTROL_VERSION_STRING}
	DEPENDS gplates-extract-svn-info)
# Create a dummy output file so that CMake doesn't complain about it being missing.
file(WRITE ${GPlates_EXTRACT_SVN_INFO_OUTPUT_PATH} "")

#
# Set top-level target dependencies.
# A top-level target is one created by ADD_EXECUTABLE, ADD_LIBRARY, or ADD_CUSTOM_TARGET.
#
# The 'qt-widgets' target generates header files so it should get built before anything that depends on it.
#
add_dependencies(canvas-tools qt-widgets)
add_dependencies(view-operations qt-widgets)
add_dependencies(presentation qt-widgets)
add_dependencies(gui qt-widgets)
if(NOT GPLATES_NO_PYTHON)
add_dependencies(api gui)
endif(NOT GPLATES_NO_PYTHON)
add_dependencies(global run-gplates-extract-svn-info)
# because gplates-extract-svn-info executable generates global/SubversionInfo.cc
#add_dependencies(qt-widgets qt-resources)

# Does the following:
# - finds '.ui' files and adds commands to create 'Ui.h' files.
# - adds dependencies on generated header files.
# - scans '.h' files for Q_OBJECT and adds commands to run Qt's moc.exe to generate 'moc_*.cc' files.
# - finds '.qrc' files and adds commands to generate '.cc' files from them.
# - adds header files to Visual Studio projects.
# - groups files in Visual Studio IDE according to type.
GPLATES_GENERATE_TARGET_SOURCES(gplates target_GPlates_SRCS ${GPlates_SRCS})
GPLATES_GENERATE_TARGET_SOURCES(gplates-no-gui target_GPlates_no_gui_SRCS ${GPlates_no_gui_SRCS})

if (ADD_UNIT_TEST_TARGET)
	GPLATES_GENERATE_TARGET_SOURCES(gplates-unit-test target_GPlates_unit_test_SRCS ${GPlates_unit_test_SRCS})
endif (ADD_UNIT_TEST_TARGET)

#
# Set the platform-dependent icon file.
#
if (APPLE)
	set(GPlates_ICON "${CMAKE_SOURCE_DIR}/cmake/distribution/gplates_desktop_icon.icns")
elseif (MSVC)
	set(GPlates_ICON "${CMAKE_SOURCE_DIR}/cmake/distribution/gplates_desktop_icon.rc")
else (APPLE)
endif (APPLE)

#
# Add executable target(s).
#
# NOTE: On MacOS X some libraries referenced by these executable targets do not have
# their full path specified in the executables themselves. When this happens
# the error "dyld: Library not loaded:" error is generated when trying to
# run the executable. An example is CGAL installed via Macports.
# GPLATES_FIX_MACOS_LIBRARY_PATHS_IN_EXECUTABLE_TARGET() creates a post-build command
# that looks at a built executable target and does these fixups.
#
message(STATUS "Adding executable for GPlates application.")
add_executable(gplates
	${target_GPlates_SRCS}
	${GPlates_ICON}
	${GPlates_EXTRACT_SVN_INFO_OUTPUT_PATH})
GPLATES_FIX_MACOS_LIBRARY_PATHS_IN_EXECUTABLE_TARGET(gplates TRUE)

# The "EXCLUDE_FROM_ALL" flag means you have to type 'make gplates-no-gui' to build this target -
# it is not built by default.
add_executable(gplates-no-gui EXCLUDE_FROM_ALL ${target_GPlates_no_gui_SRCS})
GPLATES_FIX_MACOS_LIBRARY_PATHS_IN_EXECUTABLE_TARGET(gplates-no-gui FALSE)

if (ADD_UNIT_TEST_TARGET)
    message(STATUS "Adding executable for GPlates unit test application.")
	add_executable(gplates-unit-test EXCLUDE_FROM_ALL
		${target_GPlates_unit_test_SRCS}
		${GPlates_EXTRACT_SVN_INFO_OUTPUT_PATH})
	GPLATES_FIX_MACOS_LIBRARY_PATHS_IN_EXECUTABLE_TARGET(gplates-unit-test FALSE)
endif (ADD_UNIT_TEST_TARGET)

# Referenced in "Package.cmake" so it knows which executable to package up.
set(GPLATES_MAIN_TARGET gplates)

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
		PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/distribution/MacOSXBundleInfo.plist.in")
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
		PROPERTIES MACOSX_BUNDLE_SHORT_VERSION_STRING "${GPLATES_PACKAGE_VERSION}")
	set_target_properties(${GPLATES_MAIN_TARGET}
		PROPERTIES MACOSX_BUNDLE_BUNDLE_VERSION "${GPLATES_PACKAGE_VERSION}")
	set_target_properties(${GPLATES_MAIN_TARGET}
		PROPERTIES MACOSX_BUNDLE_COPYRIGHT "${GPLATES_COPYRIGHT_STRING}")
endif (APPLE)

# NOTE: Even though Qt includes are taken care of by including QT_USE_FILE
# their libraries still have to be explicitly listed at each 'target_link_libraries()' call.
# CGAL also uses a CGAL_USE_FILE but it differs from QT in that it uses the deprecated
# 'link_libraries()' to link its library to all targets (it's not physically deprecated, just
# advised not to use). See http://www.cmake.org/pipermail/cmake/2009-April/028439.html
# for a description of the difference between 'link_libraries' and 'target_link_libraries'.
set(dependency_LIBS
	${GDAL_LIBRARY}
	${QT_LIBRARIES}
	${GLEW_LIBRARY}
	${OPENGL_LIBRARIES}
        ${PROJ_LIBRARY}
        ${QWT_LIBRARY})
	
if(NOT GPLATES_NO_PYTHON)
	set(dependency_LIBS
		${dependency_LIBS}
		${PYTHON_LIBRARIES})
endif(NOT GPLATES_NO_PYTHON)

if(GMP_FOUND)
	set(dependency_LIBS
		${dependency_LIBS}
		${GMP_LIBRARY})
endif(GMP_FOUND)

# Visual Studio and Borland compilers use autolinking for the Boost libraries
# (where pragma in header file brings in the library).
if (NOT WIN32)
	if (Boost_LIBRARY_DIRS)
		set(dependency_LIBS ${dependency_LIBS} ${Boost_LIBRARIES})
	endif (Boost_LIBRARY_DIRS)
endif (NOT WIN32)



# Avoid all GPlates library link dependencies on the linker command-line by
# specifying a 'group' around GPlates libraries. This gets around some dependencies
# like 'gui' depends on 'canvas-tools' depends on 'gui' depends on 'maths'.
# The --start-group / --end-group options are specific to Linux' ld command, and
# the -Xlinker option is necessary to pass those options through 'c++' into 'ld'.
# The Windows and MacOS linkers appear to work fine without needing to specify
# this extra resolution behaviour.
if (NOT WIN32 AND NOT APPLE)
	set(GPlates_LINK_CMD -Xlinker --start-group ${GPlates_LIBS} -Xlinker --end-group)
	set(GPlates_unit_test_LINK_CMD -Xlinker --start-group ${GPlates_unit_test_LIBS} -Xlinker --end-group)
else (NOT WIN32 AND NOT APPLE)
	set(GPlates_LINK_CMD ${GPlates_LIBS})	
	set(GPlates_unit_test_LINK_CMD ${GPlates_unit_test_LIBS})
endif (NOT WIN32 AND NOT APPLE)

# Actual link dependencies get assembled here.
# NOTE: The command-line version of GPlates is no longer a separate executable to the GUI version of GPlates.
target_link_libraries(gplates ${GPlates_LINK_CMD} ${dependency_LIBS})
target_link_libraries(gplates-no-gui ${GPlates_LINK_CMD} ${dependency_LIBS})
if (ADD_UNIT_TEST_TARGET)
	target_link_libraries(gplates-unit-test ${GPlates_unit_test_LINK_CMD} ${dependency_LIBS})
endif (ADD_UNIT_TEST_TARGET)
	
# Add Python module library target.
if(NOT GPLATES_NO_PYTHON)

	add_subdirectory(pygplates)
	add_dependencies(pygplates run-gplates-extract-svn-info)
	
endif(NOT GPLATES_NO_PYTHON)

# Current binary directory must be included first for g++ pre-compiled headers.
# There's a '_pch.h' file in current source and current binary directory however the '_pch.h.gch' pre-compiled header
# is only in current binary directory and it will only be used by the compiler if it's in the same directory as the found '_pch.h' file.
# So we need to ensure that we find the '_pch.h' file from the current binary directory first.
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# External library include directories are treated as system include directories.
include_directories(${SYSTEM_INCLUDE_FLAG} ${external_INCLUDE_DIRS})

# Generated source header include directories.
include_directories(${internal_INCLUDE_DIRS})

# Perform commands that modify target properties.
# We do this last because pre-compiled headers are done here and they
# require access to directory and target properties (eg, include directories).
GPLATES_POST_ADD_TARGET(gplates target_GPlates_SRCS)
GPLATES_POST_ADD_TARGET(gplates-no-gui target_GPlates_no_gui_SRCS)

if (ADD_UNIT_TEST_TARGET)
	GPLATES_POST_ADD_TARGET(gplates-unit-test target_GPlates_unit_test_SRCS)
endif (ADD_UNIT_TEST_TARGET)

if (APPLE)
    install(TARGETS gplates
    	RUNTIME DESTINATION bin
    	BUNDLE DESTINATION bin
    	CONFIGURATIONS release)
else (APPLE)
    install(TARGETS gplates
    	RUNTIME DESTINATION bin
    	CONFIGURATIONS release)
endif (APPLE)

# Include the package module which adds a 'package' target that packages up
# a standalone version of GPlates (with dependent libraries included) for distribution
# to another computer.
#
include(Package)

# We have a nasty case of a variable shadow warning in the boost program_options library.
# Since it's a system header it should not emit a warning but it does on g++ 4.1.2 and g++ 4.1.3 
# only ones tested so far).
# We use the nasty hack of turning off shadow warnings for the source files involved.
# 'COMPILE_FLAGS' just adds to the default compiler flags.
# TODO: find a better way of doing this sort of thing.
if (CMAKE_COMPILER_IS_GNUCXX)
	set_source_files_properties(gplates_main.cc gplates_cli.cc gplates_unit_test_main.cc PROPERTIES COMPILE_FLAGS "-Wno-shadow")
endif (CMAKE_COMPILER_IS_GNUCXX)

# Test whether Python embedding works.
# This must be run after the include and library paths have all been set.
if(NOT GPLATES_NO_PYTHON)
	include(TestPythonEmbedding)
endif(NOT GPLATES_NO_PYTHON)
