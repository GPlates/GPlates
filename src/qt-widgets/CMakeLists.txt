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
    AboutDialog.cc
    AboutDialog.h
    AboutDialogUi.ui
    AbstractEditWidget.cc
    AbstractEditWidget.h
    ActionButtonBox.cc
    ActionButtonBox.h
    AddPropertyDialog.cc
    AddPropertyDialog.h
    AddPropertyDialogUi.ui
    AnimateDialog.cc
    AnimateDialog.h
    AnimateDialogUi.ui
    ApplicationState.cc
    ApplicationState.h
    ApplyReconstructionPoleAdjustmentDialog.cc
    ApplyReconstructionPoleAdjustmentDialog.h
    ApplyReconstructionPoleAdjustmentDialogUi.ui
    CreateFeatureDialog.cc
    CreateFeatureDialog.h
    CreateFeatureDialogUi.ui
    DigitisationUndoParadoxException.h
    DigitisationWidget.cc
    DigitisationWidget.h
    DigitisationWidgetUi.ui
    DigitisationWidgetUndoCommands.h
    EditAngleWidget.cc
    EditAngleWidget.h
    EditAngleWidgetUi.ui
    EditBooleanWidget.cc
    EditBooleanWidget.h
    EditBooleanWidgetUi.ui
    EditDoubleWidget.cc
    EditDoubleWidget.h
    EditDoubleWidgetUi.ui
    EditEnumerationWidget.cc
    EditEnumerationWidget.h
    EditEnumerationWidgetUi.ui
    EditFeaturePropertiesWidget.cc
    EditFeaturePropertiesWidget.h
    EditFeaturePropertiesWidgetUi.ui
    EditGeometryActionWidget.cc
    EditGeometryActionWidget.h
    EditGeometryActionWidgetUi.ui
    EditGeometryWidget.cc
    EditGeometryWidget.h
    EditGeometryWidgetUi.ui
    EditIntegerWidget.cc
    EditIntegerWidget.h
    EditIntegerWidgetUi.ui
    EditOldPlatesHeaderWidget.cc
    EditOldPlatesHeaderWidget.h
    EditOldPlatesHeaderWidgetUi.ui
    EditPlateIdWidget.cc
    EditPlateIdWidget.h
    EditPlateIdWidgetUi.ui
    EditPolarityChronIdWidget.cc
    EditPolarityChronIdWidget.h
    EditPolarityChronIdWidgetUi.ui
    EditShapefileAttributesWidgetUi.ui
    EditStringWidget.cc
    EditStringWidget.h
    EditStringWidgetUi.ui
    EditTimeInstantWidget.cc
    EditTimeInstantWidget.h
    EditTimeInstantWidgetUi.ui
    EditTimePeriodWidget.cc
    EditTimePeriodWidget.h
    EditTimePeriodWidgetUi.ui
    EditWidgetChooser.cc
    EditWidgetChooser.h
    EditWidgetGroupBox.cc
    EditWidgetGroupBox.h
    EulerPoleDialog.cc
    EulerPoleDialog.h
    EulerPoleDialogUi.ui
    ExportCoordinatesDialog.cc
    ExportCoordinatesDialog.h
    ExportCoordinatesDialogUi.ui
    FeaturePropertiesDialog.cc
    FeaturePropertiesDialog.h
    FeaturePropertiesDialogUi.ui
    FeatureSummaryWidget.cc
    FeatureSummaryWidget.h
    FeatureSummaryWidgetUi.ui
    GlobeCanvas.cc
    GlobeCanvas.h
    InformationDialog.cc
    InformationDialog.h
    InformationDialogUi.ui
    InvalidPropertyValueException.h
    LicenseDialog.cc
    LicenseDialog.h
    ManageFeatureCollectionsActionWidget.cc
    ManageFeatureCollectionsActionWidget.h
    ManageFeatureCollectionsActionWidgetUi.ui
    ManageFeatureCollectionsDialog.cc
    ManageFeatureCollectionsDialog.h
    ManageFeatureCollectionsDialogUi.ui
    NoActiveEditWidgetException.h
    PropertyValueNotSupportedException.h
	PlateClosureWidget.cc
	PlateClosureWidget.h
	PlateClosureWidgetUi.ui
    QueryFeaturePropertiesWidget.cc
    QueryFeaturePropertiesWidget.h
    QueryFeaturePropertiesWidgetUi.ui
    ReadErrorAccumulationDialog.cc
    ReadErrorAccumulationDialog.h
    ReadErrorAccumulationDialogUi.ui
    ReconstructionPoleWidget.cc
    ReconstructionPoleWidget.h
    ReconstructionPoleWidgetUi.ui
    ReconstructionViewWidget.cc
    ReconstructionViewWidget.h
    ReconstructionViewWidgetUi.ui
    SetCameraViewpointDialog.cc
    SetCameraViewpointDialog.h
    SetCameraViewpointDialogUi.ui
    ShapefileAttributeMapperDialog.cc
    ShapefileAttributeMapperDialog.h
    ShapefileAttributeMapperDialogUi.ui
    ShapefileAttributeRemapperDialog.cc
    ShapefileAttributeRemapperDialog.h
    ShapefileAttributeRemapperDialogUi.ui
    ShapefileAttributeWidget.cc
    ShapefileAttributeWidget.h
    ShapefileAttributeWidgetUi.ui
    ShapefilePropertyMapper.cc
    ShapefilePropertyMapper.h
    SpecifyFixedPlateDialog.cc
    SpecifyFixedPlateDialog.h
    SpecifyFixedPlateDialogUi.ui
    TaskPanel.cc
    TaskPanel.h
    TaskPanelUi.ui
    UninitialisedEditWidgetException.h
    ViewFeatureGeometriesWidget.cc
    ViewFeatureGeometriesWidget.h
    ViewFeatureGeometriesWidgetUi.ui
    ViewportWindow.cc
    ViewportWindow.h
    ViewportWindowUi.ui
)





set(target_name qt-widgets)

# Does the following:
# - finds '.ui' files and adds commands to create 'Ui.h' files.
# - adds dependencies on generated header files.
# - scans '.h' files for Q_OBJECT and adds commands to run Qt's moc.exe to generate 'moc_*.cc' files.
# - finds '.qrc' files and adds commands to generate '.cc' files from them.
# - adds header files to Visual Studio projects.
# - groups files in Visual Studio IDE according to type.
GPLATES_GENERATE_TARGET_SOURCES(${target_name} target_srcs ${srcs})

add_library(${target_name} ${target_srcs})

GPLATES_POST_ADD_TARGET(${target_name} target_srcs)
