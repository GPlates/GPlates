## Makefile.am -- Process this file with automake to produce Makefile.in

INCLUDES		= -I$(top_srcdir)/src

noinst_LIBRARIES	= libgui_qt.a
libgui_qt_a_SOURCES	= moc_GlCanvas.cc

# This rule lets GNU make create any moc_*.cc from the equivalent *.h
moc_%.cc: %.h
	$(MOC) $< -o $@
