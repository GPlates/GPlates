#
# Generate a "doc-python-api" target that uses Sphinx to generate HTML from the docstrings in the 'pygplates' python module.
#

find_package(Sphinx)

IF (SPHINX_EXECUTABLE)
  SET (SPHINX_FOUND "YES")
  IF (NOT SPHINX_FIND_QUIETLY)
    MESSAGE(STATUS "Looking for sphinx... - found ${SPHINX_EXECUTABLE}")
  ENDIF (NOT SPHINX_FIND_QUIETLY)
ELSE (SPHINX_EXECUTABLE)
  IF (NOT SPHINX_FIND_QUIETLY)
    IF (SPHINX_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Looking for sphinx... - NOT found")
    ELSE (SPHINX_FIND_REQUIRED)
      MESSAGE(STATUS "Looking for sphinx... - NOT found")
      MESSAGE(STATUS "... no build target created to generate python API documentation")
    ENDIF (SPHINX_FIND_REQUIRED)
  ENDIF (NOT SPHINX_FIND_QUIETLY)
ENDIF (SPHINX_EXECUTABLE)

if (SPHINX_FOUND)
	if(NOT DEFINED SPHINX_THEME)
		set(SPHINX_THEME default)
	endif(NOT DEFINED SPHINX_THEME)

	if(NOT DEFINED SPHINX_THEME_DIR)
		set(SPHINX_THEME_DIR)
	endif(NOT DEFINED SPHINX_THEME_DIR)

	# configured documentation tools and intermediate build results
	set(BINARY_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/_build")

	# Sphinx cache with pickled ReST documents
	set(SPHINX_CACHE_DIR "${CMAKE_CURRENT_BINARY_DIR}/_doctrees")

	# HTML output directory
	set(SPHINX_HTML_DIR "${CMAKE_CURRENT_BINARY_DIR}/html")
	
	# Copy the ".rst" files from our source directory to our binary/output directory.
	# This is because the sphinx 'autosummary' command (when used with ':toctree:') will generate a
	# bunch of stub ".rst" files and we don't want them in our source code.
	FILE(GLOB RST_FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*.rst" "${CMAKE_CURRENT_SOURCE_DIR}/sample-code/*.rst")
	file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/sample-code") # Ensure 'sample-code' sub-directory exists in binary build directory.
	FOREACH(RST_FILE ${RST_FILES})
		configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${RST_FILE} ${CMAKE_CURRENT_BINARY_DIR}/${RST_FILE} @ONLY)
	ENDFOREACH(RST_FILE ${RST_FILES})
	
	# Copy the image (".png") files from our source directory to our binary/output directory.
	# This is because we're making that the Sphinx source directory (since ".rst" files are also copied there as mentioned above).
	FILE(GLOB IMAGE_FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/images/*.png")
	file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/images") # Ensure 'images' sub-directory exists in binary build directory.
	FOREACH(IMAGE_FILE ${IMAGE_FILES})
		configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${IMAGE_FILE} ${CMAKE_CURRENT_BINARY_DIR}/${IMAGE_FILE} COPYONLY)
	ENDFOREACH(IMAGE_FILE ${IMAGE_FILES})

	# Configure the Sphinx configuration file using @VAR@ CMake variable substitutions.
	configure_file(conf.py.in "${CMAKE_CURRENT_BINARY_DIR}/conf.py.intermediate" @ONLY)

	# Convert the "$<TARGET_FILE_DIR:pygplates>" generator expression (in the 'configure_file' output file) to the directory
	# containing pygplates target. Also the final Sphinx configuration file must be called 'conf.py', however file(GENERATE ...)
	# creates a separate file for each build configuration (eg, Debug/Release/etc), since the above generator expression is
	# build-specific, and so we place each generated file in a build-specific directory (using $<CONFIG>) - which we specify
	# to sphinx-build with the '-c "${BINARY_BUILD_DIR}/$<CONFIG>"' option.
	# Note that this is all done at 'generate' time (not during 'configure).
	file(GENERATE
		OUTPUT "${BINARY_BUILD_DIR}/$<CONFIG>/conf.py"
		INPUT "${CMAKE_CURRENT_BINARY_DIR}/conf.py.intermediate")

	# Note that the source directory for sphinx is our current binary/output directory (we've copied our source ".rst" files there).
	add_custom_target(doc-python-api
		COMMAND ${SPHINX_EXECUTABLE}
			-q -b html
			-c "${BINARY_BUILD_DIR}/$<CONFIG>"
			-d "${SPHINX_CACHE_DIR}"
			"${CMAKE_CURRENT_BINARY_DIR}"
			"${SPHINX_HTML_DIR}"
		COMMENT "Building HTML documentation with Sphinx")
	# Make sure the 'pygplates' target is built before we build the 'doc-python-api' target (Sphinx generates documentation).
	add_dependencies(doc-python-api pygplates)
endif (SPHINX_FOUND)