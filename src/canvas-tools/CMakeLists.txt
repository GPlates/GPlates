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
    AdjustFittedPoleEstimate.cc
    AdjustFittedPoleEstimate.h
    BuildTopology.cc
    BuildTopology.h
    CanvasTool.h
    CanvasToolAdapterForGlobe.cc
    CanvasToolAdapterForGlobe.h
    CanvasToolAdapterForMap.cc
    CanvasToolAdapterForMap.h
    ChangeLightDirectionGlobe.cc
    ChangeLightDirectionGlobe.h
    ChangeLightDirectionMap.cc
    ChangeLightDirectionMap.h
    ClickGeometry.cc
    ClickGeometry.h
    CreateSmallCircle.cc
    CreateSmallCircle.h
    DeleteVertex.cc
    DeleteVertex.h
    DigitiseGeometry.cc
    DigitiseGeometry.h
    EditTopology.cc
    EditTopology.h
    GeometryOperationState.h
    InsertVertex.cc
    InsertVertex.h
    ManipulatePole.cc
    ManipulatePole.h
    MeasureDistance.cc
    MeasureDistance.h
    MeasureDistanceState.cc
    MeasureDistanceState.h
    ModifyGeometryState.h
    MovePoleGlobe.cc
    MovePoleGlobe.h
    MovePoleMap.cc
    MovePoleMap.h
    MoveVertex.cc
    MoveVertex.h
    PanMap.cc
    PanMap.h
    ReorientGlobe.cc
    ReorientGlobe.h
    SelectHellingerGeometries.cc
    SelectHellingerGeometries.h
    SplitFeature.cc
    SplitFeature.h
    ZoomGlobe.cc
    ZoomGlobe.h
    ZoomMap.cc
    ZoomMap.h
)

# Add the source files to the gplates static library (or pygplates extension module).
target_sources_util(${SOURCE_TARGET} PRIVATE ${srcs})
