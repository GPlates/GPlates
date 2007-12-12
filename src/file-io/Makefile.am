# Makefile.am -- Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_srcdir)/src

noinst_LTLIBRARIES	= libfile_io.la
libfile_io_la_SOURCES	= PlatesLineFormatReader.cc \
				PlatesRotationFormatReader.cc \
				XmlOutputInterface.cc \
				GpmlOnePointFiveOutputVisitor.cc \
				ReadErrorOccurrence.cc \
				PlatesLineFormatWriter.cc \
				PlatesRotationFormatWriter.cc \
				ShapeFileReader.cc

if PYTHON
noinst_PROGRAMS		= _file_io.so
_file_io_so_SOURCES	= PythonWrapper.cc
_file_io_so_LDADD	= libfile_io.la
_file_io_so_LDFLAGS	= -shared
endif
