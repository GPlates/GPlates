## Makefile.am -- Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_srcdir)/src

noinst_LTLIBRARIES	= libfileio.la
libfileio_la_SOURCES	= PlatesLineFormatReader.cc \
				PlatesRotationFormatReader.cc \
				ReadErrorOccurrence.cc

if PYTHON
noinst_PROGRAMS		= _fileio.so
_fileio_so_SOURCES	= PythonWrapper.cc
_fileio_so_LDADD	= libfileio.la
_fileio_so_LDFLAGS	= -shared
endif

