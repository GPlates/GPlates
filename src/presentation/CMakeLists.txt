#
# List the non-generated source files (*.h, *.cc, *.ui, *.qrc).
#
# Add and remove source files here by hand, keeping the list in alphabetical order.
# (CMake discourages 'file(GLOB)': if source files are added or removed, CMake is not
# automatically re-run, so the build would be unaware of the change.)
#
set(srcs
    Application.cc
    Application.h
    DeprecatedSessionRestore.cc
    DeprecatedSessionRestore.h
    FileIOInjections.cc
    FileIOInjections.h
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

if (TARGET gplates-lib)
    # Add the source files to the gplates static library.
    # (The pygplates module compiles nothing from this directory - see
    # "cmake/pygplates_source_closure.py".)
    target_sources_util(gplates-lib PRIVATE ${srcs})
endif()
