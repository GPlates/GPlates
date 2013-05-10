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
    ArbitraryNodeProcessor.h
    ArbitraryXmlProfile.cc
    ArbitraryXmlProfile.h
    ArbitraryXmlReader.cc
    ArbitraryXmlReader.h
    CitcomsFormatVelocityVectorFieldExport.cc
    CitcomsFormatVelocityVectorFieldExport.h
    CitcomsGMTFormatResolvedTopologicalBoundaryExport.cc
    CitcomsGMTFormatResolvedTopologicalBoundaryExport.h
    CitcomsResolvedTopologicalBoundaryExport.cc
    CitcomsResolvedTopologicalBoundaryExport.h
    CitcomsResolvedTopologicalBoundaryExportImpl.cc
    CitcomsResolvedTopologicalBoundaryExportImpl.h
    CptReader.cc
    CptReader.h
    ErrorOpeningFileForReadingException.cc
    ErrorOpeningFileForReadingException.h
    ErrorOpeningFileForWritingException.cc
    ErrorOpeningFileForWritingException.h
    ErrorOpeningPipeFromGzipException.cc
    ErrorOpeningPipeFromGzipException.h
    ErrorOpeningPipeToGzipException.cc
    ErrorOpeningPipeToGzipException.h
    ErrorWritingFeatureCollectionToFileFormatException.h
    ExportTemplateFilenameSequence.cc
    ExportTemplateFilenameSequence.h
    ExportTemplateFilenameSequenceFormats.cc
    ExportTemplateFilenameSequenceFormats.h
    ExportTemplateFilenameSequenceImpl.cc
    ExportTemplateFilenameSequenceImpl.h
    ExternalProgram.cc
    ExternalProgram.h
    FeatureCollectionFileFormat.h
    FeatureCollectionFileFormatClassify.cc
    FeatureCollectionFileFormatClassify.h
    FeatureCollectionFileFormatConfiguration.h
    FeatureCollectionFileFormatConfigurations.h
    FeatureCollectionFileFormatRegistry.cc
    FeatureCollectionFileFormatRegistry.h
    File.cc
    File.h
    FileFormatNotSupportedException.h
    FileInfo.cc
    FileInfo.h
    FileLoadAbortedException.h
    Gdal.h
    GdalRasterReader.cc
    GdalRasterReader.h
    GdalUtils.cc
    GdalUtils.h
    GeometryExporter.h
    GeoscimlProfile.cc
    GeoscimlProfile.h
    GmapReader.cc
    GmapReader.h
    GMTFormatFlowlineExport.cc
    GMTFormatFlowlineExport.h
    GMTFormatGeometryExporter.cc
    GMTFormatGeometryExporter.h
    GMTFormatHeader.cc
    GMTFormatHeader.h
    GMTFormatMotionPathExport.cc
    GMTFormatMotionPathExport.h
    GMTFormatMultiPointVectorFieldExport.cc
    GMTFormatMultiPointVectorFieldExport.h
    GMTFormatReconstructedFeatureGeometryExport.cc
    GMTFormatReconstructedFeatureGeometryExport.h
    GMTFormatResolvedTopologicalGeometryExport.cc
    GMTFormatResolvedTopologicalGeometryExport.h
    GMTFormatWriter.cc
    GMTFormatWriter.h
    GpmlFeatureReaderFactory.cc
    GpmlFeatureReaderFactory.h
    GpmlFeatureReaderImpl.cc
    GpmlFeatureReaderImpl.h
    GpmlFeatureReaderInterface.cc
    GpmlFeatureReaderInterface.h
    GpmlFormatMultiPointVectorFieldExport.cc
    GpmlFormatMultiPointVectorFieldExport.h
    GpmlOutputVisitor.cc
    GpmlOutputVisitor.h
    GpmlPropertyReader.cc
    GpmlPropertyReader.h
    GpmlPropertyStructuralTypeReader.cc
    GpmlPropertyStructuralTypeReader.h
    GpmlPropertyStructuralTypeReaderUtils.cc
    GpmlPropertyStructuralTypeReaderUtils.h
    GpmlReader.cc
    GpmlReader.h
    GpmlReaderException.h
    GpmlReaderUtils.cc
    GpmlReaderUtils.h
    GpmlStructuralTypeReaderUtils.cc
    GpmlStructuralTypeReaderUtils.h
    GpmlUpgradeReaderUtils.cc
    GpmlUpgradeReaderUtils.h
    GsmlConst.h
    GsmlFeatureHandlers.cc
    GsmlFeatureHandlers.h
    GsmlFeaturesDef.h
    GsmlNodeProcessor.cc
    GsmlNodeProcessor.h
    GsmlNodeProcessorFactory.cc
    GsmlNodeProcessorFactory.h
    GsmlPropertyDef.h
    GsmlPropertyHandlers.cc
    GsmlPropertyHandlers.h
    LineReader.cc
    LineReader.h
    LogToFileHandler.cc
    LogToFileHandler.h
    MipmappedRasterFormatReader.h
    MipmappedRasterFormatWriter.cc
    MipmappedRasterFormatWriter.h
    MultiPointVectorFieldExport.cc
    MultiPointVectorFieldExport.h
    OgrException.h
    OgrFeatureCollectionWriter.cc
    OgrFeatureCollectionWriter.h
    OgrFormatFlowlineExport.cc
    OgrFormatFlowlineExport.h
    OgrFormatMotionPathExport.cc
    OgrFormatMotionPathExport.h
    OgrFormatReconstructedFeatureGeometryExport.cc
    OgrFormatReconstructedFeatureGeometryExport.h
    OgrFormatResolvedTopologicalGeometryExport.cc
    OgrFormatResolvedTopologicalGeometryExport.h
    OgrGeometryExporter.cc
    OgrGeometryExporter.h
    OgrReader.cc
    OgrReader.h
    OgrUtils.cc
    OgrUtils.h
    OgrWriter.cc
    OgrWriter.h
    PlatesFormatUtils.cc
    PlatesFormatUtils.h
    PlatesLineFormatGeometryExporter.cc
    PlatesLineFormatGeometryExporter.h
    PlatesLineFormatHeaderVisitor.cc
    PlatesLineFormatHeaderVisitor.h
    PlatesLineFormatReader.cc
    PlatesLineFormatReader.h
    PlatesLineFormatWriter.cc
    PlatesLineFormatWriter.h
    PlatesRotationFileProxy.cc
    PlatesRotationFileProxy.h
    PlatesRotationFormatReader.cc
    PlatesRotationFormatReader.h
    PlatesRotationFormatWriter.cc
    PlatesRotationFormatWriter.h
    PropertyMapper.h
    RasterBandReader.cc
    RasterBandReader.h
    RasterBandReaderHandle.cc
    RasterBandReaderHandle.h
    RasterFileCache.h
    RasterFileCacheFormat.cc
    RasterFileCacheFormat.h
    RasterFileCacheFormatReader.h
    RasterReader.cc
    RasterReader.h
    ReadErrorAccumulation.h
    ReadErrorMessages.cc
    ReadErrorMessages.h
    ReadErrorOccurrence.cc
    ReadErrorOccurrence.h
    ReadErrors.h
    ReadErrorUtils.cc
    ReadErrorUtils.h
    ReconstructedFeatureGeometryExport.cc
    ReconstructedFeatureGeometryExport.h
    ReconstructedFlowlineExport.cc
    ReconstructedFlowlineExport.h
    ReconstructedMotionPathExport.cc
    ReconstructedMotionPathExport.h
    ReconstructionGeometryExportImpl.cc
    ReconstructionGeometryExportImpl.h
    ResolvedTopologicalGeometryExport.cc
    ResolvedTopologicalGeometryExport.h
    RgbaRasterReader.cc
    RgbaRasterReader.h
    RotationAttributesRegistry.cc
    RotationAttributesRegistry.h
    ScalarField3DFileFormat.cc
    ScalarField3DFileFormat.h
    ScalarField3DFileFormatReader.cc
    ScalarField3DFileFormatReader.h
    ShapefileXmlReader.cc
    ShapefileXmlReader.h
    ShapefileXmlWriter.cc
    ShapefileXmlWriter.h
    SourceRasterFileCacheFormatReader.h
    SymbolFileReader.cc
    SymbolFileReader.h
    TemporaryFileRegistry.cc
    TemporaryFileRegistry.h
    TerraFormatVelocityVectorFieldExport.cc
    TerraFormatVelocityVectorFieldExport.h
    XmlOutputInterface.cc
    XmlOutputInterface.h
    XmlWriter.cc
    XmlWriter.h
)





set(target_name file-io)

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
