/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "PythonConverterUtils.h"
#include "PythonHashDefVisitor.h"

#include "app-logic/GeometryUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/python.h"

#include "maths/AngularExtent.h"
#include "maths/DateLineWrapper.h"
#include "maths/LatLonPoint.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	bp::object
	date_line_wrapper_wrap(
			const GPlatesMaths::DateLineWrapper &date_line_wrapper,
			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry,
			boost::optional<double> tessellate_degrees)
	{
		// Convert threshold from degrees to radians (if specified).
		boost::optional<GPlatesMaths::AngularExtent> tessellate;
		if (tessellate_degrees)
		{
			tessellate = GPlatesMaths::AngularExtent::create_from_angle(
				GPlatesMaths::convert_deg_to_rad(tessellate_degrees.get()));
		}

		const GPlatesMaths::GeometryType::Value geometry_type =
				GPlatesAppLogic::GeometryUtils::get_geometry_type(*geometry);
		switch (geometry_type)
		{
		case GPlatesMaths::GeometryType::POINT:
			{
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point =
						GPlatesUtils::dynamic_pointer_cast<const GPlatesMaths::PointOnSphere>(geometry);

				const GPlatesMaths::LatLonPoint lat_lon_point = date_line_wrapper.wrap_point(*point);

				return bp::object(lat_lon_point);
			}
			break;

		case GPlatesMaths::GeometryType::MULTIPOINT:
			{
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point =
						GPlatesUtils::dynamic_pointer_cast<const GPlatesMaths::MultiPointOnSphere>(geometry);

				const GPlatesMaths::DateLineWrapper::LatLonMultiPoint lat_lon_multi_point =
						date_line_wrapper.wrap_multi_point(multi_point);

				return bp::object(lat_lon_multi_point);
			}
			break;

		case GPlatesMaths::GeometryType::POLYLINE:
			{
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline =
						GPlatesUtils::dynamic_pointer_cast<const GPlatesMaths::PolylineOnSphere>(geometry);

				std::vector<GPlatesMaths::DateLineWrapper::LatLonPolyline> lat_lon_polylines;
				date_line_wrapper.wrap_polyline(polyline, lat_lon_polylines, tessellate);

				bp::list lat_lon_polyline_list;

				std::vector<GPlatesMaths::DateLineWrapper::LatLonPolyline>::const_iterator lat_lon_polylines_iter =
						lat_lon_polylines.begin();
				std::vector<GPlatesMaths::DateLineWrapper::LatLonPolyline>::const_iterator lat_lon_polylines_end =
						lat_lon_polylines.end();
				for ( ; lat_lon_polylines_iter != lat_lon_polylines_end; ++lat_lon_polylines_iter)
				{
					const GPlatesMaths::DateLineWrapper::LatLonPolyline &lat_lon_polyline = *lat_lon_polylines_iter;
					lat_lon_polyline_list.append(lat_lon_polyline);
				}

				return lat_lon_polyline_list;
			}
			break;

		case GPlatesMaths::GeometryType::POLYGON:
			{
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon =
						GPlatesUtils::dynamic_pointer_cast<const GPlatesMaths::PolygonOnSphere>(geometry);

				std::vector<GPlatesMaths::DateLineWrapper::LatLonPolygon> lat_lon_polygons;
				date_line_wrapper.wrap_polygon(polygon, lat_lon_polygons, tessellate);

				bp::list lat_lon_polygon_list;

				std::vector<GPlatesMaths::DateLineWrapper::LatLonPolygon>::const_iterator lat_lon_polygons_iter =
						lat_lon_polygons.begin();
				std::vector<GPlatesMaths::DateLineWrapper::LatLonPolygon>::const_iterator lat_lon_polygons_end =
						lat_lon_polygons.end();
				for ( ; lat_lon_polygons_iter != lat_lon_polygons_end; ++lat_lon_polygons_iter)
				{
					const GPlatesMaths::DateLineWrapper::LatLonPolygon &lat_lon_polygon = *lat_lon_polygons_iter;
					lat_lon_polygon_list.append(lat_lon_polygon);
				}

				return lat_lon_polygon_list;
			}
			break;

		case GPlatesMaths::GeometryType::NONE:
		default:
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					false,
					GPLATES_ASSERTION_SOURCE);
			break;
		}

		// Shouldn't be able to get here.
		return bp::object()/*Py_None*/;
	}

	bp::list
	date_line_wrapper_polygon_get_exterior_points(
			const GPlatesMaths::DateLineWrapper::LatLonPolygon &lat_lon_polygon)
	{
		bp::list exterior_point_list;

		const GPlatesMaths::DateLineWrapper::lat_lon_points_seq_type &exterior_ring_points =
				lat_lon_polygon.get_exterior_ring_points();

		GPlatesMaths::DateLineWrapper::lat_lon_points_seq_type::const_iterator exterior_points_iter =
				exterior_ring_points.begin();
		GPlatesMaths::DateLineWrapper::lat_lon_points_seq_type::const_iterator exterior_points_end =
				exterior_ring_points.end();
		for ( ; exterior_points_iter != exterior_points_end; ++exterior_points_iter)
		{
			exterior_point_list.append(*exterior_points_iter);
		}

		return exterior_point_list;
	}

	bp::list
	date_line_wrapper_polygon_get_is_original_exterior_point_flags(
			const GPlatesMaths::DateLineWrapper::LatLonPolygon &lat_lon_polygon)
	{
		bp::list is_original_exterior_point_flags_list;

		std::vector<GPlatesMaths::DateLineWrapper::LatLonPolygon::point_flags_type> exterior_ring_point_flags;
		lat_lon_polygon.get_exterior_ring_point_flags(exterior_ring_point_flags);

		std::vector<GPlatesMaths::DateLineWrapper::LatLonPolygon::point_flags_type>::const_iterator
				exterior_ring_point_flags_iter = exterior_ring_point_flags.begin();
		std::vector<GPlatesMaths::DateLineWrapper::LatLonPolygon::point_flags_type>::const_iterator
				exterior_ring_point_flags_end = exterior_ring_point_flags.end();
		for ( ;
			exterior_ring_point_flags_iter != exterior_ring_point_flags_end;
			++exterior_ring_point_flags_iter)
		{
			is_original_exterior_point_flags_list.append(
					exterior_ring_point_flags_iter->test(GPlatesMaths::DateLineWrapper::LatLonPolygon::ORIGINAL_POINT));
		}

		return is_original_exterior_point_flags_list;
	}

	bp::list
	date_line_wrapper_polyline_get_points(
			const GPlatesMaths::DateLineWrapper::LatLonPolyline &lat_lon_polyline)
	{
		bp::list point_list;

		GPlatesMaths::DateLineWrapper::lat_lon_points_seq_type::const_iterator points_iter =
				lat_lon_polyline.get_points().begin();
		GPlatesMaths::DateLineWrapper::lat_lon_points_seq_type::const_iterator points_end =
				lat_lon_polyline.get_points().end();
		for ( ; points_iter != points_end; ++points_iter)
		{
			point_list.append(*points_iter);
		}

		return point_list;
	}

	bp::list
	date_line_wrapper_polyline_get_is_original_point_flags(
			const GPlatesMaths::DateLineWrapper::LatLonPolyline &lat_lon_polyline)
	{
		bp::list is_original_point_flags_list;

		std::vector<GPlatesMaths::DateLineWrapper::LatLonPolyline::point_flags_type> point_flags;
		lat_lon_polyline.get_point_flags(point_flags);

		std::vector<GPlatesMaths::DateLineWrapper::LatLonPolyline::point_flags_type>::const_iterator
				point_flags_iter = point_flags.begin();
		std::vector<GPlatesMaths::DateLineWrapper::LatLonPolyline::point_flags_type>::const_iterator
				point_flags_end = point_flags.end();
		for ( ; point_flags_iter != point_flags_end; ++point_flags_iter)
		{
			is_original_point_flags_list.append(
					point_flags_iter->test(GPlatesMaths::DateLineWrapper::LatLonPolyline::ORIGINAL_POINT));
		}

		return is_original_point_flags_list;
	}

	bp::list
	date_line_wrapper_multi_point_get_points(
			const GPlatesMaths::DateLineWrapper::LatLonMultiPoint &lat_lon_multi_point)
	{
		bp::list point_list;

		GPlatesMaths::DateLineWrapper::lat_lon_points_seq_type::const_iterator points_iter =
				lat_lon_multi_point.get_points().begin();
		GPlatesMaths::DateLineWrapper::lat_lon_points_seq_type::const_iterator points_end =
				lat_lon_multi_point.get_points().end();
		for ( ; points_iter != points_end; ++points_iter)
		{
			point_list.append(*points_iter);
		}

		return point_list;
	}
}

void
export_date_line_wrapper()
{
	//
	// DateLineWrapper - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::scope date_line_wrapper_class = bp::class_<
			GPlatesMaths::DateLineWrapper,
			GPlatesMaths::DateLineWrapper::non_null_ptr_type,
			boost::noncopyable>(
					"DateLineWrapper",
					"Wraps geometries to the dateline.\n"
					"\n"
					"The motivation for this class is to remove horizontal lines when polylines and "
					"polygons are displayed in 2D map projections. The horizontal lines occur when "
					"the longitude of two adjacent points change from approximately ``-180`` degrees to "
					"``180`` degrees (or vice versa) causing the line segment between the adjacent points "
					"to take the long path right across the map display instead of the short path.\n"
					"\n"
					"Date line wrapping avoids this by splitting a polyline/polygon into multiple "
					"polylines/polygons at the dateline.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						GPlatesMaths::DateLineWrapper::create,
						bp::default_call_policies(),
						(bp::arg("central_meridian") = 0)),
				"__init__([central_meridian=0])\n"
				"  Create a dateline wrapper with a central meridian (longitude).\n"
				"\n"
				"  :param central_meridian: Longitude of the central meridian. Defaults to zero.\n"
				"  :type central_meridian: float\n"
				"\n"
				"  If *central_meridian* is non-zero then the dateline is essentially shifted such "
				"that the longitudes of the wrapped points lie in the range "
				"``[central_meridian - 180, central_meridian + 180]``. "
				"If *central_meridian* is zero then the output range becomes ``[-180, 180]``.\n"
				"\n"
				"  To enable wrapping to the ranges ``[-180, 180]`` and ``[-90, 270]``:\n"
				"  ::\n"
				"\n"
				"    date_line_wrapper = pygplates.DateLineWrapper()\n"
				"    date_line_wrapper_90 = pygplates.DateLineWrapper(90)\n"
				"\n"
				"  .. note:: If *central_meridian* is outside the range ``[-180, 180]`` then it will be wrapped "
				" to be within that range (eg, -200 becomes 160). This ensures that the range of longitudes "
				"of wrapped points, ``[central_meridian - 180, central_meridian + 180]``, will always be "
				"within the range ``[-360, 360]`` which is the valid range for :class:`LatLonPoint`.\n")
		.def("wrap",
				&GPlatesApi::date_line_wrapper_wrap,
				(bp::arg("geometry"),
						bp::arg("tessellate_degrees") = boost::optional<double>()),
				"wrap(geometry, [tessellate_degrees])\n"
				"  Wrap a geometry to the range ``[central_meridian - 180, central_meridian + 180]``.\n"
				"\n"
				"  :param geometry: the geometry to wrap\n"
				"  :type geometry: :class:`GeometryOnSphere`\n"
				"  :param tessellate_degrees: optional tessellation threshold (in degrees)\n"
				"  :type tessellate_degrees: float or None\n"
				"\n"
				"  The following table maps the input geometry type to the return type:\n"
				"\n"
				"  +-----------------------------+-------------------------------------------------+----------------------------------------------------------------------------+\n"
				"  | Input Geometry              | Returns                                         | Description                                                                |\n"
				"  +=============================+=================================================+============================================================================+\n"
				"  | :class:`PointOnSphere`      | :class:`LatLonPoint`                            | A single wrapped point.                                                    |\n"
				"  +-----------------------------+-------------------------------------------------+----------------------------------------------------------------------------+\n"
				"  | :class:`MultiPointOnSphere` | ``DateLineWrapper.LatLonMultiPoint``            | A single ``LatLonMultiPoint`` with the following methods:                  |\n"
				"  |                             |                                                 |                                                                            |\n"
				"  |                             |                                                 | - ``get_points``: returns a ``list`` of :class:`LatLonPoint`               |\n"
				"  |                             |                                                 |   representing the wrapped points.                                         |\n"
				"  +-----------------------------+-------------------------------------------------+----------------------------------------------------------------------------+\n"
				"  | :class:`PolylineOnSphere`   | ``list`` of ``DateLineWrapper.LatLonPolyline``  | | A list of wrapped polylines.                                             |\n"
				"  |                             |                                                 | | Each ``LatLonPolyline`` has the following methods:                       |\n"
				"  |                             |                                                 |                                                                            |\n"
				"  |                             |                                                 | - ``get_points``: returns a ``list`` of :class:`LatLonPoint`               |\n"
				"  |                             |                                                 |   representing the wrapped points (of one wrapped polyline).               |\n"
				"  |                             |                                                 | - ``get_is_original_point_flags``: returns a ``list`` of ``bool``          |\n"
				"  |                             |                                                 |   indicating whether each point in ``get_points`` is an original point in  |\n"
				"  |                             |                                                 |   the polyline. Newly added points due to dateline wrapping and            |\n"
				"  |                             |                                                 |   tessellation will be ``False``. Note that both lists are the same length.|\n"
				"  +-----------------------------+-------------------------------------------------+----------------------------------------------------------------------------+\n"
				"  | :class:`PolygonOnSphere`    | ``list`` of ``DateLineWrapper.LatLonPolygon``   | | A list of wrapped polygons.                                              |\n"
				"  |                             |                                                 | | Each ``LatLonPolygon`` has the following methods:                        |\n"
				"  |                             |                                                 |                                                                            |\n"
				"  |                             |                                                 | - ``get_exterior_points``: returns a ``list`` of :class:`LatLonPoint`      |\n"
				"  |                             |                                                 |   representing the wrapped exterior points (of one wrapped polygon).       |\n"
				"  |                             |                                                 |   In future, interior points (holes) will also be supported.               |\n"
				"  |                             |                                                 | - ``get_is_original_exterior_point_flags``: returns a ``list`` of ``bool`` |\n"
				"  |                             |                                                 |   indicating whether each point in ``get_exterior_points`` is an original  |\n"
				"  |                             |                                                 |   exterior point in the polygon. Newly added points due to dateline        |\n"
				"  |                             |                                                 |   wrapping and tessellation will be ``False``. Note that both lists are    |\n"
				"  |                             |                                                 |   the same length.                                                         |\n"
				"  |                             |                                                 |                                                                            |\n"
				"  |                             |                                                 | .. note:: The start and end points are generally *not* the same.           |\n"
				"  |                             |                                                 |    This is similar to :class:`pygplates.PolygonOnSphere`.                  |\n"
				"  +-----------------------------+-------------------------------------------------+----------------------------------------------------------------------------+\n"
				"\n"
				"  Note that, unlike points and multi-points, when wrapping an input polyline (or polygon) "
				"you can get more than one wrapped output polyline (or polygon) if it crosses the dateline.\n"
				"  ::\n"
				"\n"
				"    date_line_wrapper = pygplates.DateLineWrapper(90.0)\n"
				"    \n"
				"    # Wrap a point to the range [-90, 270].\n"
				"    point = pygplates.PointOnSphere(...)\n"
				"    wrapped_point = date_line_wrapper.wrap(point)\n"
				"    wrapped_point_lat_lon = wrapped_point.get_latitude(), wrapped_point.get_longitude()\n"
				"    \n"
				"    # Wrap a multi-point to the range [-90, 270].\n"
				"    multi_point = pygplates.MultiPointOnSphere(...)\n"
				"    wrapped_multi_point = date_line_wrapper.wrap(multi_point)\n"
				"    for wrapped_point in wrapped_multi_point.get_points():\n"
				"      wrapped_point_lat_lon = wrapped_point.get_latitude(), wrapped_point.get_longitude()\n"
				"    \n"
				"    # Wrap a polyline to the range [-90, 270].\n"
				"    polyline = pygplates.PolylineOnSphere(...)\n"
				"    wrapped_polylines = date_line_wrapper.wrap(polyline)\n"
				"    for wrapped_polyline in wrapped_polylines:\n"
				"      for wrapped_point in wrapped_polyline.get_points():\n"
				"        wrapped_point_lat_lon = wrapped_point.get_latitude(), wrapped_point.get_longitude()\n"
				"    \n"
				"    # Wrap a polygon to the range [-90, 270].\n"
				"    polygon = pygplates.PolygonOnSphere(...)\n"
				"    wrapped_polygons = date_line_wrapper.wrap(polygon)\n"
				"    for wrapped_polygon in wrapped_polygons:\n"
				"      for wrapped_point in wrapped_polygon.get_exterior_points():\n"
				"        wrapped_point_lat_lon = wrapped_point.get_latitude(), wrapped_point.get_longitude()\n"
				"\n"
				"  | If *tessellate_degrees* is specified then tessellation (of polylines and polygons) is also performed.\n"
				"  | Each :class:`segment<GreatCircleArc>` is then tessellated such that adjacent points are separated by "
				"no more than *tessellate_degrees* on the globe.\n"
				"  | This is useful both for geometries that cross the dateline and those that don't. "
				"It helps ensure each polyline or polygon does not deviate too much from the true path where "
				"each *great circle arc* segment can be curved in 2D map projection space (rather than a straight line segment).\n"
				"  | But it is **especially** useful for wrapped *polygons* in 2D map projections where the boundary "
				"of the projection is curved (such as *Mollweide*). Without tessellation the segment of the wrapped polygon "
				"along the boundary will be a straight line instead of curved to follow the map boundary.\n"
				"\n"
				"  Wrapping and tessellating a polyline and a polygon to a central meridian of 90 degrees:\n"
				"  ::\n"
				"\n"
				"    date_line_wrapper = pygplates.DateLineWrapper(90.0)\n"
				"    \n"
				"    # Wrap a polyline to the range [-90, 270] and tessellate to at least 2 degrees.\n"
				"    polyline = pygplates.PolylineOnSphere(...)\n"
				"    wrapped_and_tessellated_polylines = date_line_wrapper.wrap(polyline, 2.0)\n"
				"    ...\n"
				"    \n"
				"    # Wrap a polygon to the range [-90, 270] and tessellate to at least 2 degrees.\n"
				"    polygon = pygplates.PolygonOnSphere(...)\n"
				"    wrapped_and_tessellated_polygons = date_line_wrapper.wrap(polygon, 2.0)\n"
				"    ...\n"
				"\n"
				"  .. note:: *tessellate_degrees* is ignored for :class:`points<PointOnSphere>` and :class:`multi-points<MultiPointOnSphere>`.\n"
				"\n"
				"  | Wrapping (and tessellating) can introduce new points into the original polyline or polygon.\n"
				"  | In some cases it is desirable to know which points are original points and which are not.\n"
				"  | For example, if the original points in a polyline are decorated with point symbols in a 2D map rendering. "
				"Any newly introduced points (from wrapping/tessellating) should not be decorated.\n"
				"  | As such both ``LatLonPolyline`` and ``LatLonPolygon`` have methods to support this (see the above wrapped geometry table).\n"
				"\n"
				"  Determining whether points in a wrapped polyline are original polyline points:\n"
				"  ::\n"
				"\n"
				"    date_line_wrapper = pygplates.DateLineWrapper()\n"
				"    \n"
				"    # Wrap a polyline (and tessellate to at least 2 degrees).\n"
				"    polyline = pygplates.PolylineOnSphere(...)\n"
				"    wrapped_polylines = date_line_wrapper.wrap(polyline, 2.0)\n"
				"    for wrapped_polyline in wrapped_polylines:\n"
				"      wrapped_points = wrapped_polyline.get_points()\n"
				"      is_original_point_flags = wrapped_polyline.get_is_original_point_flags()\n"
				"      for wrapped_point_index in range(len(wrapped_points)):\n"
				"        if is_original_point_flags[wrapped_point_index]:\n"
				"          wrapped_point_lat, wrapped_point_lon = wrapped_points[wrapped_point_index].to_lat_lon()\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// A nested class within python class DateLineWrapper (due to above 'bp::scope').
	bp::class_<GPlatesMaths::DateLineWrapper::LatLonPolygon>("LatLonPolygon", bp::no_init)
		.def("get_exterior_points", &GPlatesApi::date_line_wrapper_polygon_get_exterior_points)
		.def("get_is_original_exterior_point_flags", &GPlatesApi::date_line_wrapper_polygon_get_is_original_exterior_point_flags)
	;

	// A nested class within python class DateLineWrapper (due to above 'bp::scope').
	bp::class_<GPlatesMaths::DateLineWrapper::LatLonPolyline>("LatLonPolyline", bp::no_init)
		.def("get_points", &GPlatesApi::date_line_wrapper_polyline_get_points)
		.def("get_is_original_point_flags", &GPlatesApi::date_line_wrapper_polyline_get_is_original_point_flags)
	;

	// A nested class within python class DateLineWrapper (due to above 'bp::scope').
	bp::class_<GPlatesMaths::DateLineWrapper::LatLonMultiPoint>("LatLonMultiPoint", bp::no_init)
		.def("get_points", &GPlatesApi::date_line_wrapper_multi_point_get_points)
	;

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesMaths::DateLineWrapper>();
}

#endif // GPLATES_NO_PYTHON
