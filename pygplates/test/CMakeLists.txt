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
# generate time by the pygplates target in 'src/CMakeLists.txt'. The Qt major version decides
# whether the Qt5-only GeoSciML sources in 'src/file-io' are part of the closure.
add_test(
	NAME pygplates-source-closure-test
	COMMAND ${GPLATES_PYTHON_EXECUTABLE} "${PROJECT_SOURCE_DIR}/cmake/pygplates_source_closure.py"
		--check-sources "${PROJECT_BINARY_DIR}/pygplates_sources.txt"
		--check-doc "${PROJECT_SOURCE_DIR}/doc-cpp/design/architecture/dependency-matrix.md"
		--qt-version-major ${QT_VERSION_MAJOR}
	# Match the other pygplates tests (see the note on 'pygplates-test' above).
	CONFIGURATIONS Release MinSizeRel)

# Add the linkage test: the built module's *direct* shared-library dependencies must not
# include any of GPlates' GUI/rendering libraries (Qt Widgets, Qt Svg, the Qt OpenGL
# modules, Qwt, GLEW). The binary-level complement of pygplates-source-closure-test above.
#
# The script lives in 'cmake/' beside the source-closure tracer rather than here: both
# check the build graph rather than the pyGPlates API, which is what the tests in this
# directory are for.
#
# Note: on Windows the dependency lister is dumpbin, so (like the build itself) this test
# must run from an MSVC-enabled environment; the script fails loudly rather than silently
# passing if the lister is unavailable.
add_test(
	NAME pygplates-linkage-test
	COMMAND ${GPLATES_PYTHON_EXECUTABLE} "${PROJECT_SOURCE_DIR}/cmake/check_linkage.py" "$<TARGET_FILE:pygplates>"
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
	ENVIRONMENT "PYTHONPATH=$<TARGET_FILE_DIR:pygplates>/..")

# Note: we deliberately do NOT set FAIL_REGULAR_EXPRESSION here, matching 'pygplates-stub-test'
# (see pygplates/stub/CMakeLists.txt). Pass/fail is by exit code only.
#
# "test.py" exits non-zero whenever a sub-test fails, and 'unittest' prints its "FAILED (...)"
# verdict under exactly the condition that makes it exit non-zero, so a regex over the output can
# never fail a run that the exit code would not have failed anyway. It can only add false
# positives - and it did: the broad "FAIL|Fail|fail" this replaces also matched output from
# neither the tests nor pygplates, so an OpenMP runtime warning on stderr ("Can't set size of
# /tmp file failed", emitted when /tmp is not writable) failed a run in which all 441 sub-tests
# passed. Matching the verdict line instead is no better, only quieter: Python 3.14 writes it as
# "FAILED" and " (...)" with colour escapes in between, so a "FAILED \(" regex silently stops
# matching whenever colour is enabled - a guard that fails open.
#
# What neither mechanism catches is tests that never ran (an empty suite reports "NO TESTS RAN"
# yet still exits 0), so "test.py" guards that itself - see MINIMUM_EXPECTED_TESTS there.
