# Add the test of the pygplates module.
add_test(
	NAME pygplates-test
	COMMAND ${GPLATES_PYTHON_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/test.py"
	# We only test non-debug builds because GPlatesGlobal::Assert<> statements in C++ code throw an exception in
	# non-debug builds but abort in debug builds. These exceptions get translated to their equivalent in Python.
	# Some of our tests purposefully check that these exceptions are raised (and that will only happen in non-debug builds).
	CONFIGURATIONS Release MinSizeRel)

# Set test properties.
set_tests_properties(pygplates-test PROPERTIES
	# Set the PYTHONPATH used in the test environment.
	# PYTHONPATH should have precedence over everything including 'site-packages' (but not the directory containing "test.py").
	# Set it to the location of the pygplates build output.
	ENVIRONMENT "PYTHONPATH=$<TARGET_FILE_DIR:pygplates>"
	# If 'FAIL' is anywhere in the output then the test failed.
	FAIL_REGULAR_EXPRESSION "FAIL|Fail|fail")
