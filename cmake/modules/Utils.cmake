#
# target_sources_util:
#
# Emulates the CMake 3.13 'target_source()' command for earlier CMake 3.x versions.
# As with CMake 3.13 'target_source()', each source should be specified either:
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

# NOTE: On MacOS X some libraries referenced by executable targets do not have
# their full path specified in the executables themselves. When this happens
# the error "dyld: Library not loaded:" error is generated when trying to
# run the executable. An example is CGAL installed via Macports.
# GPLATES_FIX_MACOS_LIBRARY_PATHS_IN_EXECUTABLE_TARGET() creates a post-build command
# that looks at a built executable target and does these fixups.
#
MACRO (GPLATES_FIX_MACOS_LIBRARY_PATHS_IN_EXECUTABLE_TARGET _target_name _is_target_bundle)

	if (APPLE)
		# If the target is a bundle then it has a different path.
		set(is_target_bundle ${_is_target_bundle})
		if(is_target_bundle)
			set(_target_path ${GPlates_BINARY_DIR}/bin/gplates.app/Contents/MacOS/${_target_name})
		else(is_target_bundle)
			set(_target_path ${GPlates_BINARY_DIR}/bin/${_target_name})
		endif(is_target_bundle)

		set(_eol_char "E")
		# First character of the line is a tab then look for
		# a library path that does not start with '/' (ie, not an absolute path).
		set(_lib_no_path_regex "^\\t([^/ ]+) .*${_eol_char}$")

		# Write a cmake script to a file so we can execute it later when the target is built.
		set(_util_fix_macos_library_path_cmake_script "${GPlates_BINARY_DIR}/cmake/modules/Util_FixMacosLibraryPaths_${_target_name}.cmake")
		file(WRITE ${_util_fix_macos_library_path_cmake_script}
			"# Pass some cmake variables used by 'find_library()'...\n"
			"set(CMAKE_FIND_LIBRARY_PREFIXES ${CMAKE_FIND_LIBRARY_PREFIXES})\n"
			"set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})\n"
			"set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH})\n"
			"set(CMAKE_FRAMEWORK_PATH ${CMAKE_FRAMEWORK_PATH})\n"
			"set(CMAKE_SYSTEM_LIBRARY_PATH ${CMAKE_SYSTEM_LIBRARY_PATH})\n"
			"set(CMAKE_SYSTEM_FRAMEWORK_PATH ${CMAKE_SYSTEM_FRAMEWORK_PATH})\n"
			"set(CMAKE_FIND_FRAMEWORK ${CMAKE_FIND_FRAMEWORK})\n"
			"# Get a list of libraries referenced by the target executable...\n"
			"execute_process(\n"
			"  COMMAND otool -L ${_target_path} OUTPUT_VARIABLE _otool_output)\n"
			"# In the output replace newlines with 'E;'.\n"
			"# The ';' makes it a cmake list that we can iterate over.\n"
			"string(REGEX REPLACE \"\\n\" \"${_eol_char};\" _lines \"\${_otool_output}\")\n"
			"foreach(_line \${_lines})\n"
			"  # If the library path is not absolute then make it absolute...\n"
			"  if (_line MATCHES \"${_lib_no_path_regex}\")\n"
			"   # Extract the library name from the line...\n"
			"    string(REGEX REPLACE \"${_lib_no_path_regex}\" \"\\\\1\" _lib_no_path \"\${_line}\")\n"
			"    # Find the library path...\n"
			"    find_library(_lib_full_path \${_lib_no_path})\n"
			"    if (_lib_full_path)\n"
			"      # If the library is a framework then we need to append the library name to the path...\n"
			"      if (_lib_full_path MATCHES \"\\\\.framework\")\n"
			"        set(_lib_full_path \"\${_lib_full_path}/\${_lib_no_path}\")\n"
			"      endif (_lib_full_path MATCHES \"\\\\.framework\")\n"
			"      # Replace the library name with full path inside the target executable...\n"
			"      execute_process(\n"
			"        COMMAND install_name_tool -change \${_lib_no_path} \${_lib_full_path} ${_target_path})\n"
			"    else (_lib_full_path)\n"
			"      message(\"Unable to find location of library '\${_lib_no_path}' - executable may not run.\")\n"
			"    endif (_lib_full_path)\n"
			"  endif (_line MATCHES \"${_lib_no_path_regex}\")\n"
			"endforeach(_line)\n"
		)

		# The custom command will run just after the executable target is built
		# and the custom command will execute the cmake script that we wrote to the file above.
		add_custom_command(TARGET ${_target_name} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -P "${_util_fix_macos_library_path_cmake_script}"
		)

	endif (APPLE)

ENDMACRO (GPLATES_FIX_MACOS_LIBRARY_PATHS_IN_EXECUTABLE_TARGET _target_name)
