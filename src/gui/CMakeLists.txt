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
    CanvasToolWorkflow.cc
    CanvasToolWorkflow.h
    CanvasToolWorkflows.cc
    CanvasToolWorkflows.h
    ChooseCanvasToolUndoCommand.cc
    ChooseCanvasToolUndoCommand.h
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
    CommandServer.cc
    CommandServer.h
    ConfigGuiUtils.cc
    ConfigGuiUtils.h
    ConfigModel.cc
    ConfigModel.h
    ConfigValueDelegate.cc
    ConfigValueDelegate.h
    CptColourPalette.cc
    CptColourPalette.h
    CsvExport.cc
    CsvExport.h
    Dialogs.cc
    Dialogs.h
    DigitisationCanvasToolWorkflow.cc
    DigitisationCanvasToolWorkflow.h
    DockState.cc
    DockState.h
    DrawStyleAdapters.cc
    DrawStyleAdapters.h
    DrawStyleManager.cc
    DrawStyleManager.h
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
    ExportCitcomsResolvedTopologyAnimationStrategy.cc
    ExportCitcomsResolvedTopologyAnimationStrategy.h
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
    ExportStageRotationAnimationStrategy.cc
    ExportStageRotationAnimationStrategy.h
    ExportSvgAnimationStrategy.cc
    ExportSvgAnimationStrategy.h
    ExportTotalRotationAnimationStrategy.cc
    ExportTotalRotationAnimationStrategy.h
    ExportVelocityAnimationStrategy.cc
    ExportVelocityAnimationStrategy.h
    ExternalSyncController.cc
    ExternalSyncController.h
    FeatureFocus.cc
    FeatureFocus.h
    FeatureInspectionCanvasToolWorkflow.cc
    FeatureInspectionCanvasToolWorkflow.h
    FeaturePropertyTableModel.cc
    FeaturePropertyTableModel.h
    FeatureTableModel.cc
    FeatureTableModel.h
    FeatureTypeColourPalette.cc
    FeatureTypeColourPalette.h
    FeedbackOpenGLToQPainter.cc
    FeedbackOpenGLToQPainter.h
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
    GraticuleSettings.h
    GuiDebug.cc
    GuiDebug.h
    GuiException.h
    HTMLColourNames.cc
    HTMLColourNames.h
    ImportMenu.cc
    ImportMenu.h
    LayerPainter.cc
    LayerPainter.h
    LogFilterModel.cc
    LogFilterModel.h
    Map.cc
    Map.h
    MapCanvasTool.cc
    MapCanvasTool.h
    MapCanvasToolAdapter.cc
    MapCanvasToolAdapter.h
    MapGrid.cc
    MapGrid.h
    MapProjection.cc
    MapProjection.h
    MapRenderedGeometryCollectionPainter.cc
    MapRenderedGeometryCollectionPainter.h
    MapRenderedGeometryLayerPainter.cc
    MapRenderedGeometryLayerPainter.h
    MapTransform.cc
    MapTransform.h
    Mipmapper.cc
    Mipmapper.h
    OpaqueSphere.cc
    OpaqueSphere.h
    Palette.cc
    Palette.h
    PlateIdColourPalettes.cc
    PlateIdColourPalettes.h
    PoleManipulationCanvasToolWorkflow.cc
    PoleManipulationCanvasToolWorkflow.h
    ProjectionException.h
    PythonConfiguration.cc
    PythonConfiguration.h
    PythonConsoleHistory.cc
    PythonConsoleHistory.h
    PythonManager.cc
    PythonManager.h
    QGLWidgetTextRenderer.cc
    QGLWidgetTextRenderer.h
    QPainterTextRenderer.cc
    QPainterTextRenderer.h
    RasterColourPalette.cc
    RasterColourPalette.h
    ReconstructionGeometrySequence.h
    RenderSettings.h
    SceneLightingParameters.h
    SessionMenu.cc
    SessionMenu.h
    SimpleGlobeOrientation.cc
    SimpleGlobeOrientation.h
    SingleColourScheme.cc
    SingleColourScheme.h
    SmallCircleCanvasToolWorkflow.cc
    SmallCircleCanvasToolWorkflow.h
    SphericalGrid.cc
    SphericalGrid.h
    Stars.cc
    Stars.h
    Symbol.cc
    Symbol.h
    TextOverlay.cc
    TextOverlay.h
    TextOverlaySettings.cc
    TextOverlaySettings.h
    TextRenderer.cc
    TextRenderer.h
    TopologyCanvasToolWorkflow.cc
    TopologyCanvasToolWorkflow.h
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
    ViewCanvasToolWorkflow.cc
    ViewCanvasToolWorkflow.h
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
