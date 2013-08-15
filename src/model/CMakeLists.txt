# Current binary directory must be included first for g++ pre-compiled headers.
# There's a '_pch.h' file in current source and current binary directory however the '_pch.h.gch' pre-compiled header
# is only in current binary directory and it will only be used by the compiler if it's in the same directory as the found '_pch.h' file.
# So we need to ensure that we find the '_pch.h' file from the current binary directory first.
include_directories(${CMAKE_CURRENT_BINARY_DIR})
# Current source directory (directory that this 'CMakeLists.txt' file is in).
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# The variables used here are set in the parent 'CMakeList.txt' file.
include_directories(${SYSTEM_INCLUDE_FLAG} ${external_INCLUDE_DIRS})
include_directories(${internal_INCLUDE_DIRS})

set(srcs
    BasicHandle.cc
    BasicHandle.h
    BasicRevision.h
    ChangesetHandle.cc
    ChangesetHandle.h
    FeatureCollectionHandle.cc
    FeatureCollectionHandle.h
    FeatureCollectionRevision.cc
    FeatureCollectionRevision.h
    FeatureHandle.cc
    FeatureHandle.h
    FeatureHandleWeakRefBackInserter.h
    FeatureId.h
    FeatureRevision.cc
    FeatureRevision.h
    FeatureStoreRootHandle.cc
    FeatureStoreRootHandle.h
    FeatureStoreRootRevision.cc
    FeatureStoreRootRevision.h
    FeatureType.h
    FeatureVisitor.cc
    FeatureVisitor.h
    Gpgim.cc
    Gpgim.h
    GpgimEnumerationType.h
    GpgimFeatureClass.cc
    GpgimFeatureClass.h
    GpgimInitialisationException.cc
    GpgimInitialisationException.h
    GpgimProperty.cc
    GpgimProperty.h
    GpgimStructuralType.h
    GpgimVersion.cc
    GpgimVersion.h
    HandleTraits.h
    IdTypeGenerator.h
    Metadata.cc
    Metadata.h
    Model.cc
    Model.h
    ModelInterface.cc
    ModelInterface.h
    ModelTransaction.cc
    ModelTransaction.h
    ModelUtils.cc
    ModelUtils.h
    NotificationGuard.cc
    NotificationGuard.h
    PropertyName.h
    PropertyValue.cc
    PropertyValue.h
    PropertyValueBubbleUpRevisionHandler.cc
    PropertyValueBubbleUpRevisionHandler.h
    PropertyValueRevision.h
    PropertyValueRevisionContext.h
    PropertyValueRevisionedReference.cc
    PropertyValueRevisionedReference.h
    QualifiedXmlName.h
    RevisionAwareIterator.cc
    RevisionAwareIterator.h
    RevisionId.h
    StringContentTypeGenerator.h
    StringSetSingletons.cc
    StringSetSingletons.h
    TopLevelProperty.cc
    TopLevelProperty.h
    TopLevelPropertyInline.cc
    TopLevelPropertyInline.h
    TopLevelPropertyRef.cc
    TopLevelPropertyRef.h
    types.h
    WeakObserver.h
    WeakObserverPublisher.h
    WeakObserverVisitor.h
    WeakReference.h
    WeakReferenceCallback.h
    WeakReferenceVisitors.h
    XmlAttributeName.h
    XmlAttributeValue.h
    XmlElementName.h
    XmlNode.cc
    XmlNode.h
    XmlNodeUtils.cc
    XmlNodeUtils.h
)





set(target_name model)

# Does the following:
# - finds '.ui' files and adds commands to create 'Ui.h' files.
# - adds dependencies on generated header files.
# - scans '.h' files for Q_OBJECT and adds commands to run Qt's moc.exe to generate 'moc_*.cc' files.
# - finds '.qrc' files and adds commands to generate '.cc' files from them.
# - adds header files to Visual Studio projects.
# - groups files in Visual Studio IDE according to type.
GPLATES_GENERATE_TARGET_SOURCES(${target_name} target_srcs ${srcs})

add_library(${target_name} STATIC ${target_srcs})

GPLATES_POST_ADD_TARGET(${target_name} target_srcs)
