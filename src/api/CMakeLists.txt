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
    PyFeatureCollectionFileFormatRegistry.cc
    PyFeatureCollectionFileFormatRegistry.h
    PyFeatureCollectionFunctionArgument.cc
    PyFeatureCollectionFunctionArgument.h
    PyFilePathFunctionArgument.cc
    PyFilePathFunctionArgument.h
    PyFiniteRotation.cc
    PyGeometriesOnSphere.cc
    PyGeometriesOnSphere.h
    PyGeoTimeInstant.cc
    PyGPlatesModule.cc
    PyGreatCircleArc.cc
    PyGreatCircleArc.h
    PyIds.cc
    PyInformationModel.cc
    PyInformationModel.h
    PyInteger.cc
    PyInterpolationException.h
    PyLatLonPoint.cc
    PyLocalCartesian.cc
    PyNetRotation.cc
    PyNetRotation.h
    PyNetworkTriangulation.cc
    PyNetworkTriangulation.h
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
    PyReconstructModel.cc
    PyReconstructModel.h
    PyReconstructionGeometries.cc
    PyReconstructionGeometries.h
    PyReconstructionTree.cc
    PyReconstructionTree.h
    PyReconstructSnapshot.cc
    PyReconstructSnapshot.h
    PyResolveTopologies.cc
    PyResolveTopologyParameters.h
    PyResolveTopologyParameters.cc
    PyRevisionedVector.cc
    PyRevisionedVector.h
    PyRotationModel.cc
    PyRotationModel.h
    PyStrain.cc
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
    PythonPickle.cc
    PythonPickle.h
    PythonRunner.cc
    PythonRunner.h
    PythonUtils.cc
    PythonUtils.h
    PythonVariableFunctionArguments.cc
    PythonVariableFunctionArguments.h
    PyTopologicalFeatureCollectionFunctionArgument.cc
    PyTopologicalFeatureCollectionFunctionArgument.h
    PyTopologicalModel.cc
    PyTopologicalModel.h
    PyTopologicalSnapshot.cc
    PyTopologicalSnapshot.h
    PyTopologyTools.cc
    PyVector3D.cc
    PyViewportWindow.cc
    Sleeper.cc
    Sleeper.h
)

# GPlates-only sources: excluded from the pygplates module, whose source list is the
# include closure of the pyGPlates API (see "cmake/pygplates_source_closure.py"; the
# pygplates-source-closure test fails if this list drifts from that closure).
# Keep ".h"/".cc" pairs together: AUTOMOC mocs any Q_OBJECT header listed in the
# module's sources, and a moc'ed header whose ".cc" is excluded fails at link time.
set(gplates_only_srcs
    AbstractConsole.h
    AbstractPythonRunner.h
    CoReg.cc
    ConsoleReader.cc
    ConsoleReader.h
    ConsoleWriter.cc
    ConsoleWriter.h
    DeferredApiCall.h
    DeferredApiCallImpl.h
    PyApplication.cc
    PyColour.cc
    PyCoregistrationLayerProxy.cc
    PyCoregistrationLayerProxy.h
    PyOldFeature.cc
    PyOldFeature.h
    PyOldFeatureCollection.cc
    PyOldFeatureCollection.h
    PyTopologyTools.cc
    PyViewportWindow.cc
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
    Sleeper.cc
    Sleeper.h
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
