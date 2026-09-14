#
# Generate a "doc-python-api" target that uses Sphinx to generate HTML from the docstrings in the 'pygplates' python module.
#

# Sphinx must run in the *same* Python interpreter that pygplates was built for, because the
# 'autodoc'/'autosummary' extensions import the pygplates module in-process (that is how they
# read the docstrings). So we run Sphinx as a module of GPLATES_PYTHON_EXECUTABLE rather than
# searching the PATH for a 'sphinx-build' script - a 'sphinx-build' on the PATH generally
# belongs to some other Python installation, and pygplates then fails to import with the
# rather unhelpful "exception: no module named pygplates".
#
# Note: 'conf.py.in' imports 'sphinx_rtd_theme' (see 'html_theme' there), so require it too.
execute_process(
	COMMAND "${GPLATES_PYTHON_EXECUTABLE}" -c "import sphinx, sphinx_rtd_theme; print(sphinx.__version__)"
	RESULT_VARIABLE SPHINX_IMPORT_RESULT
	OUTPUT_VARIABLE SPHINX_VERSION
	OUTPUT_STRIP_TRAILING_WHITESPACE
	ERROR_QUIET)

if (SPHINX_IMPORT_RESULT EQUAL 0)
	set(SPHINX_FOUND "YES")
	message(STATUS "Looking for sphinx... - found ${SPHINX_VERSION} in ${GPLATES_PYTHON_EXECUTABLE}")
else()
	message(STATUS "Looking for sphinx... - NOT found in ${GPLATES_PYTHON_EXECUTABLE}")
	message(STATUS "... install it into *that* Python with, eg, 'conda install sphinx sphinx_rtd_theme'")
	message(STATUS "... no build target created to generate python API documentation")
endif()

if (SPHINX_FOUND)
	# configured documentation tools and intermediate build results
	set(BINARY_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/_build")

	# Sphinx cache with pickled ReST documents
	set(SPHINX_CACHE_DIR "${CMAKE_CURRENT_BINARY_DIR}/_doctrees")

	# Sphinx directory containing custom static files (such as style sheets).
	# This gets used in 'conf.py.in'.
	set(SPHINX_STATIC_DIR "${CMAKE_CURRENT_BINARY_DIR}/_static")

	# HTML output directory
	set(SPHINX_HTML_DIR "${CMAKE_CURRENT_BINARY_DIR}/html")
	
	# Copy the ".rst" files from our source directory to our binary/output directory.
	# This is because the sphinx 'autosummary' command (when used with ':toctree:') will generate a
	# bunch of stub ".rst" files and we don't want them in our source code.
	FILE(GLOB RST_FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*.rst" "${CMAKE_CURRENT_SOURCE_DIR}/sample-code/*.rst")
	file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/sample-code") # Ensure 'sample-code' sub-directory exists in binary build directory.
	FOREACH(RST_FILE ${RST_FILES})
		configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${RST_FILE} ${CMAKE_CURRENT_BINARY_DIR}/${RST_FILE} @ONLY)
	ENDFOREACH()
	
	# Copy the image (".png") files from our source directory to our binary/output directory.
	# This is because we're making that the Sphinx source directory (since ".rst" files are also copied there as mentioned above).
	FILE(GLOB IMAGE_FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/images/*.png")
	file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/images") # Ensure 'images' sub-directory exists in binary build directory.
	FOREACH(IMAGE_FILE ${IMAGE_FILES})
		configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${IMAGE_FILE} ${CMAKE_CURRENT_BINARY_DIR}/${IMAGE_FILE} COPYONLY)
	ENDFOREACH()
	
	# Copy the '_static' directory containing custom static files (such as style sheets) to our binary/output directory.
	# This is because we're making that the Sphinx source directory (since ".rst" files are also copied there as mentioned above).
	FILE(GLOB STATIC_FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/_static/*")
	file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_static") # Ensure '_static' sub-directory exists in binary build directory.
	FOREACH(STATIC_FILE ${STATIC_FILES})
		configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${STATIC_FILE} ${CMAKE_CURRENT_BINARY_DIR}/${STATIC_FILE} COPYONLY)
	ENDFOREACH()

	# Configure the Sphinx configuration file using @VAR@ CMake variable substitutions.
	configure_file(conf.py.in "${CMAKE_CURRENT_BINARY_DIR}/conf.py.intermediate" @ONLY)

	# Convert the "$<TARGET_FILE_DIR:pygplates>" generator expression (in the 'configure_file' output file) to the directory
	# containing pygplates target. Also the final Sphinx configuration file must be called 'conf.py', however file(GENERATE ...)
	# creates a separate file for each build configuration (eg, Debug/Release/etc), since the above generator expression is
	# build-specific, and so we place each generated file in a build-specific directory (using $<CONFIG>) - which we specify
	# to sphinx-build with the '-c "${BINARY_BUILD_DIR}/$<CONFIG>"' option.
	# Note that this is all done at 'generate' time (not during 'configure').
	file(GENERATE
		OUTPUT "${BINARY_BUILD_DIR}/$<CONFIG>/conf.py"
		INPUT "${CMAKE_CURRENT_BINARY_DIR}/conf.py.intermediate")

	# Note that the source directory for sphinx is our current binary/output directory (we've copied our source ".rst" files there).
	add_custom_target(doc-python-api
		COMMAND "${GPLATES_PYTHON_EXECUTABLE}" -m sphinx
			-q -b html -W -j auto
			-c "${BINARY_BUILD_DIR}/$<CONFIG>"
			-d "${SPHINX_CACHE_DIR}"
			"${CMAKE_CURRENT_BINARY_DIR}"
			"${SPHINX_HTML_DIR}"
		COMMENT "Building HTML documentation with Sphinx")
	# Make sure the 'pygplates' target is built before we build the 'doc-python-api' target (Sphinx generates documentation).
	add_dependencies(doc-python-api pygplates)
endif (SPHINX_FOUND)