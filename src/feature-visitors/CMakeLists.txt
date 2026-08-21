#
# List the non-generated source files (*.h, *.cc, *.ui, *.qrc).
#
# Add and remove source files here by hand, keeping the list in alphabetical order.
# (CMake discourages 'file(GLOB)': if source files are added or removed, CMake is not
# automatically re-run, so the build would be unaware of the change.)
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

# GPlates-only sources: excluded from the pygplates module, whose source list is the
# include closure of the pyGPlates API (see "cmake/pygplates_source_closure.py"; the
# pygplates-source-closure test fails if this list drifts from that closure).
# Keep ".h"/".cc" pairs together: AUTOMOC mocs any Q_OBJECT header listed in the
# module's sources, and a moc'ed header whose ".cc" is excluded fails at link time.
set(gplates_only_srcs
    FeatureClassifier.cc
    FeatureClassifier.h
    FromQvariantConverter.cc
    FromQvariantConverter.h
    GeometryRotator.cc
    GeometryRotator.h
    QueryFeaturePropertiesWidgetPopulator.cc
    QueryFeaturePropertiesWidgetPopulator.h
    TopologySectionsFinder.cc
    TopologySectionsFinder.h
    TotalReconstructionSequenceRotationInserter.cc
    TotalReconstructionSequenceRotationInserter.h
    TotalReconstructionSequenceRotationInterpolater.cc
    TotalReconstructionSequenceRotationInterpolater.h
    TotalReconstructionSequenceTimePeriodFinder.cc
    TotalReconstructionSequenceTimePeriodFinder.h
    ViewFeatureGeometriesWidgetPopulator.cc
    ViewFeatureGeometriesWidgetPopulator.h
)
# Guard against stale entries (eg, after a file rename).
foreach(gplates_only_src ${gplates_only_srcs})
    if (NOT gplates_only_src IN_LIST srcs)
        message(FATAL_ERROR "'${gplates_only_src}' is not in srcs")
    endif()
endforeach()
if (NOT TARGET gplates-lib)
    list(REMOVE_ITEM srcs ${gplates_only_srcs})
endif()

# Add the source files to the gplates static library (or pygplates extension module).
target_sources_util(${SOURCE_TARGET} PRIVATE ${srcs})
