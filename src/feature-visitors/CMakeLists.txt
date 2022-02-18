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
    FeatureClassifier.cc
    FeatureClassifier.h
    FromQvariantConverter.cc
    FromQvariantConverter.h
    GeometryFinder.cc
    GeometryFinder.h
    GeometryRotator.cc
    GeometryRotator.h
    GeometrySetter.cc
    GeometrySetter.h
    GeometryTypeFinder.cc
    GeometryTypeFinder.h
    KeyValueDictionaryFinder.cc
    KeyValueDictionaryFinder.h
    PropertyValueFinder.cc
    PropertyValueFinder.h
    QueryFeaturePropertiesWidgetPopulator.cc
    QueryFeaturePropertiesWidgetPopulator.h
    ShapefileAttributeFinder.cc
    ShapefileAttributeFinder.h
    TopologySectionsFinder.cc
    TopologySectionsFinder.h
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
    ViewFeatureGeometriesWidgetPopulator.cc
    ViewFeatureGeometriesWidgetPopulator.h
)

# Add the source files to the gplates static library (or pygplates extension module).
target_sources_util(${SOURCE_TARGET} PRIVATE ${srcs})
