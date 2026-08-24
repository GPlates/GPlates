#
# List the non-generated source files (*.h, *.cc, *.ui, *.qrc).
#
# Add and remove source files here by hand, keeping the list in alphabetical order.
# (CMake discourages 'file(GLOB)': if source files are added or removed, CMake is not
# automatically re-run, so the build would be unaware of the change.)
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

if (TARGET gplates-lib)
    # Add the source files to the gplates static library.
    # (The pygplates module compiles nothing from this directory - see
    # "cmake/pygplates_source_closure.py".)
    target_sources_util(gplates-lib PRIVATE ${srcs})
endif()
