/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <algorithm>
#include <iterator>
#include <utility>
#include <vector>
#include <boost/cast.hpp>
#include <boost/foreach.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>

#include "PyGeometriesOnSphere.h"

#include "PythonConverterUtils.h"
#include "PythonHashDefVisitor.h"

#include "app-logic/GeometryUtils.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/python.h"
// This is not included by <boost/python.hpp>.
// Also we must include this after <boost/python.hpp> which means after "global/python.h".
#include <boost/python/slice.hpp>
#include <boost/python/stl_iterator.hpp>

#include "maths/AngularDistance.h"
#include "maths/AngularExtent.h"
#include "maths/GeometryDistance.h"
#include "maths/GeometryInterpolation.h"
#include "maths/GeometryOnSphere.h"
#include "maths/LatLonPoint.h"
#include "maths/MathsUtils.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"

#include "utils/non_null_intrusive_ptr.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	/**
	 * A from-python converter from a sequence of points or a GeometryOnSphere to a
	 * @a PointSequenceFunctionArgument.
	 */
	struct python_PointSequenceFunctionArgument :
			private boost::noncopyable
	{
		static
		void *
		convertible(
				PyObject *obj)
		{
			namespace bp = boost::python;

			if (PointSequenceFunctionArgument::is_convertible(
				bp::object(bp::handle<>(bp::borrowed(obj)))))
			{
				return obj;
			}

			return NULL;
		}

		static
		void
		construct(
				PyObject *obj,
				boost::python::converter::rvalue_from_python_stage1_data *data)
		{
			namespace bp = boost::python;

			void *const storage = reinterpret_cast<
					bp::converter::rvalue_from_python_storage<
							PointSequenceFunctionArgument> *>(
									data)->storage.bytes;

			new (storage) PointSequenceFunctionArgument(
					bp::object(bp::handle<>(bp::borrowed(obj))));

			data->convertible = storage;
		}
	};


	/**
	 * Registers converter from a sequence of points or a GeometryOnSphere to a @a PointSequenceFunctionArgument.
	 */
	void
	register_point_sequence_function_argument_conversion()
	{
		// Register function argument types variant.
		PythonConverterUtils::register_variant_conversion<
				PointSequenceFunctionArgument::function_argument_type>();

		// NOTE: We don't define a to-python conversion.

		// From python conversion.
		bp::converter::registry::push_back(
				&python_PointSequenceFunctionArgument::convertible,
				&python_PointSequenceFunctionArgument::construct,
				bp::type_id<PointSequenceFunctionArgument>());
	}
}


bool
GPlatesApi::PointSequenceFunctionArgument::is_convertible(
		bp::object python_function_argument)
{
	// If we fail to extract or iterate over the supported types then catch exception and return NULL.
	try
	{
		// Test all supported types (in function_argument_type) except the bp::object (since that's a sequence).
		if (bp::extract< GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PointOnSphere> >(python_function_argument).check() ||
			bp::extract< GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::MultiPointOnSphere> >(python_function_argument).check() ||
			bp::extract< GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PolylineOnSphere> >(python_function_argument).check() ||
			bp::extract< GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PolygonOnSphere> >(python_function_argument).check())
		{
			return true;
		}

		// Else it's a boost::python::object so we're expecting it to be a sequence of
		// points which requires further checking.

		const bp::object sequence = python_function_argument;

		// Iterate over the sequence of points.
		//
		// NOTE: We avoid iterating using 'bp::stl_input_iterator<GPlatesMaths::PointOnSphere>'
		// because we want to avoid actually extracting the points.
		// We're just checking if there's a sequence of points here.
		bp::object iter = sequence.attr("__iter__")();
		while (bp::handle<> item = bp::handle<>(bp::allow_null(PyIter_Next(iter.ptr()))))
		{
			if (!bp::extract<GPlatesMaths::PointOnSphere>(bp::object(item)).check())
			{
				return false;
			}
		}

		if (PyErr_Occurred())
		{
			PyErr_Clear();
			return false;
		}

		return true;
	}
	catch (const bp::error_already_set &)
	{
		PyErr_Clear();
	}

	return false;
}


GPlatesApi::PointSequenceFunctionArgument::PointSequenceFunctionArgument(
		bp::object python_function_argument) :
	d_points(
			initialise_points(
					bp::extract<function_argument_type>(python_function_argument)))
{
}


GPlatesApi::PointSequenceFunctionArgument::PointSequenceFunctionArgument(
		const function_argument_type &function_argument) :
	d_points(initialise_points(function_argument))
{
}


boost::shared_ptr<GPlatesApi::PointSequenceFunctionArgument::point_seq_type>
GPlatesApi::PointSequenceFunctionArgument::initialise_points(
		const function_argument_type &function_argument)
{
	if (const GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PointOnSphere> *point_function_argument =
		boost::get< GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PointOnSphere> >(&function_argument))
	{
		return boost::shared_ptr<point_seq_type>(new point_seq_type(1, **point_function_argument));
	}
	else if (const GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::MultiPointOnSphere> *multi_point_function_argument =
		boost::get< GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::MultiPointOnSphere> >(&function_argument))
	{
		boost::shared_ptr<point_seq_type> points(new point_seq_type());
		GPlatesAppLogic::GeometryUtils::get_geometry_points(**multi_point_function_argument, *points);
		return points;
	}
	else if (const GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PolylineOnSphere> *polyline_function_argument =
		boost::get< GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PolylineOnSphere> >(&function_argument))
	{
		boost::shared_ptr<point_seq_type> points(new point_seq_type());
		GPlatesAppLogic::GeometryUtils::get_geometry_points(**polyline_function_argument, *points);
		return points;
	}
	else if (const GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PolygonOnSphere> *polygon_function_argument =
		boost::get< GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PolygonOnSphere> >(&function_argument))
	{
		boost::shared_ptr<point_seq_type> points(new point_seq_type());
		GPlatesAppLogic::GeometryUtils::get_geometry_points(**polygon_function_argument, *points);
		return points;
	}
	else
	{
		boost::shared_ptr<point_seq_type> points(new point_seq_type());

		const bp::object sequence = boost::get<bp::object>(function_argument);

		bp::stl_input_iterator<GPlatesMaths::PointOnSphere> points_iter(sequence);
		bp::stl_input_iterator<GPlatesMaths::PointOnSphere> points_end;
		for ( ; points_iter != points_end; ++points_iter)
		{
			points->push_back(*points_iter);
		}

		return points;
	}
}


namespace GPlatesApi
{
	/**
	 * Calculate the (minimum) distance between two geometries.
	 */
	bp::object
	geometry_on_sphere_distance(
			const GPlatesMaths::GeometryOnSphere &geometry1,
			const GPlatesMaths::GeometryOnSphere &geometry2,
			boost::optional<GPlatesMaths::real_t> distance_threshold_radians,
			bool return_closest_positions,
			bool return_closest_indices,
			bool geometry1_is_solid,
			bool geometry2_is_solid)
	{
		// Specify the distance threshold if requested.
		boost::optional<const GPlatesMaths::AngularExtent &> minimum_distance_threshold;
		GPlatesMaths::AngularExtent threshold_storage = GPlatesMaths::AngularExtent::PI/*dummy value*/;
		if (distance_threshold_radians)
		{
			threshold_storage = GPlatesMaths::AngularExtent::create_from_angle(distance_threshold_radians.get());
			minimum_distance_threshold = threshold_storage;
		}

		// Reference closest points on each geometry if requested.
		boost::optional<
				boost::tuple<
						GPlatesMaths::UnitVector3D &/*geometry1*/,
						GPlatesMaths::UnitVector3D &/*geometry2*/>
						> closest_positions;
		GPlatesMaths::UnitVector3D closest_point1 = GPlatesMaths::UnitVector3D::xBasis()/*dummy value*/;
		GPlatesMaths::UnitVector3D closest_point2 = GPlatesMaths::UnitVector3D::xBasis()/*dummy value*/;
		if (return_closest_positions)
		{
			closest_positions = boost::make_tuple(boost::ref(closest_point1), boost::ref(closest_point2));
		}

		// Reference closest point or segment indices on each geometry if requested.
		boost::optional< boost::tuple<unsigned int &/*geometry1*/, unsigned int &/*geometry2*/> > closest_indices;
		unsigned int closest_index1;
		unsigned int closest_index2;
		if (return_closest_indices)
		{
			closest_indices = boost::make_tuple(boost::ref(closest_index1), boost::ref(closest_index2));
		}

		const GPlatesMaths::AngularDistance angular_distance =
				GPlatesMaths::minimum_distance(
						geometry1,
						geometry2,
						geometry1_is_solid,
						geometry2_is_solid,
						minimum_distance_threshold,
						closest_positions,
						closest_indices);

		// If a threshold was requested and was exceeded then return Py_None.
		if (minimum_distance_threshold &&
			angular_distance == GPlatesMaths::AngularDistance::PI)
		{
			return bp::object()/*Py_None*/;
		}

		// The distance (in radians).
		const GPlatesMaths::real_t distance = angular_distance.calculate_angle();

		//
		// If returning closest positions and/or closest indices then return a python tuple.
		//

		if (return_closest_positions)
		{
			if (return_closest_indices)
			{
				return bp::make_tuple(
								distance,
								GPlatesMaths::PointOnSphere(closest_point1),
								GPlatesMaths::PointOnSphere(closest_point2),
								closest_index1,
								closest_index2);
			}

			return bp::make_tuple(
							distance,
							GPlatesMaths::PointOnSphere(closest_point1),
							GPlatesMaths::PointOnSphere(closest_point2));
		}

		if (return_closest_indices)
		{
			return bp::make_tuple(
							distance,
							closest_index1,
							closest_index2);
		}

		// Just return the distance.
		return bp::object(distance);
	}
}

void
export_geometry_on_sphere()
{
	/*
	 * GeometryOnSphere - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	 *
	 * Base geometry-on-sphere wrapper class.
	 */
	bp::class_<
			GPlatesMaths::GeometryOnSphere,
			// NOTE: See note in 'register_to_python_const_to_non_const_geometry_on_sphere_conversion()'
			// for why this is 'non-const' instead of 'const'...
			GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::GeometryOnSphere>,
			boost::noncopyable>(
					"GeometryOnSphere",
					"The base class inherited by all derived classes representing geometries on the sphere.\n"
					"\n"
					"The list of derived geometry on sphere classes is:\n"
					"\n"
					"* :class:`PointOnSphere`\n"
					"* :class:`MultiPointOnSphere`\n"
					"* :class:`PolylineOnSphere`\n"
					"* :class:`PolygonOnSphere`\n",
					bp::no_init)
		.def("clone",
				&GPlatesMaths::GeometryOnSphere::clone_as_geometry,
				"clone()\n"
				"  Create a duplicate of this geometry (derived) instance.\n"
				"\n"
				"  :rtype: :class:`GeometryOnSphere`\n")
		.def("distance",
				&GPlatesApi::geometry_on_sphere_distance,
				(bp::arg("geometry1"), bp::arg("geometry2"),
						bp::arg("distance_threshold_radians") = boost::optional<GPlatesMaths::real_t>(),
						bp::arg("return_closest_positions") = false,
						bp::arg("return_closest_indices") = false,
						bp::arg("geometry1_is_solid") = false,
						bp::arg("geometry2_is_solid") = false),
				"distance(geometry1, geometry2, [distance_threshold_radians], "
				"[return_closest_positions=False], [return_closest_indices=False], "
				"[geometry1_is_solid=False], [geometry2_is_solid=False])\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Returns the (minimum) distance between two geometries (in radians).\n"
				"\n"
				"  :param geometry1: the first geometry\n"
				"  :type geometry1: :class:`GeometryOnSphere`\n"
				"  :param geometry2: the second geometry\n"
				"  :type geometry2: :class:`GeometryOnSphere`\n"
				"  :param distance_threshold_radians: optional distance threshold in radians - "
				"threshold should be in the range [0,PI] if specified\n"
				"  :type distance_threshold_radians: float or None\n"
				"  :param return_closest_positions: whether to also return the closest point on each "
				"geometry - default is ``False``\n"
				"  :type return_closest_positions: bool\n"
				"  :param return_closest_indices: whether to also return the index of the closest "
				":class:`point<PointOnSphere>` (for multi-points) or the index of the closest "
				":class:`segment<GreatCircleArc>` (for polylines and polygons) - default is ``False``\n"
				"  :type return_closest_indices: bool\n"
				"  :param geometry1_is_solid: whether the interior of *geometry1* is solid "
				"or not - this parameter is ignored if *geometry1* is not a :class:`PolygonOnSphere` "
				"- default is ``False``\n"
				"  :type geometry1_is_solid: bool\n"
				"  :param geometry2_is_solid: whether the interior of *geometry2* is solid "
				"or not - this parameter is ignored if *geometry2* is not a :class:`PolygonOnSphere` "
				"- default is ``False``\n"
				"  :type geometry2_is_solid: bool\n"
				"  :returns: distance (in radians), or a tuple containing distance and the "
				"closest point on each geometry if *return_closest_positions* is ``True``, or a tuple "
				"containing distance and the indices of the closest :class:`point<PointOnSphere>` "
				"(for multi-points) or :class:`segment<GreatCircleArc>` (for polylines and polygons) "
				"on each geometry if *return_closest_indices* is ``True``, or a tuple containing distance "
				"and the closest point on each geometry and the indices of the closest :class:`point<PointOnSphere>` "
				"(for multi-points) or :class:`segment<GreatCircleArc>` (for polylines and polygons) "
				"on each geometry if both *return_closest_positions* and *return_closest_indices* are ``True``, "
				"or ``None`` if *distance_threshold_radians* is specified and exceeded\n"
				"  :rtype: float, or tuple (float, :class:`PointOnSphere`, :class:`PointOnSphere`) if "
				"*return_closest_positions* is True, or tuple (float, int, int) if *return_closest_indices* "
				"is True, or tuple (float, :class:`PointOnSphere`, :class:`PointOnSphere`, int, int) "
				"if both *return_closest_positions* and *return_closest_indices* are True, or None\n"
				"\n"
				"  The returned distance is the shortest path between *geometry1* and *geometry2* along the "
				"surface of the sphere (great circle arc path). To convert the distance from radians "
				"(distance on a unit radius sphere) to real distance you will need to multiply it by "
				"the Earth's radius (see :class:`Earth`).\n"
				"\n"
				"  Each geometry (*geometry1* and *geometry2*) can be any of the four geometry types "
				"(:class:`PointOnSphere`, :class:`MultiPointOnSphere`, :class:`PolylineOnSphere` and "
				":class:`PolygonOnSphere`) allowing all combinations of distance calculations:\n"
				"  ::\n"
				"\n"
				"    distance_radians = pygplates.GeometryOnSphere.distance(point1, point2)\n"
				"    distance_radians = pygplates.GeometryOnSphere.distance(point1, multi_point2)\n"
				"    distance_radians = pygplates.GeometryOnSphere.distance(point1, polyline2)\n"
				"    distance_radians = pygplates.GeometryOnSphere.distance(point1, polygon2)\n"
				"\n"
				"    distance_radians = pygplates.GeometryOnSphere.distance(multi_point1, point2)\n"
				"    distance_radians = pygplates.GeometryOnSphere.distance(multi_point1, multi_point2)\n"
				"    distance_radians = pygplates.GeometryOnSphere.distance(multi_point1, polyline2)\n"
				"    distance_radians = pygplates.GeometryOnSphere.distance(multi_point1, polygon2)\n"
				"\n"
				"    distance_radians = pygplates.GeometryOnSphere.distance(polyline1, point2)\n"
				"    distance_radians = pygplates.GeometryOnSphere.distance(polyline1, multi_point2)\n"
				"    distance_radians = pygplates.GeometryOnSphere.distance(polyline1, polyline2)\n"
				"    distance_radians = pygplates.GeometryOnSphere.distance(polyline1, polygon2)\n"
				"\n"
				"    distance_radians = pygplates.GeometryOnSphere.distance(polygon1, point2)\n"
				"    distance_radians = pygplates.GeometryOnSphere.distance(polygon1, multi_point2)\n"
				"    distance_radians = pygplates.GeometryOnSphere.distance(polygon1, polyline2)\n"
				"    distance_radians = pygplates.GeometryOnSphere.distance(polygon1, polygon2)\n"
				"\n"
				"  If *distance_threshold_radians* is specified and the (minimum) distance between the "
				"two geometries exceeds this threshold then ``None`` is returned.\n"
				"  ::\n"
				"\n"
				"    # Perform a region-of-interest query between two geometries to see if\n"
				"    # they are within 1 degree of each other.\n"
				"    #\n"
				"    # Note that we explicitly test against None because a distance of zero is equilavent to False.\n"
				"    if pygplates.GeometryOnSphere.distance(geometry1, geometry2, math.radians(1)) is not None:\n"
				"        ...\n"
				"\n"
				"  Note that it is more efficient to specify a distance threshold parameter (as shown "
				"in the above example) than it is to explicitly compare the returned distance to a threshold "
				"yourself. This is because internally each polyline/polygon geometry has an inbuilt spatial "
				"tree that optimises distance queries.\n"
				"\n"
				"  The minimum distance between two geometries is zero (and hence does not exceed any "
				"distance threshold) **if**:\n"
				"\n"
				"  * both geometries are a polyline/polygon and they intersect each other, or\n"
				"  * *geometry1_is_solid* is ``True`` and *geometry1* is a :class:`PolygonOnSphere` "
				"and *geometry2* overlaps the interior of the polygon (even if it doesn't intersect the "
				"polygon boundary) - similarly for *geometry2_is_solid*. However note that "
				"*geometry1_is_solid* is ignored if *geometry1* is not a :class:`PolygonOnSphere` - "
				"similarly for *geometry2_is_solid*.\n"
				"\n"
				"  If *return_closest_positions* is ``True`` then the closest point on each geometry "
				"is returned (unless the distance threshold is exceeded, if specified). "
				"Note that for polygons the closest point is always on the polygon boundary regardless "
				"of whether the polygon is solid or not (see *geometry1_is_solid* and *geometry2_is_solid*). "
				"Also note that the closest position on a polyline/polygon can be anywhere along any of its "
				":class:`segments<GreatCircleArc>`. In other words it's not the nearest vertex of the "
				"polyline/polygon - it's the nearest point *on* the polyline/polygon itself. "
				"If both geometries are polyline/polygon and they intersect then the intersection point "
				"is returned (same point for both geometries). If both geometries are polyline/polygon and "
				"they intersect more than once then any intersection point can be returned (but the same "
				"point is returned for both geometries). If one geometry is a solid :class:`PolygonOnSphere` "
				"and the other geometry is a :class:`MultiPointOnSphere` with more than one of its points "
				"inside the interior of the polygon then the closest point in the multi-point could be any "
				"of those inside points.\n"
				"  ::\n"
				"\n"
				"    distance_radians, closest_point_on_geometry1, closest_point_on_geometry2 = \\\n"
				"        pygplates.GeometryOnSphere.distance(geometry1, geometry2, return_closest_positions=True)\n"
				"\n"
				"  If *return_closest_indices* is ``True`` then the index of the closest :class:`point<PointOnSphere>` "
				"(for multi-points) or the index of the closest :class:`segment<GreatCircleArc>` "
				"(for polylines and polygons) is returned (unless the threshold is exceeded, if specified). "
				"Note that for :class:`point<PointOnSphere>` geometries the index will always be zero. "
				"The point indices can be used to index directly into :class:`MultiPointOnSphere` and the segment "
				"indices can be used with :meth:`PolylineOnSphere.get_segments` or "
				":meth:`PolygonOnSphere.get_segments` as shown in the following example:\n"
				"  ::\n"
				"\n"
				"    distance_radians, closest_point_index_on_multipoint, closest_segment_index_on_polyline = \\\n"
				"        pygplates.GeometryOnSphere.distance(multipoint, polyline, return_closest_indices=True)\n"
				"\n"
				"    closest_point_on_multipoint = multipoint[closest_point_index_on_multipoint]\n"
				"    closest_segment_on_polyline = polyline.get_segments()[closest_segment_index_on_polyline]\n"
				"    closest_segment_normal_vector = closest_segment_on_polyline.get_rotation_axis()\n"
				"\n"
				"  If both *return_closest_positions* and *return_closest_indices* are ``True``:\n"
				"  ::\n"
				"\n"
				"    # Distance between a polyline and a solid polygon.\n"
				"    distance_radians, polyline_point, polygon_point, polyline_segment_index, polygon_segment_index = \\\n"
				"        pygplates.GeometryOnSphere.distance(polyline, polygon,\n"
				"            return_closest_positions=True, return_closest_indices=True,\n"
				"            geometry2_is_solid=True)\n")
		.staticmethod("distance")
	;

	// Register to-python conversion for GeometryOnSphere::non_null_ptr_to_const_type.
	GPlatesApi::PythonConverterUtils::register_to_python_const_to_non_const_geometry_on_sphere_conversion();
	// Register from-python 'non-const' to 'const' conversion.
	boost::python::implicitly_convertible<
			GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::GeometryOnSphere>,
			GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::GeometryOnSphere> >();

	// Enable boost::optional<GeometryOnSphere::non_null_ptr_to_const_type> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>();

	// Register converter from a sequence of points or a GeometryOnSphere to a @a PointSequenceFunctionArgument.
	GPlatesApi::register_point_sequence_function_argument_conversion();
}


namespace GPlatesApi
{
	/**
	 * Enables LatLonPoint to be passed from python (to a PointOnSphere).
	 *
	 * For more information on boost python to/from conversions, see:
	 *   http://misspent.wordpress.com/2009/09/27/how-to-write-boost-python-converters/
	 */
	struct python_PointOnSphereFromLatLonPoint :
			private boost::noncopyable
	{
		explicit
		python_PointOnSphereFromLatLonPoint()
		{
			namespace bp = boost::python;

			// From python conversion.
			bp::converter::registry::push_back(
					&convertible,
					&construct,
					bp::type_id<GPlatesMaths::PointOnSphere>());
		}

		static
		void *
		convertible(
				PyObject *obj)
		{
			namespace bp = boost::python;

			// PointOnSphere is created from a LatLonPoint.
			return bp::extract<GPlatesMaths::LatLonPoint>(obj).check() ? obj : NULL;
		}

		static
		void
		construct(
				PyObject *obj,
				boost::python::converter::rvalue_from_python_stage1_data *data)
		{
			namespace bp = boost::python;

			void *const storage = reinterpret_cast<
					bp::converter::rvalue_from_python_storage<GPlatesMaths::PointOnSphere> *>(
							data)->storage.bytes;

			new (storage) GPlatesMaths::PointOnSphere(
					make_point_on_sphere(bp::extract<GPlatesMaths::LatLonPoint>(obj)));

			data->convertible = storage;
		}
	};


	/**
	 * Enables a sequence, such as tuple or list, of (x,y,z) or (latitude,longitude) to be passed
	 * from python (to a PointOnSphere).
	 *
	 * For more information on boost python to/from conversions, see:
	 *   http://misspent.wordpress.com/2009/09/27/how-to-write-boost-python-converters/
	 */
	struct python_PointOnSphereFromXYZOrLatLonSequence :
			private boost::noncopyable
	{
		explicit
		python_PointOnSphereFromXYZOrLatLonSequence()
		{
			namespace bp = boost::python;

			// From python conversion.
			bp::converter::registry::push_back(
					&convertible,
					&construct,
					bp::type_id<GPlatesMaths::PointOnSphere>());
		}

		static
		void *
		convertible(
				PyObject *obj)
		{
			namespace bp = boost::python;

			bp::object py_obj = bp::object(bp::handle<>(bp::borrowed(obj)));

			// If the check fails then we'll catch a python exception and return NULL.
			try
			{
				// A sequence containing floats.
				bp::stl_input_iterator<double>
						float_seq_begin(py_obj),
						float_seq_end;

				// Copy into a vector.
				std::vector<double> float_vector;
				std::copy(float_seq_begin, float_seq_end, std::back_inserter(float_vector));
				if (float_vector.size() != 2 && // (lat,lon)
					float_vector.size() != 3)   // (x,y,z)
				{
					return NULL;
				}
			}
			catch (const boost::python::error_already_set &)
			{
				PyErr_Clear();
				return NULL;
			}

			return obj;
		}

		static
		void
		construct(
				PyObject *obj,
				boost::python::converter::rvalue_from_python_stage1_data *data)
		{
			namespace bp = boost::python;

			bp::object py_obj = bp::object(bp::handle<>(bp::borrowed(obj)));

			// A sequence containing floats.
			bp::stl_input_iterator<double>
					float_seq_begin(py_obj),
					float_seq_end;

			// Copy into a vector.
			std::vector<double> float_vector;
			std::copy(float_seq_begin, float_seq_end, std::back_inserter(float_vector));

			void *const storage = reinterpret_cast<
					bp::converter::rvalue_from_python_storage<GPlatesMaths::PointOnSphere> *>(
							data)->storage.bytes;

			if (float_vector.size() == 2)
			{
				// (lat,lon) sequence.
				new (storage) GPlatesMaths::PointOnSphere(
						make_point_on_sphere(
								GPlatesMaths::LatLonPoint(float_vector[0], float_vector[1])));
			}
			else
			{
				// Note that 'convertible' has checked for the correct size of the vector (size either 2 or 3).
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						float_vector.size() == 3,
						GPLATES_ASSERTION_SOURCE);

				// (x,y,z) sequence.
				new (storage) GPlatesMaths::PointOnSphere(
						GPlatesMaths::UnitVector3D(float_vector[0], float_vector[1], float_vector[2]));
			}

			data->convertible = storage;
		}
	};


	// North and south poles.
	const GPlatesMaths::PointOnSphere point_on_sphere_north_pole(GPlatesMaths::UnitVector3D::zBasis());
	const GPlatesMaths::PointOnSphere point_on_sphere_south_pole(-GPlatesMaths::UnitVector3D::zBasis());


	// Convenience constructor to create from (x,y,z).
	//
	// We can't use bp::make_constructor with a function that returns a non-pointer, so instead we
	// return 'non_null_intrusive_ptr'.
	//
	// With boost 1.42 we get the following compile error...
	//   pointer_holder.hpp:145:66: error: invalid conversion from 'const void*' to 'void*'
	// ...if we return 'GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type' and rely on
	// 'python_ConstGeometryOnSphere' to convert for us - despite the fact that this conversion works
	// successfully for python bindings in other source files. It's possibly due to
	// 'bp::make_constructor' which isn't used for MultiPointOnSphere which has no problem with 'const'.
	//
	// So we avoid it by using returning a pointer to 'non-const' PointOnSphere.
	GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PointOnSphere>
	point_on_sphere_create_xyz(
			const GPlatesMaths::real_t &x,
			const GPlatesMaths::real_t &y,
			const GPlatesMaths::real_t &z,
			bool normalise)
	{
		return GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PointOnSphere>(
				new GPlatesMaths::PointOnSphere(
						normalise
						? GPlatesMaths::Vector3D(x, y, z).get_normalisation()
						: GPlatesMaths::UnitVector3D(x, y, z)));
	}

	GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PointOnSphere>
	point_on_sphere_create_lat_lon(
			const GPlatesMaths::real_t &latitude,
			const GPlatesMaths::real_t &longitude)
	{
		return GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PointOnSphere>(
				new GPlatesMaths::PointOnSphere(
						make_point_on_sphere(
								GPlatesMaths::LatLonPoint(latitude.dval(), longitude.dval()))));
	}

	GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PointOnSphere>
	point_on_sphere_create(
			bp::object point_object)
	{
		// There are from-python converters from LatLonPoint and sequence(latitude,longitude) and
		// sequence(x,y,z) so they will also get matched by this...
		bp::extract<GPlatesMaths::PointOnSphere> extract_point_on_sphere(point_object);
		if (extract_point_on_sphere.check())
		{
			return GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PointOnSphere>(
					new GPlatesMaths::PointOnSphere(extract_point_on_sphere()));
		}

		PyErr_SetString(PyExc_TypeError, "Expected PointOnSphere or LatLonPoint or "
				"sequence (latitude,longitude) or sequence (x,y,z)");
		bp::throw_error_already_set();

		// Shouldn't be able to get here.
		return GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PointOnSphere>(NULL);
	}

	GPlatesMaths::Real
	point_on_sphere_get_x(
			const GPlatesMaths::PointOnSphere &point_on_sphere)
	{
		return point_on_sphere.position_vector().x();
	}

	GPlatesMaths::Real
	point_on_sphere_get_y(
			const GPlatesMaths::PointOnSphere &point_on_sphere)
	{
		return point_on_sphere.position_vector().y();
	}

	GPlatesMaths::Real
	point_on_sphere_get_z(
			const GPlatesMaths::PointOnSphere &point_on_sphere)
	{
		return point_on_sphere.position_vector().z();
	}

	bp::tuple
	point_on_sphere_to_xyz(
			const GPlatesMaths::PointOnSphere &point_on_sphere)
	{
		const GPlatesMaths::UnitVector3D &position_vector = point_on_sphere.position_vector();

		return bp::make_tuple(position_vector.x(), position_vector.y(), position_vector.z());
	}

	bp::tuple
	point_on_sphere_to_lat_lon(
			const GPlatesMaths::PointOnSphere &point_on_sphere)
	{
		const GPlatesMaths::LatLonPoint lat_lon_point = make_lat_lon_point(point_on_sphere);

		return bp::make_tuple(lat_lon_point.latitude(), lat_lon_point.longitude());
	}
}

void
export_point_on_sphere()
{
	//
	// PointOnSphere - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesMaths::PointOnSphere,
			// NOTE: See note in 'register_to_python_const_to_non_const_geometry_on_sphere_conversion()'
			// for why this is 'non-const' instead of 'const'...
			GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PointOnSphere>,
			bp::bases<GPlatesMaths::GeometryOnSphere>
			// NOTE: PointOnSphere is the only GeometryOnSphere type that is copyable.
			// And it needs to be copyable (in our wrapper) so that, eg, MultiPointOnSphere's iterator
			// can copy raw PointOnSphere elements (in its vector) into 'non_null_intrusive_ptr' versions
			// of PointOnSphere as it iterates over its collection (this conversion is one of those
			// cool things that boost-python takes care of automatically).
			//
			// Also note that PointOnSphere (like all GeometryOnSphere types) is immutable so the fact
			// that it can be copied into a python object (unlike the other GeometryOnSphere types) does
			// not mean we have to worry about a PointOnSphere modification from the C++ side not being
			// visible on the python side, or vice versa (because cannot modify since immutable - or at least
			// the python interface is immutable - the C++ has an assignment operator we should be careful of).
#if 0
			boost::noncopyable
#endif
			>(
					"PointOnSphere",
					"Represents a point on the surface of the unit length sphere in 3D cartesian coordinates.\n"
					"\n"
					"Points are equality (``==``, ``!=``) comparable (but not hashable "
					"- cannot be used as a key in a ``dict``). Two points are considered "
					"equal if their coordinates match within a *very* small numerical epsilon that accounts "
					"for the limits of floating-point precision. Note that usually two points will only compare "
					"equal if they are the same point or created from the exact same input data. If two "
					"points are generated in two different ways (eg, two different processing paths) they "
					"will most likely *not* compare equal even if mathematically they should be identical.\n"
					"\n"
					".. note:: Since a *PointOnSphere* is **immutable** it contains no operations or "
					"methods that modify its state.\n"
					"\n"
					"Convenience class static data are available for the North and South poles:\n"
					"\n"
					"* ``pygplates.PointOnSphere.north_pole``\n"
					"* ``pygplates.PointOnSphere.south_pole``\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::point_on_sphere_create,
						bp::default_call_policies(),
						(bp::arg("point"))),
				// General overloaded signature (must be in first overloaded 'def' - used by Sphinx)...
				// Specific overload signature...
				"__init__(...)\n"
				"A *PointOnSphere* object can be constructed in more than one way...\n"
				"\n"
				// Specific overload signature...
				"__init__(point)\n"
				"  Create a *PointOnSphere* instance from a (x,y,z) or (latitude,longitude) point.\n"
				"\n"
				"  :param point: (x,y,z) point, or (latitude,longitude) point (in degrees)\n"
				"  :type point: :class:`PointOnSphere` or :class:`LatLonPoint` or "
				"tuple (float,float,float) or tuple (float,float)\n"
				"  :raises: InvalidLatLonError if *latitude* or *longitude* is invalid\n"
				"  :raises: ViolatedUnitVectorInvariantError if (x,y,z) is not unit magnitude\n"
				"\n"
				"  The following example shows a few different ways to use this method:\n"
				"  ::\n"
				"\n"
				"    point = pygplates.PointOnSphere((x,y,z))\n"
				"    point = pygplates.PointOnSphere([x,y,z])\n"
				"    point = pygplates.PointOnSphere(numpy.array([x,y,z]))\n"
				"    point = pygplates.PointOnSphere(pygplates.LatLonPoint(latitude,longitude))\n"
				"    point = pygplates.PointOnSphere((latitude,longitude))\n"
				"    point = pygplates.PointOnSphere([latitude,longitude])\n"
				"    point = pygplates.PointOnSphere(numpy.array([latitude,longitude]))\n"
				"    point = pygplates.PointOnSphere(pygplates.PointOnSphere(x,y,z))\n")
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::point_on_sphere_create_lat_lon,
						bp::default_call_policies(),
						(bp::arg("latitude"), bp::arg("longitude"))),
				// Specific overload signature...
				"__init__(latitude, longitude)\n"
				"  Create a *PointOnSphere* instance from a *latitude* and *longitude*.\n"
				"\n"
				"  :param latitude: the latitude (in degrees)\n"
				"  :type latitude: float\n"
				"  :param longitude: the longitude (in degrees)\n"
				"  :type longitude: float\n"
				"  :raises: InvalidLatLonError if *latitude* or *longitude* is invalid\n"
				"\n"
				"  .. note:: *latitude* must satisfy :meth:`LatLonPoint.is_valid_latitude` and "
				"*longitude* must satisfy :meth:`LatLonPoint.is_valid_longitude`, otherwise "
				"*InvalidLatLonError* will be raised.\n"
				"\n"
				"  ::\n"
				"\n"
				"    point = pygplates.PointOnSphere(latitude, longitude)\n")
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::point_on_sphere_create_xyz,
						bp::default_call_policies(),
						(bp::arg("x"), bp::arg("y"), bp::arg("z"), bp::arg("normalise") = false)),
				"__init__(x, y, z, [normalise=False])\n"
				"  Create a *PointOnSphere* instance from a 3D cartesian coordinate consisting of "
				"floating-point coordinates *x*, *y* and *z*.\n"
				"\n"
				"  :param x: the *x* component of the 3D unit vector\n"
				"  :type x: float\n"
				"  :param y: the *y* component of the 3D unit vector\n"
				"  :type y: float\n"
				"  :param z: the *z* component of the 3D unit vector\n"
				"  :type z: float\n"
				"  :param normalise: whether to normalise (to unit-length magnitude) the "
				"vector (x,y,z) - defaults to ``False``\n"
				"  :type normalise: bool\n"
				"  :raises: ViolatedUnitVectorInvariantError if *normalise* is ``False`` and "
				"the resulting vector does not have unit magnitude\n"
				"  :raises: UnableToNormaliseZeroVectorError if *normalise* is ``True`` and "
				"the resulting vector is (0,0,0) (ie, has zero magnitude)\n"
				"\n"
				"  **NOTE:** If the length of the 3D vector (x,y,z) is not 1.0 then you should set "
				"*normalise* to ``True`` (to normalise the vector components such that the 3D vector "
				"has unit magnitude). Otherwise if (x,y,z) is not unit magnitude then "
				"*ViolatedUnitVectorInvariantError* is raised.\n"
				"  ::\n"
				"\n"
				"    # If you know that (x,y,z) has unit magnitude (is on the unit globe).\n"
				"    point = pygplates.PointOnSphere(x, y, z)\n"
				"\n"
				"    # If (x,y,z) might not be on the unit globe.\n"
				"    point = pygplates.PointOnSphere(x, y, z, normalise=True)\n")
		// Static property 'pygplates.PointOnSphere.north_pole'...
		.def_readonly("north_pole", GPlatesApi::point_on_sphere_north_pole)
		// Static property 'pygplates.PointOnSphere.south_pole'...
		.def_readonly("south_pole", GPlatesApi::point_on_sphere_south_pole)
		.def("get_x",
				&GPlatesApi::point_on_sphere_get_x,
				"get_x()\n"
				"  Returns the *x* coordinate.\n"
				"\n"
				"  :rtype: float\n")
		.def("get_y",
				&GPlatesApi::point_on_sphere_get_y,
				"get_y()\n"
				"  Returns the *y* coordinate.\n"
				"\n"
				"  :rtype: float\n")
		.def("get_z",
				&GPlatesApi::point_on_sphere_get_z,
				"get_z()\n"
				"  Returns the *z* coordinate.\n"
				"\n"
				"  :rtype: float\n")
		.def("to_xyz",
				&GPlatesApi::point_on_sphere_to_xyz,
				"to_xyz()\n"
				"  Returns the cartesian coordinates as the tuple (x,y,z).\n"
				"\n"
				"  :rtype: the tuple (float,float,float)\n"
				"\n"
				"  ::\n"
				"\n"
				"    x, y, z = point.to_xyz()\n"
				"\n"
				"  This is also useful for performing vector dot and cross products:\n"
				"  ::\n"
				"\n"
				"    dot_product = pygplates.Vector3D.dot(point1.to_xyz(), point2.to_xyz())\n"
				"    cross_product = pygplates.Vector3D.cross(point1.to_xyz(), point2.to_xyz())\n")
		.def("to_lat_lon_point",
				&GPlatesMaths::make_lat_lon_point,
				"to_lat_lon_point()\n"
				"  Returns the (latitude,longitude) equivalent of this :class:`PointOnSphere`.\n"
				"\n"
				"  :rtype: :class:`LatLonPoint`\n")
		.def("to_lat_lon",
				&GPlatesApi::point_on_sphere_to_lat_lon,
				"to_lat_lon()\n"
				"  Returns the tuple (latitude,longitude) in degrees.\n"
				"\n"
				"  :rtype: the tuple (float, float)\n"
				"\n"
				"  ::\n"
				"\n"
				"    latitude, longitude = point.to_lat_lon()\n"
				"\n"
				"  This is similar to :meth:`LatLonPoint.to_lat_lon`.\n")
		// Due to the numerical tolerance in comparisons we cannot make hashable.
		// Make unhashable, with no *equality* comparison operators (we explicitly define them)...
		.def(GPlatesApi::NoHashDefVisitor(false, true))
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
		// Generate '__str__' from 'operator<<'...
		// Note: Seems we need to qualify with 'self_ns::' to avoid MSVC compile error.
		.def(bp::self_ns::str(bp::self))
	;

	// Register to-python conversion for PointOnSphere::non_null_ptr_to_const_type.
	//
	// PointOnSphere is a bit of an exception to the immutable rule since it can be allocated on
	// the stack or heap (unlike the other geometry types) and it has an assignment operator.
	// However we treat it the same as the other geometry types.
	GPlatesApi::PythonConverterUtils::register_to_python_const_to_non_const_non_null_intrusive_ptr_conversion<
			GPlatesMaths::PointOnSphere>();
	// Register from-python 'non-const' to 'const' conversion.
	boost::python::implicitly_convertible<
			GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PointOnSphere>,
			GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PointOnSphere> >();

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Note that we're only dealing with 'const' conversions here.
	// The 'non-const' workaround to get boost-python to compile is handled by 'python_ConstGeometryOnSphere'.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			const GPlatesMaths::PointOnSphere,
			const GPlatesMaths::GeometryOnSphere>();

	// Registers the from-python converter from GPlatesMaths::LatLonPoint.
	GPlatesApi::python_PointOnSphereFromLatLonPoint();

	// Registers the from-python converter from a (x,y,z) or (lat,lon) sequence.
	GPlatesApi::python_PointOnSphereFromXYZOrLatLonSequence();
}


namespace GPlatesApi
{
	// Create a multi-point from a sequence of points.
	GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::MultiPointOnSphere>
	multi_point_on_sphere_create_from_points(
			bp::object points) // Any python sequence (eg, list, tuple).
	{
		// Begin/end iterators over the python points sequence.
		bp::stl_input_iterator<GPlatesMaths::PointOnSphere>
				points_begin(points),
				points_end;

		// Copy into a vector.
		// Note: We can't pass the iterators directly to 'MultiPointOnSphere::create_on_heap'
		// because they are *input* iterators (ie, one pass only) and 'MultiPointOnSphere' expects
		// forward iterators (it actually makes two passes over the points).
 		std::vector<GPlatesMaths::PointOnSphere> points_vector;
		std::copy(points_begin, points_end, std::back_inserter(points_vector));

		// With boost 1.42 we get the following compile error...
		//   pointer_holder.hpp:145:66: error: invalid conversion from 'const void*' to 'void*'
		// ...if we return 'GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type' and rely on
		// 'python_ConstGeometryOnSphere' to convert for us - despite the fact that this conversion works
		// successfully for python bindings in other source files. It's likely due to 'bp::make_constructor'.
		//
		// So we avoid it by using returning a pointer to 'non-const' MultiPointOnSphere.
		return GPlatesUtils::const_pointer_cast<GPlatesMaths::MultiPointOnSphere>(
				GPlatesMaths::MultiPointOnSphere::create_on_heap(
						points_vector.begin(),
						points_vector.end()));
	}

	// Create a MultiPointOnSphere from a GeometryOnSphere.
	GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::MultiPointOnSphere>
	multi_point_on_sphere_create_from_geometry(
			const GPlatesMaths::GeometryOnSphere &geometry)
	{
		// With boost 1.42 we get the following compile error...
		//   pointer_holder.hpp:145:66: error: invalid conversion from 'const void*' to 'void*'
		// ...if we return 'GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type' and rely on
		// 'python_ConstGeometryOnSphere' to convert for us - despite the fact that this conversion works
		// successfully for python bindings in other source files. It's likely due to 'bp::make_constructor'.
		//
		// So we avoid it by returning a pointer to 'non-const' MultiPointOnSphere.
		return GPlatesUtils::const_pointer_cast<GPlatesMaths::MultiPointOnSphere>(
				GPlatesAppLogic::GeometryUtils::convert_geometry_to_multi_point(geometry));
	}

	bool
	multi_point_on_sphere_contains_point(
			const GPlatesMaths::MultiPointOnSphere &multi_point_on_sphere,
			// There are from-python converters from LatLonPoint and sequence(latitude,longitude) and
			// sequence(x,y,z) to PointOnSphere so they will also get matched by this...
			const GPlatesMaths::PointOnSphere &point_on_sphere)
	{
		return std::find(
				multi_point_on_sphere.begin(),
				multi_point_on_sphere.end(),
				point_on_sphere)
						!= multi_point_on_sphere.end();
	}

	//
	// Support for "__getitem__".
	//
	boost::python::object
	multi_point_on_sphere_get_item(
			const GPlatesMaths::MultiPointOnSphere &multi_point_on_sphere,
			boost::python::object i)
	{
		namespace bp = boost::python;

		// Set if the index is a slice object.
		bp::extract<bp::slice> extract_slice(i);
		if (extract_slice.check())
		{
			bp::list slice_list;

			try
			{
				// Use boost::python::slice to manage index variations such as negative indices or
				// indices that are None.
				bp::slice slice = extract_slice();
				bp::slice::range<GPlatesMaths::MultiPointOnSphere::const_iterator> slice_range =
						slice.get_indicies(multi_point_on_sphere.begin(), multi_point_on_sphere.end());

				GPlatesMaths::MultiPointOnSphere::const_iterator iter = slice_range.start;
				for ( ; iter != slice_range.stop; std::advance(iter, slice_range.step))
				{
					slice_list.append(*iter);
				}
				slice_list.append(*iter);
			}
			catch (std::invalid_argument)
			{
				// Invalid slice - return empty list.
				return bp::list();
			}

			return slice_list;
		}

		// See if the index is an integer.
		bp::extract<long> extract_index(i);
		if (extract_index.check())
		{
			long index = extract_index();
			if (index < 0)
			{
				index += multi_point_on_sphere.number_of_points();
			}

			if (index >= boost::numeric_cast<long>(multi_point_on_sphere.number_of_points()) ||
				index < 0)
			{
				PyErr_SetString(PyExc_IndexError, "Index out of range");
				bp::throw_error_already_set();
			}

			GPlatesMaths::MultiPointOnSphere::const_iterator iter = multi_point_on_sphere.begin();
			std::advance(iter, index); // Should be fast since 'iter' is random access.
			const GPlatesMaths::PointOnSphere &point = *iter;

			return bp::object(point);
		}

		PyErr_SetString(PyExc_TypeError, "Invalid index type");
		bp::throw_error_already_set();

		return bp::object();
	}

	GPlatesMaths::PointOnSphere
	multi_point_on_sphere_get_centroid(
			const GPlatesMaths::MultiPointOnSphere &multi_point_on_sphere)
	{
		return GPlatesMaths::PointOnSphere(multi_point_on_sphere.get_centroid());
	}
}

void
export_multi_point_on_sphere()
{
	//
	// MultiPointOnSphere - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesMaths::MultiPointOnSphere,
			// NOTE: See note in 'register_to_python_const_to_non_const_geometry_on_sphere_conversion()'
			// for why this is 'non-const' instead of 'const'...
			GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::MultiPointOnSphere>,
			bp::bases<GPlatesMaths::GeometryOnSphere>,
			boost::noncopyable>(
					"MultiPointOnSphere",
					"Represents a multi-point (collection of points) on the surface of the unit length sphere. "
					"Multi-points are equality (``==``, ``!=``) comparable (but not hashable "
					"- cannot be used as a key in a ``dict``). See :class:`PointOnSphere` "
					"for an overview of equality in the presence of limited floating-point precision.\n"
					"\n"
					"A multi-point instance is iterable over its points:\n"
					"::\n"
					"\n"
					"  multi_point = pygplates.MultiPointOnSphere(points)\n"
					"  for point in multi_point:\n"
					"      ...\n"
					"\n"
					"The following operations for accessing the points are supported:\n"
					"\n"
					"=========================== ==========================================================\n"
					"Operation                   Result\n"
					"=========================== ==========================================================\n"
					"``len(mp)``                 number of points in *mp*\n"
					"``for p in mp``             iterates over the points *p* of multi-point *mp*\n"
					"``p in mp``                 ``True`` if *p* is equal to a point in *mp*\n"
					"``p not in mp``             ``False`` if *p* is equal to a point in *mp*\n"
					"``mp[i]``                   the point of *mp* at index *i*\n"
					"``mp[i:j]``                 slice of *mp* from *i* to *j*\n"
					"``mp[i:j:k]``               slice of *mp* from *i* to *j* with step *k*\n"
					"=========================== ==========================================================\n"
					"\n"
					".. note:: Since a *MultiPointOnSphere* is **immutable** it contains no operations or "
					"methods that modify its state (such as adding or removing points).\n"
					"\n"
					"There are also methods that return the sequence of points as (latitude,longitude) "
					"values and (x,y,z) values contained in lists and numpy arrays "
					"(:meth:`GeometryOnSphere.to_lat_lon_list`, :meth:`GeometryOnSphere.to_lat_lon_array`, "
					":meth:`GeometryOnSphere.to_xyz_list` and :meth:`GeometryOnSphere.to_xyz_array`).\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::multi_point_on_sphere_create_from_points,
						bp::default_call_policies(),
						(bp::arg("points"))),
				// General overloaded signature (must be in first overloaded 'def' - used by Sphinx)...
				// Specific overload signature...
				"__init__(...)\n"
				"A *MultiPointOnSphere* object can be constructed in more than one way...\n"
				"\n"
				// Specific overload signature...
				"__init__(points)\n"
				"  Create a multi-point from a sequence of (x,y,z) or (latitude,longitude) points.\n"
				"\n"
				"  :param points: A sequence of (x,y,z) points, or (latitude,longitude) points (in degrees).\n"
				"  :type points: Any sequence of :class:`PointOnSphere` or :class:`LatLonPoint` or "
				"tuple (float,float,float) or tuple (float,float)\n"
				"  :raises: InvalidLatLonError if any *latitude* or *longitude* is invalid\n"
				"  :raises: ViolatedUnitVectorInvariantError if any (x,y,z) is not unit magnitude\n"
				"  :raises: InsufficientPointsForMultiPointConstructionError if point sequence is empty\n"
				"\n"
				"  .. note:: The sequence must contain at least one point, otherwise "
				"*InsufficientPointsForMultiPointConstructionError* will be raised.\n"
				"\n"
				"  The following example shows a few different ways to create a :class:`multi-point<MultiPointOnSphere>`:\n"
				"  ::\n"
				"\n"
				"    points = []\n"
				"    points.append(pygplates.PointOnSphere(...))\n"
				"    points.append(pygplates.PointOnSphere(...))\n"
				"    points.append(pygplates.PointOnSphere(...))\n"
				"    multi_point = pygplates.MultiPointOnSphere(points)\n"
				"    \n"
				"    points = []\n"
				"    points.append((lat1,lon1))\n"
				"    points.append((lat2,lon2))\n"
				"    points.append((lat3,lon3))\n"
				"    multi_point = pygplates.MultiPointOnSphere(points)\n"
				"    \n"
				"    points = []\n"
				"    points.append([x1,y1,z1])\n"
				"    points.append([x2,y2,z2])\n"
				"    points.append([x3,y3,z3])\n"
				"    multi_point = pygplates.MultiPointOnSphere(points)\n"
				"\n"
				"  If you have latitude/longitude values but they are not a sequence of tuples or "
				"if the latitude/longitude order is swapped then the following examples demonstrate "
				"how you could restructure them:\n"
				"  ::\n"
				"\n"
				"    # Flat lat/lon array.\n"
				"    points = numpy.array([lat1, lon1, lat2, lon2, lat3, lon3])\n"
				"    multi_point = pygplates.MultiPointOnSphere(zip(points[::2],points[1::2]))\n"
				"    \n"
				"    # Flat lon/lat list (ie, different latitude/longitude order).\n"
				"    points = [lon1, lat1, lon2, lat2, lon3, lat3]\n"
				"    multi_point = pygplates.MultiPointOnSphere(zip(points[1::2],points[::2]))\n"
				"    \n"
				"    # Separate lat/lon arrays.\n"
				"    lats = numpy.array([lat1, lat2, lat3])\n"
				"    lons = numpy.array([lon1, lon2, lon3])\n"
				"    multi_point = pygplates.MultiPointOnSphere(zip(lats,lons))\n"
				"    \n"
				"    # Lon/lat list of tuples (ie, different latitude/longitude order).\n"
				"    points = [(lon1, lat1), (lon2, lat2), (lon3, lat3)]\n"
				"    multi_point = pygplates.MultiPointOnSphere([(lat,lon) for lon, lat in points])\n")
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::multi_point_on_sphere_create_from_geometry,
						bp::default_call_policies(),
						(bp::arg("geometry"))),
				// Specific overload signature...
				"__init__(geometry)\n"
				"  Create a multipoint from a :class:`GeometryOnSphere`.\n"
				"\n"
				"  :param geometry: The point, multi-point, polyline or polygon geometry to convert from.\n"
				"  :type geometry: :class:`GeometryOnSphere`\n"
				"\n"
				"  To create a MultiPointOnSphere from any geometry type:\n"
				"  ::\n"
				"\n"
				"    multipoint = pygplates.MultiPointOnSphere(geometry)\n")
		.def("get_centroid",
				&GPlatesApi::multi_point_on_sphere_get_centroid,
				"get_centroid()\n"
				"  Returns the centroid of this multi-point.\n"
				"\n"
				"  :rtype: :class:`PointOnSphere`\n"
				"\n"
				"  The centroid is calculated as the sum of the points of the multi-point with the "
				"result normalized to a vector of unit length.\n")
		.def("__iter__", bp::iterator<const GPlatesMaths::MultiPointOnSphere>())
		.def("__len__", &GPlatesMaths::MultiPointOnSphere::number_of_points)
		.def("__contains__", &GPlatesApi::multi_point_on_sphere_contains_point)
		.def("__getitem__", &GPlatesApi::multi_point_on_sphere_get_item)
		// Due to the numerical tolerance in comparisons we cannot make hashable.
		// Make unhashable, with no *equality* comparison operators (we explicitly define them)...
		.def(GPlatesApi::NoHashDefVisitor(false, true))
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
	;

	// Register to-python conversion for MultiPointOnSphere::non_null_ptr_to_const_type.
	GPlatesApi::PythonConverterUtils::register_to_python_const_to_non_const_non_null_intrusive_ptr_conversion<
			GPlatesMaths::MultiPointOnSphere>();
	// Register from-python 'non-const' to 'const' conversion.
	boost::python::implicitly_convertible<
			GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::MultiPointOnSphere>,
			GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::MultiPointOnSphere> >();

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Note that we're only dealing with 'const' conversions here.
	// The 'non-const' workaround to get boost-python to compile is handled by 'python_ConstGeometryOnSphere'.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			const GPlatesMaths::MultiPointOnSphere,
			const GPlatesMaths::GeometryOnSphere>();
}


namespace GPlatesApi
{
	// Create a polyline/polygon from a sequence of points.
	template <class PolyGeometryOnSphereType>
	GPlatesUtils::non_null_intrusive_ptr<PolyGeometryOnSphereType>
	poly_geometry_on_sphere_create_from_points(
			bp::object points) // Any python sequence (eg, list, tuple).
	{
		// Begin/end iterators over the python points sequence.
		bp::stl_input_iterator<GPlatesMaths::PointOnSphere>
				points_begin(points),
				points_end;

		// Copy into a vector.
		// Note: We can't pass the iterators directly to 'PolylineOnSphere::create_on_heap' or
		// 'PolygonOnSphere::create_on_heap' because they are *input* iterators (ie, one pass only)
		// and 'PolylineOnSphere/PolygonOnSphere' expects forward iterators (they actually makes
		// two passes over the points).
 		std::vector<GPlatesMaths::PointOnSphere> points_vector;
		std::copy(points_begin, points_end, std::back_inserter(points_vector));

		// With boost 1.42 we get the following compile error...
		//   pointer_holder.hpp:145:66: error: invalid conversion from 'const void*' to 'void*'
		// ...if we return 'PolyGeometryOnSphereType::non_null_ptr_to_const_type' and rely on
		// 'python_ConstGeometryOnSphere' to convert for us - despite the fact that this conversion works
		// successfully for python bindings in other source files. It's likely due to 'bp::make_constructor'.
		//
		// So we avoid it by using returning a pointer to 'non-const' PolyGeometryOnSphereType.
		return GPlatesUtils::const_pointer_cast<PolyGeometryOnSphereType>(
				PolyGeometryOnSphereType::create_on_heap(
						points_vector.begin(),
						points_vector.end()));
	}

	// Create a PolylineOnSphere from a GeometryOnSphere.
	GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PolylineOnSphere>
	polyline_on_sphere_create_from_geometry(
			const GPlatesMaths::GeometryOnSphere &geometry,
			bool allow_one_point)
	{
		boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> polyline =
				GPlatesAppLogic::GeometryUtils::convert_geometry_to_polyline(geometry);
		if (!polyline)
		{
			// There were less than two points.
			// 
			// Retrieve the point.
			std::vector<GPlatesMaths::PointOnSphere> geometry_points;
			GPlatesAppLogic::GeometryUtils::get_geometry_points(geometry, geometry_points);

			// There should be a single point.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					!geometry_points.empty(),
					GPLATES_ASSERTION_SOURCE);

			// Duplicate the point if caller allows one point, otherwise let
			// 'PolylineOnSphere::create_on_heap()' throw InvalidPointsForPolylineConstructionError.
			if (allow_one_point)
			{
				geometry_points.push_back(geometry_points.back());
			}

			polyline = GPlatesMaths::PolylineOnSphere::create_on_heap(geometry_points);
		}

		// With boost 1.42 we get the following compile error...
		//   pointer_holder.hpp:145:66: error: invalid conversion from 'const void*' to 'void*'
		// ...if we return 'GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type' and rely on
		// 'python_ConstGeometryOnSphere' to convert for us - despite the fact that this conversion works
		// successfully for python bindings in other source files. It's likely due to 'bp::make_constructor'.
		//
		// So we avoid it by returning a pointer to 'non-const' PolylineOnSphere.
		return GPlatesUtils::const_pointer_cast<GPlatesMaths::PolylineOnSphere>(polyline.get());
	}

	/**
	 * Wrapper class for functions accessing the *points* of a PolylineOnSphere/PolygonOnSphere.
	 *
	 * This is a view into the internal points of a PolylineOnSphere/PolygonOnSphere in that an
	 * iterator can be obtained from the view and the view supports indexing.
	 */
	template <class PolyGeometryOnSphereType>
	class PolyGeometryOnSpherePointsView
	{
	public:
		explicit
		PolyGeometryOnSpherePointsView(
				typename PolyGeometryOnSphereType::non_null_ptr_to_const_type poly_geometry_on_sphere) :
			d_poly_geometry_on_sphere(poly_geometry_on_sphere)
		{  }

		typedef typename PolyGeometryOnSphereType::vertex_const_iterator const_iterator;

		const_iterator
		begin() const
		{
			return d_poly_geometry_on_sphere->vertex_begin();
		}

		const_iterator
		end() const
		{
			return d_poly_geometry_on_sphere->vertex_end();
		}

		typename PolyGeometryOnSphereType::size_type
		get_number_of_points() const
		{
			return d_poly_geometry_on_sphere->number_of_vertices();
		}

		bool
		contains_point(
				const GPlatesMaths::PointOnSphere &point_on_sphere) const
		{
			return std::find(
					d_poly_geometry_on_sphere->vertex_begin(),
					d_poly_geometry_on_sphere->vertex_end(),
					point_on_sphere)
							!= d_poly_geometry_on_sphere->vertex_end();
		}

		//
		// Support for "__getitem__".
		//
		boost::python::object
		get_item(
				boost::python::object i) const
		{
			namespace bp = boost::python;

			// Set if the index is a slice object.
			bp::extract<bp::slice> extract_slice(i);
			if (extract_slice.check())
			{
				bp::list slice_list;

				try
				{
					// Use boost::python::slice to manage index variations such as negative indices or
					// indices that are None.
					bp::slice slice = extract_slice();
					bp::slice::range<typename PolyGeometryOnSphereType::vertex_const_iterator> slice_range =
							slice.get_indicies(
									d_poly_geometry_on_sphere->vertex_begin(),
									d_poly_geometry_on_sphere->vertex_end());

					typename PolyGeometryOnSphereType::vertex_const_iterator iter = slice_range.start;
					for ( ; iter != slice_range.stop; std::advance(iter, slice_range.step))
					{
						slice_list.append(*iter);
					}
					slice_list.append(*iter);
				}
				catch (std::invalid_argument)
				{
					// Invalid slice - return empty list.
					return bp::list();
				}

				return slice_list;
			}

			// See if the index is an integer.
			bp::extract<long> extract_index(i);
			if (extract_index.check())
			{
				long index = extract_index();
				if (index < 0)
				{
					index += d_poly_geometry_on_sphere->number_of_vertices();
				}

				if (index >= boost::numeric_cast<long>(d_poly_geometry_on_sphere->number_of_vertices()) ||
					index < 0)
				{
					PyErr_SetString(PyExc_IndexError, "Index out of range");
					bp::throw_error_already_set();
				}

				typename PolyGeometryOnSphereType::vertex_const_iterator iter =
						d_poly_geometry_on_sphere->vertex_begin();
				std::advance(iter, index); // Should be fast since 'iter' is random access.
				const GPlatesMaths::PointOnSphere &point = *iter;

				return bp::object(point);
			}

			PyErr_SetString(PyExc_TypeError, "Invalid index type");
			bp::throw_error_already_set();

			return bp::object();
		}

	private:
		typename PolyGeometryOnSphereType::non_null_ptr_to_const_type d_poly_geometry_on_sphere;
	};

	template <class PolyGeometryOnSphereType>
	PolyGeometryOnSpherePointsView<PolyGeometryOnSphereType>
	poly_geometry_on_sphere_get_points_view(
			typename PolyGeometryOnSphereType::non_null_ptr_to_const_type poly_geometry_on_sphere)
	{
		return PolyGeometryOnSpherePointsView<PolyGeometryOnSphereType>(poly_geometry_on_sphere);
	}


	/**
	 * Wrapper class for functions accessing the *great circle arcs* of a PolylineOnSphere/PolygonOnSphere.
	 *
	 * This is a view into the internal arcs of a PolylineOnSphere/PolygonOnSphere in that an
	 * iterator can be obtained from the view and the view supports indexing.
	 */
	template <class PolyGeometryOnSphereType>
	class PolyGeometryOnSphereArcsView
	{
	public:
		explicit
		PolyGeometryOnSphereArcsView(
				typename PolyGeometryOnSphereType::non_null_ptr_to_const_type poly_geometry_on_sphere) :
			d_poly_geometry_on_sphere(poly_geometry_on_sphere)
		{  }

		typedef typename PolyGeometryOnSphereType::const_iterator const_iterator;

		const_iterator
		begin() const
		{
			return d_poly_geometry_on_sphere->begin();
		}

		const_iterator
		end() const
		{
			return d_poly_geometry_on_sphere->end();
		}

		typename PolyGeometryOnSphereType::size_type
		get_number_of_arcs() const
		{
			return d_poly_geometry_on_sphere->number_of_segments();
		}

		bool
		contains_arc(
				const GPlatesMaths::GreatCircleArc &gca) const
		{
			return std::find(
					d_poly_geometry_on_sphere->begin(),
					d_poly_geometry_on_sphere->end(),
					gca)
							!= d_poly_geometry_on_sphere->end();
		}

		//
		// Support for "__getitem__".
		//
		boost::python::object
		get_item(
				boost::python::object i) const
		{
			namespace bp = boost::python;

			// Set if the index is a slice object.
			bp::extract<bp::slice> extract_slice(i);
			if (extract_slice.check())
			{
				bp::list slice_list;

				try
				{
					// Use boost::python::slice to manage index variations such as negative indices or
					// indices that are None.
					bp::slice slice = extract_slice();
					bp::slice::range<typename PolyGeometryOnSphereType::const_iterator> slice_range =
							slice.get_indicies(
									d_poly_geometry_on_sphere->begin(),
									d_poly_geometry_on_sphere->end());

					typename PolyGeometryOnSphereType::const_iterator iter = slice_range.start;
					for ( ; iter != slice_range.stop; std::advance(iter, slice_range.step))
					{
						slice_list.append(*iter);
					}
					slice_list.append(*iter);
				}
				catch (std::invalid_argument)
				{
					// Invalid slice - return empty list.
					return bp::list();
				}

				return slice_list;
			}

			// See if the index is an integer.
			bp::extract<long> extract_index(i);
			if (extract_index.check())
			{
				long index = extract_index();
				if (index < 0)
				{
					index += d_poly_geometry_on_sphere->number_of_segments();
				}

				if (index >= boost::numeric_cast<long>(d_poly_geometry_on_sphere->number_of_segments()) ||
					index < 0)
				{
					PyErr_SetString(PyExc_IndexError, "Index out of range");
					bp::throw_error_already_set();
				}

				typename PolyGeometryOnSphereType::const_iterator iter = d_poly_geometry_on_sphere->begin();
				std::advance(iter, index); // Should be fast since 'iter' is random access.
				const GPlatesMaths::GreatCircleArc &gca = *iter;

				return bp::object(gca);
			}

			PyErr_SetString(PyExc_TypeError, "Invalid index type");
			bp::throw_error_already_set();

			return bp::object();
		}

	private:
		typename PolyGeometryOnSphereType::non_null_ptr_to_const_type d_poly_geometry_on_sphere;
	};

	template <class PolyGeometryOnSphereType>
	PolyGeometryOnSphereArcsView<PolyGeometryOnSphereType>
	poly_geometry_on_sphere_get_arcs_view(
			typename PolyGeometryOnSphereType::non_null_ptr_to_const_type poly_geometry_on_sphere)
	{
		return PolyGeometryOnSphereArcsView<PolyGeometryOnSphereType>(poly_geometry_on_sphere);
	}

	template <class PolyGeometryOnSphereType>
	bool
	poly_geometry_on_sphere_contains_point(
			typename PolyGeometryOnSphereType::non_null_ptr_to_const_type poly_geometry_on_sphere,
			// There are from-python converters from LatLonPoint and sequence(latitude,longitude) and
			// sequence(x,y,z) to PointOnSphere so they will also get matched by this...
			const GPlatesMaths::PointOnSphere &point_on_sphere)
	{
		PolyGeometryOnSpherePointsView<PolyGeometryOnSphereType> points_view(poly_geometry_on_sphere);

		return points_view.contains_point(point_on_sphere);
	}

	//
	// Support for "__getitem__".
	//
	template <class PolyGeometryOnSphereType>
	boost::python::object
	poly_geometry_on_sphere_get_item(
			typename PolyGeometryOnSphereType::non_null_ptr_to_const_type poly_geometry_on_sphere,
			boost::python::object i)
	{
		PolyGeometryOnSpherePointsView<PolyGeometryOnSphereType> points_view(poly_geometry_on_sphere);

		return points_view.get_item(i);
	}

	GPlatesMaths::PointOnSphere
	polyline_on_sphere_get_centroid(
			const GPlatesMaths::PolylineOnSphere &polyline_on_sphere)
	{
		return GPlatesMaths::PointOnSphere(polyline_on_sphere.get_centroid());
	}

	GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PolylineOnSphere>
	polyline_on_sphere_to_tessellated(
			const GPlatesMaths::PolylineOnSphere &polyline_on_sphere,
			const double &tessellate_degrees)
	{
		// With boost 1.42 we get the following compile error...
		//   pointer_holder.hpp:145:66: error: invalid conversion from 'const void*' to 'void*'
		// ...if we return 'GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type' and rely on
		// 'python_ConstGeometryOnSphere' to convert for us - despite the fact that this conversion works
		// successfully for python bindings in other source files. It's likely due to 'bp::make_constructor'.
		//
		// So we avoid it by using returning a pointer to 'non-const' GPlatesMaths::PolylineOnSphere.
		return GPlatesUtils::const_pointer_cast<GPlatesMaths::PolylineOnSphere>(
				tessellate(
						polyline_on_sphere,
						GPlatesMaths::convert_deg_to_rad(tessellate_degrees)));
	}

	/**
	 * Enumeration to determine conversion of geometries to PolylineOnSphere.
	 */
	namespace PolylineConversion
	{
		enum Value
		{
			CONVERT_TO_POLYLINE,   // Arguments that are not a PolylineOnSphere are converted to one.
			IGNORE_NON_POLYLINE,   // Ignore arguments that are not a PolylineOnSphere - may result in no-op.
			RAISE_IF_NON_POLYLINE  // Raises GeometryTypeError if argument(s) is not a PolylineOnSphere.
		};
	};

	bp::object
	polyline_on_sphere_rotation_interpolate(
			const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &from_geometry_on_sphere,
			const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &to_geometry_on_sphere,
			// There are from-python converters from LatLonPoint and sequence(latitude,longitude) and
			// sequence(x,y,z) to PointOnSphere so they will also get matched by this...
			const GPlatesMaths::PointOnSphere &rotation_axis,
			bp::object interpolate_object,
			const double &minimum_latitude_overlap_radians,
			const double &maximum_latitude_non_overlap_radians,
			boost::optional<double> maximum_distance_threshold_radians,
			GPlatesMaths::FlattenLongitudeOverlaps::Value flatten_longitude_overlaps,
			PolylineConversion::Value polyline_conversion)
	{
		boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> from_polyline_on_sphere;
		boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> to_polyline_on_sphere;

		if (polyline_conversion == PolylineConversion::CONVERT_TO_POLYLINE)
		{
			from_polyline_on_sphere = GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type(
					polyline_on_sphere_create_from_geometry(
							*from_geometry_on_sphere,
							/*allow_one_point*/true));
			to_polyline_on_sphere = GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type(
					polyline_on_sphere_create_from_geometry(
							*to_geometry_on_sphere,
							/*allow_one_point*/true));
		}
		else
		{
			from_polyline_on_sphere = GPlatesAppLogic::GeometryUtils::get_polyline_on_sphere(*from_geometry_on_sphere);
			to_polyline_on_sphere = GPlatesAppLogic::GeometryUtils::get_polyline_on_sphere(*to_geometry_on_sphere);

			if (polyline_conversion == PolylineConversion::IGNORE_NON_POLYLINE)
			{
				// If either or both are not polylines then there is no overlap - return Py_None.
				if (!from_polyline_on_sphere ||
					!to_polyline_on_sphere)
				{
					return bp::object()/*Py_None*/;
				}
			}
			else // ...raise 'GeometryTypeError' if geometries are not polylines...
			{
				// Doing this enables user to try/except in case they don't know the types of the geometries.
				GPlatesGlobal::Assert<GeometryTypeException>(
						from_polyline_on_sphere && to_polyline_on_sphere,
						GPLATES_ASSERTION_SOURCE,
						"Expected PolylineOnSphere geometries");
			}
		}

		std::vector<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> interpolated_polylines;

		// Attempt to extract a single interpolate interval parameter.
		bp::extract<double> extract_interpolate_resolution_radians(interpolate_object);
		if (extract_interpolate_resolution_radians.check())
		{
			const double interpolate_resolution_radians =
					extract_interpolate_resolution_radians();

			if (!GPlatesMaths::interpolate(
					interpolated_polylines,
					from_polyline_on_sphere.get(),
					to_polyline_on_sphere.get(),
					rotation_axis.position_vector(),
					interpolate_resolution_radians,
					minimum_latitude_overlap_radians,
					maximum_latitude_non_overlap_radians,
					maximum_distance_threshold_radians,
					flatten_longitude_overlaps))
			{
				return bp::object()/*Py_None*/;
			}
		}
		else
		{
			// Attempt to extract a sequence of interpolate ratios.
			std::vector<double> interpolate_ratios;
			try
			{
				bp::stl_input_iterator<double>
						interpolate_ratio_seq_begin(interpolate_object),
						interpolate_ratio_seq_end;

				// Copy into a sequence.
				std::copy(interpolate_ratio_seq_begin, interpolate_ratio_seq_end, std::back_inserter(interpolate_ratios));
			}
			catch (const boost::python::error_already_set &)
			{
				PyErr_Clear();

				PyErr_SetString(PyExc_TypeError,
						"Expected an interpolate resolution (float) or "
						"a sequence of interpolate ratios (eg, list or tuple of float)");
				bp::throw_error_already_set();
			}

			if (!GPlatesMaths::interpolate(
					interpolated_polylines,
					from_polyline_on_sphere.get(),
					to_polyline_on_sphere.get(),
					rotation_axis.position_vector(),
					interpolate_ratios,
					minimum_latitude_overlap_radians,
					maximum_latitude_non_overlap_radians,
					maximum_distance_threshold_radians,
					flatten_longitude_overlaps))
			{
				return bp::object()/*Py_None*/;
			}
		}

		bp::list interpolated_polylines_list;

		BOOST_FOREACH(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type interpolated_polyline,
				interpolated_polylines)
		{
			interpolated_polylines_list.append(interpolated_polyline);
		}

		return interpolated_polylines_list;
	}
}


void
export_polyline_on_sphere()
{
	// An enumeration nested within 'pygplates (ie, current) module.
	bp::enum_<GPlatesMaths::FlattenLongitudeOverlaps::Value>("FlattenLongitudeOverlaps")
			.value("no", GPlatesMaths::FlattenLongitudeOverlaps::NO)
			.value("use_from", GPlatesMaths::FlattenLongitudeOverlaps::USE_FROM)
			.value("use_to", GPlatesMaths::FlattenLongitudeOverlaps::USE_TO);

	// An enumeration nested within 'pygplates (ie, current) module.
	bp::enum_<GPlatesApi::PolylineConversion::Value>("PolylineConversion")
			.value("convert_to_polyline", GPlatesApi::PolylineConversion::CONVERT_TO_POLYLINE)
			.value("ignore_non_polyline", GPlatesApi::PolylineConversion::IGNORE_NON_POLYLINE)
			.value("raise_if_non_polyline", GPlatesApi::PolylineConversion::RAISE_IF_NON_POLYLINE);

	//
	// A wrapper around view access to the *points* of a PolylineOnSphere.
	//
	// We don't document this wrapper (using docstrings) since it's documented in "PolylineOnSphere".
	bp::class_< GPlatesApi::PolyGeometryOnSpherePointsView<GPlatesMaths::PolylineOnSphere> >(
			"PolylineOnSpherePointsView",
			bp::no_init)
		.def("__iter__",
				bp::iterator< const GPlatesApi::PolyGeometryOnSpherePointsView<GPlatesMaths::PolylineOnSphere> >())
		.def("__len__",
				&GPlatesApi::PolyGeometryOnSpherePointsView<GPlatesMaths::PolylineOnSphere>::get_number_of_points)
		.def("__contains__",
				&GPlatesApi::PolyGeometryOnSpherePointsView<GPlatesMaths::PolylineOnSphere>::contains_point)
		.def("__getitem__",
				&GPlatesApi::PolyGeometryOnSpherePointsView<GPlatesMaths::PolylineOnSphere>::get_item)
	;

	//
	// A wrapper around view access to the *great circle arcs* of a PolylineOnSphere.
	//
	// We don't document this wrapper (using docstrings) since it's documented in "PolylineOnSphere".
	bp::class_< GPlatesApi::PolyGeometryOnSphereArcsView<GPlatesMaths::PolylineOnSphere> >(
			"PolylineOnSphereArcsView",
			bp::no_init)
		.def("__iter__",
				bp::iterator< const GPlatesApi::PolyGeometryOnSphereArcsView<GPlatesMaths::PolylineOnSphere> >())
		.def("__len__",
				&GPlatesApi::PolyGeometryOnSphereArcsView<GPlatesMaths::PolylineOnSphere>::get_number_of_arcs)
		.def("__contains__",
				&GPlatesApi::PolyGeometryOnSphereArcsView<GPlatesMaths::PolylineOnSphere>::contains_arc)
		.def("__getitem__",
				&GPlatesApi::PolyGeometryOnSphereArcsView<GPlatesMaths::PolylineOnSphere>::get_item)
	;

	//
	// PolylineOnSphere - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesMaths::PolylineOnSphere,
			// NOTE: See note in 'register_to_python_const_to_non_const_geometry_on_sphere_conversion()'
			// for why this is 'non-const' instead of 'const'...
			GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PolylineOnSphere>,
			bp::bases<GPlatesMaths::GeometryOnSphere>,
			boost::noncopyable>(
					"PolylineOnSphere",
					"Represents a polyline on the surface of the unit length sphere. "
					"Polylines are equality (``==``, ``!=``) comparable (but not hashable "
					"- cannot be used as a key in a ``dict``). See :class:`PointOnSphere` "
					"for an overview of equality in the presence of limited floating-point precision.\n"
					"\n"
					"A polyline instance is both:\n"
					"\n"
					"* a sequence of :class:`points<PointOnSphere>` - see :meth:`get_points`, and\n"
					"* a sequence of :class:`segments<GreatCircleArc>` (between adjacent points) "
					"- see :meth:`get_segments`.\n"
					"\n"
					"In addition a polyline instance is *directly* iterable over its points (without "
					"having to use :meth:`get_points`):\n"
					"::\n"
					"\n"
					"  polyline = pygplates.PolylineOnSphere(points)\n"
					"  for point in polyline:\n"
					"      ...\n"
					"\n"
					"...and so the following operations for accessing the points are supported:\n"
					"\n"
					"=========================== ==========================================================\n"
					"Operation                   Result\n"
					"=========================== ==========================================================\n"
					"``len(polyline)``           number of vertices in *polyline*\n"
					"``for p in polyline``       iterates over the vertices *p* of *polyline*\n"
					"``p in polyline``           ``True`` if *p* is equal to a **vertex** of *polyline*\n"
					"``p not in polyline``       ``False`` if *p* is equal to a **vertex** of *polyline*\n"
					"``polyline[i]``             the vertex of *polyline* at index *i*\n"
					"``polyline[i:j]``           slice of *polyline* from *i* to *j*\n"
					"``polyline[i:j:k]``         slice of *polyline* from *i* to *j* with step *k*\n"
					"=========================== ==========================================================\n"
					"\n"
					"| Since a *PolylineOnSphere* is **immutable** it contains no operations or "
					"methods that modify its state (such as adding or removing points). "
					"This is similar to other immutable types in python such as ``str``.\n"
					"| So instead of modifying an existing polyline you will need to create a new "
					":class:`PolylineOnSphere` instance as the following example demonstrates:\n"
					"\n"
					"::\n"
					"\n"
					"  # Get a list of points from an existing PolylineOnSphere 'polyline'.\n"
					"  points = list(polyline)\n"
					"\n"
					"  # Modify the points list somehow.\n"
					"  points[0] = pygplates.PointOnSphere(...)\n"
					"  points.append(pygplates.PointOnSphere(...))\n"
					"\n"
					"  # 'polyline' now references a new PolylineOnSphere instance.\n"
					"  polyline = pygplates.PolylineOnSphere(points)\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::poly_geometry_on_sphere_create_from_points<GPlatesMaths::PolylineOnSphere>,
						bp::default_call_policies(),
						(bp::arg("points"))),
				// General overloaded signature (must be in first overloaded 'def' - used by Sphinx)...
				// Specific overload signature...
				"__init__(...)\n"
				"A *PolylineOnSphere* object can be constructed in more than one way...\n"
				"\n"
				// Specific overload signature...
				"__init__(points)\n"
				"  Create a polyline from a sequence of (x,y,z) or (latitude,longitude) points.\n"
				"\n"
				"  :param points: A sequence of (x,y,z) points, or (latitude,longitude) points (in degrees).\n"
				"  :type points: Any sequence of :class:`PointOnSphere` or :class:`LatLonPoint` or "
				"tuple (float,float,float) or tuple (float,float)\n"
				"  :raises: InvalidLatLonError if any *latitude* or *longitude* is invalid\n"
				"  :raises: ViolatedUnitVectorInvariantError if any (x,y,z) is not unit magnitude\n"
				"  :raises: InvalidPointsForPolylineConstructionError if sequence has less than two points "
				"or if any two points (adjacent in the *points* sequence) are antipodal to each other "
				"(on opposite sides of the globe)\n"
				"\n"
				"  .. note:: The sequence must contain at least two points in order to be a valid "
				"polyline, otherwise *InvalidPointsForPolylineConstructionError* will be raised.\n"
				"\n"
				"  During creation, a :class:`GreatCircleArc` is created between each adjacent pair of "
				"points in *points* - see :meth:`get_segments`.\n"
				"\n"
				"  It is *not* an error for adjacent points in the sequence to be coincident. In this "
				"case each :class:`GreatCircleArc` between two such adjacent points will have zero length "
				"(:meth:`GreatCircleArc.is_zero_length` will return ``True``) and will have no "
				"rotation axis (:meth:`GreatCircleArc.get_rotation_axis` will raise an error). "
				"However if two such adjacent points are antipodal (on opposite sides of the globe) "
				"then InvalidPointsForPolylineConstructionError will be raised.\n"
				"\n"
				"  The following example shows a few different ways to create a :class:`polyline<PolylineOnSphere>`:\n"
				"  ::\n"
				"\n"
				"    points = []\n"
				"    points.append(pygplates.PointOnSphere(...))\n"
				"    points.append(pygplates.PointOnSphere(...))\n"
				"    points.append(pygplates.PointOnSphere(...))\n"
				"    polyline = pygplates.PolylineOnSphere(points)\n"
				"    \n"
				"    points = []\n"
				"    points.append((lat1,lon1))\n"
				"    points.append((lat2,lon2))\n"
				"    points.append((lat3,lon3))\n"
				"    polyline = pygplates.PolylineOnSphere(points)\n"
				"    \n"
				"    points = []\n"
				"    points.append([x1,y1,z1])\n"
				"    points.append([x2,y2,z2])\n"
				"    points.append([x3,y3,z3])\n"
				"    polyline = pygplates.PolylineOnSphere(points)\n"
				"\n"
				"  If you have latitude/longitude values but they are not a sequence of tuples or "
				"if the latitude/longitude order is swapped then the following examples demonstrate "
				"how you could restructure them:\n"
				"  ::\n"
				"\n"
				"    # Flat lat/lon array.\n"
				"    points = numpy.array([lat1, lon1, lat2, lon2, lat3, lon3])\n"
				"    polyline = pygplates.PolylineOnSphere(zip(points[::2],points[1::2]))\n"
				"    \n"
				"    # Flat lon/lat list (ie, different latitude/longitude order).\n"
				"    points = [lon1, lat1, lon2, lat2, lon3, lat3]\n"
				"    polyline = pygplates.PolylineOnSphere(zip(points[1::2],points[::2]))\n"
				"    \n"
				"    # Separate lat/lon arrays.\n"
				"    lats = numpy.array([lat1, lat2, lat3])\n"
				"    lons = numpy.array([lon1, lon2, lon3])\n"
				"    polyline = pygplates.PolylineOnSphere(zip(lats,lons))\n"
				"    \n"
				"    # Lon/lat list of tuples (ie, different latitude/longitude order).\n"
				"    points = [(lon1, lat1), (lon2, lat2), (lon3, lat3)]\n"
				"    polyline = pygplates.PolylineOnSphere([(lat,lon) for lon, lat in points])\n")
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::polyline_on_sphere_create_from_geometry,
						bp::default_call_policies(),
						(bp::arg("geometry"),
								bp::arg("allow_one_point") = true)),
				// Specific overload signature...
				"__init__(geometry, [allow_one_point=True])\n"
				"  Create a polyline from a :class:`GeometryOnSphere`.\n"
				"\n"
				"  :param geometry: The point, multi-point, polyline or polygon geometry to convert from.\n"
				"  :type geometry: :class:`GeometryOnSphere`\n"
				"  :param allow_one_point: Whether *geometry* is allowed to be a :class:`PointOnSphere` or "
				"a :class:`MultiPointOnSphere` containing only a single point - if allowed then that single "
				"point is duplicated since a PolylineOnSphere requires at least two points - "
				"default is ``True``.\n"
				"  :type allow_one_point: bool\n"
				"  :raises: InvalidPointsForPolylineConstructionError if *geometry* is a "
				":class:`PointOnSphere` (and *allow_one_point* is ``False``), or a "
				":class:`MultiPointOnSphere` with one point (and *allow_one_point* is ``False``), or "
				"if any two consecutive points in a :class:`MultiPointOnSphere` are antipodal to each "
				"other (on opposite sides of the globe)\n"
				"\n"
				"  If *allow_one_point* is ``True`` then *geometry* can be :class:`PointOnSphere`, "
				":class:`MultiPointOnSphere`, :class:`PolylineOnSphere` or :class:`PolygonOnSphere`. "
				"However if *allow_one_point* is ``False`` then *geometry* must be a :class:`PolylineOnSphere`, "
				"or a :class:`PolygonOnSphere`, or a :class:`MultiPointOnSphere` containing at least "
				"two points to avoid raising *InvalidPointsForPolylineConstructionError*.\n"
				"\n"
				"  During creation, a :class:`GreatCircleArc` is created between each adjacent pair of "
				"geometry points - see :meth:`get_segments`.\n"
				"\n"
				"  It is *not* an error for adjacent points in a geometry sequence to be coincident. In this "
				"case each :class:`GreatCircleArc` between two such adjacent points will have zero length "
				"(:meth:`GreatCircleArc.is_zero_length` will return ``True``) and will have no "
				"rotation axis (:meth:`GreatCircleArc.get_rotation_axis` will raise an error). "
				"However if two such adjacent points are antipodal (on opposite sides of the globe) "
				"then InvalidPointsForPolylineConstructionError will be raised\n"
				"\n"
				"  To create a PolylineOnSphere from any geometry type:\n"
				"  ::\n"
				"\n"
				"    polyline = pygplates.PolylineOnSphere(geometry)\n"
				"\n"
				"  To create a PolylineOnSphere from any geometry containing at least two points:\n"
				"  ::\n"
				"\n"
				"    try:\n"
				"        polyline = pygplates.PolylineOnSphere(geometry, allow_one_point=False)\n"
				"    except pygplates.InvalidPointsForPolylineConstructionError:\n"
				"        ... # Handle failure to convert 'geometry' to a PolylineOnSphere.\n")
		.def("rotation_interpolate",
				&GPlatesApi::polyline_on_sphere_rotation_interpolate,
				(bp::arg("from_polyline"), bp::arg("to_polyline"),
						bp::arg("rotation_pole"),
						bp::arg("interpolate"),
						bp::arg("minimum_latitude_overlap_radians") = 0,
						bp::arg("maximum_latitude_non_overlap_radians") = 0,
						bp::arg("maximum_distance_threshold_radians") = boost::optional<double>(),
						bp::arg("flatten_longitude_overlaps") = GPlatesMaths::FlattenLongitudeOverlaps::NO,
						bp::arg("polyline_conversion") = GPlatesApi::PolylineConversion::IGNORE_NON_POLYLINE),
				"rotation_interpolate(from_polyline, to_polyline, rotation_pole, "
				"interpolate, "
				"[minimum_latitude_overlap_radians=0], "
				"[maximum_latitude_non_overlap_radians=0], "
				"[maximum_distance_threshold_radians], "
				"[flatten_longitude_overlaps=FlattenLongitudeOverlaps.no], "
				"[polyline_conversion=PolylineConversion.ignore_non_polyline])\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Interpolates between two polylines about a rotation pole.\n"
				"\n"
				"  :param from_polyline: the polyline to interpolate *from*\n"
				"  :type from_polyline: :class:`GeometryOnSphere`\n"
				"  :param to_polyline: the polyline to interpolate *to*\n"
				"  :type to_polyline: :class:`GeometryOnSphere`\n"
				"  :param rotation_pole: the rotation axis to interpolate around\n"
				"  :type rotation_pole: :class:`PointOnSphere` or :class:`LatLonPoint` or tuple (latitude,longitude)"
				", in degrees, or tuple (x,y,z)\n"
				"  :param interpolate: if a single number then *interpolate* is the interval spacing, in radians, "
				"between *from_polyline* and *to_polyline* at which to generate interpolated polylines - "
				"otherwise if a sequence of numbers (eg, list or tuple) then *interpolate* is the sequence of "
				"interpolate ratios, in the range [0,1], at which to generate interpolated polylines "
				"(with 0 meaning *from_polyline* and 1 meaning *to_polyline*)\n"
				"  :type interpolate: float, or list of float\n"
				"  :param minimum_latitude_overlap_radians: required amount of latitude overlap of polylines\n"
				"  :type minimum_latitude_overlap_radians: float - defaults to zero\n"
				"  :param maximum_latitude_non_overlap_radians: allowed non-overlapping latitude region\n"
				"  :type maximum_latitude_non_overlap_radians: float - defaults to zero\n"
				"  :param maximum_distance_threshold_radians: maximum distance (in radians) between "
				"*from_polyline* and *to_polyline* - if specified and if exceeded then ``None`` is returned\n"
				"  :type maximum_distance_threshold_radians: float - default is no threshold detection\n"
				"  :param flatten_longitude_overlaps: whether or not to ensure *from_polyline* and *to_polyline* "
				"do not overlap in longitude (in North pole reference frame of *rotation_pole*) and how to "
				"correct the overlap\n"
				"  :type flatten_longitude_overlaps: *FlattenLongitudeOverlaps.no*, *FlattenLongitudeOverlaps.use_from* "
				"or *FlattenLongitudeOverlaps.use_to* - defaults to *FlattenLongitudeOverlaps.no*\n"
				"  :param polyline_conversion: whether to raise error, convert to :class:`PolylineOnSphere` or ignore "
				"*from_polyline* and *to_polyline* if they are not :class:`PolylineOnSphere` (ignoring equates "
				"to returning ``None``) - defaults to *PolylineConversion.ignore_non_polyline*\n"
				"  :type polyline_conversion: *PolylineConversion.convert_to_polyline*, "
				"*PolylineConversion.ignore_non_polyline* or *PolylineConversion.raise_if_non_polyline*\n"
				"  :returns: list of interpolated polylines - or ``None`` if polylines do not have overlapping "
				"latitude ranges or if maximum distance threshold exceeded or if either polyline is not a "
				":class:`PolylineOnSphere` (and *polyline_conversion* is *PolylineConversion.ignore_non_polyline*)\n"
				"  :rtype: list of :class:`PolylineOnSphere` or None\n"
				"  :raises: GeometryTypeError if *from_polyline* or *to_polyline* are not of type "
				":class:`PolylineOnSphere` (and *polyline_conversion* is *PolylineConversion.raise_if_non_polyline*)\n"
				"\n"
				"  If *interpolate* is a single number then it is the distance interval spacing, in radians, "
				"between *from_polyline* and *to_polyline* at which to generate interpolated polylines. "
				"Also modified versions of *from_polyline* and *to_polyline* are returned along with the "
				"interpolated polylines.\n"
				"\n"
				"  If *interpolate* is a sequence of numbers (eg, list or tuple) then it is the sequence of "
				"interpolate ratios, in the range [0,1], at which to generate interpolated polylines "
				"(with 0 meaning *from_polyline* and 1 meaning *to_polyline* and values between meaning "
				"interpolated polylines).\n"
				"\n"
				"  The points in the returned polylines are ordered from closest (latitude) to *rotation_pole* "
				"to furthest (which may be different than the order in the original polylines). The modified "
				"versions of polylines *from_polyline* and *to_polyline*, and hence all interpolated polylines, "
				"have monotonically decreasing latitudes (in North pole reference frame of *rotation_pole*) "
				"starting with the northmost polyline end-point and (monotonically) decreasing southward such "
				"that subsequent points have latitudes lower than, or equal to, all previous points as shown "
				"in the following diagram:\n"
				"  ::\n"
				"\n"
				"     /|\n"
				"    / |\n"
				"   /  |                       ___\n"
				"      |                          |\n"
				"      | /|                       |\n"
				"      |/ |                       |__\n"
				"         |                          |\n"
				"         |           ===>           |\n"
				"         | /|                       |\n"
				"         |/ |                       |__\n"
				"            |                          |\n"
				"             \\                          \\\n"
				"              \\                          \\\n"
				"               |                          |\n"
				"               | /                        |\n"
				"               |/                         |__\n"
				"\n"
				"  The modified versions of polylines *from_polyline* and *to_polyline* are also clipped "
				"to have a common overlapping latitude range (with a certain amount of non-overlapping "
				"allowed if *max_latitude_non_overlap_radians* is non-zero).\n"
				"\n"
				"  *minimum_latitude_overlap_radians* specifies the amount that *from_polyline* and "
				"*to_polyline* must overlap in latitude (North pole reference frame of *rotation_pole*), "
				"otherwise ``None`` is returned. Note that this also means if the range of latitudes "
				"of either polyline is smaller than the minimum overlap then ``None`` is returned. "
				"The following diagram shows the original latitude overlapping polylines on the left "
				"and the resultant interpolated polylines on the right clipped to the latitude "
				"overlapping range (in *rotation_pole* reference frame):\n"
				"  ::\n"
				"\n"
				"    |_\n"
				"      |\n"
				"      |         |                         |    |    |\n"
				"      |_        |_          ===>          |_   |_   |_\n"
				"        |         |                         |    |    |\n"
				"        |         |                         |    |    |\n"
				"                  |_\n"
				"                    |\n"
				"\n"
				"  However problems can arise if *rotation_pole* is placed such that one, or both, the "
				"original polylines (*from_polyline* and *to_polyline*) strongly overlaps itself "
				"(in *rotation_pole* reference frame) causing the monotonically-decreasing-latitude "
				"requirement to severely distort its geometry. The following diagram shows the original "
				"polylines in the top of the diagram and the resultant interpolated polylines in the "
				"bottom of the diagram:\n"
				"  ::\n"
				"\n"
				"                                 \\\n"
				"                                  \\\n"
				"             ______                \\\n"
				"        ____|      |____            \\\n"
				"     __|                |__          \\\n"
				"    /                      \\          \\\n"
				"                            \\          \\\n"
				"                             |          |\n"
				"                             |          |\n"
				"                             |          |\n"
				"\n"
				"                 ||\n"
				"                 ||\n"
				"                 \\/\n"
				"\n"
				"    ______________________________\n"
				"                            \\     \\    \\\n"
				"                             |     |    |\n"
				"                             |     |    |\n"
				"                             |     |    |\n"
				"\n"
				"  If *maximum_latitude_non_overlap_radians* is non-zero then an extra range of non-overlapping "
				"latitudes at the North and South (in *rotation_pole* reference frame) of *from_polyline* and "
				"*to_polyline* is allowed. "
				"The following diagram shows the original latitude overlapping polylines on the left and the "
				"resultant interpolated polylines on the right with a limited amount of non-overlapping "
				"interpolation from the North end of one polyline and from the South end of the other "
				"(in *rotation_pole* reference frame):\n"
				"  ::\n"
				"\n"
				"      |\n"
				"      |\n"
				"      |                                   |\n"
				"      |                                   |    |\n"
				"      |         |                         |    |    |\n"
				"      |_        |_          ===>          |_   |_   |_\n"
				"        |         |                         |    |    |\n"
				"        |         |                         |    |    |\n"
				"                  |                              |    |\n"
				"                  |                                   |\n"
				"                  |\n"
				"                  |\n"
				"\n"
				"  If *flatten_longitude_overlaps* is *FlattenLongitudeOverlaps.use_from* or "
				"*FlattenLongitudeOverlaps.use_to* then this function ensures the longitudes of each point pair "
				"of *from_polyline* and *to_polyline* (in North pole reference frame of *rotation_pole*) "
				"at the same latitude don't overlap. For those point pairs where overlap occurs, the points "
				"in *from_polyline* are copied to the corresponding (same latitude) points in *to_polyline* "
				"if *FlattenLongitudeOverlaps.use_from* is used (and vice versa if *FlattenLongitudeOverlaps.use_to* "
				"is used). This essentially removes or flattens overlaps in longitude. The following diagram "
				"shows the original longitude overlapping polylines on the left and the resultant interpolated "
				"polylines on the right (in *rotation_pole* reference frame) after longitude flattening with "
				"*FlattenLongitudeOverlaps.use_from*:\n"
				"  ::\n"
				"\n"
				"     from     to\n"
				"       \\     /                             \\  |  /\n"
				"        \\   /                               \\ | /\n"
				"         \\ /                                 \\|/\n"
				"          .                                   .\n"
				"         / \\                                   \\\n"
				"        /   \\                                   \\\n"
				"       /     \\                                   \\\n"
				"      |       |                                   |\n"
				"      |       |           ===>                    |\n"
				"      |       |                                   |\n"
				"       \\     /                                   /\n"
				"        \\   /                                   /\n"
				"         \\ /                                   /\n"
				"          .                                   .\n"
				"         / \\                                 /|\\\n"
				"        /   \\                               / | \\\n"
				"       /     \\                             /  |  \\\n"
				"      /       \\                           /   |   \\\n"
				"\n"
				"  Returns ``None`` if:\n"
				"\n"
				"  * the polylines do not overlap by at least *minimum_latitude_overlap_radians* "
				"radians (where North pole is *rotation_axis*), or\n"
				"  * any corresponding pair of points (same latitude) of the polylines are separated by a "
				"distance of more than *max_distance_threshold_radians* (if specified).\n"
				"\n"
				"  To interpolate polylines with a spacing of 2 minutes (with a minimum required latitude "
				"overlap of 1 degree and with an allowed latitude non-overlap of up to 3 degrees and "
				"with no distance threshold and with no longitude overlaps flattened):\n"
				"  ::\n"
				"\n"
				"    interpolated_polylines = pygplates.PolylineOnSphere.rotation_interpolate(\n"
				"        from_polyline, to_polyline, rotation_pole, math.radians(2.0/60), "
				"math.radians(1), math.radians(3))\n"
				"\n"
				"  To interpolate polylines at interpolate ratios between 0 and 1 at 0.1 intervals "
				"(with a minimum required latitude overlap of 1 degree and with an allowed latitude "
				"non-overlap of up to 3 degrees and with no distance threshold and with no longitude "
				"overlaps flattened):\n"
				"  ::\n"
				"\n"
				"    interpolated_polylines = pygplates.PolylineOnSphere.rotation_interpolate(\n"
				"        from_polyline, to_polyline, rotation_pole, range(0, 1.01, 0.1), "
				"math.radians(1), math.radians(3))\n"
				"\n"
				"  An easy way to test whether two polylines can possibly be interpolated without "
				"actually interpolating anything is to specify an empty list of interpolate ratios:\n"
				"  ::\n"
				"\n"
				"    if pygplates.PolylineOnSphere.rotation_interpolate(\n"
				"            from_polyline, to_polyline, rotation_pole, [], ...) is not None:\n"
				"        # 'from_polyline' and 'to_polyline' can be interpolated (ie, they overlap\n"
				"        # and don't exceed the maximum distance threshold)\n"
				"        ...\n")
		.staticmethod("rotation_interpolate")
		// To be deprecated in favour of 'get_points()'...
		.def("get_points_view",
				&GPlatesApi::poly_geometry_on_sphere_get_points_view<GPlatesMaths::PolylineOnSphere>,
				"get_points_view()\n"
				"  To be **deprecated** - use :meth:`get_points` instead.\n")
		.def("get_segments",
				&GPlatesApi::poly_geometry_on_sphere_get_arcs_view<GPlatesMaths::PolylineOnSphere>,
				"get_segments()\n"
				"  Returns a *read-only* sequence of :class:`segments<GreatCircleArc>` in this polyline.\n"
				"\n"
				"  :rtype: a read-only sequence of :class:`GreatCircleArc`\n"
				"\n"
				"  The following operations for accessing the great circle arcs in the returned "
				"read-only sequence are supported:\n"
				"\n"
				"  =========================== ==========================================================\n"
				"  Operation                   Result\n"
				"  =========================== ==========================================================\n"
				"  ``len(seq)``                number of segments of the polyline\n"
				"  ``for s in seq``            iterates over the segments *s* of the polyline\n"
				"  ``s in seq``                ``True`` if *s* is an segment of the polyline\n"
				"  ``s not in seq``            ``False`` if *s* is an segment of the polyline\n"
				"  ``seq[i]``                  the segment of the polyline at index *i*\n"
				"  ``seq[i:j]``                slice of segments of the polyline from *i* to *j*\n"
				"  ``seq[i:j:k]``              slice of segments of the polyline from *i* to *j* with step *k*\n"
				"  =========================== ==========================================================\n"
				"\n"
				"  .. note:: Between each adjacent pair of :class:`points<PointOnSphere>` there "
				"is an :class:`segment<GreatCircleArc>` such that the number of points exceeds the number of "
				"segments by one.\n"
				"\n"
				"  The following example demonstrates some uses of the above operations:\n"
				"  ::\n"
				"\n"
				"    polyline = pygplates.PolylineOnSphere(points)\n"
				"    ...\n"
				"    segments = polyline.get_segments()\n"
				"    for segment in segments:\n"
				"        if not segment.is_zero_length():\n"
				"            segment_midpoint_direction = segment.get_arc_direction(0.5)\n"
				"    first_segment = segments[0]\n"
				"    last_segment = segments[-1]\n"
				"\n"
				"  .. note:: The returned sequence is *read-only* and cannot be modified.\n"
				"\n"
				"  .. note:: If you want a modifiable sequence consider wrapping the returned sequence in a ``list`` "
				"using something like ``segments = list(polyline.get_segments())`` **but** note that modifying "
				"the ``list`` (eg, appending a new segment) will **not** modify the original polyline.\n")
		// To be deprecated in favour of 'get_segments()'...
		.def("get_great_circle_arcs_view",
				&GPlatesApi::poly_geometry_on_sphere_get_arcs_view<GPlatesMaths::PolylineOnSphere>,
				"get_great_circle_arcs_view()\n"
				"  To be **deprecated** - use :meth:`get_segments` instead.\n")
		.def("get_arc_length",
				&GPlatesMaths::PolylineOnSphere::get_arc_length,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_arc_length()\n"
				"  Returns the total arc length of this polyline (in radians).\n"
				"\n"
				"  :rtype: float\n"
				"\n"
				"  | This is the sum of the arc lengths of the :meth:`segments<get_segments>` of this polyline.\n"
				"  | To convert to distance, multiply the result by the Earth radius (see :class:`Earth`).\n")
		.def("get_centroid",
				&GPlatesApi::polyline_on_sphere_get_centroid,
				"get_centroid()\n"
				"  Returns the centroid of this polyline.\n"
				"\n"
				"  :rtype: :class:`PointOnSphere`\n"
				"\n"
				"  The centroid is calculated as a weighted average of the mid-points of the "
				":class:`great circle arcs<GreatCircleArc>` of this polyline with weighting "
				"proportional to the individual arc lengths.\n")
		.def("to_tessellated",
				&GPlatesApi::polyline_on_sphere_to_tessellated,
				(bp::arg("tessellate_degrees")),
				"to_tessellated(tessellate_degrees)\n"
				"  Returns a new polyline that is tessellated version of this polyline.\n"
				"\n"
				"  :param tessellate_degrees: maximum tessellation angle (in degrees)\n"
				"  :type tessellate_degrees: float\n"
				"\n"
				"  Adjacent points (in the returned tessellated polyline) are separated by no more than "
				"*tessellate_degrees* on the globe.\n"
				"\n"
				"  Create a polyline tessellated to 2 degrees:\n"
				"  ::\n"
				"\n"
				"    tessellated_polyline = polyline.to_tessellated(2)\n"
				"\n"
				"  .. note: The distance between adjacent points (in the tessellated polyline) will not be exactly "
				"*uniform*. This is because each :class:`segment<GreatCircleArc>` in the original polyline is "
				"tessellated to the nearest integer number of points (that keeps that segment under the threshold) "
				"and hence each original *segment* will have a slightly different tessellation angle.\n")
		.def("__iter__",
				bp::range(
						&GPlatesMaths::PolylineOnSphere::vertex_begin,
						&GPlatesMaths::PolylineOnSphere::vertex_end))
		.def("__len__", &GPlatesMaths::PolylineOnSphere::number_of_vertices)
		.def("__contains__", &GPlatesApi::poly_geometry_on_sphere_contains_point<GPlatesMaths::PolylineOnSphere>)
		.def("__getitem__", &GPlatesApi::poly_geometry_on_sphere_get_item<GPlatesMaths::PolylineOnSphere>)
		// Due to the numerical tolerance in comparisons we cannot make hashable.
		// Make unhashable, with no *equality* comparison operators (we explicitly define them)...
		.def(GPlatesApi::NoHashDefVisitor(false, true))
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
	;

	// Register to-python conversion for PolylineOnSphere::non_null_ptr_to_const_type.
	GPlatesApi::PythonConverterUtils::register_to_python_const_to_non_const_non_null_intrusive_ptr_conversion<
			GPlatesMaths::PolylineOnSphere>();
	// Register from-python 'non-const' to 'const' conversion.
	boost::python::implicitly_convertible<
			GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PolylineOnSphere>,
			GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PolylineOnSphere> >();

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Note that we're only dealing with 'const' conversions here.
	// The 'non-const' workaround to get boost-python to compile is handled by 'python_ConstGeometryOnSphere'.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			const GPlatesMaths::PolylineOnSphere,
			const GPlatesMaths::GeometryOnSphere>();
}


namespace GPlatesApi
{
	// Create a PolygonOnSphere from a GeometryOnSphere.
	GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PolygonOnSphere>
	polygon_on_sphere_create_from_geometry(
			const GPlatesMaths::GeometryOnSphere &geometry,
			bool allow_one_or_two_points)
	{
		boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> polygon =
				GPlatesAppLogic::GeometryUtils::convert_geometry_to_polygon(geometry);
		if (!polygon)
		{
			// There were less than three points.
			// 
			// Retrieve the points.
			std::vector<GPlatesMaths::PointOnSphere> geometry_points;
			GPlatesAppLogic::GeometryUtils::get_geometry_points(geometry, geometry_points);

			// There should be one or two points.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					!geometry_points.empty(),
					GPLATES_ASSERTION_SOURCE);

			// Duplicate the last point until we have three points if caller allows one or two points,
			// otherwise let 'PolygonOnSphere::create_on_heap()' throw InvalidPointsForPolygonConstructionError.
			if (allow_one_or_two_points)
			{
				while (geometry_points.size() < 3)
				{
					geometry_points.push_back(geometry_points.back());
				}
			}

			polygon = GPlatesMaths::PolygonOnSphere::create_on_heap(geometry_points);
		}

		// With boost 1.42 we get the following compile error...
		//   pointer_holder.hpp:145:66: error: invalid conversion from 'const void*' to 'void*'
		// ...if we return 'GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type' and rely on
		// 'python_ConstGeometryOnSphere' to convert for us - despite the fact that this conversion works
		// successfully for python bindings in other source files. It's likely due to 'bp::make_constructor'.
		//
		// So we avoid it by returning a pointer to 'non-const' PolygonOnSphere.
		return GPlatesUtils::const_pointer_cast<GPlatesMaths::PolygonOnSphere>(polygon.get());
	}

	bool
	polygon_on_sphere_is_point_in_polygon(
			const GPlatesMaths::PolygonOnSphere &polygon_on_sphere,
			// There are from-python converters from LatLonPoint and sequence(latitude,longitude) and
			// sequence(x,y,z) to PointOnSphere so they will also get matched by this...
			const GPlatesMaths::PointOnSphere &point)
	{
		return polygon_on_sphere.is_point_in_polygon(point);
	}

	GPlatesMaths::PointOnSphere
	polygon_on_sphere_get_boundary_centroid(
			const GPlatesMaths::PolygonOnSphere &polygon_on_sphere)
	{
		return GPlatesMaths::PointOnSphere(polygon_on_sphere.get_boundary_centroid());
	}

	GPlatesMaths::PointOnSphere
	polygon_on_sphere_get_interior_centroid(
			const GPlatesMaths::PolygonOnSphere &polygon_on_sphere)
	{
		return GPlatesMaths::PointOnSphere(polygon_on_sphere.get_interior_centroid());
	}

	GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PolygonOnSphere>
	polygon_on_sphere_to_tessellated(
			const GPlatesMaths::PolygonOnSphere &polygon_on_sphere,
			const double &tessellate_degrees)
	{
		// With boost 1.42 we get the following compile error...
		//   pointer_holder.hpp:145:66: error: invalid conversion from 'const void*' to 'void*'
		// ...if we return 'GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type' and rely on
		// 'python_ConstGeometryOnSphere' to convert for us - despite the fact that this conversion works
		// successfully for python bindings in other source files. It's likely due to 'bp::make_constructor'.
		//
		// So we avoid it by using returning a pointer to 'non-const' GPlatesMaths::PolygonOnSphere.
		return GPlatesUtils::const_pointer_cast<GPlatesMaths::PolygonOnSphere>(
				tessellate(
						polygon_on_sphere,
						GPlatesMaths::convert_deg_to_rad(tessellate_degrees)));
	}
}


void
export_polygon_on_sphere()
{
	//
	// A wrapper around view access to the *points* of a PolygonOnSphere.
	//
	// We don't document this wrapper (using docstrings) since it's documented in "PolygonOnSphere".
	bp::class_< GPlatesApi::PolyGeometryOnSpherePointsView<GPlatesMaths::PolygonOnSphere> >(
			"PolygonOnSpherePointsView",
			bp::init<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type>())
		.def("__iter__",
				bp::iterator< const GPlatesApi::PolyGeometryOnSpherePointsView<GPlatesMaths::PolygonOnSphere> >())
		.def("__len__",
				&GPlatesApi::PolyGeometryOnSpherePointsView<GPlatesMaths::PolygonOnSphere>::get_number_of_points)
		.def("__contains__",
				&GPlatesApi::PolyGeometryOnSpherePointsView<GPlatesMaths::PolygonOnSphere>::contains_point)
		.def("__getitem__",
				&GPlatesApi::PolyGeometryOnSpherePointsView<GPlatesMaths::PolygonOnSphere>::get_item)
	;

	//
	// A wrapper around view access to the *great circle arcs* of a PolygonOnSphere.
	//
	// We don't document this wrapper (using docstrings) since it's documented in "PolygonOnSphere".
	bp::class_< GPlatesApi::PolyGeometryOnSphereArcsView<GPlatesMaths::PolygonOnSphere> >(
			"PolygonOnSphereArcsView",
			bp::init<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type>())
		.def("__iter__",
				bp::iterator< const GPlatesApi::PolyGeometryOnSphereArcsView<GPlatesMaths::PolygonOnSphere> >())
		.def("__len__",
				&GPlatesApi::PolyGeometryOnSphereArcsView<GPlatesMaths::PolygonOnSphere>::get_number_of_arcs)
		.def("__contains__",
				&GPlatesApi::PolyGeometryOnSphereArcsView<GPlatesMaths::PolygonOnSphere>::contains_arc)
		.def("__getitem__",
				&GPlatesApi::PolyGeometryOnSphereArcsView<GPlatesMaths::PolygonOnSphere>::get_item)
	;

	//
	// PolygonOnSphere - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::scope polygon_on_sphere_class = bp::class_<
			GPlatesMaths::PolygonOnSphere,
			// NOTE: See note in 'register_to_python_const_to_non_const_geometry_on_sphere_conversion()'
			// for why this is 'non-const' instead of 'const'...
			GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PolygonOnSphere>,
			bp::bases<GPlatesMaths::GeometryOnSphere>,
			boost::noncopyable>(
					"PolygonOnSphere",
					"Represents a polygon on the surface of the unit length sphere. "
					"Polygons are equality (``==``, ``!=``) comparable (but not hashable "
					"- cannot be used as a key in a ``dict``). See :class:`PointOnSphere` "
					"for an overview of equality in the presence of limited floating-point precision.\n"
					"\n"
					"A polygon instance is both:\n"
					"\n"
					"* a sequence of :class:`points<PointOnSphere>` - see :meth:`get_points`, and\n"
					"* a sequence of :class:`segments<GreatCircleArc>` (between adjacent points) "
					"- see :meth:`get_segments`.\n"
					"\n"
					"In addition a polygon instance is *directly* iterable over its points (without "
					"having to use :meth:`get_points`):\n"
					"::\n"
					"\n"
					"  polygon = pygplates.PolygonOnSphere(points)\n"
					"  for point in polygon:\n"
					"      ...\n"
					"\n"
					"...and so the following operations for accessing the points are supported:\n"
					"\n"
					"=========================== ==========================================================\n"
					"Operation                   Result\n"
					"=========================== ==========================================================\n"
					"``len(polygon)``            number of vertices in *polygon*\n"
					"``for p in polygon``        iterates over the vertices *p* of *polygon*\n"
					"``p in polygon``            ``True`` if *p* is equal to a **vertex** of *polygon*\n"
					"``p not in polygon``        ``False`` if *p* is equal to a **vertex** of *polygon*\n"
					"``polygon[i]``              the vertex of *polygon* at index *i*\n"
					"``polygon[i:j]``            slice of *polygon* from *i* to *j*\n"
					"``polygon[i:j:k]``          slice of *polygon* from *i* to *j* with step *k*\n"
					"=========================== ==========================================================\n"
					"\n"
					".. note:: ``if p in polygon`` does **not** test whether a point ``p`` is *inside* the "
					"the *interior* of a polygon - use :meth:`is_point_in_polygon` for that instead.\n"
					"\n"
					"| Since a *PolygonOnSphere* is **immutable** it contains no operations or "
					"methods that modify its state (such as adding or removing points). "
					"This is similar to other immutable types in python such as ``str``.\n"
					"| So instead of modifying an existing polygon you will need to create a new "
					":class:`PolygonOnSphere` instance as the following example demonstrates:\n"
					"\n"
					"::\n"
					"\n"
					"  # Get a list of points from an existing 'polygon'.\n"
					"  points = list(polygon)\n"
					"\n"
					"  # Modify the points list somehow.\n"
					"  points[0] = pygplates.PointOnSphere(...)\n"
					"  points.append(pygplates.PointOnSphere(...))\n"
					"\n"
					"  # 'polygon' now references a new PolygonOnSphere instance.\n"
					"  polygon = pygplates.PolygonOnSphere(points)\n"
					"\n"
					"The following example demonstrates creating a :class:`PolygonOnSphere` from a "
					":class:`PolylineOnSphere`:\n"
					"::\n"
					"\n"
					"  polygon = pygplates.PolygonOnSphere(polyline)\n"
					"\n"
					".. note:: A polygon closes the loop between its last and first points "
					"so there's no need to make the first and last points equal.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::poly_geometry_on_sphere_create_from_points<GPlatesMaths::PolygonOnSphere>,
						bp::default_call_policies(),
						(bp::arg("points"))),
				// General overloaded signature (must be in first overloaded 'def' - used by Sphinx)...
				// Specific overload signature...
				"__init__(...)\n"
				"A *PolygonOnSphere* object can be constructed in more than one way...\n"
				"\n"
				// Specific overload signature...
				"__init__(points)\n"
				"  Create a polygon from a sequence of (x,y,z) or (latitude,longitude) points.\n"
				"\n"
				"  :param points: A sequence of (x,y,z) points, or (latitude,longitude) points (in degrees).\n"
				"  :type points: Any sequence of :class:`PointOnSphere` or :class:`LatLonPoint` or "
				"tuple (float,float,float) or tuple (float,float)\n"
				"  :raises: InvalidLatLonError if any *latitude* or *longitude* is invalid\n"
				"  :raises: ViolatedUnitVectorInvariantError if any (x,y,z) is not unit magnitude\n"
				"  :raises: InvalidPointsForPolygonConstructionError if sequence has less than three points "
				"or if any two points (adjacent in the *points* sequence) are antipodal to each other "
				"(on opposite sides of the globe)\n"
				"\n"
				"  .. note:: The sequence must contain at least three points in order to be a valid "
				"polygon, otherwise *InvalidPointsForPolygonConstructionError* will be raised.\n"
				"\n"
				"  During creation, a :class:`GreatCircleArc` is created between each adjacent pair of "
				"of points in *points* - see :meth:`get_segments`. The last arc is "
				"created between the last and first points to close the loop of the polygon. "
				"For this reason you do *not* need to ensure that the first and last points have the "
				"same position (although it's not an error if this is the case because the final arc "
				"will then just have a zero length).\n"
				"\n"
				"  It is *not* an error for adjacent points in the sequence to be coincident. In this "
				"case each :class:`GreatCircleArc` between two such adjacent points will have zero length "
				"(:meth:`GreatCircleArc.is_zero_length` will return ``True``) and will have no "
				"rotation axis (:meth:`GreatCircleArc.get_rotation_axis` will raise an error).\n"
				"\n"
				"  The following example shows a few different ways to create a :class:`polygon<PolygonOnSphere>`:\n"
				"  ::\n"
				"\n"
				"    points = []\n"
				"    points.append(pygplates.PointOnSphere(...))\n"
				"    points.append(pygplates.PointOnSphere(...))\n"
				"    points.append(pygplates.PointOnSphere(...))\n"
				"    polygon = pygplates.PolygonOnSphere(points)\n"
				"    \n"
				"    points = []\n"
				"    points.append((lat1,lon1))\n"
				"    points.append((lat2,lon2))\n"
				"    points.append((lat3,lon3))\n"
				"    polygon = pygplates.PolygonOnSphere(points)\n"
				"    \n"
				"    points = []\n"
				"    points.append([x1,y1,z1])\n"
				"    points.append([x2,y2,z2])\n"
				"    points.append([x3,y3,z3])\n"
				"    polygon = pygplates.PolygonOnSphere(points)\n"
				"\n"
				"  If you have latitude/longitude values but they are not a sequence of tuples or "
				"if the latitude/longitude order is swapped then the following examples demonstrate "
				"how you could restructure them:\n"
				"  ::\n"
				"\n"
				"    # Flat lat/lon array.\n"
				"    points = numpy.array([lat1, lon1, lat2, lon2, lat3, lon3])\n"
				"    polygon = pygplates.PolygonOnSphere(zip(points[::2],points[1::2]))\n"
				"    \n"
				"    # Flat lon/lat list (ie, different latitude/longitude order).\n"
				"    points = [lon1, lat1, lon2, lat2, lon3, lat3]\n"
				"    polygon = pygplates.PolygonOnSphere(zip(points[1::2],points[::2]))\n"
				"    \n"
				"    # Separate lat/lon arrays.\n"
				"    lats = numpy.array([lat1, lat2, lat3])\n"
				"    lons = numpy.array([lon1, lon2, lon3])\n"
				"    polygon = pygplates.PolygonOnSphere(zip(lats,lons))\n"
				"    \n"
				"    # Lon/lat list of tuples (ie, different latitude/longitude order).\n"
				"    points = [(lon1, lat1), (lon2, lat2), (lon3, lat3)]\n"
				"    polygon = pygplates.PolygonOnSphere([(lat,lon) for lon, lat in points])\n")
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::polygon_on_sphere_create_from_geometry,
						bp::default_call_policies(),
						(bp::arg("geometry"),
								bp::arg("allow_one_or_two_points") = true)),
				// Specific overload signature...
				"__init__(geometry, [allow_one_or_two_points=True])\n"
				"  Create a polygon from a :class:`GeometryOnSphere`.\n"
				"\n"
				"  :param geometry: The point, multi-point, polyline or polygon geometry to convert from.\n"
				"  :type geometry: :class:`GeometryOnSphere`\n"
				"  :param allow_one_or_two_points: Whether *geometry* is allowed to be a :class:`PointOnSphere` or "
				"a :class:`MultiPointOnSphere` containing only one or two points - if allowed then one of those "
				"points is duplicated since a PolygonOnSphere requires at least three points - "
				"default is ``True``.\n"
				"  :type allow_one_or_two_points: bool\n"
				"  :raises: InvalidPointsForPolygonConstructionError if *geometry* is a "
				":class:`PointOnSphere`, or a :class:`MultiPointOnSphere` with one or two points "
				"(and *allow_one_or_two_points* is ``False``), or if any two consecutive points in a "
				":class:`MultiPointOnSphere` are antipodal to each other (on opposite sides of the globe)\n"
				"\n"
				"  If *allow_one_or_two_points* is ``True`` then *geometry* can be :class:`PointOnSphere`, "
				":class:`MultiPointOnSphere`, :class:`PolylineOnSphere` or :class:`PolygonOnSphere`. "
				"However if *allow_one_or_two_points* is ``False`` then *geometry* must be a :class:`PolygonOnSphere`, "
				"or a :class:`MultiPointOnSphere` or :class:`PolylineOnSphere` containing at least "
				"three points to avoid raising *InvalidPointsForPolygonConstructionError*.\n"
				"\n"
				"  During creation, a :class:`GreatCircleArc` is created between each adjacent pair of "
				"geometry points - see :meth:`get_segments`.\n"
				"\n"
				"  It is *not* an error for adjacent points in a geometry sequence to be coincident. In this "
				"case each :class:`GreatCircleArc` between two such adjacent points will have zero length "
				"(:meth:`GreatCircleArc.is_zero_length` will return ``True``) and will have no "
				"rotation axis (:meth:`GreatCircleArc.get_rotation_axis` will raise an error). "
				"However if two such adjacent points are antipodal (on opposite sides of the globe) "
				"then InvalidPointsForPolygonConstructionError will be raised\n"
				"\n"
				"  To create a PolygonOnSphere from any geometry type:\n"
				"  ::\n"
				"\n"
				"    polygon = pygplates.PolygonOnSphere(geometry)\n"
				"\n"
				"  To create a PolygonOnSphere from any geometry containing at least three points:\n"
				"  ::\n"
				"\n"
				"    try:\n"
				"        polygon = pygplates.PolygonOnSphere(geometry, allow_one_or_two_points=False)\n"
				"    except pygplates.InvalidPointsForPolygonConstructionError:\n"
				"        ... # Handle failure to convert 'geometry' to a PolygonOnSphere.\n")
		// To be deprecated in favour of 'get_points()'...
		.def("get_points_view",
				&GPlatesApi::poly_geometry_on_sphere_get_points_view<GPlatesMaths::PolygonOnSphere>,
				"get_points_view()\n"
				"  To be **deprecated** - use :meth:`get_points` instead.\n")
		.def("get_segments",
				&GPlatesApi::poly_geometry_on_sphere_get_arcs_view<GPlatesMaths::PolygonOnSphere>,
				"get_segments()\n"
				"  Returns a *read-only* sequence of :class:`segments<GreatCircleArc>` in this polyline.\n"
				"\n"
				"  :rtype: a read-only sequence of :class:`GreatCircleArc`\n"
				"\n"
				"  The following operations for accessing the great circle arcs in the returned "
				"read-only sequence are supported:\n"
				"\n"
				"  =========================== ==========================================================\n"
				"  Operation                   Result\n"
				"  =========================== ==========================================================\n"
				"  ``len(seq)``                number of segments of the polygon\n"
				"  ``for s in seq``            iterates over the segments *s* of the polygon\n"
				"  ``s in seq``                ``True`` if *s* is an segment of the polygon\n"
				"  ``s not in seq``            ``False`` if *s* is an segment of the polygon\n"
				"  ``seq[i]``                  the segment of the polygon at index *i*\n"
				"  ``seq[i:j]``                slice of segments of the polygon from *i* to *j*\n"
				"  ``seq[i:j:k]``              slice of segments of the polygon from *i* to *j* with step *k*\n"
				"  =========================== ==========================================================\n"
				"\n"
				"  .. note:: Between each adjacent pair of :class:`points<PointOnSphere>` there "
				"is an :class:`segment<GreatCircleArc>` such that the number of points equals the number of segments.\n"
				"\n"
				"  The following example demonstrates some uses of the above operations:\n"
				"  ::\n"
				"\n"
				"    polygon = pygplates.PolygonOnSphere(points)\n"
				"    ...\n"
				"    segments = polygon.get_segments()\n"
				"    for segment in segments:\n"
				"        if not segment.is_zero_length():\n"
				"            segment_midpoint_direction = segment.get_arc_direction(0.5)\n"
				"    first_segment = segments[0]\n"
				"    last_segment = segments[-1]\n"
				"\n"
				"  .. note:: The :meth:`end point<GreatCircleArc.get_end_point>` of the last segment is "
				"equal to the :meth:`start point<GreatCircleArc.get_start_point>` of the first segment.\n"
				"\n"
				"  .. note:: The returned sequence is *read-only* and cannot be modified.\n"
				"\n"
				"  .. note:: If you want a modifiable sequence consider wrapping the returned sequence in a ``list`` "
				"using something like ``segments = list(polygon.get_segments())`` **but** note that modifying "
				"the ``list`` (eg, appending a new segment) will **not** modify the original polygon.\n")
		// To be deprecated in favour of 'get_segments()'...
		.def("get_great_circle_arcs_view",
				&GPlatesApi::poly_geometry_on_sphere_get_arcs_view<GPlatesMaths::PolygonOnSphere>,
				"get_great_circle_arcs_view()\n"
				"  To be **deprecated** - use :meth:`get_segments` instead.\n")
		.def("get_arc_length",
				&GPlatesMaths::PolygonOnSphere::get_arc_length,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_arc_length()\n"
				"  Returns the total arc length of this polygon (in radians).\n"
				"\n"
				"  :rtype: float\n"
				"\n"
				"  | This is the sum of the arc lengths of the :meth:`segments<get_segments>` of this polygon.\n"
				"  | To convert to distance, multiply the result by the Earth radius (see :class:`Earth`).\n")
		.def("get_area",
				&GPlatesMaths::PolygonOnSphere::get_area,
				"get_area()\n"
				"  Returns the area of this polygon (on a sphere of unit radius).\n"
				"\n"
				"  :rtype: float\n"
				"\n"
				"  The area is essentially the absolute value of the :meth:`signed area<get_signed_area>`.\n"
				"\n"
				"  To convert to area on the Earth's surface, multiply the result by the Earth radius squared "
				"(see :class:`Earth`).\n")
		.def("get_signed_area",
				&GPlatesMaths::PolygonOnSphere::get_signed_area,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_signed_area()\n"
				"  Returns the *signed* area of this polygon (on a sphere of unit radius).\n"
				"\n"
				"  :rtype: float\n"
				"\n"
				"  If this polygon is clockwise (when viewed from above the surface of the sphere) "
				"then the returned area will be negative, otherwise it will be positive. "
				"However if you only want to determine the orientation of this polygon then "
				":meth:`get_orientation` is more efficient than comparing the sign of the area.\n"
				"\n"
				"  To convert to *signed* area on the Earth's surface, multiply the result by the "
				"Earth radius squared (see :class:`Earth`).\n")
		.def("get_orientation",
				&GPlatesMaths::PolygonOnSphere::get_orientation,
				"get_orientation()\n"
				"  Returns whether this polygon is clockwise or counter-clockwise.\n"
				"\n"
				"  :rtype: PolygonOnSphere.Orientation\n"
				"\n"
				"  If this polygon is clockwise (when viewed from above the surface of the sphere) "
				"then *PolygonOnSphere.Orientation.clockwise* is returned, otherwise "
				"*PolygonOnSphere.Orientation.counter_clockwise* is returned.\n"
				"\n"
				"  ::\n"
				"\n"
				"    if polygon.get_orientation() == pygplates.PolygonOnSphere.Orientation.clockwise:\n"
				"      print 'Orientation is clockwise'\n"
				"    else:\n"
				"      print 'Orientation is counter-clockwise'\n")
		.def("is_point_in_polygon",
				&GPlatesApi::polygon_on_sphere_is_point_in_polygon,
				"is_point_in_polygon(point)\n"
				"  Determines whether the specified point lies within the interior of this polygon.\n"
				"\n"
				"  :param point: the point to be tested\n"
				"  :type point: :class:`PointOnSphere` or :class:`LatLonPoint` or (latitude,longitude)"
				", in degrees, or (x,y,z)\n"
				"  :rtype: bool\n"
				"\n"
				"  Test if a (latitude, longitude) point is inside a polygon:\n"
				"\n"
				"  ::\n"
				"\n"
				"    if polygon.is_point_in_polygon((latitude, longitude)):\n"
				"      ...\n")
		.def("get_boundary_centroid",
				&GPlatesApi::polygon_on_sphere_get_boundary_centroid,
				"get_boundary_centroid()\n"
				"  Returns the *boundary* centroid of this polygon.\n"
				"\n"
				"  :rtype: :class:`PointOnSphere`\n"
				"\n"
				"  The *boundary* centroid is calculated as a weighted average of the mid-points of the "
				":class:`great circle arcs<GreatCircleArc>` of this polygon with weighting "
				"proportional to the individual arc lengths.\n"
				"\n"
				"  Note that if you want a centroid closer to the centre-of-mass of the polygon interior "
				"then use :meth:`get_interior_centroid` instead.\n")
		.def("get_interior_centroid",
				&GPlatesApi::polygon_on_sphere_get_interior_centroid,
				"get_interior_centroid()\n"
				"  Returns the *interior* centroid of this polygon.\n"
				"\n"
				"  :rtype: :class:`PointOnSphere`\n"
				"\n"
				"  The *interior* centroid is calculated as a weighted average of the centroids of "
				"spherical triangles formed by the :class:`great circle arcs<GreatCircleArc>` of this polygon "
				"and its (boundary) centroid with weighting proportional to the signed area of each individual "
				"spherical triangle. The three vertices of each spherical triangle consist of the "
				"polygon (boundary) centroid and the two end points of a great circle arc.\n"
				"\n"
				"  This centroid is useful when the centre-of-mass of the polygon interior is desired. "
				"For example, the *interior* centroid of a bottom-heavy, pear-shaped polygon will be "
				"closer to the bottom of the polygon. This centroid is not exactly at the centre-of-mass, "
				"but it will be a lot closer to the real centre-of-mass than :meth:`get_boundary_centroid`.\n")
		.def("to_tessellated",
				&GPlatesApi::polygon_on_sphere_to_tessellated,
				(bp::arg("tessellate_degrees")),
				"to_tessellated(tessellate_degrees)\n"
				"  Returns a new polygon that is tessellated version of this polygon.\n"
				"\n"
				"  :param tessellate_degrees: maximum tessellation angle (in degrees)\n"
				"  :type tessellate_degrees: float\n"
				"\n"
				"  Adjacent points (in the returned tessellated polygon) are separated by no more than "
				"*tessellate_degrees* on the globe.\n"
				"\n"
				"  Create a polygon tessellated to 2 degrees:\n"
				"  ::\n"
				"\n"
				"    tessellated_polygon = polygon.to_tessellated(2)\n"
				"\n"
				"  .. note: The distance between adjacent points (in the tessellated polygon) will not be exactly "
				"*uniform*. This is because each :class:`segment<GreatCircleArc>` in the original polygon is "
				"tessellated to the nearest integer number of points (that keeps that segment under the threshold) "
				"and hence each original *segment* will have a slightly different tessellation angle.\n")
		.def("__iter__",
				bp::range(
						&GPlatesMaths::PolygonOnSphere::vertex_begin,
						&GPlatesMaths::PolygonOnSphere::vertex_end))
		.def("__len__", &GPlatesMaths::PolygonOnSphere::number_of_vertices)
		.def("__contains__", &GPlatesApi::poly_geometry_on_sphere_contains_point<GPlatesMaths::PolygonOnSphere>)
		.def("__getitem__", &GPlatesApi::poly_geometry_on_sphere_get_item<GPlatesMaths::PolygonOnSphere>)
		// Due to the numerical tolerance in comparisons we cannot make hashable.
		// Make unhashable, with no *equality* comparison operators (we explicitly define them)...
		.def(GPlatesApi::NoHashDefVisitor(false, true))
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
	;

	// An enumeration nested within python class PolygonOnSphere (due to above 'bp::scope').
	bp::enum_<GPlatesMaths::PolygonOrientation::Orientation>("Orientation")
			.value("clockwise", GPlatesMaths::PolygonOrientation::CLOCKWISE)
			.value("counter_clockwise", GPlatesMaths::PolygonOrientation::COUNTERCLOCKWISE);

	// Enable boost::optional<GPlatesMaths::PolygonOrientation::Orientation> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesMaths::PolygonOrientation::Orientation>();


	// Register to-python conversion for PolygonOnSphere::non_null_ptr_to_const_type.
	GPlatesApi::PythonConverterUtils::register_to_python_const_to_non_const_non_null_intrusive_ptr_conversion<
			GPlatesMaths::PolygonOnSphere>();
	// Register from-python 'non-const' to 'const' conversion.
	boost::python::implicitly_convertible<
			GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PolygonOnSphere>,
			GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PolygonOnSphere> >();

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Note that we're only dealing with 'const' conversions here.
	// The 'non-const' workaround to get boost-python to compile is handled by 'python_ConstGeometryOnSphere'.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			const GPlatesMaths::PolygonOnSphere,
			const GPlatesMaths::GeometryOnSphere>();
}


void
export_geometries_on_sphere()
{
	export_geometry_on_sphere();

	export_point_on_sphere();
	export_multi_point_on_sphere();
	export_polyline_on_sphere();
	export_polygon_on_sphere();
}

#endif // GPLATES_NO_PYTHON
