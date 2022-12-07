#
# Read the list of non-generated source files (*.h, *.cc, *.ui, *.qrc).
#
# The "read_sources_util()" function is in "Utils.cmake".
#
read_sources_util(cmake_sources.txt sources)

# Qt6 removed the QtXmlPatterns module providing support for XPath, XQuery, XSLT, and XML Schema validation.
# It has been deprecated since Qt 5.13.
#
# TODO: Find a replacement library.
if (QT_VERSION_MAJOR LESS 6)
    read_sources_util(cmake_sources_qt5_only.txt sources_qt5_only)
    list(APPEND sources ${sources_qt5_only})
endif()


# Add the source files to the gplates static library (or pygplates extension module).
target_sources(${SOURCE_TARGET} PRIVATE ${sources})
