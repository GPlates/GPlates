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
    Application.cc
    Application.h
    DeprecatedSessionRestore.cc
    DeprecatedSessionRestore.h
    InternalSession.cc
    InternalSession.h
    LayerOutputRenderer.cc
    LayerOutputRenderer.h
    ProjectSession.cc
    ProjectSession.h
    RasterVisualLayerParams.cc
    RasterVisualLayerParams.h
    ReconstructionGeometryRenderer.cc
    ReconstructionGeometryRenderer.h
    ReconstructScalarCoverageVisualLayerParams.cc
    ReconstructScalarCoverageVisualLayerParams.h
    ReconstructVisualLayerParams.cc
    ReconstructVisualLayerParams.h
    RemappedColourPaletteParameters.cc
    RemappedColourPaletteParameters.h
    ScalarField3DVisualLayerParams.cc
    ScalarField3DVisualLayerParams.h
    Session.cc
    Session.h
    SessionManagement.cc
    SessionManagement.h
    TopologyGeometryVisualLayerParams.h
    TopologyNetworkVisualLayerParams.cc
    TopologyNetworkVisualLayerParams.h
    TranscribeSession.cc
    TranscribeSession.h
    VelocityFieldCalculatorVisualLayerParams.h
    ViewState.cc
    ViewState.h
    VisualLayer.cc
    VisualLayer.h
    VisualLayerGroup.h
    VisualLayerInputChannelName.cc
    VisualLayerInputChannelName.h
    VisualLayerParams.h
    VisualLayerParamsVisitor.h
    VisualLayerRegistry.cc
    VisualLayerRegistry.h
    VisualLayers.cc
    VisualLayers.h
    VisualLayerType.h
)

# Add the source files to the gplates static library (or pygplates extension module).
target_sources_util(${SOURCE_TARGET} PRIVATE ${srcs})
