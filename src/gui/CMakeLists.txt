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
    AddClickedGeometriesToFeatureTable.cc
    AddClickedGeometriesToFeatureTable.h
    AgeColourPalettes.cc
    AgeColourPalettes.h
    AnimationController.cc
    AnimationController.h
    ChooseCanvasTool.cc
    ChooseCanvasTool.h
    Colour.cc
    Colour.h
    ColourFilter.h
    ColourNameSet.cc
    ColourNameSet.h
    ColourPalette.h
    ColourPaletteAdapter.h
    ColourPaletteVisitor.h
    ColourProxy.cc
    ColourProxy.h
    ColourRawRaster.h
    ColourScheme.h
    ColourSchemeContainer.cc
    ColourSchemeContainer.h
    ColourSchemeDelegator.cc
    ColourSchemeDelegator.h
    ColourSchemeInfo.h
    ColourSpectrum.cc
    ColourSpectrum.h
    CptColourPalette.cc
    CptColourPalette.h
    CsvExport.cc
    CsvExport.h
    DockState.cc
    DockState.h
    EnableCanvasTool.cc
    EnableCanvasTool.h
    EventBlackout.cc
    EventBlackout.h
    ExportAnimationContext.cc
    ExportAnimationContext.h
    ExportAnimationRegistry.cc
    ExportAnimationRegistry.h
    ExportAnimationStrategy.cc
    ExportAnimationStrategy.h
    ExportAnimationType.cc
    ExportAnimationType.h
    ExportCoRegistrationAnimationStrategy.cc
    ExportCoRegistrationAnimationStrategy.h
    ExportFileNameTemplateValidationUtils.cc
    ExportFileNameTemplateValidationUtils.h
    ExportFlowlineAnimationStrategy.cc
    ExportFlowlineAnimationStrategy.h
    ExportMotionPathAnimationStrategy.cc
    ExportMotionPathAnimationStrategy.h
    ExportOptionsUtils.h
    ExportRasterAnimationStrategy.cc
    ExportRasterAnimationStrategy.h
    ExportReconstructedGeometryAnimationStrategy.cc
    ExportReconstructedGeometryAnimationStrategy.h
    ExportResolvedTopologyAnimationStrategy.cc
    ExportResolvedTopologyAnimationStrategy.h
    ExportRotationAnimationStrategy.cc
    ExportRotationAnimationStrategy.h
    ExportRotationParamsAnimationStrategy.cc
    ExportRotationParamsAnimationStrategy.h
    ExportSvgAnimationStrategy.cc
    ExportSvgAnimationStrategy.h
    ExportVelocityAnimationStrategy.cc
    ExportVelocityAnimationStrategy.h
    FeatureFocus.cc
    FeatureFocus.h
    FeaturePropertyTableModel.cc
    FeaturePropertyTableModel.h
    FeatureTableModel.cc
    FeatureTableModel.h
    FeatureTypeColourPalette.cc
    FeatureTypeColourPalette.h
    FileIOFeedback.cc
    FileIOFeedback.h
    FullScreenMode.cc
    FullScreenMode.h
    GenericColourScheme.h
    GeometryFocusHighlight.cc
    GeometryFocusHighlight.h
    Globe.cc
    Globe.h
    GlobeCanvasTool.cc
    GlobeCanvasTool.h
    GlobeCanvasToolAdapter.cc
    GlobeCanvasToolAdapter.h
    GlobeCanvasToolChoice.cc
    GlobeCanvasToolChoice.h
    GlobeOrientation.h
    GlobeRenderedGeometryCollectionPainter.cc
    GlobeRenderedGeometryCollectionPainter.h
    GlobeRenderedGeometryLayerPainter.cc
    GlobeRenderedGeometryLayerPainter.h
    GlobeVisibilityTester.cc
    GlobeVisibilityTester.h
    GMTColourNames.cc
    GMTColourNames.h
    GPlatesQApplication.cc
    GPlatesQApplication.h
    GPlatesQtMsgHandler.cc
    GPlatesQtMsgHandler.h
    GraticuleSettings.h
    GuiDebug.cc
    GuiDebug.h
    GuiException.h
    HTMLColourNames.cc
    HTMLColourNames.h
    ImportMenu.cc
    ImportMenu.h
    Map.cc
    Map.h
    MapCanvasPainter.cc
    MapCanvasPainter.h
    MapCanvasTool.cc
    MapCanvasTool.h
    MapCanvasToolAdapter.cc
    MapCanvasToolAdapter.h
    MapCanvasToolChoice.cc
    MapCanvasToolChoice.h
    MapProjection.cc
    MapProjection.h
    MapTransform.cc
    MapTransform.h
    Mipmapper.cc
    Mipmapper.h
    OpaqueSphere.cc
    OpaqueSphere.h
    PersistentOpenGLObjects.cc
    PersistentOpenGLObjects.h
    PlateIdColourPalettes.cc
    PlateIdColourPalettes.h
    ProjectionException.h
    PythonConsoleHistory.cc
    PythonConsoleHistory.h
    QGLWidgetTextRenderer.cc
    QGLWidgetTextRenderer.h
    RasterColourPalette.cc
    RasterColourPalette.h
    ReconstructionGeometrySequence.h
    RenderSettings.h
    SessionMenu.cc
    SessionMenu.h
    SimpleGlobeOrientation.cc
    SimpleGlobeOrientation.h
    SingleColourScheme.cc
    SingleColourScheme.h
    SphericalGrid.cc
    SphericalGrid.h
    Stars.cc
    Stars.h
    SvgExport.cc
    SvgExport.h
    Symbol.cc
    Symbol.h
    TextOverlay.cc
    TextOverlay.h
    TextOverlaySettings.cc
    TextOverlaySettings.h
    TextRenderer.h
    TopologySectionsContainer.cc
    TopologySectionsContainer.h
    TopologySectionsTable.cc
    TopologySectionsTable.h
    TopologySectionsTableColumns.cc
    TopologySectionsTableColumns.h
    TopologyTools.cc
    TopologyTools.h
    TreeWidgetBuilder.cc
    TreeWidgetBuilder.h
    TrinketArea.cc
    TrinketArea.h
    UnsavedChangesTracker.cc
    UnsavedChangesTracker.h
    UtilitiesMenu.cc
    UtilitiesMenu.h
    ViewportProjection.h
    ViewportZoom.cc
    ViewportZoom.h
    VisualLayersListModel.cc
    VisualLayersListModel.h
    VisualLayersProxy.cc
    VisualLayersProxy.h
)





set(target_name gui)

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
