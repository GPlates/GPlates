## Makefile.am -- Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_srcdir)/src

noinst_LTLIBRARIES	= libmodel.la
libmodel_la_SOURCES	= ConstFeatureVisitor.cc FeatureVisitor.cc GeoTimeInstant.cc \
				GmlLineString.cc GmlPoint.cc GpmlOnePointFiveOutputVisitor.cc \
				GpmlFiniteRotation.cc Model.cc \
				ReconstructedFeatureGeometryPopulator.cc ReconstructionTree.cc \
				ReconstructionTreePopulator.cc StringSetSingletons.cc \
				XmlOutputInterface.cc FeatureCollectionRevision.cc \
				FeatureStoreRootRevision.cc FeatureRevision.cc \
				DummyTransactionHandle.cc

noinst_PROGRAMS		= _model.so
_model_so_SOURCES	= PythonWrapper.cc
_model_so_LDADD		= libmodel.la
_model_so_LDFLAGS	= -shared

