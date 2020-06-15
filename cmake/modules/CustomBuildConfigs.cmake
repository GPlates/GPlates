# Copyright (C) 2020 The University of Sydney, Australia
#
# This file is part of GPlates.
#
# GPlates is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License, version 2, as published by
# the Free Software Foundation.
#
# GPlates is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

###############################
# Custom build configurations #
###############################
#
# Add new custom build configurations (ProfileGprof and ProfileGplates) to
# existing Debug/Release/RelWithDebInfo/MinSizeRel configurations.
#

# Determine if the current generator is multi-configuration (Visual Studio and XCode), or not (Makefile).
if (CMAKE_VERSION VERSION_LESS 3.9)
	# The pre-CMake 3.9 way of detecting a multi-config generator:
	#
	# The following is from http://mail.kde.org/pipermail/kde-buildsystem/2008-November/005112.html...
	#
	# "The way to identify whether a generator is multi-configuration is to
	# check whether CMAKE_CONFIGURATION_TYPES is set.  The VS/XCode generators
	# set it (and ignore CMAKE_BUILD_TYPE).  The Makefile generators do not
	# set it (and use CMAKE_BUILD_TYPE).  If CMAKE_CONFIGURATION_TYPES is not
	# already set, don't set it."
	if (CMAKE_CONFIGURATION_TYPES)
		set(_GENERATOR_IS_MULTI_CONFIG TRUE)
	else()
		set(_GENERATOR_IS_MULTI_CONFIG FALSE)
	endif()
else()
	# The robust way to detect a multi-config generator (requires CMake 3.9).
	get_property(_GENERATOR_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
endif()

#
# Multi-configuration generators list the build types in CMAKE_CONFIGURATION_TYPES.
#
# Single-configuration generators have a single build type specified with CMAKE_BUILD_TYPE (eg, using
# "cmake -DCMAKE_BUILD_TYPE:STRING=Debug .") so we need to be explicit about allowed build configs.
#
if (_GENERATOR_IS_MULTI_CONFIG)
	set(_CONFIGURATION_TYPES ${CMAKE_CONFIGURATION_TYPES})
else()
	set(_CONFIGURATION_TYPES Debug Release RelWithDebInfo MinSizeRel)
endif()

#
# Add a ProfileGprof (gprof) build config, but only when using a GNU compiler (and not releasing to the public).
#
# Use '-DCMAKE_BUILD_TYPE:STRING=ProfileGprof'.
#
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU") 
    # Disable profiling when releasing to the public.
    if (NOT GPLATES_PUBLIC_RELEASE)
        # Create our own build type for profiling since there are no defaults that suit it.
        # Activate CMAKE_<tool>_FLAGS_PROFILEGPROF (note: 'CMAKE_<tool>_FLAGS' will get used too).
		set(CMAKE_CXX_FLAGS_PROFILEGPROF "-pg -ggdb3 ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}" CACHE STRING "")
		# CMake gives error if we don't set these too...
		set(CMAKE_C_FLAGS_PROFILEGPROF "-pg -ggdb3 ${CMAKE_C_FLAGS_RELWITHDEBINFO}" CACHE STRING "")
		set(CMAKE_EXE_LINKER_FLAGS_PROFILEGPROF "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO}" CACHE STRING "")
		set(CMAKE_SHARED_LINKER_FLAGS_PROFILEGPROF "${CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO}" CACHE STRING "")
		set(CMAKE_STATIC_LINKER_FLAGS_PROFILEGPROF "${CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO}" CACHE STRING "")
		set(CMAKE_MODULE_LINKER_FLAGS_PROFILEGPROF "${CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO}" CACHE STRING "")
		mark_as_advanced(
			CMAKE_CXX_FLAGS_PROFILEGPROF
			CMAKE_C_FLAGS_PROFILEGPROF
			CMAKE_EXE_LINKER_FLAGS_PROFILEGPROF
			CMAKE_SHARED_LINKER_FLAGS_PROFILEGPROF
			CMAKE_STATIC_LINKER_FLAGS_PROFILEGPROF
			CMAKE_MODULE_LINKER_FLAGS_PROFILEGPROF)
			
        # We have an extra build configuration.
        list(APPEND _CONFIGURATION_TYPES ProfileGprof)
    endif (NOT GPLATES_PUBLIC_RELEASE)
endif()

#
# Add a ProfileGplates build config for profiling with GPlates inbuilt profiler (but not when releasing to the public).
#
# Use '-DCMAKE_BUILD_TYPE:STRING=ProfileGplates'.
#
# Disable profiling when releasing to the public.
if (NOT GPLATES_PUBLIC_RELEASE)
	# Activate CMAKE_<tool>_FLAGS_PROFILEGPLATES (note: 'CMAKE_<tool>_FLAGS' will get used too).
	set(CMAKE_CXX_FLAGS_PROFILEGPLATES "-DPROFILE_GPLATES ${CMAKE_CXX_FLAGS_RELEASE}" CACHE STRING "")
	# CMake gives error if we don't set these too...
	set(CMAKE_C_FLAGS_PROFILEGPLATES "-DPROFILE_GPLATES ${CMAKE_CXX_FLAGS_RELEASE}" CACHE STRING "")
	set(CMAKE_EXE_LINKER_FLAGS_PROFILEGPLATES "${CMAKE_EXE_LINKER_FLAGS_RELEASE}" CACHE STRING "")
	set(CMAKE_SHARED_LINKER_FLAGS_PROFILEGPLATES "${CMAKE_SHARED_LINKER_FLAGS_RELEASE}" CACHE STRING "")
	set(CMAKE_STATIC_LINKER_FLAGS_PROFILEGPLATES "${CMAKE_STATIC_LINKER_FLAGS_RELEASE}" CACHE STRING "")
	set(CMAKE_MODULE_LINKER_FLAGS_PROFILEGPLATES "${CMAKE_MODULE_LINKER_FLAGS_RELEASE}" CACHE STRING "")
	mark_as_advanced(
		CMAKE_CXX_FLAGS_PROFILEGPLATES
		CMAKE_C_FLAGS_PROFILEGPLATES
		CMAKE_EXE_LINKER_FLAGS_PROFILEGPLATES
		CMAKE_SHARED_LINKER_FLAGS_PROFILEGPLATES
		CMAKE_STATIC_LINKER_FLAGS_PROFILEGPLATES
		CMAKE_MODULE_LINKER_FLAGS_PROFILEGPLATES)

	# We have an extra build configuration.
	list(APPEND _CONFIGURATION_TYPES ProfileGplates)
endif()

#
# Update CMake's list of build configs (with the new custom build configs).
#
if (_GENERATOR_IS_MULTI_CONFIG)
	# Add any custom build configs into CMAKE_CONFIGURATION_TYPES (ie, any configs not already in CMAKE_CONFIGURATION_TYPES).
	#
	# Note that we don't set this as a cache variable. Setting as cache is not recommended because it could overwrite something the user added
	# to the cache. But we could set the cache since we're not overriding the cache (because we've only *added* to the cache variable).
	# However not setting the cache has the benefit of not causing the CMake GUI to have one red line per custom config tempting us to
	# press 'configure' a third time (should only need to do it twice) because it added CMAKE_RC_FLAGS_<CONFIG> that we didn't add above
	# (for example, with ProfileGplates it adds CMAKE_RC_FLAGS_PROFILEGPLATES). We could have explicitly added that above though, but it's
	# not required (unlike CMAKE_{C,CXX}_FLAGS_<CONFIG> and CMAKE_{EXE,SHARED,STATIC,MODULE}_LINKER_FLAGS_<CONFIG>).
	set(CMAKE_CONFIGURATION_TYPES ${_CONFIGURATION_TYPES})
else()
	# Get the CMake GUI to present a combo box of allowed build configs.
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "${_CONFIGURATION_TYPES}")
	
	# If no build configuration type was given as a command-line option to 'cmake' then a default cache entry is set here.
	# Set default build config to "Release" since most users will not specify a build type but want it to generate a fast (release) build.
	if (NOT CMAKE_BUILD_TYPE)
		set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
	endif()

	# Require a valid build type.
	if (NOT CMAKE_BUILD_TYPE IN_LIST _CONFIGURATION_TYPES)
		message(FATAL_ERROR "Invalid build type: ${CMAKE_BUILD_TYPE} not in ${_CONFIGURATION_TYPES}.")
	endif()
endif()
