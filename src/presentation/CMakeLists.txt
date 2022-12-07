#
# Read the list of non-generated source files (*.h, *.cc, *.ui, *.qrc).
#
# The "read_sources_util()" function is in "Utils.cmake".
#
read_sources_util(cmake_sources.txt sources)

# Add the source files to the gplates static library (or pygplates extension module).
target_sources(${SOURCE_TARGET} PRIVATE ${sources})
