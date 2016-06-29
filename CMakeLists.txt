project(GPlates)

cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)

# Where to find our CMake modules (this variable is visible in subdirectories).
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules/")

# Include configuration options (default options and options overridden by user).
include (ConfigCMake)

#
# CMake version 2.6 introduces policies to tell CMake to behave like a certain version ( > 2.4 ).
#
include (Policies_26)

#
# Recurse into source code directory.
#
add_subdirectory(src)

#
# Recurse into documentation directory.
#
add_subdirectory(doc)


if(EXISTS "${CMAKE_SOURCE_DIR}/scripts/hellinger.py")
    install(FILES  "${CMAKE_SOURCE_DIR}/scripts/hellinger.py"
        DESTINATION share/gplates/scripts/)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/scripts/hellinger_utils.py")
    install(FILES  "${CMAKE_SOURCE_DIR}/scripts/hellinger_utils.py"
        DESTINATION share/gplates/scripts/)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/SampleData/")
    install(DIRECTORY SampleData/ DESTINATION share/gplates/SampleData/)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/doc/gplates.1.gz")
    install(FILES  "${CMAKE_SOURCE_DIR}/doc/gplates.1.gz"
        DESTINATION share/man/man1/)
endif()
