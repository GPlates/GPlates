# Convert the "$<TARGET_FILE_DIR:pygplates>" generator expression (in the main 'test.py' file) to the directory
# containing pygplates target. Note that file(GENERATE ...) creates a separate file for each build configuration
# (eg, Debug/Release/etc), since the above generator expression is build-specific, and so we must name each
# generated file differently (currently using "test_$<CONFIG>.py").
# Note that this is all done at 'generate' time (not during 'configure).
file(GENERATE
	OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/test_$<CONFIG>.py"
	INPUT "${CMAKE_CURRENT_SOURCE_DIR}/test.py.in")
