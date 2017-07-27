#######################
# C++ compile options #
#######################

#
# Compiler flags.
#

# NOTE: Setting 'CMAKE_CXX_FLAGS*' variables here will override the cached versions in 'CMakeCache.txt' file
# (the ones that appear in the GUI editor) when generating a native build environment, but won't change ' CMakeCache.txt' file.
# The cached versions are set to sensible defaults by CMake when it detects the compiler (defaults change with different compilers).

# WARNING: The CGAL library requires applications that use it to have the same settings as
# when the CGAL library itself was built.
# So do not override the cached versions completely using set(CMAKE_CXX_FLAGS ...).
# Instead use set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ...") to add to the current versions
# already set by CGAL when find_package("CGAL") was called - actually CGAL overrides CMAKE_CXX_FLAGS the first time cmake
# configures (which overrides our settings in this file) but in the second configure
# (there appears to be two configure stages in cmake) CGAL does not override and then we add to CGAL's settings here.

# Our Visual Studio configuration:
if(MSVC)
    # Automatically adds compiler definitions to all subdirectories too.
    add_definitions(/D__WINDOWS__ /D_CRT_SECURE_NO_DEPRECATE)

    # Flags common to all build types.
    #       The default warning level /W3 seems sufficient (/W4 generates informational warnings which are not necessary to write good code).
    #       /WX - treat all warnings as errors.
    if (GPLATES_PUBLIC_RELEASE)
        # Disable all warnings when releasing source code to non-developers.
        # Remove any warning flags that are on by default.
        string(REGEX REPLACE "/[Ww][a-zA-Z0-9]*" "" CMAKE_CXX_FLAGS_NO_WARNINGS "${CMAKE_CXX_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_NO_WARNINGS} /w")
		add_definitions(/DGPLATES_PUBLIC_RELEASE)
    else (GPLATES_PUBLIC_RELEASE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")
    endif (GPLATES_PUBLIC_RELEASE)
    
    # If we've been asked to output a list of header files included by source files.
    if (GPLATES_SHOW_INCLUDES)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /showIncludes")
    endif (GPLATES_SHOW_INCLUDES)
    
    # If we've been asked to do parallel builds in Visual Studio 2005 within a project.
    if (GPLATES_MSVC80_PARALLEL_BUILD)
    	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    endif (GPLATES_MSVC80_PARALLEL_BUILD)

	# Enable 4Gb of virtual address space instead of 2Gb (default for Windows).
	# This doubles addressable memory if GPlates is compiled as 32-bit but run on a 64-bit Windows OS.
	# On a 32-bit Windows OS this won't help because only 2Gb (by default) is accessible
	# (the 2-4Gb process address range is reserved for the system).
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LARGEADDRESSAWARE")
	# The following are mainly for the pygplates DLL/pyd.
	# But it doesn't seem to be needed since '/LARGEADDRESSAWARE' only needs to be built into the
	# 'python.exe' (if building Python from source code) - which it isn't set anyway
	# (in the Python 2.7 source code) - so it doesn't matter.
	# We'll set it on the pygplates DLL anyway in case any of the above changes.
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /LARGEADDRESSAWARE")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /LARGEADDRESSAWARE")
	
	# Increase pre-compiled header memory allocation limit to avoid compile error.
	# Error happens on 12-core Windows 8.1 machine (in Visual Studio 2005).
   	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zm1000")

    # Build configuration-specific flags.
    # The defaults look reasonable...
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /DGPLATES_DEBUG")
	# set(CMAKE_CXX_FLAGS_RELEASE )
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /DGPLATES_DEBUG")
	# set(CMAKE_CXX_FLAGS_MINSIZEREL )
    
    # There are _DEBUG, _RELEASE, _RELWITHDEBINFO and _MINSIZEREL suffixes for CMAKE_*_LINKER_FLAGS
    # where '*' is EXE, SHARED and MODULE.
endif(MSVC)

# Our G++ configuration:
if(CMAKE_COMPILER_IS_GNUCXX)

	# Mac OSX specific configuration options:
	if(APPLE)
		# Detect Mac OSX version.
		execute_process(COMMAND "sw_vers" "-productVersion"
			OUTPUT_VARIABLE OSX_VERSION
			RESULT_VARIABLE OSX_VERSION_RESULT)
		if (NOT OSX_VERSION_RESULT)
			# Convert 10.4.11 to 10.4 for example.
			string(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.[0-9]+[ \t\r\n]*" "\\1.\\2" OSX_MAJOR_MINOR_VERSION ${OSX_VERSION})
			string(REGEX REPLACE "([0-9]+)\\.[0-9]+" "\\1" OSX_MAJOR_VERSION ${OSX_MAJOR_MINOR_VERSION})
			string(REGEX REPLACE "[0-9]+\\.([0-9]+)" "\\1" OSX_MINOR_VERSION ${OSX_MAJOR_MINOR_VERSION})
			add_definitions(-DMAC_OSX_MAJOR_VERSION=${OSX_MAJOR_VERSION})
			add_definitions(-DMAC_OSX_MINOR_VERSION=${OSX_MINOR_VERSION})
			if (OSX_MAJOR_MINOR_VERSION STREQUAL "10.4")
				message("Mac OSX version=${OSX_MAJOR_VERSION}.${OSX_MINOR_VERSION} (Tiger)")
				add_definitions(-DMAC_OSX_TIGER)
			elseif (OSX_MAJOR_MINOR_VERSION STREQUAL "10.5")
				message("Mac OSX version=${OSX_MAJOR_VERSION}.${OSX_MINOR_VERSION} (Leopard)")
				add_definitions(-DMAC_OSX_LEOPARD)
			endif (OSX_MAJOR_MINOR_VERSION STREQUAL "10.4")
		endif (NOT OSX_VERSION_RESULT)

		# Automatically adds compiler definitions to all subdirectories too.
		add_definitions(-D__APPLE__)

		# Mac OSX uses CMAKE_COMPILER_IS_GNUCXX compiler (always?) which is set later below.
		# 'bind_at_load' causes undefined symbols to be referenced at load/launch.
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -bind_at_load")
	endif(APPLE)

	# Detect g++ compiler version.
	# Generate a fatal error if the version is not 4.2 or above.
	# This is required by the CGAL dependency library because apparently there is a bug in g++ 4.0 on MacOS X
	# that causes CGAL to fail (see http://www.cgal.org/FAQ.html#mac_optimization_bug).
	# This is also required because we use certain GCC pragmas only available on 4.2 and above.
	execute_process(COMMAND "${CMAKE_CXX_COMPILER}" "-dumpversion"
		OUTPUT_VARIABLE CXX_VERSION
		RESULT_VARIABLE CXX_VERSION_RESULT)
	if (CXX_VERSION_RESULT)
		message(FATAL_ERROR "Could not detect g++ compiler version.")
	else (CXX_VERSION_RESULT)
		# Convert 4.2.1 to 4.2 for example.
		string(STRIP ${CXX_VERSION} CXX_VERSION)
		string(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.[0-9]+[ \t\r\n]*" "\\1.\\2" CXX_MAJOR_MINOR_VERSION ${CXX_VERSION})
		string(REGEX REPLACE "([0-9]+)\\.[0-9]+" "\\1" CXX_MAJOR_VERSION ${CXX_MAJOR_MINOR_VERSION})
		string(REGEX REPLACE "[0-9]+\\.([0-9]+)" "\\1" CXX_MINOR_VERSION ${CXX_MAJOR_MINOR_VERSION})
		add_definitions(-DCXX_MAJOR_VERSION=${CXX_MAJOR_VERSION})
		add_definitions(-DCXX_MINOR_VERSION=${CXX_MINOR_VERSION})
		message(STATUS "Found g++ version ${CXX_VERSION}")
		if (CXX_MAJOR_VERSION STRLESS "4")
			message(FATAL_ERROR "g++ compiler version less than 4.0.")
		elseif (CXX_MINOR_VERSION STRLESS "2")
			message(FATAL_ERROR "Require g++ compiler version 4.2 or above. "
					"Try using 'cmake -DCMAKE_CXX_COMPILER=/usr/bin/g++-4.2 ...'.")
		endif (CXX_MAJOR_VERSION STRLESS "4")
	endif (CXX_VERSION_RESULT)

    if(APPLE)
        # The compilers under OSX seem to behave oddly with '-isystem'.
        # Headers in system include paths (and '-isystem' paths) should not generate warnings.
        # However on OSX they do and because we have '-Werror' they become errors.
        # FIXME: temporary solution is to turn off warnings for OSX.
        # All GPlates developers currently use Linux or Windows.
        set(warnings_flags_list )
    else(APPLE)
        # Use a list instead of a string so we can have multiple lines (instead of one giant line).
        set(warnings_flags_list
            -W -Wall -Werror -Wcast-align -Wwrite-strings -Wfloat-equal
            -Wno-unused-parameter -Wpointer-arith -Wshadow -Wnon-virtual-dtor
            -Woverloaded-virtual -Wno-long-long -Wold-style-cast)
 	
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem /usr/include/qt4")
    endif(APPLE)
    # Convert the semi-colon separated list 'warnings_flags_list' to the string 'warnings_flags' 
    # (otherwise semi-colons will appear on the compiler command-line).
    foreach(warning ${warnings_flags_list})
        set(warnings_flags "${warnings_flags} ${warning}")
    endforeach(warning ${warnings_flags_list})
	
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ansi -fno-strict-aliasing")

    # Flags common to all build types.
    if (GPLATES_PUBLIC_RELEASE)
        # Disable all warnings when releasing source code to non-developers.
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
		add_definitions(-DGPLATES_PUBLIC_RELEASE)
    else (GPLATES_PUBLIC_RELEASE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${warnings_flags}")
		# G++ 4.2 is not respecting pragmas turning off -Wuninitialized so we'll just
		# shut it up here.
		if (CXX_MAJOR_VERSION EQUAL "4")
			if (CXX_MINOR_VERSION STRLESS "4")
				set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-uninitialized")
			endif (CXX_MINOR_VERSION STRLESS "4")
			if ((CXX_MINOR_VERSION EQUAL "7") OR (CXX_MINOR_VERSION EQUAL "8") OR (CXX_MINOR_VERSION EQUAL "9"))
				#The gcc 4.7, 4.8.1 and 4.9.1 report maybe-uninitialized warning when the default boost::optional<> declaration is present.
				#The boost::optional<> has been used widely throughout gplates. So supress the error for now.
				set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-maybe-uninitialized")
			endif ((CXX_MINOR_VERSION EQUAL "7") OR (CXX_MINOR_VERSION EQUAL "8") OR (CXX_MINOR_VERSION EQUAL "9"))
			if (CXX_MINOR_VERSION EQUAL "9")
				# gcc 4.9.1 reports warnings on unused functions. We prefer to keep unused functions available,
				# so suppress the warning.
				set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function -Wno-clobbered")
			endif (CXX_MINOR_VERSION EQUAL "9")
		endif (CXX_MAJOR_VERSION EQUAL "4")
		if (CXX_MAJOR_VERSION EQUAL "5")
			# gcc 5.x reports warnings on unused functions. We prefer to keep unused functions available,
			# so suppress the warning.
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function")
			# boost::optional<> is used widely throughout GPlates. So supress the error for now.
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-maybe-uninitialized")
		endif (CXX_MAJOR_VERSION EQUAL "5")
    endif (GPLATES_PUBLIC_RELEASE)

    #set(CMAKE_EXE_LINKER_FLAGS )
    #set(CMAKE_SHARED_LINKER_FLAGS )
    #set(CMAKE_MODULE_LINKER_FLAGS )

    # Build configuration-specific flags.
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb3 -DGPLATES_DEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O3 -ggdb3 -DGPLATES_DEBUG")
	set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -Os")
	
    # Disable profiling when releasing to the public.
    if (NOT GPLATES_PUBLIC_RELEASE)
        # Create our own build type for profiling since there are no defaults that suit it.
        # Use '-DCMAKE_BUILD_TYPE:STRING=profilegprof' option to 'cmake' to generate a gprof profile build environment
        # and activate 'CMAKE_CXX_FLAGS_PROFILEGPROF' (note: 'CMAKE_CXX_FLAGS' will get used too).
        set(CMAKE_CXX_FLAGS_PROFILEGPROF "-pg -O2 -ggdb3" CACHE STRING
            "Flags used by the C++ compiler during gprof profile builds."
            FORCE)
        mark_as_advanced(CMAKE_CXX_FLAGS_PROFILEGPROF)
        set(CMAKE_EXE_LINKER_FLAGS_PROFILEGPROF "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
        set(CMAKE_SHARED_LINKER_FLAGS_PROFILEGPROF "${CMAKE_SHARED_LINKER_FLAGS_RELEASE}")
        set(CMAKE_MODULE_LINKER_FLAGS_PROFILEGPROF "${CMAKE_MODULE_LINKER_FLAGS_RELEASE}")
        # We have an extra build configuration.
        set(extra_build_configurations "${extra_build_configurations} ProfileGprof")
        
        # Apparently this variable should only be set if it currently exists because it's not used in all native build environments.
        # See http://mail.kde.org/pipermail/kde-buildsystem/2008-November/005108.html.
        if(CMAKE_CONFIGURATION_TYPES)
          set(CMAKE_CONFIGURATION_TYPES ${CMAKE_CONFIGURATION_TYPES} ProfileGprof CACHE STRING "" FORCE)
        endif(CMAKE_CONFIGURATION_TYPES)
    endif (NOT GPLATES_PUBLIC_RELEASE)

    # There are _DEBUG, _RELEASE, _RELWITHDEBINFO, _MINSIZEREL and _PROFILEGPROF suffixes for CMAKE_*_LINKER_FLAGS
    # where '*' is EXE, SHARED and MODULE.
endif(CMAKE_COMPILER_IS_GNUCXX)

# Suppress warnings under clang (at least under Apple LLVM 5.1)
# Otherwise we get a lot of redeclared-class-member warnings from boost (from boost 1.47 at least), related to BOOST_BIMAP, and
# unused argument warnings -L/Library/Frameworks - possibly due to multiple installations of python, an unused one
# of which may be in /Library/Frameworks
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    message(STATUS "Using ${CMAKE_CXX_COMPILER_ID}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-redeclared-class-member -Qunused-arguments")
endif()


# The 64-bit C99 macro UINT64_C macro fails to compile on Visual Studio 2005 using boost 1.36.
# Boost 1.42 defines __STDC_CONSTANT_MACROS in <boost/cstdint.hpp> but prior to that the application
# is required to define it and it needs to be defined before any header inclusion to ensure it is defined
# before it is accessed (which means before pre-compiled headers). So we define it on the compiler command-line.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__STDC_CONSTANT_MACROS")

# Boost 1.58 introduced a breaking change in boost::variant that does compile-time with boost::get<U>(variant)
# to see if U is one of the variant types. However it seems to generate compile errors for references and boost::optional.
# So we'll default to using the old relaxed (run-time) method.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_VARIANT_USE_RELAXED_GET_BY_DEFAULT")

# Create our own build type for profiling with GPlates inbuilt profiler.
# Use '-DCMAKE_BUILD_TYPE:STRING=profilegplates' option to 'cmake' to generate a gplates profile
# build environment and activate 'CMAKE_CXX_FLAGS_PROFILEGPLATES' (note: 'CMAKE_CXX_FLAGS' will get used too).
set(CMAKE_CXX_FLAGS_PROFILEGPLATES "-DPROFILE_GPLATES ${CMAKE_CXX_FLAGS_RELEASE}" CACHE STRING
    "Flags used by the C++ compiler during gplates profile builds."
    FORCE)
mark_as_advanced(CMAKE_CXX_FLAGS_PROFILEGPLATES)
set(CMAKE_EXE_LINKER_FLAGS_PROFILEGPLATES "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
set(CMAKE_SHARED_LINKER_FLAGS_PROFILEGPLATES "${CMAKE_SHARED_LINKER_FLAGS_RELEASE}")
set(CMAKE_MODULE_LINKER_FLAGS_PROFILEGPLATES "${CMAKE_MODULE_LINKER_FLAGS_RELEASE}")
# We have an extra build configuration.
set(extra_build_configurations "${extra_build_configurations} ProfileGplates")

# Apparently this variable should only be set if it currently exists because it's not used in all native build environments.
# See http://mail.kde.org/pipermail/kde-buildsystem/2008-November/005108.html.
if(CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES ${CMAKE_CONFIGURATION_TYPES} ProfileGplates CACHE STRING "" FORCE)
endif(CMAKE_CONFIGURATION_TYPES)

# Let the user know what flags we are using.
# NOTE: This is only here because CGAL also prints the compiler/linker flags but then we add our own flags
# so this prints out the final version of the compiler/linker flags.
string( TOUPPER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_UPPER )
message( STATUS "USING CXXFLAGS = '${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE_UPPER}}'" )
message( STATUS "USING EXEFLAGS = '${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_EXE_LINKER_FLAGS_${CMAKE_BUILD_TYPE_UPPER}}'" )

if(GPLATES_PYTHON_3)
    add_definitions(-DGPLATES_PYTHON_3)
endif(GPLATES_PYTHON_3)
