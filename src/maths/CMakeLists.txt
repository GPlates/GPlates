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
    AngularDistance.cc
    AngularDistance.h
    AngularExtent.cc
    AngularExtent.h
    AzimuthalEqualAreaProjection.cc
    AzimuthalEqualAreaProjection.h
    CalculateVelocity.cc
    CalculateVelocity.h
    CartesianConvMatrix3D.cc
    CartesianConvMatrix3D.h
    Centroid.cc
    Centroid.h
    ConstGeometryOnSphereVisitor.h
    CubeCoordinateFrame.cc
    CubeCoordinateFrame.h
    CubeQuadTree.h
    CubeQuadTreeLocation.cc
    CubeQuadTreeLocation.h
    CubeQuadTreePartition.h
    CubeQuadTreePartitionUtils.h
    DateLineWrapper.cc
    DateLineWrapper.h
    EllipseGenerator.cc
    EllipseGenerator.h
    FiniteRotation.cc
    FiniteRotation.h
    FiniteRotationSnapshotTable.h
    FunctionDomainException.h
    GeneratePoints.cc
    GeneratePoints.h
    GenericVectorOps3D.h
    GeometryCrossing.cc
    GeometryCrossing.h
    GeometryDistance.cc
    GeometryDistance.h
    GeometryForwardDeclarations.h
    GeometryInterpolation.cc
    GeometryInterpolation.h
    GeometryIntersect.cc
    GeometryIntersect.h
    GeometryOnSphere.h
    GeometryType.h
    GnomonicProjection.cc
    GnomonicProjection.h
    GreatCircle.cc
    GreatCircle.h
    GreatCircleArc.cc
    GreatCircleArc.h
    HighPrecision.h
    IndeterminateArcRotationAxisException.h
    IndeterminateResultException.h
    InvalidGreatCircleArcException.h
    InvalidGridException.h
    InvalidLatLonCoordinateException.cc
    InvalidLatLonCoordinateException.h
    InvalidLatLonException.cc
    InvalidLatLonException.h
    InvalidOperationException.h
    InvalidPolylineContainsOnlyOnePointException.h
    InvalidPolylineContainsZeroPointsException.h
    LatLonPoint.cc
    LatLonPoint.h
    MathematicalException.h
    MathsUtils.cc
    MathsUtils.h
    MultiPointOnSphere.cc
    MultiPointOnSphere.h
    MultiPointProximityHitDetail.h
    PointInPolygon.cc
    PointInPolygon.h
    PointLiesOnGreatCircleArc.cc
    PointLiesOnGreatCircleArc.h
    PointOnSphere.cc
    PointOnSphere.h
    PointProximityHitDetail.h
    PolygonFan.cc
    PolygonFan.h
    PolygonMesh.cc
    PolygonMesh.h
    PolygonOnSphere.cc
    PolygonOnSphere.h
    PolygonOrientation.cc
    PolygonOrientation.h
    PolygonPartitioner.cc
    PolygonPartitioner.h
    PolygonProximityHitDetail.h
    PolyGreatCircleArcBoundingTree.h
    PolylineEquivalencePredicates.h
    PolylineIntersections.cc
    PolylineIntersections.h
    PolylineOnSphere.cc
    PolylineOnSphere.h
    PolylineProximityHitDetail.h
    ProximityCriteria.cc
    ProximityCriteria.h
    ProximityHitDetail.cc
    ProximityHitDetail.h
    ProximityHitDetailVisitor.cc
    ProximityHitDetailVisitor.h
    Real.cc
    Real.h
    Rotation.cc
    Rotation.h
    ScribeExportMaths.h
    SmallCircle.cc
    SmallCircle.h
    SmallCircleArc.cc
    SmallCircleArc.h
    SmallCircleBounds.cc
    SmallCircleBounds.h
    SmallCircleCoverageMesh.cc
    SmallCircleCoverageMesh.h
    SmallCircleProximityHitDetail.h
    SphericalArea.cc
    SphericalArea.h
    SphericalSubdivision.cc
    SphericalSubdivision.h
    TrailingLatLonCoordinateException.cc
    TrailingLatLonCoordinateException.h
    types.h
    UnableToExtendPointlikeArcException.h
    UnableToIntersectEquivalentGreatCirclesException.h
    UnableToNormaliseZeroVectorException.h
    UnitQuaternion3D.cc
    UnitQuaternion3D.h
    UnitVector3D.cc
    UnitVector3D.h
    Vector3D.cc
    Vector3D.h
    ViolatedClassInvariantException.h
    ViolatedDirVectorInvariantException.h
    ViolatedSmallCircleInvariantException.h
    ViolatedUnitVectorInvariantException.h
)

# GPlates-only sources: excluded from the pygplates module, whose source list is the
# include closure of the pyGPlates API (see "cmake/pygplates_source_closure.py"; the
# pygplates-source-closure test fails if this list drifts from that closure).
# Keep ".h"/".cc" pairs together: AUTOMOC mocs any Q_OBJECT header listed in the
# module's sources, and a moc'ed header whose ".cc" is excluded fails at link time.
set(gplates_only_srcs
    CubeCoordinateFrame.cc
    CubeCoordinateFrame.h
    CubeQuadTree.h
    CubeQuadTreeLocation.cc
    CubeQuadTreeLocation.h
    CubeQuadTreePartition.h
    CubeQuadTreePartitionUtils.h
    EllipseGenerator.cc
    EllipseGenerator.h
    FiniteRotationSnapshotTable.h
    GeneratePoints.cc
    GeneratePoints.h
    GeometryCrossing.cc
    GeometryCrossing.h
    GeometryForwardDeclarations.h
    InvalidGreatCircleArcException.h
    InvalidGridException.h
    PolygonFan.cc
    PolygonFan.h
    PolygonMesh.cc
    PolygonMesh.h
    PolylineEquivalencePredicates.h
    SmallCircleArc.cc
    SmallCircleArc.h
    SmallCircleCoverageMesh.cc
    SmallCircleCoverageMesh.h
    SmallCircleProximityHitDetail.h
    SphericalSubdivision.cc
    SphericalSubdivision.h
    UnableToExtendPointlikeArcException.h
    UnableToIntersectEquivalentGreatCirclesException.h
    ViolatedDirVectorInvariantException.h
    ViolatedSmallCircleInvariantException.h
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
