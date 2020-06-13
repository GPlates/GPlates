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
    Scribe.cc
    Scribe.h
    ScribeAccess.cc
    ScribeAccess.h
    ScribeArchiveCommon.cc
    ScribeArchiveCommon.h
    ScribeArchiveReader.h
    ScribeArchiveWriter.h
    ScribeBinaryArchiveReader.cc
    ScribeBinaryArchiveReader.h
    ScribeBinaryArchiveWriter.cc
    ScribeBinaryArchiveWriter.h
    ScribeConstructObject.h
    ScribeExceptions.cc
    ScribeExceptions.h
    ScribeExportExternal.h
    ScribeExportRegistration.h
    ScribeExportRegistry.cc
    ScribeExportRegistry.h
    ScribeInternalAccess.h
    ScribeInternalUtils.h
    ScribeInternalUtilsImpl.h
    ScribeLoadRef.h
    ScribeLoadRefImpl.h
    ScribeObjectTag.cc
    ScribeObjectTag.h
    ScribeOptions.h
    ScribeSaveLoadConstructObject.h
    ScribeTextArchiveReader.cc
    ScribeTextArchiveReader.h
    ScribeTextArchiveWriter.cc
    ScribeTextArchiveWriter.h
    ScribeVoidCastRegistry.cc
    ScribeVoidCastRegistry.h
    ScribeXmlArchiveReader.cc
    ScribeXmlArchiveReader.h
    ScribeXmlArchiveWriter.cc
    ScribeXmlArchiveWriter.h
    Transcribe.h
    TranscribeArray.h
    TranscribeBoost.h
    TranscribeContext.h
    TranscribeDelegateProtocol.h
    TranscribeEnumProtocol.cc
    TranscribeEnumProtocol.h
    TranscribeExternal.h
    TranscribeImpl.h
    TranscribeMappingProtocol.h
    TranscribeNonNullIntrusivePtr.h
    TranscribeQt.cc
    TranscribeQt.h
    TranscribeResult.h
    TranscribeSequenceProtocol.h
    TranscribeSmartPointerProtocol.h
    TranscribeStd.h
    TranscribeUtils.cc
    TranscribeUtils.h
    Transcription.cc
    Transcription.h
    TranscriptionScribeContext.cc
    TranscriptionScribeContext.h
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
