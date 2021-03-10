# Use the basename of the build directory in the test filename.
#
# This is because we might have multiple build trees pointing into the same source tree and
# each build tree compiles a different build of pygplates (potentially for a different version of Python).
# When you run the test you then choose the appropriate test file based on the pygplates build you are testing.
get_filename_component(_build_directory_basename "${GPlates_BINARY_DIR}" NAME)

# Convert the "$<TARGET_FILE_DIR:pygplates>" generator expression (in the main 'test.py' file) to the directory
# containing pygplates target. Note that file(GENERATE ...) creates a separate file for each build configuration
# (eg, Debug/Release/etc), since the TARGET_FILE_DIR generator expression is build-specific, and so we must name
# each generated file differently (currently using $<CONFIG> in the filename).
# Note that this is all done at 'generate' time (not during the 'configure' stage).
file(GENERATE
	OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/test_$<CONFIG>_${_build_directory_basename}.py"
	INPUT "${CMAKE_CURRENT_SOURCE_DIR}/test.py.in")
