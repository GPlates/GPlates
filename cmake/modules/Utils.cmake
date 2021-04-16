#
# target_sources_util:
#
# Emulates the CMake 3.13 'target_sources()' command for earlier CMake 3.x versions.
# As with CMake 3.13 'target_sources()', each source should be specified either:
# - with a path relative to the current source directory (not the source directory of the target), or
# - with an absolute path.
function(target_sources_util target)
	if(POLICY CMP0076)
		# We have the new behaviour (that converts relative paths to absolute paths when necessary).
		# Make sure we use it by setting the behaviour to NEW (but just in the scope of this function).
		cmake_policy(PUSH)
		cmake_policy(SET CMP0076 NEW)
		target_sources(${target} ${ARGN})
		cmake_policy(POP)
		return()
	endif()
	
	#
	# Using CMake 3.12 or earlier, so simulate the new behavior.
	#
	
	# The value of the CMAKE_CURRENT_SOURCE_DIR variable in the directory in which the target was defined.
	get_target_property(_target_source_dir ${target} SOURCE_DIR)
	
	# For each target source that is a relative path, convert to a path that is relative to the
	# target source directory (as expected for CMake 3.12 and earlier).
	unset(_srcs)
	foreach(src ${ARGN})
		if(NOT src STREQUAL "PRIVATE" AND
			NOT src STREQUAL "PUBLIC" AND
			NOT src STREQUAL "INTERFACE" AND
			NOT IS_ABSOLUTE "${src}")
			file(RELATIVE_PATH src "${_target_source_dir}" "${CMAKE_CURRENT_LIST_DIR}/${src}")
		endif()
		list(APPEND _srcs ${src})
	endforeach()
	
	target_sources(${target} ${_srcs})
endfunction()
