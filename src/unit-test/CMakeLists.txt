#
# List the non-generated source files (*.h, *.cc, *.ui, *.qrc).
#
# You can either explicitly add/remove source files here, or
# run 'python cmake/add_sources.py' to automatically list them (here).
#
# Note that CMake discourages use of the 'file(GLOB)' CMake command to automatically collect source files.
# One of the reasons is if source files are added or removed, CMake is not automatically re-run,
# so the build is unaware of the change.
#
set(srcs
    ApplicationStateTest.cc
    CptPaletteTest.cc
    DataAssociationDataTableTest.cc
    GenerateVelocityDomainCitcomsTest.cc
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
