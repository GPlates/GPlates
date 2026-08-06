# Add the freshness test for the generated pygplates type stub.
#
# Note that '--module-dir' is added to 'sys.path' in order to "import pygplates", so it is the *parent* of the
# built 'pygplates' package directory. "$<TARGET_FILE_DIR:pygplates>" is the package directory itself (the
# pygplates library is built into a 'pygplates/' sub-directory of the build output directory - see the
# pygplates target in 'src/CMakeLists.txt'), so we go up one level from it.
add_test(
	NAME pygplates-stub-test
	COMMAND ${GPLATES_PYTHON_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/generate_stub.py"
		--module-dir "$<TARGET_FILE_DIR:pygplates>/.."
		--check "${CMAKE_CURRENT_SOURCE_DIR}/__init__.pyi"
		--output "${CMAKE_CURRENT_BINARY_DIR}/__init__.pyi"
	# We only run this for non-debug builds, matching 'pygplates-test' (see pygplates/test/CMakeLists.txt).
	CONFIGURATIONS Release MinSizeRel)

# Note: we deliberately do NOT set FAIL_REGULAR_EXPRESSION here (unlike 'pygplates-test') since the
# printed docstring diff on failure could itself contain the word "fail". Pass/fail is by exit code only.
