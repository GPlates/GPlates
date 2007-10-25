## Makefile.am -- Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES	= Makefile.in

EXTRA_DIST		= plates.clr.inc

INCLUDES		= -I$(top_srcdir)/src

noinst_LTLIBRARIES	= libgui.la
libgui_la_SOURCES	= CanvasTool.cc Colour.cc Globe.cc NurbsRenderer.cc \
				OpaqueSphere.cc PlatesColourTable.cc \
				ProximityTests.cc Quadrics.cc \
				SimpleGlobeOrientation.cc SphericalGrid.cc \
				ViewportZoom.cc
