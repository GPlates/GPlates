#
# List the non-generated source files (*.h, *.cc, *.ui, *.qrc).
#
# Add and remove source files here by hand, keeping the list in alphabetical order.
# (CMake discourages 'file(GLOB)': if source files are added or removed, CMake is not
# automatically re-run, so the build would be unaware of the change.)
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
    ScribeBool.cc
    ScribeBool.h
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
    TranscribeBoost.cc
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

# GPlates-only sources: excluded from the pygplates module, whose source list is the
# include closure of the pyGPlates API (see "cmake/pygplates_source_closure.py"; the
# pygplates-source-closure test fails if this list drifts from that closure).
# Keep ".h"/".cc" pairs together: AUTOMOC mocs any Q_OBJECT header listed in the
# module's sources, and a moc'ed header whose ".cc" is excluded fails at link time.
set(gplates_only_srcs
    ScribeTextArchiveReader.cc
    ScribeTextArchiveReader.h
    ScribeTextArchiveWriter.cc
    ScribeTextArchiveWriter.h
    ScribeXmlArchiveReader.cc
    ScribeXmlArchiveReader.h
    ScribeXmlArchiveWriter.cc
    ScribeXmlArchiveWriter.h
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
