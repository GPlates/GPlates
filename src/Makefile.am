## Makefile.am -- Process this file with automake to produce Makefile.in

## SUBDIRS		= application controls fileio geo global gui maths model state util
SUBDIRS			= gui maths model state \
				gplates-qt-demo cube-qt-demo

INCLUDES		= -I$(top_srcdir)/src

bin_PROGRAMS		= cube_qt_demo gplates_qt_demo

gplates_SOURCES		= gplates_main.cc
gplates_LDADD		= application/libapplication.a gui/libgui.a

gplates_new_SOURCES	= gplates_new_main.cc
gplates_new_LDADD	= gui/libgui.a controls/libcontrols.a fileio/libfileio.a \
				geo/libgeo.a model/libmodel.a state/libstate.a \
				maths/libmaths.a global/libglobal.a util/libutil.a

gplates_qt_demo_SOURCES	= gplates_qt_demo.cc
gplates_qt_demo_LDADD	= gplates-qt-demo/libgplates_qt_demo.a model/libmodel.a \
				maths/libmaths.a gui/libgui.a state/libstate.a

cube_qt_demo_SOURCES	= cube_qt_demo.cc
cube_qt_demo_LDADD	= cube-qt-demo/libcube_qt_demo.a
