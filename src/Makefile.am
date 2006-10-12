## Makefile.am -- Process this file with automake to produce Makefile.in

SUBDIRS			= controls fileio geo global gui maths state util

INCLUDES		= -I$(top_srcdir)/src

bin_PROGRAMS		= gplates

gplates_SOURCES		= gplates_main.cc
gplates_LDADD		= gui/libgui.a controls/libcontrols.a fileio/libfileio.a \
				geo/libgeo.a controls/libcontrols.a state/libstate.a \
				maths/libmaths.a global/libglobal.a # util/libutil.a

