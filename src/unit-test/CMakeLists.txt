# Current binary directory must be included first for g++ pre-compiled headers.
# There's a '_pch.h' file in current source and current binary directory however the '_pch.h.gch' pre-compiled header
# is only in current binary directory and it will only be used by the compiler if it's in the same directory as the found '_pch.h' file.
# So we need to ensure that we find the '_pch.h' file from the current binary directory first.
include_directories(${CMAKE_CURRENT_BINARY_DIR})
# Current source directory (directory that this 'CMakeLists.txt' file is in).
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# The variables used here are set in the parent 'CMakeList.txt' file.
include_directories(${SYSTEM_INCLUDE_FLAG} ${external_INCLUDE_DIRS})
include_directories(${internal_INCLUDE_DIRS})

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


# We have a nasty case of a old style cast  warning in a boost
# Macro BOOST_CHECK. Since it's a system header it should
# not emit a warning but it does on g++ 4.1.2 and g++ 4.1.3 (only ones tested
# so far). So we turn off the old style cast warning for the unit test directory
# TODO: find a better way of doing this sort of thing.

if (CMAKE_COMPILER_IS_GNUCXX)
        foreach(src ${srcs})
                if (src MATCHES ".*\\.cc$")
                        set_source_files_properties(${src} PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast")
                endif (src MATCHES ".*\\.cc$")
        endforeach(src)
endif (CMAKE_COMPILER_IS_GNUCXX)



set(target_name unit-test)

# Does the following:
# - finds '.ui' files and adds commands to create 'Ui.h' files.
# - adds dependencies on generated header files.
# - scans '.h' files for Q_OBJECT and adds commands to run Qt's moc.exe to generate 'moc_*.cc' files.
# - finds '.qrc' files and adds commands to generate '.cc' files from them.
# - adds header files to Visual Studio projects.
# - groups files in Visual Studio IDE according to type.
GPLATES_GENERATE_TARGET_SOURCES(${target_name} target_srcs ${srcs})

add_library(${target_name} STATIC ${target_srcs})

GPLATES_POST_ADD_TARGET(${target_name} target_srcs)
