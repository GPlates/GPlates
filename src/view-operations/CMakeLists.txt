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
    AddPointGeometryOperation.cc
    AddPointGeometryOperation.h
    CloneOperation.cc
    CloneOperation.h
    DeleteFeatureOperation.cc
    DeleteFeatureOperation.h
    DeleteVertexGeometryOperation.cc
    DeleteVertexGeometryOperation.h
    FocusedFeatureGeometryManipulator.cc
    FocusedFeatureGeometryManipulator.h
    GeometryBuilder.cc
    GeometryBuilder.h
    GeometryBuilderUndoCommands.cc
    GeometryBuilderUndoCommands.h
    GeometryOperation.cc
    GeometryOperation.h
    GeometryOperationUndo.cc
    GeometryOperationUndo.h
    GeometryType.h
    InsertVertexGeometryOperation.cc
    InsertVertexGeometryOperation.h
    InternalGeometryBuilder.cc
    InternalGeometryBuilder.h
    MoveVertexGeometryOperation.cc
    MoveVertexGeometryOperation.h
    QueryProximityThreshold.h
    RenderedArrowedPolyline.h
    RenderedCircleSymbol.h
    RenderedCrossSymbol.h
    RenderedDirectionArrow.h
    RenderedEllipse.h
    RenderedGeometry.cc
    RenderedGeometry.h
    RenderedGeometryCollection.cc
    RenderedGeometryCollection.h
    RenderedGeometryCollectionVisitor.h
    RenderedGeometryFactory.cc
    RenderedGeometryFactory.h
    RenderedGeometryImpl.h
    RenderedGeometryLayer.cc
    RenderedGeometryLayer.h
    RenderedGeometryLayerVisitor.h
    RenderedGeometryParameters.h
    RenderedGeometryProximity.cc
    RenderedGeometryProximity.h
    RenderedGeometryUtils.cc
    RenderedGeometryUtils.h
    RenderedGeometryVisitor.h
    RenderedMultiPointOnSphere.h
    RenderedPointOnSphere.h
    RenderedPolygonOnSphere.h
    RenderedPolylineOnSphere.h
    RenderedReconstructionGeometry.h
    RenderedResolvedRaster.h
    RenderedResolvedScalarField3D.h
    RenderedSmallCircle.h
    RenderedSmallCircleArc.h
    RenderedSquareSymbol.h
    RenderedString.h
    RenderedTriangleSymbol.h
    SplitFeatureGeometryOperation.cc
    SplitFeatureGeometryOperation.h
    SplitFeatureUndoCommand.cc
    SplitFeatureUndoCommand.h
    UndoRedo.cc
    UndoRedo.h
    VisibleReconstructionGeometryExport.cc
    VisibleReconstructionGeometryExport.h
)





set(target_name view-operations)

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
