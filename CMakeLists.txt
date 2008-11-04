project(GPlates)

cmake_minimum_required(VERSION 2.4.0)

#
# Useful CMAKE variables.
#
# There are three configuration files:
#   1) "ConfigDefault.cmake" - is version controlled and used to add new default variables and set defaults for everyone.
#   2) "ConfigUser.cmake" in the source tree - is not version controlled (currently listed in svn:ignore property) and used to override defaults on a per-user basis.
#   3) "ConfigUser.cmake" in the build tree - is used to override "ConfigUser.cmake" in the source tree.
#
# NOTE: If you want to change CMake behaviour just for yourself then modify the "ConfigUser.cmake" file (not "ConfigDefault.cmake").
#
include ("${CMAKE_SOURCE_DIR}/cmake/ConfigDefault.cmake")

# If "ConfigUser.cmake" doesn't exist then create one for convenience.
if (NOT EXISTS "${CMAKE_SOURCE_DIR}/cmake/ConfigUser.cmake")
    file(WRITE "${CMAKE_SOURCE_DIR}/cmake/ConfigUser.cmake"
        "# Use this file to override variables in 'ConfigDefault.cmake' on a per-user basis.\n"
        "# This file is not version controlled.\n"
        "\n"
        "# Pre-compiled headers are turned off by default.\n"
        "# Developers of GPlates may want to turn them on here to speed up build times.\n"
        "set(GPLATES_USE_PCH false)\n\n"
        "# When using Visual Studio this shows included headers (used by 'list_external_includes.py').\n"
        "# This disables pre-compiled headers (regardless of value of 'GPLATES_USE_PCH').\n"
        "set(GPLATES_SHOW_INCLUDES false)\n\n"
        "# If Visual Studio 2005 then enable parallel builds within a project.\n"
        "#\n"
        "# To also enable parallel project builds set\n"
        "# “Tools->Options->Programs and Solutions->Build and Run->maximum number of parallel project builds” to\n"
        "# the number of cores on your CPU.\n"
        "if (MSVC80)\n"
        "	#set(CMAKE_CXX_FLAGS \"\${CMAKE_CXX_FLAGS} /MP\")\n"
        "endif (MSVC80)\n\n")
endif (NOT EXISTS "${CMAKE_SOURCE_DIR}/cmake/ConfigUser.cmake")
include ("${CMAKE_SOURCE_DIR}/cmake/ConfigUser.cmake")

# If you've got a 'ConfigUser.cmake' in the build tree then that overrides the
# one in the source tree.
if (EXISTS "${CMAKE_BINARY_DIR}/cmake/ConfigUser.cmake")
    include ("${CMAKE_BINARY_DIR}/cmake/ConfigUser.cmake")
endif (EXISTS "${CMAKE_BINARY_DIR}/cmake/ConfigUser.cmake")

# Disable pre-compiled headers if showing include headers.
# The only reason to show include headers is to use 'list_external_includes.py' script to generates pch header.
if (GPLATES_SHOW_INCLUDES)
    set(GPLATES_USE_PCH false)
endif (GPLATES_SHOW_INCLUDES)

#
# CMake version 2.6 introduces policies to tell CMake to behave like a certain version ( > 2.4 ).
# NOTE: we don't include if CMake version is less than 2.6 because we know 2.4.6 has trouble parsing "if (POLICY...)".
#
if ("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" STRGREATER "2.5")
    include (${CMAKE_MODULE_PATH}/Policies_26.cmake)
endif ("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" STRGREATER "2.5")

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
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")
    
    # If we've been asked to output a list of header files included by source files.
    if (GPLATES_SHOW_INCLUDES)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /showIncludes")
    endif (GPLATES_SHOW_INCLUDES)

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
    # Use a list instead of a string so we can have multiple lines (instead of one giant line).
    if(APPLE)
        # The compilers under OSX seem to behave oddly with '-isystem'.
        # Headers in system include paths (and '-isystem' paths) should not generate warnings.
        # However on OSX they do and because we have '-Werror' they become errors.
        # FIXME: temporary solution is to turn off warnings for OSX.
        # All GPlates developers currently use Linux or Windows.
        set(warnings_flags_list )
    else(APPLE)
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
	set(CMAKE_CXX_FLAGS "${warnings_flags}")
    #set(CMAKE_EXE_LINKER_FLAGS )
    #set(CMAKE_SHARED_LINKER_FLAGS )
    #set(CMAKE_MODULE_LINKER_FLAGS )

    # Build configuration-specific flags.
	set(CMAKE_CXX_FLAGS_DEBUG "-O0 -ggdb3")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO  "-O3 -ggdb3")
	set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os")

    # Create our own build type for profiling since there are no defaults that suit it.
    # Use '-DCMAKE_BUILD_TYPE:STRING=profile' option to 'cmake' to generate a profile build environment
    # and activate 'CMAKE_CXX_FLAGS_PROFILE' (note: 'CMAKE_CXX_FLAGS' will get used too).
    set(CMAKE_CXX_FLAGS_PROFILE "-pg -O0 -ggdb3" CACHE STRING
        "Flags used by the C++ compiler during profile builds."
        FORCE)
    mark_as_advanced(CMAKE_CXX_FLAGS_PROFILE)
    # We have an extra build configuration.
    set(extra_build_configurations "Profile")
    
    # There are _DEBUG, _RELEASE, _RELWITHDEBINFO, _MINSIZEREL and _PROFILE suffixes for CMAKE_*_LINKER_FLAGS
    # where '*' is EXE, SHARED and MODULE.
endif(CMAKE_COMPILER_IS_GNUCXX)


#
# Set default build configuration (for non Visual Studio builds).
#

add_subdirectory(src)
