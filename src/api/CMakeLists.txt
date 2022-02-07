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
    APIVersion.cc
    APIVersion.h
    ConsoleReader.cc
    ConsoleReader.h
    ConsoleWriter.cc
    ConsoleWriter.h
    CoReg.cc
    DeferredApiCall.h
    DeferredApiCallImpl.h
    PyApplication.cc
    PyCalculateVelocities.cc
    PyColour.cc
    PyCoregistrationLayerProxy.cc
    PyCoregistrationLayerProxy.h
    PyDateLineWrapper.cc
    PyEarth.cc
    PyExceptions.cc
    PyExceptions.h
    PyFeature.cc
    PyFeature.h
    PyFeatureCollection.cc
    PyFeatureCollection.h
    PyFeatureCollectionFileFormatRegistry.cc
    PyFeatureCollectionFileFormatRegistry.h
    PyFiniteRotation.cc
    PyGeometriesOnSphere.cc
    PyGeometriesOnSphere.h
    PyGeoTimeInstant.cc
    PyGPlatesModule.cc
    PyGPlatesModule.h
    PyGreatCircleArc.cc
    PyGreatCircleArc.h
    PyIds.cc
    PyInformationModel.cc
    PyInformationModel.h
    PyInteger.cc
    PyInterpolationException.h
    PyLatLonPoint.cc
    PyLocalCartesian.cc
    PyOldFeature.cc
    PyOldFeature.h
    PyOldFeatureCollection.cc
    PyOldFeatureCollection.h
    PyPlatePartitioner.cc
    PyPlatePartitioner.h
    PyProperty.cc
    PyPropertyValues.cc
    PyPropertyValues.h
    PyPropertyValueVisitor.cc
    PyPurePython.cc
    PyQualifiedXmlNames.cc
    PyQualifiedXmlNames.h
    PyReal.cc
    PyReconstruct.cc
    PyReconstructionGeometries.cc
    PyReconstructionGeometries.h
    PyReconstructionTree.cc
    PyReconstructionTree.h
    PyResolveTopologies.cc
    PyRevisionedVector.cc
    PyRevisionedVector.h
    PyRotationModel.cc
    PyRotationModel.h
    PyStrings.cc
    PythonConverterUtils.h
    PythonExecutionMonitor.cc
    PythonExecutionMonitor.h
    PythonExecutionThread.cc
    PythonExecutionThread.h
    PythonExtractUtils.cc
    PythonExtractUtils.h
    PythonHashDefVisitor.h
    PythonInterpreterLocker.cc
    PythonInterpreterLocker.h
    PythonInterpreterUnlocker.cc
    PythonInterpreterUnlocker.h
    PythonRunner.cc
    PythonRunner.h
    PythonUtils.cc
    PythonUtils.h
    PythonVariableFunctionArguments.cc
    PythonVariableFunctionArguments.h
    PyTopologyTools.cc
    PyVector3D.cc
    PyViewportWindow.cc
    Sleeper.cc
    Sleeper.h
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
