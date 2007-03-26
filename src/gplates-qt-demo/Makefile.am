## Makefile.am -- Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES	= Makefile.in

EXTRA_DIST		= plates.clr.inc

INCLUDES		= -I$(top_srcdir)/src

noinst_LTLIBRARIES		= libgplates_qt_demo.la
libgplates_qt_demo_la_SOURCES	= Document.cc DocumentUi.h GlobeCanvas.cc \
					moc_GlobeCanvas.cc InformationDialog.cc \
					InformationDialogUi.h moc_Document.cc

CLEANFILES		= DocumentUi.h InformationDialogUi.h moc_*.cc

# This rule lets GNU make create any moc_*.cc from the equivalent *.h
moc_%.cc: %.h
	$(MOC) $< -o $@

# This rule lets GNU make create any *.h from the equivalent *.ui
%.h: %.ui
	$(UIC) $< -o $@
