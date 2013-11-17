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
    AngularDistance.cc
    AngularDistance.h
    AngularExtent.cc
    AngularExtent.h
    CalculateVelocity.cc
    CalculateVelocity.h
    CartesianConvMatrix3D.cc
    CartesianConvMatrix3D.h
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
    FastMaths.cc
    FastMaths.h
    FiniteRotation.cc
    FiniteRotation.h
    FiniteRotationSnapshotTable.h
    FunctionDomainException.h
    GenericVectorOps3D.h
    GeometryDistance.cc
    GeometryDistance.h
    GeometryForwardDeclarations.h
    GeometryOnSphere.h
    GeometryType.h
    GreatCircle.cc
    GreatCircle.h
    GreatCircleArc.cc
    GreatCircleArc.h
    HierarchicalTriangularMesh.h
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
    MathsFwd.h
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
    ProjectionUtils.cc
    ProjectionUtils.h
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
    SphereSettings.h
    SphericalArea.cc
    SphericalArea.h
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





set(target_name maths)

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
