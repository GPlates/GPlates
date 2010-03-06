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

    # Detect g++ compiler version.
    # Generate a fatal error if the version is not 4.2 or above.
    # This is required by the CGAL dependency library because apparently there is a bug in g++ 4.0 on MacOS X
    # that causes CGAL to fail (see http://www.cgal.org/FAQ.html#mac_optimization_bug).
    execute_process(COMMAND "${CMAKE_CXX_COMPILER}" "-dumpversion"
        OUTPUT_VARIABLE CXX_VERSION
        RESULT_VARIABLE CXX_VERSION_RESULT)
    if (NOT CXX_VERSION_RESULT)
		message("g++ version=${CXX_VERSION}")
        # Convert 4.2.1 to 4.2 for example.
        string(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.[0-9]+[ \t\r\n]*" "\\1.\\2" CXX_MAJOR_MINOR_VERSION ${CXX_VERSION})
        string(REGEX REPLACE "([0-9]+)\\.[0-9]+" "\\1" CXX_MAJOR_VERSION ${CXX_MAJOR_MINOR_VERSION})
        string(REGEX REPLACE "[0-9]+\\.([0-9]+)" "\\1" CXX_MINOR_VERSION ${CXX_MAJOR_MINOR_VERSION})
        add_definitions(-DCXX_MAJOR_VERSION=${CXX_MAJOR_VERSION})
        add_definitions(-DCXX_MINOR_VERSION=${CXX_MINOR_VERSION})
        if (CXX_MAJOR_VERSION STRLESS "4")
			message(FATAL_ERROR "g++ compiler version less than 4.0.")
        elseif (CXX_MINOR_VERSION STRLESS "2")
			message(FATAL_ERROR "Require g++ compiler version 4.2 or above for GPlates on MacOS X. "
					"Try using 'cmake -DCMAKE_CXX_COMPILER=/usr/bin/g++-4.2 ...'.")
        endif (CXX_MAJOR_VERSION STRLESS "4")
    endif (NOT CXX_VERSION_RESULT)

    # Automatically adds compiler definitions to all subdirectories too.
    add_definitions(-D__APPLE__)

    # Mac OSX uses CMAKE_COMPILER_IS_GNUCXX compiler (always?) which is set later below.
    # 'bind_at_load' causes undefined symbols to be referenced at load/launch.
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -bind_at_load")
endif(APPLE)


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

    #set(CMAKE_EXE_LINKER_FLAGS )
    #set(CMAKE_SHARED_LINKER_FLAGS )
    #set(CMAKE_MODULE_LINKER_FLAGS )

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
            -W -Wall -ansi -Werror -Wcast-align -Wwrite-strings -Wfloat-equal
            -Wno-unused-parameter -Wpointer-arith -Wshadow -Wnon-virtual-dtor
            -Woverloaded-virtual -Wno-long-long -Wold-style-cast
            -isystem /usr/include/qt4 -fno-strict-aliasing)
    endif(APPLE)
    # Convert the semi-colon separated list 'warnings_flags_list' to the string 'warnings_flags' 
    # (otherwise semi-colons will appear on the compiler command-line).
    foreach(warning ${warnings_flags_list})
        set(warnings_flags "${warnings_flags} ${warning}")
    endforeach(warning ${warnings_flags_list})

    # Flags common to all build types.
    if (GPLATES_PUBLIC_RELEASE)
        # Disable all warnings when releasing source code to non-developers.
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
		add_definitions(-DGPLATES_PUBLIC_RELEASE)
    else (GPLATES_PUBLIC_RELEASE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${warnings_flags}")
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

# Disable profiling when releasing to the public.
# This is also because cmake 2.4 won't let you add a new build type to Visual Studio.
# Cmake 2.6 does allow this but we don't want to force cmake 2.6 on the public.
# GPlates developers will have to use cmake 2.6 if they're using Visual Studio (otherwise 2.4.6 and above is fine otherwise).
if (NOT GPLATES_PUBLIC_RELEASE)
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
endif (NOT GPLATES_PUBLIC_RELEASE)

# Let the user know what flags we are using.
# NOTE: This is only here because CGAL also prints the compiler/linker flags but then we add our own flags
# so this prints out the final version of the compiler/linker flags.
message( STATUS "USING CXXFLAGS = '${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${CGAL_BUILD_TYPE_UPPER}}'" )
message( STATUS "USING EXEFLAGS = '${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_EXE_LINKER_FLAGS_${CGAL_BUILD_TYPE_UPPER}}'" )
