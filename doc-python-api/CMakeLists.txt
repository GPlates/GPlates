#
# Generate a "doc-python-api" target that uses Sphinx to generate HTML from the docstrings in the 'pygplates' python module.
#
# The build itself is done by 'build_docs.py' in this directory. That script is also what runs against a
# released wheel, where there is no build tree at all, so keeping a single implementation means the two
# paths cannot drift apart. See its docstring for the details that are easy to get wrong - in particular
# why the documentation needs a *flat* "import pygplates" rather than the package import.
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
	# 'build_docs.py' copies the ".rst" sources, images and '_static' into its scratch directory and
	# generates 'conf.py' there, so the stub ".rst" files that the sphinx 'autosummary' command (when used
	# with ':toctree:') generates never land in our source directory. Making the scratch directory our
	# binary directory keeps the output where it has always been: '<build-dir>/doc-python-api/html'.
	#
	# Note that the copying now happens at build time rather than at configure time, so editing a ".rst"
	# file no longer requires CMake to re-configure before the change shows up.
	#
	# '--module-dir' is the *parent* of the built 'pygplates' package directory, which is the same value
	# 'generate_stub.py' takes ("$<TARGET_FILE_DIR:pygplates>" is the package directory itself, because the
	# pygplates library is built into a 'pygplates/' sub-directory of the build output directory - see the
	# pygplates target in 'src/CMakeLists.txt'). The generator expression is evaluated per-configuration at
	# build time, so multi-config generators (Visual Studio/Xcode) need no special handling here.
	#
	# Note that this always builds from scratch - 'build_docs.py' deletes the doctree and generated-stub
	# caches first, because an incremental Sphinx build silently drops *every* entry from
	# 'searchindex.js'. When iterating on a single page, run 'build_docs.py --incremental' directly.
	add_custom_target(doc-python-api
		COMMAND "${GPLATES_PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/build_docs.py"
			--module-dir "$<TARGET_FILE_DIR:pygplates>/.."
			--scratch-dir "${CMAKE_CURRENT_BINARY_DIR}"
			--output "${CMAKE_CURRENT_BINARY_DIR}/html"
		COMMENT "Building HTML documentation with Sphinx")
	# Make sure the 'pygplates' target is built before we build the 'doc-python-api' target (Sphinx generates documentation).
	add_dependencies(doc-python-api pygplates)
endif (SPHINX_FOUND)
