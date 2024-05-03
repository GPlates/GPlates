#
# Enable installation of pygplates into Python site-packages.
#
# This involves:
# - Creating files to be used by 'pip install' such as 'setup.py'.
# - Creating a CMake custom target to:
#   (1) install pygplates locally (using CMake) and then
#   (2) copy that into site-packages (using Python's pip).
#

# Copy the 'setup.py' and 'MANIFEST.in' files from our source directory to our binary/output directory.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in ${CMAKE_CURRENT_BINARY_DIR}/setup.py @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/MANIFEST.in.in ${CMAKE_CURRENT_BINARY_DIR}/MANIFEST.in @ONLY)

# Copy the license file to where 'setup.py' will get run so that it will get included by default when building a wheel.
configure_file(${PROJECT_SOURCE_DIR}/COPYING ${CMAKE_CURRENT_BINARY_DIR}/COPYING COPYONLY)

#
# Create a custom target to install pygplates into the Python site-packages directory.
#
# The first step is to install the 'pygplates/' Python package in the current binary directory.
#
# Then the second step is to run 'pip install' to copy the local installation into the site-packages of Python.
#
add_custom_target(install-into-python
    # First install 'pygplates/' Python package in current binary directory...
    COMMAND ${CMAKE_COMMAND} --install ${PROJECT_BINARY_DIR} --prefix ${CMAKE_CURRENT_BINARY_DIR} --config $<CONFIG>
    #
    # Then run 'pip install' to copy the local installation into site-packages...
    #
    # Note: We could instead have run "${GPLATES_PYTHON_EXECUTABLE} setup.py install" but 'pip install' is the standard way now and Python >=3.4 comes with pip.
    COMMAND ${GPLATES_PYTHON_EXECUTABLE} -m pip install "${CMAKE_CURRENT_BINARY_DIR}")
# Make sure 'pygplates' gets built before we try to install it.
add_dependencies(install-into-python pygplates)

#
# Traverse test sub-directory.
#
add_subdirectory(test)
