## Makefile.am -- Process this file with automake to produce Makefile.in

SUBDIRS			= gui maths model state util gplates-qt-demo

INCLUDES		= -I$(top_srcdir)/src

bin_PROGRAMS		= gplates_new gplates_qt_demo

gplates_SOURCES		= gplates_main.cc
gplates_LDADD		= application/libapplication.a gui/libgui.a

gplates_new_SOURCES	= gplates_new_main.cc
gplates_new_LDADD	= gui/libgui.a model/libmodel.a state/libstate.a \
				maths/libmaths.a util/libutil.a

gplates_qt_demo_SOURCES	= gplates_qt_demo.cc
gplates_qt_demo_LDADD	= gplates-qt-demo/libgplates_qt_demo.a model/libmodel.a \
				maths/libmaths.a gui/libgui.a state/libstate.a \
				util/libutil.a
