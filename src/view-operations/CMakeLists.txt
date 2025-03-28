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
    AddPointGeometryOperation.cc
    AddPointGeometryOperation.h
    ChangeLightDirectionOperation.cc
    ChangeLightDirectionOperation.h
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
    InsertVertexGeometryOperation.cc
    InsertVertexGeometryOperation.h
    InternalGeometryBuilder.cc
    InternalGeometryBuilder.h
    MovePoleOperation.cc
    MovePoleOperation.h
    MoveVertexGeometryOperation.cc
    MoveVertexGeometryOperation.h
    QueryProximityThreshold.h
    RenderedArrowedPolyline.h
    RenderedCircleSymbol.h
    RenderedColouredEdgeSurfaceMesh.h
    RenderedColouredMultiPointOnSphere.h
    RenderedColouredPolygonOnSphere.h
    RenderedColouredPolylineOnSphere.h
    RenderedColouredTriangleSurfaceMesh.h
    RenderedCrossSymbol.h
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
    RenderedMultiReconstructionGeometry.h
    RenderedPointOnSphere.h
    RenderedPolygonOnSphere.h
    RenderedPolylineOnSphere.h
    RenderedRadialArrow.h
    RenderedReconstructionGeometry.h
    RenderedResolvedRaster.h
    RenderedResolvedScalarField3D.h
    RenderedSmallCircle.h
    RenderedSmallCircleArc.h
    RenderedSquareSymbol.h
    RenderedStrainMarkerSymbol.h
    RenderedString.h
    RenderedSubductionTeethPolyline.h
    RenderedTangentialArrow.h
    RenderedTriangleSymbol.h
    ScalarField3DRenderParameters.cc
    ScalarField3DRenderParameters.h
    SplitFeatureGeometryOperation.cc
    SplitFeatureGeometryOperation.h
    SplitFeatureUndoCommand.cc
    SplitFeatureUndoCommand.h
    UndoRedo.cc
    UndoRedo.h
    VisibleReconstructionGeometryExport.cc
    VisibleReconstructionGeometryExport.h
)

# Add the source files to the gplates static library (or pygplates extension module).
target_sources_util(${SOURCE_TARGET} PRIVATE ${srcs})
