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
    AbstractWindow.h
    AgeColourTable.cc
    AgeColourTable.h
    AnimationController.cc
    AnimationController.h
    ChooseCanvasTool.cc
    ChooseCanvasTool.h
    Colour.cc
    Colour.h
    ColourNameSet.cc
    ColourNameSet.h
    ColourPalette.h
    ColourScheme.h
    ColourSchemeFactory.h
    ColourSpectrum.cc
    ColourSpectrum.h
    ColourTable.h
    ColourTableDelegator.h
    CsvExport.cc
    CsvExport.h
    EnableCanvasTool.cc
    EnableCanvasTool.h
    ExportAnimationContext.cc
    ExportAnimationContext.h
    ExportAnimationStrategy.cc
    ExportAnimationStrategy.h
    ExportReconstructedGeometryAnimationStrategy.cc
    ExportReconstructedGeometryAnimationStrategy.h
    ExportResolvedTopologyAnimationStrategy.cc
    ExportResolvedTopologyAnimationStrategy.h
    ExportSvgAnimationStrategy.cc
    ExportSvgAnimationStrategy.h
    ExportVelocityAnimationStrategy.cc
    ExportVelocityAnimationStrategy.h
    FeatureColourTable.cc
    FeatureColourTable.h
    FeatureFocus.cc
    FeatureFocus.h
    FeaturePropertyTableModel.cc
    FeaturePropertyTableModel.h
    FeatureTableModel.cc
    FeatureTableModel.h
    FeatureWeakRefSequence.h
    FullScreenMode.cc
    FullScreenMode.h
    GenericColourScheme.h
    GenericContinuousColourPalette.cc
    GenericContinuousColourPalette.h
    GenericDiscreteColourPalette.h
    GeometricPropertyValueConstructor.cc
    GeometricPropertyValueConstructor.h
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
    GuiException.h
    HTMLColourNames.cc
    HTMLColourNames.h
    Interface.h
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
    NurbsRenderer.cc
    NurbsRenderer.h
    OpaqueSphere.cc
    OpaqueSphere.h
    OpenGL.h
    OpenGLBadAllocException.h
    OpenGLException.h
    PlateIdColourPalettes.cc
    PlateIdColourPalettes.h
    ProjectionException.h
    ProximityTests.cc
    ProximityTests.h
    QGLWidgetTextRenderer.cc
    QGLWidgetTextRenderer.h
    Quadrics.cc
    Quadrics.h
    ReconstructionGeometrySequence.h
    RenderSettings.h
    SimpleGlobeOrientation.cc
    SimpleGlobeOrientation.h
    SingleColourScheme.cc
    SingleColourScheme.h
    SphericalGrid.cc
    SphericalGrid.h
    SvgExport.cc
    SvgExport.h
    TemporaryGeometryFocus.cc
    TemporaryGeometryFocus.h
    TextRenderer.h
    Texture.cc
    Texture.h
    TopologySectionsContainer.cc
    TopologySectionsContainer.h
    TopologySectionsTable.cc
    TopologySectionsTable.h
    TopologyTools.cc
    TopologyTools.h
    TreeWidgetBuilder.cc
    TreeWidgetBuilder.h
    Viewport.h
    ViewportZoom.cc
    ViewportZoom.h
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

add_library(${target_name} ${target_srcs})

GPLATES_POST_ADD_TARGET(${target_name} target_srcs)
