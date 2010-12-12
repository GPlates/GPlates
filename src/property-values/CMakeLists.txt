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
    GpmlStringList.cc
    GpmlStringList.h
    GpmlTimeSample.cc
    GpmlTimeSample.h
    GpmlTimeWindow.cc
    GpmlTimeWindow.h
    GpmlTopologicalIntersection.cc
    GpmlTopologicalIntersection.h
    GpmlTopologicalLineSection.cc
    GpmlTopologicalLineSection.h
    GpmlTopologicalPoint.cc
    GpmlTopologicalPoint.h
    GpmlTopologicalPolygon.cc
    GpmlTopologicalPolygon.h
    GpmlTopologicalSection.cc
    GpmlTopologicalSection.h
    PropertyValues.cc
    PropertyValues.h
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
    TemplateTypeParameterType.h
    TextContent.h
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





set(target_name property-values)

# Does the following:
# - adds dependencies on generated header files.
# - finds '.ui' files and adds commands to create 'Ui.h' files.
# - scans '.h' files for Q_OBJECT and adds commands to run Qt's moc.exe to generate 'moc_*.cc' files.
# - finds '.qrc' files and adds commands to generate '.cc' files from them.
# - adds header files to Visual Studio projects.
# - groups files in Visual Studio IDE according to type.
GPLATES_GENERATE_TARGET_SOURCES(${target_name} target_srcs ${srcs})

add_library(${target_name} STATIC ${target_srcs})

GPLATES_POST_ADD_TARGET(${target_name} target_srcs)
