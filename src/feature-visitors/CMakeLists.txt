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
    FromQvariantConverter.cc
    FromQvariantConverter.h
    GeometryFinder.cc
    GeometryFinder.h
    GeometrySetter.cc
    GeometrySetter.h
    GmlTimePeriodFinder.cc
    GmlTimePeriodFinder.h
    PlateIdFinder.cc
    PlateIdFinder.h
	ComputationalMeshSolver.cc
	ComputationalMeshSolver.h
	PlatepolygonResolver.cc
	PlatepolygonResolver.h
	ReconstructedFeatureGeometryFinder.cc
	ReconstructedFeatureGeometryFinder.h
	TopologyResolver.cc
	TopologyResolver.h
    QueryFeaturePropertiesWidgetPopulator.cc
    QueryFeaturePropertiesWidgetPopulator.h
    ShapefileAttributeFinder.cc
    ShapefileAttributeFinder.h
    ToQvariantConverter.cc
    ToQvariantConverter.h
    TotalReconstructionSequencePlateIdFinder.cc
    TotalReconstructionSequencePlateIdFinder.h
    TotalReconstructionSequenceRotationInserter.cc
    TotalReconstructionSequenceRotationInserter.h
    TotalReconstructionSequenceRotationInterpolater.cc
    TotalReconstructionSequenceRotationInterpolater.h
    TotalReconstructionSequenceTimePeriodFinder.cc
    TotalReconstructionSequenceTimePeriodFinder.h
	ValueFinder.cc
	ValueFinder.h
    ViewFeatureGeometriesWidgetPopulator.cc
    ViewFeatureGeometriesWidgetPopulator.h
    XsStringFinder.cc
    XsStringFinder.h
)





set(target_name feature-visitors)

# Does the following:
# - finds '.ui' files and adds commands to create 'Ui.h' files.
# - adds dependencies on generated header files.
# - scans '.h' files for Q_OBJECT and adds commands to run Qt's moc.exe to generate 'moc_*.cc' files.
# - finds '.qrc' files and adds commands to generate '.cc' files from them.
# - adds header files to Visual Studio projects.
# - groups files in Visual Studio IDE according to type.
GPLATES_GENERATE_TARGET_SOURCES(${target_name} target_srcs ${srcs})

add_library(${target_name} ${target_srcs})

GPLATES_POST_ADD_TARGET(${target_name} target_srcs)
