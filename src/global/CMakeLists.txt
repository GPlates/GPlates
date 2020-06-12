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

# Add the source files to the gplates static library.
target_sources_util(gplates-lib PRIVATE ${srcs})

# Add the source files to the pygplates extension module.
#
# Note this is the external Python library that is not embedded inside GPlates.
# All sources files compiled in pygplates need to be position independent and hence
# compiled differently (POSITION_INDEPENDENT_CODE target property is set on pygplates).
set(pygplates_srcs ${srcs})
target_sources_util(pygplates PRIVATE ${pygplates_srcs})
