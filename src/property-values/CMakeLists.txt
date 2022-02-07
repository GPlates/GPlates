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

# Add the source files to the gplates static library.
target_sources_util(gplates-lib PRIVATE ${srcs})

# Add the source files to the pygplates extension module.
#
# Note this is the external Python library that is not embedded inside GPlates.
# All sources files compiled in pygplates need to be position independent and hence
# compiled differently (POSITION_INDEPENDENT_CODE target property is set on pygplates).
set(pygplates_srcs ${srcs})
target_sources_util(pygplates PRIVATE ${pygplates_srcs})
