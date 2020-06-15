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
    GpgimTemplateStructuralType.h
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
    ModelUtils.cc
    ModelUtils.h
    NotificationGuard.cc
    NotificationGuard.h
    PropertyName.h
    PropertyValue.cc
    PropertyValue.h
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
    TranscribeQualifiedXmlName.h
    TranscribeStringContentTypeGenerator.h
    types.h
    WeakObserver.h
    WeakObserverPublisher.h
    WeakObserverVisitor.cc
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

# Add the source files to the gplates static library.
target_sources_util(gplates-lib PRIVATE ${srcs})

# Add the source files to the pygplates extension module.
#
# Note this is the external Python library that is not embedded inside GPlates.
# All sources files compiled in pygplates need to be position independent and hence
# compiled differently (POSITION_INDEPENDENT_CODE target property is set on pygplates).
set(pygplates_srcs ${srcs})
target_sources_util(pygplates PRIVATE ${pygplates_srcs})
