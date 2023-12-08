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
    PyFeature.cc
    PyFeature.h
    PyFeatureCollection.cc
    PyFeatureCollection.h
    PyFunctions.cc
    Python.cc
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
    PyViewportWindow.cc
    Sleeper.cc
    Sleeper.h
)

# Add the source files to the gplates static library (or pygplates extension module).
target_sources_util(${SOURCE_TARGET} PRIVATE ${srcs})
