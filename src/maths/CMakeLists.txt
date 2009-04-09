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
    CalculateVelocityOfPoint.cc
    CalculateVelocityOfPoint.h
    CartesianConvMatrix3D.cc
    CartesianConvMatrix3D.h
    ConstGeometryOnSphereVisitor.h
    FiniteRotation.cc
    FiniteRotation.h
    FiniteRotationSnapshotTable.h
    FunctionDomainException.h
    GenericVectorOps3D.h
    GeometryForwardDeclarations.h
    GeometryOnSphere.h
    GreatCircle.cc
    GreatCircle.h
    GreatCircleArc.cc
    GreatCircleArc.h
    GridOnSphere.cc
    GridOnSphere.h
    HighPrecision.h
    IndeterminateArcRotationAxisException.h
    IndeterminateResultException.h
    InvalidGreatCircleArcException.h
    InvalidGridException.h
    InvalidLatLonCoordinateException.h
    InvalidLatLonException.h
    InvalidOperationException.h
    InvalidPolylineContainsOnlyOnePointException.h
    InvalidPolylineContainsZeroPointsException.h
    LatLonPointConversions.cc
    LatLonPointConversions.h
    MathematicalException.h
    MultiPointOnSphere.cc
    MultiPointOnSphere.h
    MultiPointProximityHitDetail.h
    PointLiesOnGreatCircleArc.cc
    PointLiesOnGreatCircleArc.h
    PointOnSphere.cc
    PointOnSphere.h
    PointProximityHitDetail.h
    PolygonOnSphere.cc
    PolygonOnSphere.h
    PolygonProximityHitDetail.h
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
    SphericalCollection.h
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

add_library(${target_name} ${target_srcs})

GPLATES_POST_ADD_TARGET(${target_name} target_srcs)
