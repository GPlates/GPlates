#
# List the non-generated source files (*.h, *.cc, *.ui, *.qrc).
#
# Add and remove source files here by hand, keeping the list in alphabetical order.
# (CMake discourages 'file(GLOB)': if source files are added or removed, CMake is not
# automatically re-run, so the build would be unaware of the change.)
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

if (TARGET gplates-lib)
    # Add the source files to the gplates static library.
    target_sources_util(gplates-lib PRIVATE ${srcs})
endif()
