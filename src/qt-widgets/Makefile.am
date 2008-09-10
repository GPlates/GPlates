## Makefile.am -- Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_srcdir)/src

noinst_LTLIBRARIES	= libqt_widgets.la
libqt_widgets_la_SOURCES	= 	AboutDialog.cc AboutDialogUi.h moc_AboutDialog.cc \
	AbstractEditWidget.cc moc_AbstractEditWidget.cc \
	ActionButtonBox.cc moc_ActionButtonBox.cc \
	AddPropertyDialog.cc AddPropertyDialogUi.h \
	moc_AddPropertyDialog.cc \
	AnimateDialog.cc AnimateDialogUi.h moc_AnimateDialog.cc \
	ApplicationState.cc \
	CreateFeatureDialog.cc CreateFeatureDialogUi.h moc_CreateFeatureDialog.cc \
	DigitisationWidget.cc DigitisationWidgetUi.h moc_DigitisationWidget.cc \
	EditAngleWidget.cc EditAngleWidgetUi.h \
	moc_EditAngleWidget.cc \
	EditBooleanWidget.cc EditBooleanWidgetUi.h \
	moc_EditBooleanWidget.cc \
	EditDoubleWidget.cc EditDoubleWidgetUi.h \
	moc_EditDoubleWidget.cc \
	EditEnumerationWidget.cc EditEnumerationWidgetUi.h \
	moc_EditEnumerationWidget.cc \
	ViewFeatureGeometriesWidget.cc ViewFeatureGeometriesWidgetUi.h \
	moc_ViewFeatureGeometriesWidget.cc \
	EditFeaturePropertiesWidget.cc EditFeaturePropertiesWidgetUi.h \
	moc_EditFeaturePropertiesWidget.cc \
	EditGeometryWidget.cc EditGeometryWidgetUi.h \
	moc_EditGeometryWidget.cc \
	EditGeometryActionWidget.cc EditGeometryActionWidgetUi.h \
	moc_EditGeometryActionWidget.cc \
	EditIntegerWidget.cc EditIntegerWidgetUi.h \
	moc_EditIntegerWidget.cc \
	EditOldPlatesHeaderWidget.cc EditOldPlatesHeaderWidgetUi.h \
	moc_EditOldPlatesHeaderWidget.cc \
	EditPlateIdWidget.cc EditPlateIdWidgetUi.h \
	moc_EditPlateIdWidget.cc \
	EditPolarityChronIdWidget.cc EditPolarityChronIdWidgetUi.h \
	moc_EditPolarityChronIdWidget.cc \
	EditStringWidget.cc EditStringWidgetUi.h \
	moc_EditStringWidget.cc \
	EditTimeInstantWidget.cc EditTimeInstantWidgetUi.h \
	moc_EditTimeInstantWidget.cc \
	EditTimePeriodWidget.cc EditTimePeriodWidgetUi.h \
	moc_EditTimePeriodWidget.cc \
	EditWidgetChooser.cc \
	EditWidgetGroupBox.cc moc_EditWidgetGroupBox.cc \
	EulerPoleDialog.cc EulerPoleDialogUi.h moc_EulerPoleDialog.cc \
	ExportCoordinatesDialog.cc ExportCoordinatesDialogUi.h moc_ExportCoordinatesDialog.cc \
	FeatureSummaryWidget.cc FeatureSummaryWidgetUi.h moc_FeatureSummaryWidget.cc \
	FeaturePropertiesDialog.cc FeaturePropertiesDialogUi.h \
	moc_FeaturePropertiesDialog.cc \
	GlobeCanvas.cc moc_GlobeCanvas.cc \
	InformationDialog.cc InformationDialogUi.h moc_InformationDialog.cc \
	LicenseDialog.cc moc_LicenseDialog.cc \
	ManageFeatureCollectionsActionWidget.cc ManageFeatureCollectionsActionWidgetUi.h \
	moc_ManageFeatureCollectionsActionWidget.cc \
	ManageFeatureCollectionsDialog.cc ManageFeatureCollectionsDialogUi.h \
	moc_ManageFeatureCollectionsDialog.cc \
	QueryFeaturePropertiesWidget.cc QueryFeaturePropertiesWidgetUi.h \
	moc_QueryFeaturePropertiesWidget.cc \
	ReadErrorAccumulationDialog.cc ReadErrorAccumulationDialogUi.h \
	moc_ReadErrorAccumulationDialog.cc \
	ReconstructionPoleWidget.cc ReconstructionPoleWidgetUi.h moc_ReconstructionPoleWidget.cc \
	ReconstructionViewWidget.cc ReconstructionViewWidgetUi.h \
	moc_ReconstructionViewWidget.cc \
	SetCameraViewpointDialog.cc SetCameraViewpointDialogUi.h \
	moc_SetCameraViewpointDialog.cc \
	ShapefileAttributeMapperDialog.cc ShapefileAttributeMapperDialogUi.h \
 	moc_ShapefileAttributeMapperDialog.cc \
	ShapefileAttributeRemapperDialog.cc ShapefileAttributeRemapperDialogUi.h \
	moc_ShapefileAttributeRemapperDialog.cc \
	ShapefileAttributeWidget.cc ShapefileAttributeWidgetUi.h \
	moc_ShapefileAttributeWidget.cc \
	ShapefilePropertyMapper.cc \
	SpecifyFixedPlateDialog.cc SpecifyFixedPlateDialogUi.h \
	moc_SpecifyFixedPlateDialog.cc \
	TaskPanel.cc TaskPanelUi.h moc_TaskPanel.cc \
	ViewportWindow.cc ViewportWindowUi.h moc_ViewportWindow.cc 


CLEANFILES		= ViewportWindowUi.h InformationDialogUi.h \
				SpecifyFixedPlateDialogUi.h AnimateDialogUi.h AboutDialogUi.h \
				QueryFeaturePropertiesWidgetUi.h ReconstructionPoleWidgetUi.h \
				ReconstructionViewWidgetUi.h \
				ReadErrorAccumulationDialogUi.h ManageFeatureCollectionsDialogUi.h \
				ManageFeatureCollectionsActionWidgetUi.h SetCameraViewpointDialogUi.h \
				CreateFeatureDialogUi.h DigitisationWidgetUi.h \
				EditFeaturePropertiesWidgetUi.h EditTimePeriodWidgetUi.h \
				EditOldPlatesHeaderWidgetUi.h AddPropertyDialogUi.h \
				EditStringWidgetUi.h EditEnumerationWidgetUi.h \
				EditTimeInstantWidgetUi.h EditPlateIdWidgetUi.h \
				EditGeometryWidgetUi.h EditGeometryActionWidgetUi.h \
				EditIntegerWidgetUi.h EditDoubleWidgetUi.h \
				EditBooleanWidgetUi.h EditPolarityChronIdWidgetUi.h \
				EditAngleWidgetUi.h \
				EulerPoleDialogUi.h \
				ExportCoordinatesDialogUi.h \
				FeatureSummaryWidgetUi.h \
				FeaturePropertiesDialogUi.h ViewFeatureGeometriesWidgetUi.h \
				ShapefileAttributeMapperDialogUi.h \
				ShapefileAttributeRemapperDialogUi.h \
				ShapefileAttributeWidgetUi.h \
				TaskPanelUi.h \
				moc_*.cc

# This rule lets GNU make create any moc_*.cc from the equivalent *.h
moc_%.cc: %.h
	$(MOC) $< -o $@

# This rule lets GNU make create any *.h from the equivalent *.ui
%.h: %.ui
	$(UIC) $< -o $@

