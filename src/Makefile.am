## Makefile.am -- Process this file with automake to produce Makefile.in

SUBDIRS			= controls fileio geo global gui maths model state util

INCLUDES		= -I$(top_srcdir)/src

bin_PROGRAMS		= gplates gplates-new

gplates_SOURCES		= gplates_main.cc
gplates_LDADD		= gui/libgui.a controls/libcontrols.a fileio/libfileio.a \
				geo/libgeo.a controls/libcontrols.a state/libstate.a \
				maths/libmaths.a global/libglobal.a

gplates_new_SOURCES	= gplates_new_main.cc
gplates_new_LDADD	= gui/libgui.a controls/libcontrols.a fileio/libfileio.a \
				geo/libgeo.a model/libmodel.a state/libstate.a \
				maths/libmaths.a global/libglobal.a util/libutil.a

