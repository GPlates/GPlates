# Copy the 'setup.py' and 'MANIFEST.in' files from our source directory to our binary/output directory.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in ${CMAKE_CURRENT_BINARY_DIR}/setup.py @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/MANIFEST.in.in ${CMAKE_CURRENT_BINARY_DIR}/MANIFEST.in @ONLY)

# Copy the license file to where 'setup.py' will get run so that it will get included by default when building a wheel.
configure_file(${PROJECT_SOURCE_DIR}/COPYING ${CMAKE_CURRENT_BINARY_DIR}/COPYING COPYONLY)


#
# Traverse test sub-directory.
#
add_subdirectory(test)
