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
    ApplicationStateTest.h
    AppLogicTestSuite.cc
    AppLogicTestSuite.h
    CanvasToolsTestSuite.cc
    CanvasToolsTestSuite.h
    CoregTest.cc
    CoregTest.h
    CptPaletteTest.cc
    CptPaletteTest.h
    DataAssociationDataTableTest.cc
    DataAssociationDataTableTest.h
    DataMiningTestSuite.cc
    DataMiningTestSuite.h
    FeatureHandleTest.cc
    FeatureHandleTest.h
    FeatureVisitorsTestSuite.cc
    FeatureVisitorsTestSuite.h
    FileIoTestSuite.cc
    FileIoTestSuite.h
    FilterTest.cc
    FilterTest.h
    GenerateVelocityDomainCitcomsTest.cc
    GenerateVelocityDomainCitcomsTest.h
    GeometryVisitorsTestSuite.cc
    GeometryVisitorsTestSuite.h
    GlobalTestSuite.cc
    GlobalTestSuite.h
    GPlatesGlobalFixture.h
    GPlatesTestSuite.cc
    GPlatesTestSuite.h
    GuiTestSuite.cc
    GuiTestSuite.h
    MainTestSuite.cc
    MainTestSuite.h
    MathsTestSuite.cc
    MathsTestSuite.h
    MipmapperTest.cc
    MipmapperTest.h
    ModelTestSuite.cc
    ModelTestSuite.h
    MultiThreadTest.cc
    MultiThreadTest.h
    PresentationTestSuite.cc
    PresentationTestSuite.h
    PropertyValuesTestSuite.cc
    PropertyValuesTestSuite.h
    RealTest.cc
    RealTest.h
    ScribeExportUnitTest.h
    ScribeTestSuite.cc
    ScribeTestSuite.h
    SmartNodeLinkedListTest.cc
    SmartNodeLinkedListTest.h
    StringSetTest.cc
    StringSetTest.h
    TestSuiteFilter.cc
    TestSuiteFilter.h
    TestSuiteFilterTest.cc
    TestSuiteFilterTest.h
    TranscribeTest.cc
    TranscribeTest.h
    UnitTestTestSuite.cc
    UnitTestTestSuite.h
    UtilsTestSuite.cc
    UtilsTestSuite.h
    ViewOperationsTestSuite.cc
    ViewOperationsTestSuite.h
)

# Add the source files to the gplates "unit test" executable
# (if it exists; might not exist if could not find Boost unit test framework).
#
# Note: This is different to the other sub-directories (which add to 'gplates-lib' static library).
#       This is because source code in this sub-directory is only needed for unit testing.
if (TARGET gplates-unit-test)
    target_sources_util(gplates-unit-test PRIVATE ${srcs})
endif()

# The Absolute Age draw style is a bundled Python script, so its focused tests
# run directly with the same Python interpreter selected for GPlates.
add_custom_target(absolute-age-draw-style-test
    COMMAND "${GPLATES_PYTHON_EXECUTABLE}" -B -m unittest
            "${CMAKE_CURRENT_SOURCE_DIR}/AbsoluteAgeDrawStyleTest.py"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
