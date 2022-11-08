if (TARGET gplates-lib)
    #
    # Read the list of non-generated source files (*.h, *.cc, *.ui, *.qrc).
    #
    # The "read_sources_util()" function is in "Utils.cmake".
    #
    read_sources_util(cmake_sources.txt sources)

    # Add the source files to the gplates static library.
    target_sources_util(gplates-lib PRIVATE ${sources})
endif()
