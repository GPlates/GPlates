#
# List the non-generated source files (*.h, *.cc, *.ui, *.qrc).
#
# Add and remove source files here by hand, keeping the list in alphabetical order.
# (CMake discourages 'file(GLOB)': if source files are added or removed, CMake is not
# automatically re-run, so the build would be unaware of the change.)
#
set(srcs
    AbortException.cc
    AbortException.h
    AssertionFailureException.cc
    AssertionFailureException.h
    CompilerWarnings.h
    ControlFlowException.h
    ExternalResourceFailureException.h
    GdalVersion.h
    GPlatesAssert.cc
    GPlatesAssert.h
    GPlatesException.cc
    GPlatesException.h
    IllegalParametersException.h
    InternalInconsistencyException.cc
    InternalInconsistencyException.h
    InternalObjectInconsistencyException.h
    IntrusivePointerZeroRefCountException.h
    InvalidFeatureCollectionException.h
    InvalidParametersException.h
    License.h
    LogException.h
    NotYetImplementedException.h
    NullParameterException.h
    PointerTraits.h
    PreconditionViolationError.h
    python.h
    RetrievalFromEmptyContainerException.h
    UnexpectedEmptyFeatureCollectionException.h
    unicode.h
    UninitialisedIteratorException.h
    UnsupportedFunctionException.h
    Version.h
)

# GPlates-only sources: excluded from the pygplates module, whose source list is the
# include closure of the pyGPlates API (see "cmake/pygplates_source_closure.py"; the
# pygplates-source-closure test fails if this list drifts from that closure).
# Keep ".h"/".cc" pairs together: AUTOMOC mocs any Q_OBJECT header listed in the
# module's sources, and a moc'ed header whose ".cc" is excluded fails at link time.
set(gplates_only_srcs
    ControlFlowException.h
    IllegalParametersException.h
    InternalInconsistencyException.cc
    InternalInconsistencyException.h
    License.h
    NullParameterException.h
    UnexpectedEmptyFeatureCollectionException.h
    UnsupportedFunctionException.h
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
