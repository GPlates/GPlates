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
    CliAssignPlateIdsCommand.cc
    CliAssignPlateIdsCommand.h
    CliCommand.h
    CliCommandDispatcher.cc
    CliCommandDispatcher.h
    CliCommandRegistry.h
    CliConvertFileFormatCommand.cc
    CliConvertFileFormatCommand.h
    CliEquivalentTotalRotation.cc
    CliEquivalentTotalRotation.h
    CliFeatureCollectionFileIO.cc
    CliFeatureCollectionFileIO.h
    CliInvalidOptionValue.h
    CliReconstructCommand.cc
    CliReconstructCommand.h
    CliRelativeTotalRotation.cc
    CliRelativeTotalRotation.h
    CliRequiredOptionNotPresent.h
    CliStageRotationCommand.cc
    CliStageRotationCommand.h
)

# Add the source files to the gplates static library.
target_sources_util(gplates-lib PRIVATE ${srcs})
