## Makefile.am -- Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_srcdir)/src

noinst_LTLIBRARIES	= libfeature_visitors.la
libfeature_visitors_la_SOURCES	= QueryFeaturePropertiesWidgetPopulator.cc \
				EditFeatureGeometriesWidgetPopulator.cc \
				PlateIdFinder.cc GmlTimePeriodFinder.cc XsStringFinder.cc \
				GeometryFinder.cc FromQvariantConverter.cc ToQvariantConverter.cc \
				ShapefileAttributeFinder.cc \
				TotalReconstructionSequencePlateIdFinder.cc \
				TotalReconstructionSequenceRotationInserter.cc \
				ValueFinder.cc \
				PlatepolygonResolver.cc \
				ReconstructedFeatureGeometryFinder.cc

if PYTHON
noinst_PROGRAMS		= _feature_visitors.so
_feature_visitors_so_SOURCES	= PythonWrapper.cc
_feature_visitors_so_LDADD	= libfeature_visitors.la ../maths/libmaths.la ../util/libutil.la
_feature_visitors_so_LDFLAGS	= -shared
endif

