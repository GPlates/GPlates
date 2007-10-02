## Makefile.am -- Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES	= Makefile.in

SUBDIRS			= gui gui-qt maths file-io model state utils global \
				property-values

INCLUDES		= -I$(top_srcdir)/src

bin_PROGRAMS		= gplates gplates_demo_no_gui

gplates_demo_no_gui_SOURCES	= gplates_demo_no_gui_main.cc
gplates_demo_no_gui_LDADD	= libgplates_model.la libgplates_util.la

gplates_SOURCES		= gplates_main.cc
gplates_LDADD		= libgplates_gui.la libgplates_app.la \
				libgplates_model.la libgplates_util.la

lib_LTLIBRARIES		= libgplates_gui.la libgplates_app.la \
				libgplates_model.la libgplates_util.la

libgplates_gui_la_SOURCES	=
libgplates_gui_la_LIBADD	= gui/libgui.la gui-qt/libgui_qt.la

libgplates_app_la_SOURCES	=
libgplates_app_la_LIBADD	= file-io/libfile_io.la

libgplates_model_la_SOURCES	=
libgplates_model_la_LIBADD	= model/libmodel.la \
					property-values/libproperty_values.la

libgplates_util_la_SOURCES	=
libgplates_util_la_LIBADD	= maths/libmaths.la state/libstate.la \
					utils/libutils.la global/libglobal.la
