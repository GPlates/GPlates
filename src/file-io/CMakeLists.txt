#
# List the non-generated source files (*.h, *.cc, *.ui, *.qrc).
#
# Add and remove source files here by hand, keeping the list in alphabetical order.
# (CMake discourages 'file(GLOB)': if source files are added or removed, CMake is not
# automatically re-run, so the build would be unaware of the change.)
#
set(srcs
    AgeModelReader.cc
    AgeModelReader.h
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
    DeformationExport.cc
    DeformationExport.h
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
    FeatureCollectionFileFormat.h
    FeatureCollectionFileFormatClassify.cc
    FeatureCollectionFileFormatClassify.h
    FeatureCollectionFileFormatConfiguration.h
    FeatureCollectionFileFormatConfigurations.cc
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
    GdalRasterWriter.cc
    GdalRasterWriter.h
    GdalUtils.cc
    GdalUtils.h
    GeometryExporter.h
    GmapReader.cc
    GmapReader.h
    GMTFormatDeformationExport.cc
    GMTFormatDeformationExport.h
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
    GMTFormatReconstructedScalarCoverageExport.cc
    GMTFormatReconstructedScalarCoverageExport.h
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
    GpmlFormatDeformationExport.cc
    GpmlFormatDeformationExport.h
    GpmlFormatMultiPointVectorFieldExport.cc
    GpmlFormatMultiPointVectorFieldExport.h
    GpmlFormatReconstructedScalarCoverageExport.cc
    GpmlFormatReconstructedScalarCoverageExport.h
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
    GzipFile.cc
    GzipFile.h
    HellingerReader.cc
    HellingerReader.h
    HellingerWriter.cc
    HellingerWriter.h
    LineReader.cc
    LineReader.h
    LogToFileHandler.cc
    LogToFileHandler.h
    MipmappedRasterFormatReader.h
    MipmappedRasterFormatWriter.cc
    MipmappedRasterFormatWriter.h
    MultiPointVectorFieldExport.cc
    MultiPointVectorFieldExport.h
    Ogr.h
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
    Proj.h
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
    RasterWriter.cc
    RasterWriter.h
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
    ReconstructedScalarCoverageExport.cc
    ReconstructedScalarCoverageExport.h
    ReconstructionGeometryExportImpl.cc
    ReconstructionGeometryExportImpl.h
    ResolvedTopologicalGeometryExport.cc
    ResolvedTopologicalGeometryExport.h
    RgbaRasterReader.cc
    RgbaRasterReader.h
    RgbaRasterWriter.cc
    RgbaRasterWriter.h
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
    StandaloneBundle.cc
    StandaloneBundle.h
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

# Qt6 removed the QtXmlPatterns module providing support for XPath, XQuery, XSLT, and XML Schema validation.
# It has been deprecated since Qt 5.13.
#
# TODO: Find a replacement library. Re-implementing these sources on one is a sizeable job, and whether
#       the GeoSciML support is worth preserving at all has not been investigated - so for now they
#       simply build against Qt5 and not against Qt6.
#
# Note: This group needs only Qt Core, Xml and XmlPatterns, so a Qt5 build of the pygplates module
#       compiles it too (the ".gsml" file format is registered by "FeatureCollectionFileFormatRegistry.cc",
#       which every build compiles). Its progress dialog is Qt Widgets and therefore lives in
#       "qt-widgets/GeoscimlProgressDialog.cc", injected by GPlates through
#       GeoscimlProfile::set_progress_reporter_factory(). Shipped pyGPlates is Qt6-only - the conda
#       environments ("qt6-main" in "env.*.yml") and the wheels ("QT_VERSION" in
#       "pygplates/wheel/versions.sh") - so no pyGPlates we ship compiles these files at all.
if (QT_VERSION_MAJOR LESS 6)
    # Note: a few of these headers ("ArbitraryXmlReader.h", "GeoscimlProfile.h", ...) are
    #       #included by translation units every build compiles (with the uses guarded by
    #       QT_VERSION), which is harmless - but they must only be *listed* here, in the
    #       Qt5-only block, so that a Qt6 build does not see them as target sources. The
    #       source-closure tracer treats the group the same way (see QT5_ONLY_FILES in
    #       "cmake/pygplates_source_closure.py").
    list(APPEND srcs
        ArbitraryNodeProcessor.h
        ArbitraryXmlProfile.h
        ArbitraryXmlReader.cc
        ArbitraryXmlReader.h
        GeoscimlProfile.cc
        GeoscimlProfile.h
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
        GsmlPropertyHandlers.h)
endif()


# GPlates-only sources: excluded from the pygplates module, whose source list is the
# include closure of the pyGPlates API (see "cmake/pygplates_source_closure.py"; the
# pygplates-source-closure test fails if this list drifts from that closure).
# Keep ".h"/".cc" pairs together: AUTOMOC mocs any Q_OBJECT header listed in the
# module's sources, and a moc'ed header whose ".cc" is excluded fails at link time.
set(gplates_only_srcs
    AgeModelReader.cc
    AgeModelReader.h
    CitcomsFormatVelocityVectorFieldExport.cc
    CitcomsFormatVelocityVectorFieldExport.h
    CitcomsGMTFormatResolvedTopologicalBoundaryExport.cc
    CitcomsGMTFormatResolvedTopologicalBoundaryExport.h
    CitcomsResolvedTopologicalBoundaryExport.cc
    CitcomsResolvedTopologicalBoundaryExport.h
    CptReader.cc
    CptReader.h
    DeformationExport.cc
    DeformationExport.h
    ExportTemplateFilenameSequence.cc
    ExportTemplateFilenameSequence.h
    ExportTemplateFilenameSequenceFormats.cc
    ExportTemplateFilenameSequenceFormats.h
    ExportTemplateFilenameSequenceImpl.cc
    ExportTemplateFilenameSequenceImpl.h
    GMTFormatDeformationExport.cc
    GMTFormatDeformationExport.h
    GMTFormatMultiPointVectorFieldExport.cc
    GMTFormatMultiPointVectorFieldExport.h
    GMTFormatReconstructedScalarCoverageExport.cc
    GMTFormatReconstructedScalarCoverageExport.h
    GdalRasterWriter.cc
    GdalRasterWriter.h
    GpmlFormatDeformationExport.cc
    GpmlFormatDeformationExport.h
    GpmlFormatMultiPointVectorFieldExport.cc
    GpmlFormatMultiPointVectorFieldExport.h
    GpmlFormatReconstructedScalarCoverageExport.cc
    GpmlFormatReconstructedScalarCoverageExport.h
    HellingerReader.cc
    HellingerReader.h
    HellingerWriter.cc
    HellingerWriter.h
    LogToFileHandler.cc
    LogToFileHandler.h
    MultiPointVectorFieldExport.cc
    MultiPointVectorFieldExport.h
    RasterWriter.cc
    RasterWriter.h
    ReadErrorMessages.cc
    ReadErrorMessages.h
    ReadErrorUtils.cc
    ReadErrorUtils.h
    ReconstructedScalarCoverageExport.cc
    ReconstructedScalarCoverageExport.h
    RgbaRasterReader.cc
    RgbaRasterReader.h
    RgbaRasterWriter.cc
    RgbaRasterWriter.h
    ScalarField3DFileFormat.cc
    ScalarField3DFileFormat.h
    ScalarField3DFileFormatReader.cc
    ScalarField3DFileFormatReader.h
    SymbolFileReader.cc
    SymbolFileReader.h
    TerraFormatVelocityVectorFieldExport.cc
    TerraFormatVelocityVectorFieldExport.h
    XmlOutputInterface.cc
    XmlOutputInterface.h
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
