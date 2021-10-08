## Makefile.am -- Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_srcdir)/src

noinst_LTLIBRARIES	= libmaths.la
libmaths_la_SOURCES	= FiniteRotation.cc GreatCircle.cc GreatCircleArc.cc GridOnSphere.cc \
				LatLonPointConversions.cc PointLiesOnGreatCircleArc.cc PointOnSphere.cc \
				PolylineOnSphere.cc PolygonOnSphere.cc PolylineIntersections.cc Real.cc \
				Rotation.cc SmallCircle.cc UnitQuaternion3D.cc UnitVector3D.cc Vector3D.cc

if PYTHON
noinst_PROGRAMS		= _maths.so
_maths_so_SOURCES	= PythonWrapper.cc
_maths_so_LDADD		= libmaths.la
_maths_so_LDFLAGS	= -shared
endif

