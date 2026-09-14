#
# List the non-generated source files (*.h, *.cc, *.ui, *.qrc).
#
# Add and remove source files here by hand, keeping the list in alphabetical order.
# (CMake discourages 'file(GLOB)': if source files are added or removed, CMake is not
# automatically re-run, so the build would be unaware of the change.)
#
set(srcs
    ApplicationStateTest.cc
    CommandLineParserTest.cc
    CptPaletteTest.cc
    CsvExportTest.cc
    DataAssociationDataTableTest.cc
    ExportTemplateFilenameTest.cc
    GenerateVelocityDomainCitcomsTest.cc
    GeoscimlReaderTest.cc
    GsmlXmlQueryTest.cc
    MipmapperTest.cc
    RealTest.cc
    ScribeExportUnitTest.h
    SmartNodeLinkedListTest.cc
    StringSetTest.cc
    TranscribeTest.cc
    TranscribeTest.h
)

# Add the source files to the gplates "unit test" executable
# (if it exists; might not exist if could not find GoogleTest).
#
# Note: This is different to the other sub-directories (which add to 'gplates-lib' static library).
#       This is because source code in this sub-directory is only needed for unit testing.
if (TARGET gplates-unit-test)
    target_sources_util(gplates-unit-test PRIVATE ${srcs})
endif()

# The Absolute Age draw style is a bundled Python script, so its focused tests
# run directly with the same Python interpreter selected for GPlates.
add_custom_target(absolute-age-draw-style-test
    COMMAND "${GPLATES_PYTHON_EXECUTABLE}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/AbsoluteAgeDrawStyleTest.py"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    VERBATIM)
