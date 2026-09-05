#
# List the non-generated source files (*.h, *.cc, *.ui, *.qrc).
#
# Add and remove source files here by hand, keeping the list in alphabetical order.
# (CMake discourages 'file(GLOB)': if source files are added or removed, CMake is not
# automatically re-run, so the build would be unaware of the change.)
#
set(srcs
    AnimationSequenceUtils.cc
    AnimationSequenceUtils.h
    Base2Utils.cc
    Base2Utils.h
    CallStackTracker.cc
    CallStackTracker.h
    CommandLineParser.cc
    CommandLineParser.h
    ComponentManager.h
    ConfigBundle.cc
    ConfigBundle.h
    ConfigBundleUtils.cc
    ConfigBundleUtils.h
    ConfigInterface.h
    CopyConst.h
    CopyOnWrite.h
    Counter64.h
    DeferredCallEvent.cc
    DeferredCallEvent.h
    Earth.cc
    Earth.h
    Endian.h
    Environment.cc
    Environment.h
    FeatureUtils.cc
    FeatureUtils.h
    FunctionTypes.h
    GeometryCreationUtils.h
    GetPropertyAsPythonObjVisitor.cc
    GetPropertyAsPythonObjVisitor.h
    HasFunction.h
    IdStringSet.cc
    IdStringSet.h
    IntrusiveSinglyLinkedList.h
    KeyValueCache.h
    LatLonAreaSampling.h
    Mapper.h
    NetworkUtils.cc
    NetworkUtils.h
    non_null_intrusive_ptr.h
    NullIntrusivePointerHandler.cc
    NullIntrusivePointerHandler.h
    NullNonNullIntrusivePointerException.h
    ObjectCache.h
    ObjectPool.h
    OverloadResolution.h
    Parse.h
    Profile.cc
    Profile.h
    QtFormattingUtils.h
    QtStreamable.h
    Reducer.h
    ReferenceCount.h
    SafeBool.h
    Select.h
    SetConst.h
    Singleton.h
    SmartNodeLinkedList.h
    StringFormattingUtils.cc
    StringFormattingUtils.h
    StringSet.cc
    StringSet.h
    StringUtils.cc
    StringUtils.h
    SubjectObserverToken.h
    TypeTraits.h
    UnicodeString.cc
    UnicodeString.h
    UnicodeStringUtils.h
    UniqueId.cc
    UniqueId.h
    VirtualProxy.h
    XmlNamespaces.cc
    XmlNamespaces.h
    XPath.cc
    XPath.h
)

# GPlates-only sources: excluded from the pygplates module, whose source list is the
# include closure of the pyGPlates API (see "cmake/pygplates_source_closure.py"; the
# pygplates-source-closure test fails if this list drifts from that closure).
# Keep ".h"/".cc" pairs together: AUTOMOC mocs any Q_OBJECT header listed in the
# module's sources, and a moc'ed header whose ".cc" is excluded fails at link time.
set(gplates_only_srcs
    AnimationSequenceUtils.cc
    AnimationSequenceUtils.h
    CommandLineParser.cc
    CommandLineParser.h
    ComponentManager.h
    ConfigBundle.cc
    ConfigBundle.h
    ConfigBundleUtils.cc
    ConfigBundleUtils.h
    ConfigInterface.h
    CopyOnWrite.h
    DeferredCallEvent.cc
    DeferredCallEvent.h
    Environment.cc
    Environment.h
    FeatureUtils.cc
    FeatureUtils.h
    FunctionTypes.h
    GetPropertyAsPythonObjVisitor.cc
    GetPropertyAsPythonObjVisitor.h
    LatLonAreaSampling.h
    Mapper.h
    NetworkUtils.cc
    NetworkUtils.h
    ObjectCache.h
    QtFormattingUtils.h
    Reducer.h
    SubjectObserverToken.h
    VirtualProxy.h
    XPath.cc
    XPath.h
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
