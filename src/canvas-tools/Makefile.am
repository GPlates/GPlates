## Makefile.am -- Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_srcdir)/src

noinst_LTLIBRARIES	= libcanvas_tools.la
libcanvas_tools_la_SOURCES	= ClickGeometry.cc moc_ClickGeometry.cc \
			ZoomGlobe.cc \
			ReorientGlobe.cc \
			DigitiseGeometry.cc

CLEANFILES		= moc_*.cc

# This rule lets GNU make create any moc_*.cc from the equivalent *.h
moc_%.cc: %.h
	$(MOC) $< -o $@
