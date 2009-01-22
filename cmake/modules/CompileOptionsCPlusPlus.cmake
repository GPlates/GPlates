#######################
# C++ compile options #
#######################

# Disable pre-compiled headers if showing include headers.
# The only reason to show include headers is to use 'list_external_includes.py' script to generates pch header.
if (GPLATES_SHOW_INCLUDES)
    set(GPLATES_USE_PCH false)
endif (GPLATES_SHOW_INCLUDES)

# Mac OS 10.4 produced alot of "error: template with C linkage" errors when using -isystem for 'src/system-fixes' directory.
# Mac OS 10.5 was fine though - could just be the g++ compiler version (was 4.0.0 on OS 10.4 and 4.0.1 on OS 10.5).
# Just disable on all Macs for now.
if (NOT APPLE)
	set(SYSTEM_INCLUDE_FLAG SYSTEM)
endif (NOT APPLE)

#
# Compiler flags.
#

# NOTE: Setting 'CMAKE_CXX_FLAGS*' variables here will override the cached versions in 'CMakeCache.txt' file
# (the ones that appear in the GUI editor) when generating a native build environment, but won't change ' CMakeCache.txt' file.
# The cached versions are set to sensible defaults by CMake when it detects the compiler (defaults change with different compilers).
# You can override the cached versions completely using set(CMAKE_CXX_FLAGS ...) or
# add to them using set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ...").

# Mac OSX specific configuration options:
if(APPLE)
    # Automatically adds compiler definitions to all subdirectories too.
    add_definitions(/D__APPLE__)

    # Mac OSX uses CMAKE_COMPILER_IS_GNUCXX compiler (always?) which is set later below.
    # 'bind_at_load' causes undefined symbols to be referenced at load/launch.
    set(CMAKE_EXE_LINKER_FLAGS "-bind_at_load")
endif(APPLE)


# Our Visual Studio configuration:
if(MSVC)
    # Automatically adds compiler definitions to all subdirectories too.
    add_definitions(/D__WINDOWS__ /D_CRT_SECURE_NO_DEPRECATE)

    # Flags common to all build types.
    #       The default warning level /W3 seems sufficient (/W4 generates informational warnings which are not necessary to write good code).
    #       /WX - treat all warnings as errors.
    if (GPLATES_SOURCE_RELEASE)
        # Disable all warnings when releasing source code to non-developers.
        # Remove any warning flags that are on by default.
        string(REGEX REPLACE "/[Ww][a-zA-Z0-9]*" "" CMAKE_CXX_FLAGS_NO_WARNINGS "${CMAKE_CXX_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_NO_WARNINGS} /w")
    else (GPLATES_SOURCE_RELEASE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")
    endif (GPLATES_SOURCE_RELEASE)
    
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
	#set(CMAKE_CXX_FLAGS_DEBUG )
	#set(CMAKE_CXX_FLAGS_RELEASE )
	#set(CMAKE_CXX_FLAGS_RELWITHDEBINFO  )
	#set(CMAKE_CXX_FLAGS_MINSIZEREL )
    
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
            -W -Wall -pedantic -ansi -Werror -Wcast-align -Wwrite-strings -Wfloat-equal
            -Wno-unused-parameter -Wpointer-arith -Wshadow -Wnon-virtual-dtor
            -Woverloaded-virtual -Wno-long-long -Wold-style-cast)
    endif(APPLE)
    # Convert the semi-colon separated list 'warnings_flags_list' to the string 'warnings_flags' 
    # (otherwise semi-colons will appear on the compiler command-line).
    foreach(warning ${warnings_flags_list})
        set(warnings_flags "${warnings_flags} ${warning}")
    endforeach(warning ${warnings_flags_list})

    # Flags common to all build types.
    if (GPLATES_SOURCE_RELEASE)
        # Disable all warnings when releasing source code to non-developers.
        set(CMAKE_CXX_FLAGS "-w")
    else (GPLATES_SOURCE_RELEASE)
        set(CMAKE_CXX_FLAGS "${warnings_flags}")
    endif (GPLATES_SOURCE_RELEASE)
    #set(CMAKE_EXE_LINKER_FLAGS )
    #set(CMAKE_SHARED_LINKER_FLAGS )
    #set(CMAKE_MODULE_LINKER_FLAGS )

    # Build configuration-specific flags.
	set(CMAKE_CXX_FLAGS_DEBUG "-O0 -ggdb3")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO  "-O3 -ggdb3")
	set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os")

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

    # There are _DEBUG, _RELEASE, _RELWITHDEBINFO, _MINSIZEREL and _PROFILEGPROF suffixes for CMAKE_*_LINKER_FLAGS
    # where '*' is EXE, SHARED and MODULE.
endif(CMAKE_COMPILER_IS_GNUCXX)

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

