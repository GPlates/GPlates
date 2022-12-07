#
# read_sources_util:
#
# Reads a list of source files from a file named 'source_list_filebase' (in current source directory).
# Returns list of source files in the list variable 'sources'.
#
# Also this function ensures CMake is re-run whenever 'source_list_filebase' is modified,
# such as when a source file is added or removed.
#
# Note: Lines in the list file 'source_list_filebase' starting with '#' are ignored.
#
function(read_sources_util source_list_filebase sources)
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
