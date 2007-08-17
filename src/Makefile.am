## Makefile.am -- Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES	= Makefile.in

SUBDIRS			= gui maths fileio model state util gplates-qt-demo global

INCLUDES		= -I$(top_srcdir)/src

bin_PROGRAMS		= gplates_new gplates

gplates_new_SOURCES	= gplates_new_main.cc
gplates_new_LDADD	= libgplates_gui.la libgplates_model.la

gplates_SOURCES		= gplates_main.cc
gplates_LDADD		= libgplates_model.la libgplates_gui.la \
				gplates-qt-demo/libgplates_qt_demo.la

lib_LTLIBRARIES			= libgplates_gui.la \
					libgplates_model.la \
					libgplates_util.la

libgplates_gui_la_SOURCES	=
libgplates_gui_la_LIBADD	= gui/libgui.la \
					libgplates_util.la

libgplates_model_la_SOURCES	=
libgplates_model_la_LIBADD	= model/libmodel.la \
					libgplates_util.la

libgplates_util_la_SOURCES	=
libgplates_util_la_LIBADD	= maths/libmaths.la state/libstate.la util/libutil.la \
					fileio/libfileio.la global/libglobal.la

