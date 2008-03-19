## Makefile.am -- Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES	= Makefile.in

EXTRA_DIST		= plates.clr.inc

INCLUDES		= -I$(top_srcdir)/src

noinst_LTLIBRARIES	= libgui.la
libgui_la_SOURCES	= CanvasTool.cc CanvasToolAdapter.cc \
				CanvasToolChoice.cc Colour.cc Globe.cc \
				NurbsRenderer.cc OpaqueSphere.cc \
				PlatesColourTable.cc ProximityTests.cc \
				Quadrics.cc SimpleGlobeOrientation.cc \
				SphericalGrid.cc SvgExport.cc \
				ViewportZoom.cc \
				FeatureTableModel.cc \
				CsvExport.cc \
				moc_CanvasToolAdapter.cc \
				moc_CanvasToolChoice.cc \
				moc_FeatureWeakRefSequence.cc \
				moc_SimpleGlobeOrientation.cc \
				moc_ViewportZoom.cc \
				moc_FeatureTableModel.cc

CLEANFILES		= moc_*.cc

# This rule lets GNU make create any moc_*.cc from the equivalent *.h
moc_%.cc: %.h
	$(MOC) $< -o $@
