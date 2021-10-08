## Makefile.am -- Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_srcdir)/src

noinst_LTLIBRARIES	= libproperty_values.la
libproperty_values_la_SOURCES	= GeoTimeInstant.cc GmlLineString.cc \
				GmlPoint.cc GpmlFiniteRotation.cc

if PYTHON
noinst_PROGRAMS		= _property_values.so
_property_values_so_SOURCES	= PythonWrapper.cc
_property_values_so_LDADD	= libproperty_values.la ../maths/libmaths.la \
					../util/libutil.la
_property_values_so_LDFLAGS	= -shared
endif

