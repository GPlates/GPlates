## Makefile.am -- Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_srcdir)/src

noinst_LTLIBRARIES	= libqt_resources.la
libqt_resources_la_SOURCES	= qrc_qt_widgets.cc

# This rule lets GNU make create any qrc_*.cc from the equivalent *.qrc
qrc_%.cc: %.qrc
	$(RCC) $< -o $@

