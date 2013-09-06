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

	# Get the location of the 'pygplates' python module library.
	# It gets added to the python path so Sphinx can find it and import it.
	get_property(PYGPLATES_LIBRARY_LOCATION TARGET pygplates PROPERTY LOCATION)
	get_filename_component(PYGPLATES_LIBRARY_LOCATION "${PYGPLATES_LIBRARY_LOCATION}" PATH)
	# Fix ups for Visual Studio since, unlike makefile generators, it has build configurations that
	# can be selected at build time (ie, after cmake processing).
	# Although add_custom_command can determine the full build path (in CMake 2.6) we cannot
	# get access to that path as a CMake variable. In CMake 2.8 "generator expressions" were added
	# (see http://www.cmake.org/cmake/help/v2.8.11/cmake.html#command:add_custom_command)
	# which would allow this but we still support CMake 2.6.
	# So, for now, we'll just substitute the '$(Outdir)' that gets embedded in the path
	# for the release build (assuming the user is doing a 'release' build).
	if(MSVC)
		# NOTE: If using Visual Studio, then manually change PYGPLATES_MSVC_BUILD_TYPE if you're not doing a 'release' build.
		#set(PYGPLATES_MSVC_BUILD_TYPE release)
		set(PYGPLATES_MSVC_BUILD_TYPE profilegplates)
		string(REPLACE "$(OutDir)" "${PYGPLATES_MSVC_BUILD_TYPE}" PYGPLATES_LIBRARY_LOCATION "${PYGPLATES_LIBRARY_LOCATION}")
	endif(MSVC)

	configure_file(
		"${CMAKE_CURRENT_SOURCE_DIR}/conf.py.in"
		"${BINARY_BUILD_DIR}/conf.py"
		@ONLY)
	
	# It seems that 'sphinx-build' generates the following warning unless we create the specific directory in question...
	# "WARNING: html_static_path entry '...doc-python-api/_build/_static' does not exist".
	file(MAKE_DIRECTORY "${BINARY_BUILD_DIR}/_static")

	add_custom_target(doc-python-api
		${SPHINX_EXECUTABLE}
		-q -b html
		-c "${BINARY_BUILD_DIR}"
		-d "${SPHINX_CACHE_DIR}"
		"${CMAKE_CURRENT_SOURCE_DIR}"
		"${SPHINX_HTML_DIR}"
		COMMENT "Building HTML documentation with Sphinx")
endif (SPHINX_FOUND)