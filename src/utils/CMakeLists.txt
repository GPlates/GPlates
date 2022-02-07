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
    XQueryUtils.cc
    XQueryUtils.h
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
