## Makefile.am -- Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_srcdir)/src

noinst_LTLIBRARIES	= libmodel.la
libmodel_la_SOURCES	= ConstFeatureVisitor.cc FeatureVisitor.cc \
				ReconstructedFeatureGeometry.cc \
				ReconstructedFeatureGeometryPopulator.cc \
				Model.cc ReconstructionGraph.cc \
				Reconstruction.cc \
				ReconstructionTree.cc \
				ReconstructionTreeEdge.cc \
				ReconstructionTreePopulator.cc \
				StringSetSingletons.cc \
				FeatureCollectionRevision.cc \
				FeatureStoreRootRevision.cc \
				FeatureRevision.cc \
				DummyTransactionHandle.cc \
				ModelUtils.cc \
				XmlNode.cc \
				TemporaryGeometry.cc

if PYTHON
noinst_PROGRAMS		= _model.so
_model_so_SOURCES	= PythonWrapper.cc
_model_so_LDADD		= libmodel.la ../maths/libmaths.la ../util/libutil.la
_model_so_LDFLAGS	= -shared
endif

