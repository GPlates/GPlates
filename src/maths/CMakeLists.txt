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
    PolygonIntersections.cc
    PolygonIntersections.h
    PolygonMesh.cc
    PolygonMesh.h
    PolygonOnSphere.cc
    PolygonOnSphere.h
    PolygonOrientation.cc
    PolygonOrientation.h
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

# Add the source files to the gplates static library.
target_sources_util(gplates-lib PRIVATE ${srcs})

# Add the source files to the pygplates extension module.
#
# Note this is the external Python library that is not embedded inside GPlates.
# All sources files compiled in pygplates need to be position independent and hence
# compiled differently (POSITION_INDEPENDENT_CODE target property is set on pygplates).
set(pygplates_srcs ${srcs})
target_sources_util(pygplates PRIVATE ${pygplates_srcs})
