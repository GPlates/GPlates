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
    BuildTopology.cc
    BuildTopology.h
    CanvasTool.h
    CanvasToolAdapterForGlobe.cc
    CanvasToolAdapterForGlobe.h
    CanvasToolAdapterForMap.cc
    CanvasToolAdapterForMap.h
    CanvasToolType.h
    ClickGeometry.cc
    ClickGeometry.h
    DeleteVertex.cc
    DeleteVertex.h
    DigitiseGeometry.cc
    DigitiseGeometry.h
    EditTopology.cc
    EditTopology.h
    GlobeManipulatePole.cc
    GlobeManipulatePole.h
    InsertVertex.cc
    InsertVertex.h
    MapManipulatePole.cc
    MapManipulatePole.h
    MeasureDistance.cc
    MeasureDistance.h
    MeasureDistanceState.cc
    MeasureDistanceState.h
    MoveGeometry.cc
    MoveGeometry.h
    MoveVertex.cc
    MoveVertex.h
    PanMap.cc
    PanMap.h
    ReorientGlobe.cc
    ReorientGlobe.h
    ZoomGlobe.cc
    ZoomGlobe.h
    ZoomMap.cc
    ZoomMap.h
)





set(target_name canvas-tools)

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
