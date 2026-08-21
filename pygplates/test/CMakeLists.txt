# Add the test of the pygplates module.
add_test(
	NAME pygplates-test
	COMMAND ${GPLATES_PYTHON_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/test.py"
	# We only test non-debug builds because GPlatesGlobal::Assert<> statements in C++ code throw an exception in
	# non-debug builds but abort in debug builds. These exceptions get translated to their equivalent in Python.
	# Some of our tests purposefully check that these exceptions are raised (and that will only happen in non-debug builds).
	CONFIGURATIONS Release MinSizeRel)

# Add the source-closure test: the pygplates module must compile exactly the include closure
# of the pyGPlates API (see "cmake/pygplates_source_closure.py" and
# "doc-cpp/design/architecture/README.md"). This is what stops a new file, or a new #include
# in a shared translation unit, silently re-fattening the module - and what keeps the
# committed dependency matrix honest ("--check-doc" fails if it is stale, the way
# pygplates-stub-test fails on a stale type stub). "pygplates_sources.txt" is written at
# generate time by the pygplates target in 'src/CMakeLists.txt'.
add_test(
	NAME pygplates-source-closure-test
	COMMAND ${GPLATES_PYTHON_EXECUTABLE} "${PROJECT_SOURCE_DIR}/cmake/pygplates_source_closure.py"
		--check-sources "${PROJECT_BINARY_DIR}/pygplates_sources.txt"
		--check-doc "${PROJECT_SOURCE_DIR}/doc-cpp/design/architecture/dependency-matrix.md"
	# Match the other pygplates tests (see the note on 'pygplates-test' above).
	CONFIGURATIONS Release MinSizeRel)

# Set test properties.
set_tests_properties(pygplates-test PROPERTIES
	# Set the PYTHONPATH used in the test environment.
	# PYTHONPATH should have precedence over everything including 'site-packages' (but not the directory containing "test.py").
	#
	# Set it to the *parent* of the built 'pygplates' package directory, since that is what needs to be on 'sys.path'
	# in order to "import pygplates". Note that "$<TARGET_FILE_DIR:pygplates>" is the package directory itself
	# (the pygplates library is built into a 'pygplates/' sub-directory of the build output directory - see the
	# pygplates target in 'src/CMakeLists.txt'), so we go up one level from it.
	ENVIRONMENT "PYTHONPATH=$<TARGET_FILE_DIR:pygplates>/.."
	# If 'FAIL' is anywhere in the output then the test failed.
	FAIL_REGULAR_EXPRESSION "FAIL|Fail|fail")
