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
    AgeGridLayerTask.cc
    AgeGridLayerTask.h
    AgeGridRaster.cc
    AgeGridRaster.h
    ApplicationState.cc
    ApplicationState.h
    AppLogicUtils.cc
    AppLogicUtils.h
    AssignPlateIds.cc
    AssignPlateIds.h
    CgalUtils.cc
    CgalUtils.h
    ClassifyFeatureCollection.cc
    ClassifyFeatureCollection.h
    CoRegistrationData.cc
    CoRegistrationData.h
    CoRegistrationLayerTask.cc
    CoRegistrationLayerTask.h
    FeatureCollectionFileIO.cc
    FeatureCollectionFileIO.h
    FeatureCollectionFileState.cc
    FeatureCollectionFileState.h
    FlowlineGeometryPopulator.cc
    FlowlineGeometryPopulator.h
    FlowlineUtils.cc
    FlowlineUtils.h
    GenericPartitionFeatureTask.cc
    GenericPartitionFeatureTask.h
    GeometryCookieCutter.cc
    GeometryCookieCutter.h
    GeometryUtils.cc
    GeometryUtils.h
    Layer.cc
    Layer.h
    LayerTask.cc
    LayerTask.h
    LayerTaskDataType.h
    LayerTaskParams.h
    LayerTaskRegistry.cc
    LayerTaskRegistry.h
    LayerTaskType.h
    MeshGenerator.cc
    MeshGenerator.h
    MotionPathGeometryPopulator.cc
    MotionPathGeometryPopulator.h
    MotionPathUtils.cc
    MotionPathUtils.h
    MultiPointVectorField.cc
    MultiPointVectorField.h
    PartitionFeatureTask.cc
    PartitionFeatureTask.h
    PartitionFeatureUtils.cc
    PartitionFeatureUtils.h
    PlateVelocityUtils.cc
    PlateVelocityUtils.h
    PropertyExtractors.cc
    PropertyExtractors.h
    RasterLayerTask.cc
    RasterLayerTask.h
    ReconstructedFeatureGeometry.cc
    ReconstructedFeatureGeometry.h
    ReconstructedFeatureGeometryFinder.cc
    ReconstructedFeatureGeometryFinder.h
    ReconstructedFeatureGeometryPopulator.cc
    ReconstructedFeatureGeometryPopulator.h
    ReconstructedFlowline.cc
    ReconstructedFlowline.h
    ReconstructedMotionPath.cc
    ReconstructedMotionPath.h
    ReconstructedPaleomagGeometryPopulator.h
    ReconstructedVirtualGeomagneticPole.cc
    ReconstructedVirtualGeomagneticPole.h
    ReconstructGraph.cc
    ReconstructGraph.h
    ReconstructGraphImpl.cc
    ReconstructGraphImpl.h
    Reconstruction.cc
    Reconstruction.h
    ReconstructionFeatureProperties.cc
    ReconstructionFeatureProperties.h
    ReconstructionGeometry.cc
    ReconstructionGeometry.h
    ReconstructionGeometryCollection.cc
    ReconstructionGeometryCollection.h
    ReconstructionGeometryFinder.cc
    ReconstructionGeometryFinder.h
    ReconstructionGeometryUtils.cc
    ReconstructionGeometryUtils.h
    ReconstructionGeometryVisitor.h
    ReconstructionGraph.cc
    ReconstructionGraph.h
    ReconstructionLayerTask.cc
    ReconstructionLayerTask.h
    ReconstructionMethod.cc
    ReconstructionMethod.h
    ReconstructionTree.cc
    ReconstructionTree.h
    ReconstructionTreeEdge.cc
    ReconstructionTreeEdge.h
    ReconstructionTreePopulator.cc
    ReconstructionTreePopulator.h
    ReconstructLayerTask.cc
    ReconstructLayerTask.h
    ReconstructRasterPolygons.cc
    ReconstructRasterPolygons.h
    ReconstructUtils.cc
    ReconstructUtils.h
    ResolvedRaster.cc
    ResolvedRaster.h
    ResolvedTopologicalBoundary.cc
    ResolvedTopologicalBoundary.h
    ResolvedTopologicalNetwork.cc
    ResolvedTopologicalNetwork.h
    Session.cc
    Session.h
    SessionManagement.cc
    SessionManagement.h
    TopologyBoundaryIntersections.cc
    TopologyBoundaryIntersections.h
    TopologyBoundaryResolver.cc
    TopologyBoundaryResolver.h
    TopologyBoundaryResolverLayerTask.cc
    TopologyBoundaryResolverLayerTask.h
    TopologyInternalUtils.cc
    TopologyInternalUtils.h
    TopologyNetworkResolver.cc
    TopologyNetworkResolver.h
    TopologyNetworkResolverLayerTask.cc
    TopologyNetworkResolverLayerTask.h
    TopologyUtils.cc
    TopologyUtils.h
    UserPreferences.cc
    UserPreferences.h
    VelocityFieldCalculatorLayerTask.cc
    VelocityFieldCalculatorLayerTask.h
    VgpPartitionFeatureTask.cc
    VgpPartitionFeatureTask.h
    VGPRenderSettings.cc
    VGPRenderSettings.h
)





set(target_name app-logic)

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
