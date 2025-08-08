#
# Generate a "doc" target that runs doxygen over the source code.
#

include (Doxygen)

set (doxygen_config_file_in "${CMAKE_CURRENT_SOURCE_DIR}/doxygen.conf.in")
       
# We need to transform the doxygen config file into a new config file.
# Generate the filename of the new config file (we'll put it in the binary directory out of the way
# - well it'll be in the way if we do an in-place cmake build rather than out-of-place).
SET(doxygen_config_file_out "${CMAKE_CURRENT_BINARY_DIR}/doxygen.conf")

# Set up a custom "DOC" target that can be used to generate doxygen documentation.
# The documentation will be located in the "html" subdirectory.
GENERATE_DOCUMENTATION(${doxygen_config_file_in} ${doxygen_config_file_out})
