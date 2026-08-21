#
# List the non-generated source files (*.h, *.cc, *.ui, *.qrc).
#
# Add and remove source files here by hand, keeping the list in alphabetical order.
# (CMake discourages 'file(GLOB)': if source files are added or removed, CMake is not
# automatically re-run, so the build would be unaware of the change.)
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

if (TARGET gplates-lib)
    # Add the source files to the gplates static library.
    # (The pygplates module compiles nothing from this directory - see
    # "cmake/pygplates_source_closure.py".)
    target_sources_util(gplates-lib PRIVATE ${srcs})
endif()
