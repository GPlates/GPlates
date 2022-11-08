# Add the source files to the gplates "unit test" executable
# (if it exists; might not exist if could not find Boost unit test framework).
#
# Note: This is different to the other sub-directories (which add to 'gplates-lib' static library).
#       This is because source code in this sub-directory is only needed for unit testing.
if (TARGET gplates-unit-test)
    #
    # Read the list of non-generated source files (*.h, *.cc, *.ui, *.qrc).
    #
    # The "read_sources_util()" function is in "Utils.cmake".
    #
    read_sources_util(cmake_sources.txt sources)

    # Add the source files to the gplates "unit test" executable.
    target_sources_util(gplates-unit-test PRIVATE ${sources})
endif()
