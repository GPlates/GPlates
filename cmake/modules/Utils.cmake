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

#
# read_sources:
#
# Reads a list of source files from a file named 'source_list_filebase' (in current source directory).
# Returns list of source files in the list variable 'sources'.
#
# Also this function ensures CMake is re-run whenever 'source_list_filebase' is modified,
# such as when a source file is added or removed.
#
# Note: Lines in the list file 'source_list_filebase' starting with '#' are ignored.
#
function(read_sources source_list_filebase sources)
	# Read the list file containing the source files (ignoring lines starting with '#').
	#
	# Note that CMake discourages use of the 'file(GLOB)' CMake command to automatically collect source files.
	# One of the reasons is if source files are added or removed, CMake is not automatically re-run,
	# so the build is unaware of the change.
	file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/${source_list_filebase} _sources REGEX "^[^#]")

	# Trigger CMake to re-run whenever "${CMAKE_CURRENT_SOURCE_DIR}/${source_list_filebase}" is modified.
	# The 'configure_file' destination "${CMAKE_CURRENT_BINARY_DIR}/${source_list_filebase}.depend" is a dummy file.
	#
	# This ensures CMake is re-run when a source file is added or removed.
	configure_file(${source_list_filebase} ${source_list_filebase}.depend COPYONLY)

	# Return the list to the caller.
	set(${sources} ${_sources} PARENT_SCOPE)
endfunction()
