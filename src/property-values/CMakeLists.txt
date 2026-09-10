#
# List the non-generated source files (*.h, *.cc, *.ui, *.qrc).
#
# Add and remove source files here by hand, keeping the list in alphabetical order.
# (CMake discourages 'file(GLOB)': if source files are added or removed, CMake is not
# automatically re-run, so the build would be unaware of the change.)
#
set(srcs
    CoordinateTransformation.cc
    CoordinateTransformation.h
    Enumeration.cc
    Enumeration.h
    EnumerationContent.h
    EnumerationType.h
    Georeferencing.cc
    Georeferencing.h
    GeoTimeInstant.cc
    GeoTimeInstant.h
    GmlDataBlock.cc
    GmlDataBlock.h
    GmlDataBlockCoordinateList.cc
    GmlDataBlockCoordinateList.h
    GmlFile.cc
    GmlFile.h
    GmlGridEnvelope.cc
    GmlGridEnvelope.h
    GmlLineString.cc
    GmlLineString.h
    GmlMultiPoint.cc
    GmlMultiPoint.h
    GmlOrientableCurve.cc
    GmlOrientableCurve.h
    GmlPoint.cc
    GmlPoint.h
    GmlPolygon.cc
    GmlPolygon.h
    GmlRectifiedGrid.cc
    GmlRectifiedGrid.h
    GmlTimeInstant.cc
    GmlTimeInstant.h
    GmlTimePeriod.cc
    GmlTimePeriod.h
    GpmlAge.cc
    GpmlAge.h
    GpmlArray.cc
    GpmlArray.h
    GpmlConstantValue.cc
    GpmlConstantValue.h
    GpmlFeatureReference.cc
    GpmlFeatureReference.h
    GpmlFeatureSnapshotReference.cc
    GpmlFeatureSnapshotReference.h
    GpmlFiniteRotation.cc
    GpmlFiniteRotation.h
    GpmlFiniteRotationSlerp.cc
    GpmlFiniteRotationSlerp.h
    GpmlHotSpotTrailMark.cc
    GpmlHotSpotTrailMark.h
    GpmlInterpolationFunction.cc
    GpmlInterpolationFunction.h
    GpmlIrregularSampling.cc
    GpmlIrregularSampling.h
    GpmlKeyValueDictionary.cc
    GpmlKeyValueDictionary.h
    GpmlKeyValueDictionaryElement.cc
    GpmlKeyValueDictionaryElement.h
    GpmlMeasure.cc
    GpmlMeasure.h
    GpmlMetadata.cc
    GpmlMetadata.h
    GpmlOldPlatesHeader.cc
    GpmlOldPlatesHeader.h
    GpmlPiecewiseAggregation.cc
    GpmlPiecewiseAggregation.h
    GpmlPlateId.cc
    GpmlPlateId.h
    GpmlPolarityChronId.cc
    GpmlPolarityChronId.h
    GpmlPropertyDelegate.cc
    GpmlPropertyDelegate.h
    GpmlRasterBandNames.cc
    GpmlRasterBandNames.h
    GpmlRevisionId.cc
    GpmlRevisionId.h
    GpmlScalarField3DFile.cc
    GpmlScalarField3DFile.h
    GpmlStringList.cc
    GpmlStringList.h
    GpmlTimeSample.cc
    GpmlTimeSample.h
    GpmlTimeWindow.cc
    GpmlTimeWindow.h
    GpmlTopologicalLine.cc
    GpmlTopologicalLine.h
    GpmlTopologicalLineSection.cc
    GpmlTopologicalLineSection.h
    GpmlTopologicalNetwork.cc
    GpmlTopologicalNetwork.h
    GpmlTopologicalPoint.cc
    GpmlTopologicalPoint.h
    GpmlTopologicalPolygon.cc
    GpmlTopologicalPolygon.h
    GpmlTopologicalSection.cc
    GpmlTopologicalSection.h
    OldVersionPropertyValue.cc
    OldVersionPropertyValue.h
    ProxiedRasterCache.cc
    ProxiedRasterCache.h
    ProxiedRasterResolver.cc
    ProxiedRasterResolver.h
    RasterStatistics.h
    RasterType.cc
    RasterType.h
    RawRaster.cc
    RawRaster.h
    RawRasterUtils.cc
    RawRasterUtils.h
    ScalarCoverageStatistics.h
    ScribeExportPropertyValues.h
    SpatialReferenceSystem.cc
    SpatialReferenceSystem.h
    StructuralType.h
    TextContent.h
    TimescaleBand.h
    TimescaleName.h
    UninterpretedPropertyValue.cc
    UninterpretedPropertyValue.h
    ValueObjectType.h
    XsBoolean.cc
    XsBoolean.h
    XsDouble.cc
    XsDouble.h
    XsInteger.cc
    XsInteger.h
    XsString.cc
    XsString.h
)

# GPlates-only sources: excluded from the pygplates module, whose source list is the
# include closure of the pyGPlates API (see "cmake/pygplates_source_closure.py"; the
# pygplates-source-closure test fails if this list drifts from that closure).
# Keep ".h"/".cc" pairs together: AUTOMOC mocs any Q_OBJECT header listed in the
# module's sources, and a moc'ed header whose ".cc" is excluded fails at link time.
set(gplates_only_srcs
    ScalarCoverageStatistics.h
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
