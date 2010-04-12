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
    AnimateControlWidget.cc
    AnimateControlWidget.h
    AnimateControlWidgetUi.ui
    AnimateDialog.cc
    AnimateDialog.h
    AnimateDialogUi.ui
    ApplyReconstructionPoleAdjustmentDialog.cc
    ApplyReconstructionPoleAdjustmentDialog.h
    ApplyReconstructionPoleAdjustmentDialogUi.ui
    AssignReconstructionPlateIdsDialog.cc
    AssignReconstructionPlateIdsDialog.h
    AssignReconstructionPlateIdsDialogUi.ui
    BuildTopologyWidgetUi.ui
    ColouringDialog.cc
    ColouringDialog.h
    ColouringDialogUi.ui
    ConfigureExportParametersDialog.cc
    ConfigureExportParametersDialog.h
    ConfigureExportParametersDialogUi.ui
    CreateFeatureDialog.cc
    CreateFeatureDialog.h
    CreateFeatureDialogUi.ui
    DigitisationWidget.cc
    DigitisationWidget.h
    DigitisationWidgetUi.ui
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
    EditShapefileAttributesWidget.cc
    EditShapefileAttributesWidget.h
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
    EditTopologyWidgetUi.ui
    EditWidgetChooser.cc
    EditWidgetChooser.h
    EditWidgetGroupBox.cc
    EditWidgetGroupBox.h
    ExportAnimationDialog.cc
    ExportAnimationDialog.h
    ExportAnimationDialogUi.ui
    ExportCoordinatesDialog.cc
    ExportCoordinatesDialog.h
    ExportCoordinatesDialogUi.ui
    ExportReconstructedFeatureGeometryDialog.cc
    ExportReconstructedFeatureGeometryDialog.h
    FeaturePropertiesDialog.cc
    FeaturePropertiesDialog.h
    FeaturePropertiesDialogUi.ui
    FeatureSummaryWidget.cc
    FeatureSummaryWidget.h
    FeatureSummaryWidgetUi.ui
    GlobeAndMapWidget.cc
    GlobeAndMapWidget.h
    GlobeCanvas.cc
    GlobeCanvas.h
    GMenuButton.cc
    GMenuButton.h
    GMenuButtonUi.ui
    GMTHeaderFormatDialog.cc
    GMTHeaderFormatDialog.h
    GMTHeaderFormatDialogUi.ui
    InformationDialog.cc
    InformationDialog.h
    InformationDialogUi.ui
    InsertionPointWidget.h
    InsertionPointWidgetUi.ui
    InvalidPropertyValueException.h
    LatLonCoordinatesTable.cc
    LatLonCoordinatesTable.h
    LeaveFullScreenButton.h
    LeaveFullScreenButtonUi.ui
    LicenseDialog.cc
    LicenseDialog.h
    ManageFeatureCollectionsActionWidget.cc
    ManageFeatureCollectionsActionWidget.h
    ManageFeatureCollectionsActionWidgetUi.ui
    ManageFeatureCollectionsDialog.cc
    ManageFeatureCollectionsDialog.h
    ManageFeatureCollectionsDialogUi.ui
    ManageFeatureCollectionsStateWidget.cc
    ManageFeatureCollectionsStateWidget.h
    ManageFeatureCollectionsStateWidgetUi.ui
    MapCanvas.cc
    MapCanvas.h
    MapView.cc
    MapView.h
    MeasureDistanceWidget.cc
    MeasureDistanceWidget.h
    MeasureDistanceWidgetUi.ui
    MeshDialog.cc
    MeshDialog.h
    MeshDialogUi.ui
    ModifyGeometryWidget.cc
    ModifyGeometryWidget.h
    ModifyGeometryWidgetUi.ui
    NoActiveEditWidgetException.h
    PaletteSelectionWidget.cc
    PaletteSelectionWidget.h
    ProgressDialog.h
    ProgressDialogUi.ui
    ProjectionControlWidget.cc
    ProjectionControlWidget.h
    ProjectionControlWidgetUi.ui
    PropertyValueNotSupportedException.h
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
    SaveFileDialog.cc
    SaveFileDialog.h
    SceneView.h
    SetCameraViewpointDialog.cc
    SetCameraViewpointDialog.h
    SetCameraViewpointDialogUi.ui
    SetProjectionDialog.cc
    SetProjectionDialog.h
    SetProjectionDialogUi.ui
    SetRasterSurfaceExtentDialog.cc
    SetRasterSurfaceExtentDialog.h
    SetRasterSurfaceExtentDialogUi.ui
    ShapefileAttributeMapperDialog.cc
    ShapefileAttributeMapperDialog.h
    ShapefileAttributeMapperDialogUi.ui
    ShapefileAttributeRemapperDialog.cc
    ShapefileAttributeRemapperDialog.h
    ShapefileAttributeRemapperDialogUi.ui
    ShapefileAttributeViewerDialog.cc
    ShapefileAttributeViewerDialog.h
    ShapefileAttributeViewerDialogUi.ui
    ShapefileAttributeWidget.cc
    ShapefileAttributeWidget.h
    ShapefileAttributeWidgetUi.ui
    ShapefilePropertyMapper.cc
    ShapefilePropertyMapper.h
    SpecifyAnchoredPlateIdDialog.cc
    SpecifyAnchoredPlateIdDialog.h
    SpecifyAnchoredPlateIdDialogUi.ui
    SpecifyTimeIncrementDialog.cc
    SpecifyTimeIncrementDialog.h
    SpecifyTimeIncrementDialogUi.ui
    TaskPanel.cc
    TaskPanel.h
    TimeControlWidget.cc
    TimeControlWidget.h
    TimeControlWidgetUi.ui
    TopologyToolsWidget.cc
    TopologyToolsWidget.h
    TopologyToolsWidgetUi.ui
    TotalReconstructionPolesDialog.cc
    TotalReconstructionPolesDialog.h
    TotalReconstructionPolesDialogUi.ui
    UninitialisedEditWidgetException.h
    ViewFeatureGeometriesWidget.cc
    ViewFeatureGeometriesWidget.h
    ViewFeatureGeometriesWidgetUi.ui
    ViewportWindow.cc
    ViewportWindow.h
    ViewportWindowUi.ui
    ZoomControlWidget.cc
    ZoomControlWidget.h
    ZoomControlWidgetUi.ui
    ZoomSliderWidget.cc
    ZoomSliderWidget.h
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

add_library(${target_name} STATIC ${target_srcs})

GPLATES_POST_ADD_TARGET(${target_name} target_srcs)
