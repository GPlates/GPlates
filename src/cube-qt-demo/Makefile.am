## Makefile.am -- Process this file with automake to produce Makefile.in

INCLUDES		= -I$(top_srcdir)/src

noinst_LIBRARIES		= libcube_qt_demo.a
libcube_qt_demo_a_SOURCES	= CubeWidget.cc moc_CubeWidget.cc

CLEANFILES		= moc_*.cc

# This rule lets GNU make create any moc_*.cc from the equivalent *.h
moc_%.cc: %.h
	$(MOC) $< -o $@

# This rule lets GNU make create any *.h from the equivalent *.ui
%.h: %.ui
	$(UIC) $< -o $@
