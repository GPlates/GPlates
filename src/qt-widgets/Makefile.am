## Makefile.am -- Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_srcdir)/src

noinst_LTLIBRARIES	= libqt_widgets.la
libqt_widgets_la_SOURCES	= GlobeCanvas.cc moc_GlobeCanvas.cc \
	ViewportWindow.cc ViewportWindowUi.h moc_ViewportWindow.cc \
	SpecifyFixedPlateDialog.cc SpecifyFixedPlateDialogUi.h moc_SpecifyFixedPlateDialog.cc \
	AnimateDialog.cc AnimateDialogUi.h moc_AnimateDialog.cc \
	AboutDialog.cc AboutDialogUi.h moc_AboutDialog.cc \
	QueryFeaturePropertiesWidget.cc QueryFeaturePropertiesWidgetUi.h \
	moc_QueryFeaturePropertiesWidget.cc \
	InformationDialog.cc InformationDialogUi.h moc_InformationDialog.cc \
	LicenseDialog.cc moc_LicenseDialog.cc \
	ReconstructionViewWidget.cc ReconstructionViewWidgetUi.h moc_ReconstructionViewWidget.cc \
	ReadErrorAccumulationDialog.cc ReadErrorAccumulationDialogUi.h moc_ReadErrorAccumulationDialog.cc \
	ManageFeatureCollectionsDialog.cc ManageFeatureCollectionsDialogUi.h \
	moc_ManageFeatureCollectionsDialog.cc \
	ManageFeatureCollectionsActionWidget.cc ManageFeatureCollectionsActionWidgetUi.h \
	moc_ManageFeatureCollectionsActionWidget.cc \
	ApplicationState.cc \
	SetCameraViewpointDialog.cc SetCameraViewpointDialogUi.h \
	moc_SetCameraViewpointDialog.cc \
	EulerPoleDialog.cc EulerPoleDialogUi.h moc_EulerPoleDialog.cc \
	EditFeaturePropertiesWidget.cc EditFeaturePropertiesWidgetUi.h \
	moc_EditFeaturePropertiesWidget.cc \
	EditTimePeriodWidget.cc EditTimePeriodWidgetUi.h \
	moc_EditTimePeriodWidget.cc \
	EditWidgetGroupBox.cc moc_EditWidgetGroupBox.cc \
	EditWidgetChooser.cc \
	EditOldPlatesHeaderWidget.cc EditOldPlatesHeaderWidgetUi.h \
	moc_EditOldPlatesHeaderWidget.cc \
	AddPropertyDialog.cc AddPropertyDialogUi.h \
	moc_AddPropertyDialog.cc \
	AbstractEditWidget.cc moc_AbstractEditWidget.cc \
	EditStringWidget.cc EditStringWidgetUi.h \
	moc_EditStringWidget.cc \
	EditEnumerationWidget.cc EditEnumerationWidgetUi.h \
	moc_EditEnumerationWidget.cc \
	EditTimeInstantWidget.cc EditTimeInstantWidgetUi.h \
	moc_EditTimeInstantWidget.cc \
	EditPlateIdWidget.cc EditPlateIdWidgetUi.h \
	moc_EditPlateIdWidget.cc \
	EditIntegerWidget.cc EditIntegerWidgetUi.h \
	moc_EditIntegerWidget.cc \
	EditDoubleWidget.cc EditDoubleWidgetUi.h \
	moc_EditDoubleWidget.cc \
	EditBooleanWidget.cc EditBooleanWidgetUi.h \
	moc_EditBooleanWidget.cc \
	EditPolarityChronIdWidget.cc EditPolarityChronIdWidgetUi.h \
	moc_EditPolarityChronIdWidget.cc \
	EditAngleWidget.cc EditAngleWidgetUi.h \
	moc_EditAngleWidget.cc \
	FeaturePropertiesDialog.cc FeaturePropertiesDialogUi.h \
	moc_FeaturePropertiesDialog.cc \
	EditFeatureGeometriesWidget.cc EditFeatureGeometriesWidgetUi.h \
	moc_EditFeatureGeometriesWidget.cc



CLEANFILES		= ViewportWindowUi.h InformationDialogUi.h \
				SpecifyFixedPlateDialogUi.h AnimateDialogUi.h AboutDialogUi.h \
				QueryFeaturePropertiesWidgetUi.h ReconstructionViewWidgetUi.h \
				ReadErrorAccumulationDialogUi.h ManageFeatureCollectionsDialogUi.h \
				ManageFeatureCollectionsActionWidgetUi.h SetCameraViewpointDialogUi.h \
				EulerPoleDialogUi.h \
				EditFeaturePropertiesWidgetUi.h EditTimePeriodWidgetUi.h \
				EditOldPlatesHeaderWidgetUi.h AddPropertyDialogUi.h \
				EditStringWidgetUi.h EditEnumerationWidgetUi.h \
				EditTimeInstantWidgetUi.h EditPlateIdWidgetUi.h \
				EditIntegerWidgetUi.h EditDoubleWidgetUi.h \
				EditBooleanWidgetUi.h EditPolarityChronIdWidgetUi.h \
				EditAngleWidgetUi.h \
				FeaturePropertiesDialogUi.h EditFeatureGeometriesWidgetUi.h \
				moc_*.cc

# This rule lets GNU make create any moc_*.cc from the equivalent *.h
moc_%.cc: %.h
	$(MOC) $< -o $@

# This rule lets GNU make create any *.h from the equivalent *.ui
%.h: %.ui
	$(UIC) $< -o $@
