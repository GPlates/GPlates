## Makefile.am -- Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_srcdir)/src

noinst_LTLIBRARIES	= libqt_widgets.la
libqt_widgets_la_SOURCES	= GlobeCanvas.cc moc_GlobeCanvas.cc \
	ViewportWindow.cc ViewportWindowUi.h moc_ViewportWindow.cc \
	SpecifyFixedPlateDialog.cc SpecifyFixedPlateDialogUi.h moc_SpecifyFixedPlateDialog.cc \
	AnimateDialog.cc AnimateDialogUi.h moc_AnimateDialog.cc \
	AboutDialog.cc AboutDialogUi.h moc_AboutDialog.cc \
	QueryFeaturePropertiesDialog.cc QueryFeaturePropertiesDialogUi.h \
	moc_QueryFeaturePropertiesDialog.cc \
	InformationDialog.cc InformationDialogUi.h moc_InformationDialog.cc \
	LicenseDialog.cc moc_LicenseDialog.cc \
	ReconstructionViewWidget.cc ReconstructionViewWidgetUi.h moc_ReconstructionViewWidget.cc \
	ReadErrorAccumulationDialog.cc ReadErrorAccumulationDialogUi.h moc_ReadErrorAccumulationDialog.cc  

CLEANFILES		= ViewportWindowUi.h InformationDialogUi.h \
				SpecifyFixedPlateDialogUi.h AnimateDialogUi.h AboutDialogUi.h \
				QueryFeaturePropertiesDialogUi.h ReconstructionViewWidgetUi.h \
				ReadErrorAccumulationDialogUi.h moc_*.cc

# This rule lets GNU make create any moc_*.cc from the equivalent *.h
moc_%.cc: %.h
	$(MOC) $< -o $@

# This rule lets GNU make create any *.h from the equivalent *.ui
%.h: %.ui
	$(UIC) $< -o $@
