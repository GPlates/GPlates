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
    AbstractConsole.h
    AbstractPythonRunner.h
    ConsoleReader.cc
    ConsoleReader.h
    ConsoleWriter.cc
    ConsoleWriter.h
    CoReg.cc
    DeferredApiCall.h
    DeferredApiCallImpl.h
    PyApplication.cc
    PyColour.cc
    PyCoregistrationLayerProxy.cc
    PyCoregistrationLayerProxy.h
    PyExceptions.cc
    PyFeature.cc
    PyFeatureCollection.cc
    PyFeatureCollectionFileFormatRegistry.cc
    PyFiniteRotation.cc
    PyFunctions.cc
    PyGeometriesOnSphere.cc
    PyGreatCircleArc.cc
    PyIds.cc
    PyLatLonPoint.cc
    PyOldFeature.cc
    PyOldFeature.h
    PyOldFeatureCollection.cc
    PyOldFeatureCollection.h
    PyProperty.cc
    PyPropertyValues.cc
    PyPropertyValueVisitor.cc
    PyPurePython.cc
    PyQualifiedXmlNames.cc
    PyReal.cc
    PyReconstructionTree.cc
    PyRevisionedVector.cc
    PyStrings.cc
    Python.cc
    PythonConverterUtils.cc
    PythonConverterUtils.h
    PythonExecutionMonitor.cc
    PythonExecutionMonitor.h
    PythonExecutionThread.cc
    PythonExecutionThread.h
    PythonInterpreterLocker.cc
    PythonInterpreterLocker.h
    PythonInterpreterUnlocker.cc
    PythonInterpreterUnlocker.h
    PythonRunner.cc
    PythonRunner.h
    PythonUtils.cc
    PythonUtils.h
    PyTopologyTools.cc
    PyUnitVector3D.cc
    PyViewportWindow.cc
    Sleeper.cc
    Sleeper.h
)


set(target_name api)

# Does the following:
# - adds dependencies on generated header files.
# - finds '.ui' files and adds commands to create 'Ui.h' files.
# - scans '.h' files for Q_OBJECT and adds commands to run Qt's moc.exe to generate 'moc_*.cc' files.
# - finds '.qrc' files and adds commands to generate '.cc' files from them.
# - adds header files to Visual Studio projects.
# - groups files in Visual Studio IDE according to type.
GPLATES_GENERATE_TARGET_SOURCES(${target_name} target_srcs ${srcs})

add_library(${target_name} STATIC ${target_srcs})
set_target_properties(${target_name} PROPERTIES COMPILE_FLAGS "-DGPLATES_PYTHON_EMBEDDING")
GPLATES_POST_ADD_TARGET(${target_name} target_srcs)
