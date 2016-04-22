/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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
#include <cmath>
#include <iterator>
#include <utility>
#include <vector>
#include <boost/bind.hpp>
#include <boost/utility/in_place_factory.hpp>

#include "DateLineWrapper.h"

#include "GeometryDistance.h"
#include "GreatCircleArc.h"
#include "MathsUtils.h"
#include "SmallCircleBounds.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


namespace GPlatesMaths
{
	namespace
	{
		/**
		 * Normal to sphere at the north pole.
		 */
		const UnitVector3D NORTH_POLE(0, 0, 1);

		/**
		 * Normal to sphere at the south pole.
		 */
		const UnitVector3D SOUTH_POLE(0, 0, -1);

		/**
		 * Normal to the plane of the dateline great circle arc going from south pole to north pole.
		 *
		 * The reason for going south to north is then the normal points to the positive space which
		 * is, considered below to be, the front half-space when classifying vertices.
		 * In other words the front half-space (hemisphere) has a longitude range of [0, 180].
		 * The back half-space has a longitude range of [-180, 0].
		 */
		const UnitVector3D FRONT_HALF_SPACE_NORMAL(0, 1, 0);

		/**
		 * Normal to plane dividing globe into hemisphere that contains dateline in front of it.
		 */
		const UnitVector3D DATELINE_HEMISPHERE_NORMAL(-1, 0, 0);


		// Base epsilon calculations off a cosine since that usually has the least accuracy for small angles.
		// '1 - 1e-9' in cosine corresponds to a displacement of about 4.5e-5 [=sin(acos(1 - 1e-9))].
		const double EPSILON_THICK_PLANE_COSINE = 1 - 1e-9;

		// At the dateline we use a dot product and compare near zero.
		// cos(90-epsilon) = sin(epsilon)
		const double EPSILON_THICK_PLANE_SINE = std::sin(std::acos(EPSILON_THICK_PLANE_COSINE));


		/**
		 * Returns true if the specified line segment crosses north pole, otherwise it crosses south pole.
		 *
		 * @pre line segment must lie on the 'thick' plane containing the dateline *and* the
		 * line segment must cross one of the poles.
		 */
		bool
		does_line_segment_on_dateline_plane_cross_north_pole(
				const GreatCircleArc &line_segment,
				bool is_line_segment_start_point_on_dateline)
		{
			// Dot the front half-space normal with the normal to the plane the line segment is on.
			const double dot_line_segment_normal_and_front_half_space_normal =
					dot(
							cross(
									line_segment.start_point().position_vector(),
									line_segment.end_point().position_vector()),
							FRONT_HALF_SPACE_NORMAL).dval();

			// We can be quite lenient here because have both paths covered well.
			const double EPSILON = 1e-4;

			if (dot_line_segment_normal_and_front_half_space_normal > EPSILON)
			{
				return is_line_segment_start_point_on_dateline;
			}

			if (dot_line_segment_normal_and_front_half_space_normal < -EPSILON)
			{
				return !is_line_segment_start_point_on_dateline;
			}

			// The start/end points of the current line segment are too close together so
			// test for alignment of one of the endpoints with north or south pole instead.
			// NOTE: 'dval' means bypassing the epsilon test of 'real_t' - no epsilon used here.
			if (dot(NORTH_POLE, line_segment.end_point().position_vector()).dval() > 0)
			{
				return true;
			}

			return false;
		}


		/**
		 * Shift a lat/lon in the *dateline frame* to have a longitude in the range...
		 *   [-180 + central_meridian, central_meridian + 180]
		 */
		const LatLonPoint
		shift_dateline_frame_lat_lon_point_to_central_meridian_range(
				const LatLonPoint &lat_lon_point,
				const double &central_meridian)
		{
			// Convert longitude from dateline reference frame range [-180, 180] to
			// [-180 + central_meridian, central_meridian + 180]...
			return LatLonPoint(lat_lon_point.latitude(), central_meridian + lat_lon_point.longitude());
		}


		/**
		 * Convert lat/lon with longitude in the range [-180 + central_meridian, central_meridian + 180].
		 */
		const LatLonPoint
		make_lat_lon_point_in_central_meridian_range(
				const PointOnSphere &point_on_sphere,
				const double &central_meridian)
		{
			const LatLonPoint lat_lon_point = make_lat_lon_point(point_on_sphere);

			double longitude = lat_lon_point.longitude();
			if (longitude < -180 + central_meridian)
			{
				longitude += 360;
			}
			if (longitude > central_meridian + 180)
			{
				longitude -= 360;
			}

			return LatLonPoint(lat_lon_point.latitude(), longitude);
		}


		/**
		 * Convert a point on the dateline arc to lat/lon with longitude of 'central_meridian - 180'.
		 *
		 * This is used for those polylines/polygons that are fully within the dateline arc and
		 * hence outside the dateline wrapping polygon (covering entire globe except dateline arc).
		 * In order for them not to generate horizontal lines in rectangular projections we ensure
		 * all points have the same longitude.
		 */
		const LatLonPoint
		make_lat_lon_point_on_back_dateline_of_central_meridian(
				const PointOnSphere &point_on_sphere,
				const double &central_meridian)
		{
			const LatLonPoint lat_lon_point = make_lat_lon_point(point_on_sphere);

			return LatLonPoint(
					lat_lon_point.latitude(),
					central_meridian - 180);
		}
	}
}


GPlatesMaths::DateLineWrapper::DateLineWrapper(
		double central_meridian)
{
	// If the central meridian is non-zero then we need to rotate geometries to/from
	// the dateline reference frame (the frame in which wrapping occurs).
	if (!are_almost_exactly_equal(central_meridian, 0.0))
	{
		// Wrap the central meridian to the range [-180, 180].
		// This ensures the range of longitudes of output points...
		//   [-180 + central_meridian, central_meridian + 180]
		// ...will always be in the range [-360, 360] which is the valid range for LatLonPoint.
		if (central_meridian > 180)
		{
			central_meridian = central_meridian - 360 * static_cast<int>((central_meridian + 180) / 360);
		}
		else if (central_meridian < -180)
		{
			central_meridian = central_meridian - 360 * static_cast<int>((central_meridian - 180) / 360);
		}

		d_central_meridian = boost::in_place(central_meridian);
	}
}


void
GPlatesMaths::DateLineWrapper::wrap_polyline(
		const PolylineOnSphere::non_null_ptr_to_const_type &input_polyline,
		std::vector<LatLonPolyline> &wrapped_polylines,
		boost::optional<AngularExtent> tessellate_threshold) const
{
	if (!possibly_wraps(input_polyline))
	{
		// No intersection with the dateline so just convert entire input polyline to lat/lon coordinates.
		output_input_polyline(wrapped_polylines, input_polyline, tessellate_threshold);
		return;
	}

	// The input geometry in the dateline reference frame.
	PolylineOnSphere::non_null_ptr_to_const_type dateline_frame_input_polyline = input_polyline;
	if (d_central_meridian)
	{
		// We need to shift the geometry into the reference frame where the central meridian
		// has longitude zero (because this is where we can do dateline wrapping [-180,180]).
		//
		// Convert geometry to the dateline reference frame...
		dateline_frame_input_polyline =
				d_central_meridian->rotate_to_dateline_frame * dateline_frame_input_polyline;
	}

	IntersectionGraph graph(false/*is_polygon_graph*/);
	graph.add_line_geometry(
			// The intersection graph requires an input geometry in the dateline reference frame...
			dateline_frame_input_polyline->begin(),
			dateline_frame_input_polyline->end(),
			false/*is_polygon_ring*/);

	// Output the polyline if it intersects the dateline or is entirely *off* the dateline.
	graph.output_polylines(wrapped_polylines, d_central_meridian, tessellate_threshold);

	// Output the polyline if it is entirely *on* the dateline.
	output_polyline_if_entirely_on_dateline(wrapped_polylines, input_polyline, graph, tessellate_threshold);
}


void
GPlatesMaths::DateLineWrapper::wrap_polygon(
		const PolygonOnSphere::non_null_ptr_to_const_type &input_polygon,
		std::vector<LatLonPolygon> &wrapped_polygons,
		boost::optional<AngularExtent> tessellate_threshold,
		bool group_interior_with_exterior_rings) const
{
	if (!possibly_wraps(input_polygon))
	{
		// No intersection with the dateline so just convert entire input polygon to lat/lon coordinates.
		output_input_polygon(wrapped_polygons, input_polygon, tessellate_threshold);
		return;
	}

	// The input geometry in the dateline reference frame.
	PolygonOnSphere::non_null_ptr_to_const_type dateline_frame_input_polygon = input_polygon;
	if (d_central_meridian)
	{
		// We need to shift the geometry into the reference frame where the central meridian
		// has longitude zero (because this is where we can do dateline wrapping [-180,180]).
		//
		// Convert geometry to the dateline reference frame...
		dateline_frame_input_polygon =
				d_central_meridian->rotate_to_dateline_frame * dateline_frame_input_polygon;
	}

	IntersectionGraph graph(true/*is_polygon_graph*/);

	// Add the polygon's exterior ring to the intersection graph.
	graph.add_line_geometry(
			// The intersection graph requires an input geometry in the dateline reference frame...
			dateline_frame_input_polygon->exterior_ring_begin(),
			dateline_frame_input_polygon->exterior_ring_end(),
			true/*is_polygon_ring*/);

	// Add the polygon's interior rings to the intersection graph.
	const unsigned int num_interior_rings = input_polygon->number_of_interior_rings();
	for (unsigned int interior_ring_index = 0; interior_ring_index < num_interior_rings; ++interior_ring_index)
	{
		graph.add_line_geometry(
				// The intersection graph requires an input geometry in the dateline reference frame...
				dateline_frame_input_polygon->interior_ring_begin(interior_ring_index),
				dateline_frame_input_polygon->interior_ring_end(interior_ring_index),
				true/*is_polygon_ring*/);
	}

	// Output any polygon rings that intersect the dateline.
	graph.output_intersecting_polygons(wrapped_polygons, input_polygon, d_central_meridian, tessellate_threshold);

	// Output any polygon rings that don't intersect the dateline.
	output_non_intersecting_polygon_rings(
			wrapped_polygons,
			input_polygon,
			graph,
			tessellate_threshold,
			group_interior_with_exterior_rings);
}


GPlatesMaths::DateLineWrapper::LatLonMultiPoint
GPlatesMaths::DateLineWrapper::wrap_multi_point(
		const MultiPointOnSphere::non_null_ptr_to_const_type &input_multipoint) const
{
	LatLonMultiPoint lat_lon_multipoint;
	lat_lon_multipoint.d_points->reserve(input_multipoint->number_of_points());

	const double central_meridian_longitude = d_central_meridian ? d_central_meridian->longitude : 0.0;

	MultiPointOnSphere::const_iterator point_iter = input_multipoint->begin();
	MultiPointOnSphere::const_iterator point_end = input_multipoint->end();
	for ( ; point_iter != point_end; ++point_iter)
	{
		lat_lon_multipoint.d_points->push_back(
				make_lat_lon_point_in_central_meridian_range(
						*point_iter,
						central_meridian_longitude));
	}

	return lat_lon_multipoint;
}


GPlatesMaths::LatLonPoint
GPlatesMaths::DateLineWrapper::wrap_point(
		const PointOnSphere &input_point) const
{
	return make_lat_lon_point_in_central_meridian_range(
			input_point,
			d_central_meridian ? d_central_meridian->longitude : 0.0);
}


bool
GPlatesMaths::DateLineWrapper::possibly_wraps(
		const PolylineOnSphere::non_null_ptr_to_const_type &input_polyline) const
{
	return intersects_dateline(input_polyline->get_bounding_small_circle());
}


bool
GPlatesMaths::DateLineWrapper::possibly_wraps(
		const PolygonOnSphere::non_null_ptr_to_const_type &input_polygon) const
{
	return intersects_dateline(input_polygon->get_bounding_small_circle());
}


void
GPlatesMaths::DateLineWrapper::output_input_polyline(
		std::vector<LatLonPolyline> &wrapped_polylines,
		const PolylineOnSphere::non_null_ptr_to_const_type &input_polyline,
		const boost::optional<AngularExtent> &tessellate_threshold) const
{
	LatLonPolyline lat_lon_polyline;

	output_input_line_geometry(
			input_polyline->vertex_begin(),
			input_polyline->vertex_end(),
			*lat_lon_polyline.d_line_geometry,
			boost::none/*polygon_ring_index*/,
			false/*on_dateline_arc*/,
			tessellate_threshold);

	wrapped_polylines.push_back(lat_lon_polyline);
}


void
GPlatesMaths::DateLineWrapper::output_input_polygon(
		std::vector<LatLonPolygon> &wrapped_polygons,
		const PolygonOnSphere::non_null_ptr_to_const_type &input_polygon,
		const boost::optional<AngularExtent> &tessellate_threshold) const
{
	LatLonPolygon lat_lon_polygon;

	// Output the polygon's exterior ring.
	output_input_line_geometry(
			input_polygon->exterior_ring_vertex_begin(),
			input_polygon->exterior_ring_vertex_end(),
			*lat_lon_polygon.d_exterior_ring_line_geometry,
			static_cast<unsigned int>(0)/*polygon_ring_index*/,
			false/*on_dateline_arc*/,
			tessellate_threshold);

	// Output the polygon's interior rings.
	const unsigned int num_interior_rings = input_polygon->number_of_interior_rings();
	for (unsigned int interior_ring_index = 0; interior_ring_index < num_interior_rings; ++interior_ring_index)
	{
		boost::shared_ptr<LatLonLineGeometry> interior_ring_line_geometry(new LatLonLineGeometry());

		output_input_line_geometry(
				input_polygon->interior_ring_vertex_begin(interior_ring_index),
				input_polygon->interior_ring_vertex_end(interior_ring_index),
				*interior_ring_line_geometry,
				1 + interior_ring_index /*polygon_ring_index*/, // ring index 0 is exterior ring
				false/*on_dateline_arc*/,
				tessellate_threshold);

		lat_lon_polygon.d_interior_ring_line_geometries.push_back(interior_ring_line_geometry);
	}

	wrapped_polygons.push_back(lat_lon_polygon);
}


void
GPlatesMaths::DateLineWrapper::output_polyline_if_entirely_on_dateline(
		std::vector<LatLonPolyline> &wrapped_polylines,
		const PolylineOnSphere::non_null_ptr_to_const_type &input_polyline,
		const IntersectionGraph &graph,
		const boost::optional<AngularExtent> &tessellate_threshold) const
{
	std::vector<IntersectionGraph::IntersectionResult> intersection_results;
	graph.get_line_geometry_intersection_results(intersection_results);

	// There's only one line geometry for a polyline.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
		intersection_results.size() == 1,
		GPLATES_ASSERTION_SOURCE);
	const IntersectionGraph::IntersectionResult intersection_result = intersection_results.front();

	// We're only outputing polylines entirely *on* the dateline.
	if (intersection_result == IntersectionGraph::ENTIRELY_ON_DATELINE)
	{
		LatLonPolyline lat_lon_polyline;

		// We don't have the polyline geometry in the intersection graph since it got
		// sucked into the dateline so we output the original polyline geometry.
		output_input_line_geometry(
				input_polyline->vertex_begin(),
				input_polyline->vertex_end(),
				*lat_lon_polyline.d_line_geometry,
				boost::none/*polygon_ring_index*/,
				true/*on_dateline_arc*/,
				tessellate_threshold);

		wrapped_polylines.push_back(lat_lon_polyline);
	}
}


void
GPlatesMaths::DateLineWrapper::output_non_intersecting_polygon_rings(
		std::vector<LatLonPolygon> &wrapped_polygons,
		const PolygonOnSphere::non_null_ptr_to_const_type &input_polygon,
		const IntersectionGraph &graph,
		const boost::optional<AngularExtent> &tessellate_threshold,
		bool group_interior_with_exterior_rings) const
{
	const unsigned int num_rings = 1 + input_polygon->number_of_interior_rings();

	std::vector<IntersectionGraph::IntersectionResult> ring_intersection_results;
	graph.get_line_geometry_intersection_results(ring_intersection_results);

	// There should be one intersection result for each polygon ring (line geometry).
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
		num_rings == ring_intersection_results.size(),
		GPLATES_ASSERTION_SOURCE);

	// If there are no non-intersecting rings then nothing to do.
	const unsigned int num_rings_intersecting_dateline = std::count(
			ring_intersection_results.begin(),
			ring_intersection_results.end(),
			IntersectionGraph::INTERSECTS_DATELINE);
	if (num_rings_intersecting_dateline == num_rings)
	{
		return;
	}

	// If all rings are off the dateline then just return the polygon with the same exterior
	// and interior rings (but get them from the intersection graph since it's faster).
	// This avoids us having to work out which rings below to which output polygons as is needed
	// when a polygon is clipped/wrapped.
	const unsigned int num_rings_entirely_off_dateline = std::count(
			ring_intersection_results.begin(),
			ring_intersection_results.end(),
			IntersectionGraph::ENTIRELY_OFF_DATELINE);
	if (num_rings_entirely_off_dateline == num_rings)
	{
		LatLonPolygon lat_lon_polygon;

		// Output the polygon's exterior ring.
		graph.output_non_intersecting_line_polygon_ring(
				*lat_lon_polygon.d_exterior_ring_line_geometry,
				0/*ring_index*/,
				d_central_meridian,
				tessellate_threshold);

		// Output the polygon's interior rings.
		for (unsigned int ring_index = 1; ring_index < num_rings; ++ring_index)
		{
			boost::shared_ptr<LatLonLineGeometry> interior_ring_line_geometry(new LatLonLineGeometry());
			graph.output_non_intersecting_line_polygon_ring(
					*interior_ring_line_geometry,
					ring_index,
					d_central_meridian,
					tessellate_threshold);
			lat_lon_polygon.d_interior_ring_line_geometries.push_back(interior_ring_line_geometry);
		}

		wrapped_polygons.push_back(lat_lon_polygon);

		return;
	}

	std::vector<
			std::pair<
					PolygonOnSphere::non_null_ptr_to_const_type,
					unsigned int/*wrapped polygon index*/> > polygon_exterior_rings;
	// If requested, we'll create a PolygonOnSphere for each exterior ring so we can test rings that
	// are entirely off the dateline to see if they should be an interior.
	if (group_interior_with_exterior_rings)
	{
		// Add the exterior rings of the intersected rings that have already been output.
		for (unsigned int w = 0; w < wrapped_polygons.size(); ++w)
		{
			PolygonOnSphere::non_null_ptr_to_const_type polygon_exterior_ring =
					PolygonOnSphere::create_on_heap(
							// We only need the untessellated ring points to create a polygon...
							wrapped_polygons[w].d_exterior_ring_line_geometry->get_untessellated_points_on_sphere());

			polygon_exterior_rings.push_back(std::make_pair(polygon_exterior_ring, w));
		}
	}

	// Iterate over the polygon rings (exterior and interiors).
	for (unsigned int ring_index = 0; ring_index < num_rings; ++ring_index)
	{
		const IntersectionGraph::IntersectionResult ring_intersection_result =
				ring_intersection_results[ring_index];

		if (ring_intersection_result == IntersectionGraph::INTERSECTS_DATELINE)
		{
			// Don't output intersecting polygon rings - they've already been output by the intersection graph.
		}
		else if (ring_intersection_result == IntersectionGraph::ENTIRELY_OFF_DATELINE)
		{
			if (group_interior_with_exterior_rings)
			{
				// The current ring is entirely off the dateline so add it as:
				//  1) an exterior ring of a new output polygon:
				//     - if it's not inside (or intersecting) the polygons output so far,
				//  2) an interior ring of an existing output polygon:
				//     - if it's inside (or intersecting) any polygon output so far.

				boost::shared_ptr<LatLonLineGeometry> ring_line_geometry(new LatLonLineGeometry());
				graph.output_non_intersecting_line_polygon_ring(
						*ring_line_geometry,
						ring_index,
						d_central_meridian,
						tessellate_threshold);

				// Create a PolygonOnSphere from the current ring so we can test for intersection
				// with the polygon exterior rings output so far.
				PolygonOnSphere::non_null_ptr_to_const_type ring_polygon =
						PolygonOnSphere::create_on_heap(
								ring_line_geometry->get_untessellated_points_on_sphere());

				// Test each polygon exterior ring output so far for intersection.
				unsigned int r = 0;
				for ( ; r < polygon_exterior_rings.size(); ++r)
				{
					PolygonOnSphere::non_null_ptr_to_const_type polygon_exterior_ring = polygon_exterior_rings[r].first;

					// Test for intersection - includes case where one polygon is fully inside the other.
					if (minimum_distance(*ring_polygon, *polygon_exterior_ring, true, true) == AngularDistance::ZERO)
					{
						// Make the ring an interior of the current wrapped polygon.
						const unsigned int wrapped_polygon_index = polygon_exterior_rings[r].second;
						wrapped_polygons[wrapped_polygon_index].d_interior_ring_line_geometries.push_back(
								ring_line_geometry);

						break;
					}
				}

				// If we did not add the ring as an interior of an existing output polygon then create a
				// new output polygon (using ring as its exterior).
				if (r == polygon_exterior_rings.size())
				{
					LatLonPolygon lat_lon_polygon;
					lat_lon_polygon.d_exterior_ring_line_geometry = ring_line_geometry;

					const unsigned int wrapped_polygon_index = wrapped_polygons.size();
					wrapped_polygons.push_back(lat_lon_polygon);

					// Also add the polygon ring to our list of exterior rings for intersection testing.
					// Another ring might intersect it and become its interior.
					polygon_exterior_rings.push_back(
							std::make_pair(ring_polygon, wrapped_polygon_index));
				}
			}
			else
			{
				// We don't need to determine whether ring is an interior of another ring so
				// just output a new polygon with exterior ring.
				LatLonPolygon lat_lon_polygon;
				graph.output_non_intersecting_line_polygon_ring(
						*lat_lon_polygon.d_exterior_ring_line_geometry,
						ring_index,
						d_central_meridian,
						tessellate_threshold);
				wrapped_polygons.push_back(lat_lon_polygon);
			}
		}
		else if (ring_intersection_result == IntersectionGraph::ENTIRELY_ON_DATELINE)
		{
			LatLonPolygon lat_lon_polygon;

			// The ring is entirely on the dateline so just output it as a new exterior polygon.
			//
			// We don't have the polygon ring geometry in the intersection graph since it got
			// sucked into the dateline so we output the original polygon ring geometry.
			//
			// Determine whether the current ring geometry represents the input polygon's
			// exterior ring or an interior ring. The exterior ring was added to the graph first.
			if (ring_index == 0)
			{
				// Output the input polygon's exterior ring.
				output_input_line_geometry(
						input_polygon->exterior_ring_vertex_begin(),
						input_polygon->exterior_ring_vertex_end(),
						*lat_lon_polygon.d_exterior_ring_line_geometry,
						ring_index/*polygon_ring_index*/,
						true/*on_dateline_arc*/,
						tessellate_threshold);
			}
			else
			{
				// Output an interior ring of the input polygon.
				const unsigned int interior_ring_index = ring_index - 1;
				output_input_line_geometry(
						input_polygon->interior_ring_vertex_begin(interior_ring_index),
						input_polygon->interior_ring_vertex_end(interior_ring_index),
						*lat_lon_polygon.d_exterior_ring_line_geometry,
						ring_index/*polygon_ring_index*/,
						true/*on_dateline_arc*/,
						tessellate_threshold);
			}

			wrapped_polygons.push_back(lat_lon_polygon);
		}
	}
}


template <class VertexIteratorType>
void
GPlatesMaths::DateLineWrapper::output_input_line_geometry(
		const VertexIteratorType vertex_begin,
		const VertexIteratorType vertex_end,
		LatLonLineGeometry &output_line_geometry,
		boost::optional<unsigned int> polygon_ring_index,
		bool on_dateline_arc,
		const boost::optional<AngularExtent> &tessellate_threshold) const
{
	if (vertex_begin == vertex_end)
	{
		return;
	}

	const double central_meridian_longitude = d_central_meridian ? d_central_meridian->longitude : 0.0;

	// Note that it is possible that all the original line segments got swallowed by the dateline.
	// This can happen if the original line geometry is entirely *on* the dateline which is considered
	// to be *outside* the dateline polygon (which covers the entire globe and 'effectively' excludes
	// a very thin area of size epsilon around the dateline arc).
	//
	// To avoid confusing the caller (by returning no output geometries) we will simply output
	// the entire input line geometry converted to lat/lon coordinates.
	//
	// In order for them not to generate horizontal lines in rectangular projections we ensure
	// all points have the same longitude (-180).
	const LatLonPoint (*make_lat_lon_point_function)(const PointOnSphere &, const double &) =
			on_dateline_arc
			? &make_lat_lon_point_on_back_dateline_of_central_meridian
			: &make_lat_lon_point_in_central_meridian_range;

	// If not a polygon ring (ie, a polyline) then default to zero.
	const unsigned int geometry_part_index = polygon_ring_index ? polygon_ring_index.get() : 0;

	// Iterate over the points of the line geometry.
	unsigned int line_segment_index = 0;
	for (VertexIteratorType vertex_iter = vertex_begin;
		vertex_iter != vertex_end;
		++vertex_iter, ++line_segment_index)
	{
		const PointOnSphere &point = *vertex_iter;
		const LatLonPoint lat_lon_point = make_lat_lon_point_function(point, central_meridian_longitude);

		output_line_geometry.add_point(
				lat_lon_point,
				point,
				central_meridian_longitude,
				tessellate_threshold,
				// It's the original point - it hasn't been wrapped (clipped)...
				true/*is_unwrapped_point*/,
				0.0/*segment_interpolation_ratio*/,
				line_segment_index,
				geometry_part_index,
				// Line geometry did not intersect the dateline so it can't be along the dateline...
				false/*is_dateline_segment*/);
	}

	if (polygon_ring_index &&
		tessellate_threshold)
	{
		output_line_geometry.finish_tessellating_polygon_ring(central_meridian_longitude, tessellate_threshold.get());
	}
}


bool
GPlatesMaths::DateLineWrapper::intersects_dateline(
		BoundingSmallCircle geometry_bounding_small_circle) const
{
	if (d_central_meridian)
	{
		// If the bounding small circle of the geometry (in the central meridian reference frame)
		// intersects the dateline then it's possible the line geometry does to (and hence needs wrapping).
		//
		// First we need to shift the geometry into the reference frame where the central meridian
		// has longitude zero (because this is where we can do dateline wrapping [-180,180]).
		// Instead of rotating the geometry (expensive) we rotate the centre of its bounding small circle.
		// Then we only need to rotate the geometry if the rotated bounding small circle intersects the dateline.
		geometry_bounding_small_circle =
				d_central_meridian->rotate_to_dateline_frame * geometry_bounding_small_circle;

	}

	const UnitVector3D &geometry_centroid = geometry_bounding_small_circle.get_centre();

	// NOTE: 'dval' means not using epsilon test here...
	if (dot(geometry_centroid, DATELINE_HEMISPHERE_NORMAL).dval() > 0)
	{
		// Geometry centroid is close enough to the dateline arc that we need to test
		// distance to arc itself rather than simply distance to north or south pole.

		// Instead of testing...
		//
		// angle_geometry_small_circle + angle_from_geometry_centroid_to_front_half_space_normal > 90
		//
		// ...we can test...
		//
		// cos(angle_geometry_small_circle + angle_from_geometry_centroid_to_front_half_space_normal) < 0
		//
		// ...where we can use cos(A+B) = cos(A) * cos(B) - sin(A) * sin(B)
		// This avoids the expensive 'acos' function.

		const real_t dot_centroid_and_front_half_space_normal = dot(geometry_centroid, FRONT_HALF_SPACE_NORMAL);
		// For 'cos(A+B) < 0' to work we must ensure that 'A+B' do not become large enough that
		// 'cos(A+B)' becomes greater than zero again - ie, we must ensure 'A+B < 1.5 * PI'.
		// 'angle_geometry_small_circle' can be in the range [0,PI] but we can make
		// 'angle_from_geometry_centroid_to_front_half_space_normal' be in the range [0,PI/2]
		// (thus ensuring 'A+B < 1.5 * PI') if we make its cosine (or dot product) stay positive.
		// This is the equivalent of calculating the minimum of the angles from centroid to front and
		// back half space normals.
		const real_t dot_centroid_and_closest_of_front_or_back_half_space_normal =
				// NOTE: 'dval' means not using epsilon test here...
				(dot_centroid_and_front_half_space_normal.dval() > 0)
				? dot_centroid_and_front_half_space_normal
				: -dot_centroid_and_front_half_space_normal;

		// We only used 'real_t' to take advantage of range testing in 'sqrt'.
		const double sine_angle_from_geometry_centroid_to_closest_of_front_or_back_half_space_normal =
				sqrt(1 - dot_centroid_and_closest_of_front_or_back_half_space_normal *
						dot_centroid_and_closest_of_front_or_back_half_space_normal).dval();

		// cosine(angle_from_geometry_centroid_to_front_half_space_normal)...
		const double &cosine_angle_from_geometry_centroid_to_closest_of_front_or_back_half_space_normal =
				dot_centroid_and_closest_of_front_or_back_half_space_normal.dval();

		// NOTE: No epsilon testing here...
		return 0 >=
				geometry_bounding_small_circle.get_angular_extent().get_cosine().dval() *
					cosine_angle_from_geometry_centroid_to_closest_of_front_or_back_half_space_normal -
				geometry_bounding_small_circle.get_angular_extent().get_sine().dval() *
					sine_angle_from_geometry_centroid_to_closest_of_front_or_back_half_space_normal;
	}
	else
	{
		// Only need to test distance of geometry centroid to north or south pole.
		return geometry_bounding_small_circle.test(NORTH_POLE) != BoundingSmallCircle::OUTSIDE_BOUNDS ||
			geometry_bounding_small_circle.test(SOUTH_POLE) != BoundingSmallCircle::OUTSIDE_BOUNDS;
	}
}


void
GPlatesMaths::DateLineWrapper::LatLonPolygon::get_exterior_ring_interpolate_original_segments(
		interpolate_original_segment_seq_type &exterior_ring_interpolate_original_segments) const
{
	const LatLonLineGeometry::interpolate_original_segment_seq_type &interpolate_original_segments =
			d_exterior_ring_line_geometry->get_points_interpolate_original_segments();

	const unsigned int num_interpolate_original_segments = interpolate_original_segments.size();
	exterior_ring_interpolate_original_segments.reserve(num_interpolate_original_segments);
	for (unsigned int n = 0; n < num_interpolate_original_segments; ++n)
	{
		const boost::optional<LatLonLineGeometry::InterpolateOriginalSegment> &interpolate_original_segment =
				interpolate_original_segments[n];

		if (interpolate_original_segment)
		{
			exterior_ring_interpolate_original_segments.push_back(
					InterpolateOriginalSegment(
							interpolate_original_segment->interpolate_ratio,
							interpolate_original_segment->original_segment_index,
							interpolate_original_segment->original_geometry_part_index));
		}
		else
		{
			exterior_ring_interpolate_original_segments.push_back(boost::none);
		}
	}
}


void
GPlatesMaths::DateLineWrapper::LatLonPolygon::get_interior_ring_interpolate_original_segments(
		interpolate_original_segment_seq_type &interior_ring_interpolate_original_segments,
		unsigned int interior_ring_index) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			interior_ring_index < d_interior_ring_line_geometries.size(),
			GPLATES_ASSERTION_SOURCE);

	const LatLonLineGeometry::interpolate_original_segment_seq_type &interpolate_original_segments =
			d_interior_ring_line_geometries[interior_ring_index]->get_points_interpolate_original_segments();

	const unsigned int num_interpolate_original_segments = interpolate_original_segments.size();
	interior_ring_interpolate_original_segments.reserve(num_interpolate_original_segments);
	for (unsigned int n = 0; n < num_interpolate_original_segments; ++n)
	{
		const boost::optional<LatLonLineGeometry::InterpolateOriginalSegment> &interpolate_original_segment =
				interpolate_original_segments[n];

		if (interpolate_original_segment)
		{
			interior_ring_interpolate_original_segments.push_back(
					InterpolateOriginalSegment(
							interpolate_original_segment->interpolate_ratio,
							interpolate_original_segment->original_segment_index,
							interpolate_original_segment->original_geometry_part_index));
		}
		else
		{
			interior_ring_interpolate_original_segments.push_back(boost::none);
		}
	}
}


void
GPlatesMaths::DateLineWrapper::LatLonPolyline::get_interpolate_original_segments(
		interpolate_original_segment_seq_type &polyline_interpolate_original_segments) const
{
	const LatLonLineGeometry::interpolate_original_segment_seq_type &interpolate_original_segments =
			d_line_geometry->get_points_interpolate_original_segments();

	const unsigned int num_interpolate_original_segments = interpolate_original_segments.size();
	polyline_interpolate_original_segments.reserve(num_interpolate_original_segments);
	for (unsigned int n = 0; n < num_interpolate_original_segments; ++n)
	{
		const boost::optional<LatLonLineGeometry::InterpolateOriginalSegment> &interpolate_original_segment =
				interpolate_original_segments[n];

		// There should be no points along the dateline for a polyline (like there can be for polygons) and
		// there is only one geometry part in a polyline.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				interpolate_original_segment &&
					interpolate_original_segment->original_geometry_part_index == 0,
				GPLATES_ASSERTION_SOURCE);

		polyline_interpolate_original_segments.push_back(
				InterpolateOriginalSegment(
						interpolate_original_segment->interpolate_ratio,
						interpolate_original_segment->original_segment_index));
	}
}


void
GPlatesMaths::DateLineWrapper::LatLonLineGeometry::add_point(
		const LatLonPoint &lat_lon_point,
		const PointOnSphere &point,
		const double &central_meridian_longitude,
		const boost::optional<AngularExtent> &tessellate_threshold,
		bool is_unwrapped_point,
		const double &segment_interpolation_ratio,
		unsigned int segment_index,
		unsigned int geometry_part_index,
		bool is_dateline_segment)
{
	const UntessellatedPointInfo untessellated_point_info(
			lat_lon_point,
			point,
			is_unwrapped_point,
			segment_interpolation_ratio,
			segment_index,
			geometry_part_index,
			is_dateline_segment);

	if (tessellate_threshold)
	{
		add_tessellated_points(
				untessellated_point_info,
				central_meridian_longitude,
				tessellate_threshold.get());
	}

	add_untessellated_point(untessellated_point_info);
}


void
GPlatesMaths::DateLineWrapper::LatLonLineGeometry::finish_tessellating_polygon_ring(
		const double &central_meridian_longitude,
		const AngularExtent &tessellate_threshold)
{
	// Add tessellated points on the arc between the last and first points of the polygon ring.
	if (d_start_point_info)
	{
		add_tessellated_points(
				d_start_point_info.get(),
				central_meridian_longitude,
				tessellate_threshold);
	}
}


void
GPlatesMaths::DateLineWrapper::LatLonLineGeometry::add_untessellated_point(
		const UntessellatedPointInfo &untessellated_point_info)
{
	d_lat_lon_points.push_back(untessellated_point_info.lat_lon_point);
	d_untessellated_points.push_back(untessellated_point_info.point);
	
	// We know point is untessellated - is it also unwrapped ?
	d_is_unwrapped_untessellated_point_flags.push_back(untessellated_point_info.is_unwrapped_point);

	// Record the segment index and interpolation ratio along it corresponding to the current point.
	d_points_interpolate_original_segments.push_back(
			InterpolateOriginalSegment(
					untessellated_point_info.segment_interpolation_ratio,
					untessellated_point_info.segment_index,
					untessellated_point_info.geometry_part_index));

	// Keep track of the previous untessellated point information.
	d_previous_untessellated_point_info = untessellated_point_info;

	// Keep track of the line geometry start point (if first untessellated point added in this call).
	if (!d_start_point_info)
	{
		d_start_point_info = untessellated_point_info;
	}
}


void
GPlatesMaths::DateLineWrapper::LatLonLineGeometry::add_tessellated_points(
		const UntessellatedPointInfo &untessellated_point_info,
		const double &central_meridian_longitude,
		const AngularExtent &tessellate_threshold)
{
	// Tessellate the current arc if its two endpoints are far enough apart.
	// We only have an arc if we've previously added a (untessellated) point
	// (ie, the start point of this line geometry).
	if (!d_previous_untessellated_point_info ||
		dot(
			untessellated_point_info.point.position_vector(),
			d_previous_untessellated_point_info->point.position_vector()).dval()
				> tessellate_threshold.get_cosine().dval())
	{
		return;
	}

	// Watch out for arcs with antipodal points - it's only possible for them to be
	// North and South poles (or vice versa) because all unwrapped points come from
	// great circle arcs (which by definition cannot have antipodal end points) and all
	// wrapped points are intersections of existing great circle arcs with the dateline
	// (or North/South pole).
	//
	// The dateline wrapper can generate an arc between the North and South poles when
	// a polygon intersects both poles and hence it cannot determine if either pole
	// is inside the polygon and hence chooses the wrong way around the dateline
	// (due to arbitrarily determining polygon entry/exit flags) and hence gets two
	// consecutive vertices as North and South poles.
	//
	// Since we can't create a great circle arc from antipodal points we'll instead tessellate
	// the latitudes (assumes antipodal points are North/South poles) and we'll assume the
	// longitudes of both points are the same (they should be).
	if (unit_vectors_are_antiparallel(
			untessellated_point_info.point.position_vector(),
			d_previous_untessellated_point_info->point.position_vector()))
	{
		const double longitude = untessellated_point_info.lat_lon_point.longitude();

		// Start and end latitudes should be -90 and 90, or 90 and -90.
		const double start_latitude = d_previous_untessellated_point_info->lat_lon_point.latitude();
		const double end_latitude = untessellated_point_info.lat_lon_point.latitude();
		const double latitude_direction = (end_latitude > start_latitude) ? +1 : -1;

		// Number of tessellation segments/intervals over 180 degree latitude range.
		const unsigned int num_segments = 1 +
				static_cast<unsigned int>(180.0 / tessellate_threshold.get_angle().dval());
		const double segment_angular_extent = 180.0 / num_segments;

		// Generate the tessellated points.
		for (unsigned int n = 1; n < num_segments; ++n)
		{
			const double latitude = start_latitude + n * latitude_direction * segment_angular_extent;
			const LatLonPoint tess_lat_lon(latitude, longitude);

			// We are on the dateline and not along a polygon ring, so there's no interpolation.
			add_tessellated_point(tess_lat_lon, boost::none/*interpolate_original_segment*/);
		}

		return;
	}

	const GreatCircleArc gca = GreatCircleArc::create(
			d_previous_untessellated_point_info->point,
			untessellated_point_info.point);

	// Tessellate the current great circle arc.
	std::vector<PointOnSphere> tess_points;
	tessellate(tess_points, gca, tessellate_threshold.get_angle().dval());

	// Determine interpolation information for tessellated points.
	//
	// Note that we can only do this if it's not a dateline segment we are tessellating
	// (only polygons can traverse along the dateline, not polylines).
	double segment_interpolation_ratio_at_start;
	double segment_interpolation_ratio_at_end;
	double segment_interpolation_ratio_increment;
	unsigned int segment_index;
	unsigned int geometry_part_index;
	if (!untessellated_point_info.is_dateline_segment)
	{
		geometry_part_index = untessellated_point_info.geometry_part_index;

		// Determine the original segment index and the interpolation ratio within the segment
		// for the start point.
		if (d_previous_untessellated_point_info->is_unwrapped_point)
		{
			// A wrapped point is on the original geometry and hence its interpolation along the
			// original segment is zero (at the start of segment).
			segment_interpolation_ratio_at_start = 0.0;

			// The segment index could refer to the first point in the original segment we are
			// interpolating in, or it could refer to the last point in the previous original segment.
			// In the latter case we need to increment the segment index.
			segment_index = d_previous_untessellated_point_info->segment_index;
			if (GPlatesMaths::are_almost_exactly_equal(
				d_previous_untessellated_point_info->segment_interpolation_ratio,
				// For wrapped points this will be either 0.0 or 1.0 ...
				1.0))
			{
				++segment_index;
			}
		}
		else
		{
			// The untessellated point is not a wrapped point (ie, not on original geometry)
			// so it'll be in the middle of the original segment somewhere and hence we know
			// its segment index refers to the original segment we are currently interpolating in.
			segment_interpolation_ratio_at_start =
					d_previous_untessellated_point_info->segment_interpolation_ratio;
			segment_index = d_previous_untessellated_point_info->segment_index;
		}

		// Determine the interpolation ratio within the original segment for the end point.
		if (untessellated_point_info.is_unwrapped_point)
		{
			// A wrapped point is on the original geometry and hence its interpolation along the
			// original segment is one (at the end of segment).
			segment_interpolation_ratio_at_end = 1.0;
		}
		else
		{
			// The untessellated point is not a wrapped point (ie, not on original geometry)
			// so it'll be in the middle of the original segment somewhere and hence we know its
			// interpolation ratio refers to the original segment we are currently interpolating in.
			segment_interpolation_ratio_at_end =
					untessellated_point_info.segment_interpolation_ratio;
		}

		segment_interpolation_ratio_increment =
				(segment_interpolation_ratio_at_end - segment_interpolation_ratio_at_start) /
						// 'tess_points.size()' is guaranteed to be at least two...
						(tess_points.size() - 1);
	}
	else
	{
		// Unused - set to arbitrary values.
		segment_interpolation_ratio_at_start = 0;
		segment_interpolation_ratio_at_end = 0;
		segment_interpolation_ratio_increment = 0;
		segment_index = 0;
		geometry_part_index = 0;
	}

	const real_t arc_start_point_longitude(d_previous_untessellated_point_info->lat_lon_point.longitude());
	const real_t arc_end_point_longitude(untessellated_point_info.lat_lon_point.longitude());

	// If the arc is entirely on the dateline (both end points on the dateline)...
	// NOTE: This excludes arcs at the north or south pole singularities - the ones that form
	// horizontal lines at the top and bottom of a rectangular projection but are degenerate.
	// We don't need to worry about these because they are zero length and won't contribute
	// any tessellated vertices.
	if (arc_start_point_longitude == arc_end_point_longitude &&
		abs(arc_start_point_longitude - central_meridian_longitude) == 180.0)
	{
		// Add the tessellated points skipping the *first* since it was added by the previous arc and
		// skipping the *last* since it will be added by this arc.
		for (unsigned int n = 1; n < tess_points.size() - 1; ++n)
		{
			// NOTE: These tessellated points have not been wrapped (dateline wrapped) and hence
			// could end up with -180 or +180 for the longitude (due to numerical precision).
			// So we must make sure their wrapping matches the arc end points (if both endpoints
			// are *on* the dateline). If only one of the arc end points is on the dateline then
			// the tessellated points *between* the arc end points (if any) are relatively safe
			// from this wrapping problem (since they're *off* the dateline somewhat).
			// Note that this is also why we exclude the start and end points in the tessellation
			// (we want to respect their original wrapping since they can be *on* the dateline).
			const real_t tess_latitude = asin(tess_points[n].position_vector().z());
			const LatLonPoint tess_lat_lon(
					convert_rad_to_deg(tess_latitude).dval(),
					arc_start_point_longitude.dval());

			boost::optional<InterpolateOriginalSegment> interpolate_original_segment;
			if (!untessellated_point_info.is_dateline_segment)
			{
				interpolate_original_segment = boost::in_place(
						segment_interpolation_ratio_at_start + n * segment_interpolation_ratio_increment,
						segment_index,
						geometry_part_index);
			}

			add_tessellated_point(tess_lat_lon, interpolate_original_segment);
		}
	}
	else // arc is *not* entirely on the dateline (although one of the end points could be) ...
	{
		// Add the tessellated points skipping the *first* since it was added by the previous arc and
		// skipping the *last* since it will be added by this arc.
		for (unsigned int n = 1; n < tess_points.size() - 1; ++n)
		{
			// These tessellated points have not been wrapped but they are also not *on* the
			// dateline and hence are relatively safe from wrapping problems.
			// Just make sure we keep the longitude in the range...
			//   [-180 + central_meridian, central_meridian + 180]
			// ...since we're converting from PointOnSphere to LatLonPoint (ie, [-180, 180] range).
			// Note; 'central_meridian_longitude' should be in the range [-180, 180] itself.
			LatLonPoint tess_lat_lon = make_lat_lon_point(tess_points[n]);
			if (tess_lat_lon.longitude() < -180 + central_meridian_longitude)
			{
				tess_lat_lon = LatLonPoint(
						tess_lat_lon.latitude(),
						tess_lat_lon.longitude() + 360);
			}
			else if (tess_lat_lon.longitude() > central_meridian_longitude + 180)
			{
				tess_lat_lon = LatLonPoint(
						tess_lat_lon.latitude(),
						tess_lat_lon.longitude() - 360);
			}

			boost::optional<InterpolateOriginalSegment> interpolate_original_segment;
			if (!untessellated_point_info.is_dateline_segment)
			{
				interpolate_original_segment = boost::in_place(
						segment_interpolation_ratio_at_start + n * segment_interpolation_ratio_increment,
						segment_index,
						geometry_part_index);
			}

			add_tessellated_point(tess_lat_lon, interpolate_original_segment);
		}
	}
}


void
GPlatesMaths::DateLineWrapper::LatLonLineGeometry::add_tessellated_point(
		const LatLonPoint &lat_lon_point,
		const boost::optional<InterpolateOriginalSegment> &interpolate_original_segment)
{
	d_lat_lon_points.push_back(lat_lon_point);

	// We know point is tessellated - so it can't be untessellated (and unwrapped).
	d_is_unwrapped_untessellated_point_flags.push_back(false);

	// Record the segment index and interpolation ratio along it corresponding to the current point
	// unless it's along the dateline.
	d_points_interpolate_original_segments.push_back(interpolate_original_segment);
}


// Note that the value doesn't matter - it's just used when constructing list sentinel nodes.
const GPlatesMaths::DateLineWrapper::Vertex
GPlatesMaths::DateLineWrapper::IntersectionGraph::LISTS_SENTINEL(LatLonPoint(0, 0));


GPlatesMaths::DateLineWrapper::IntersectionGraph::IntersectionGraph(
		bool is_polygon_graph_) :
	d_dateline_vertices(LISTS_SENTINEL),
	d_dateline_corner_south_front(NULL),
	d_dateline_corner_north_front(NULL),
	d_dateline_corner_north_back(NULL),
	d_dateline_corner_south_back(NULL),
	d_is_polygon_graph(is_polygon_graph_),
	d_geometry_intersected_north_pole(false),
	d_geometry_intersected_south_pole(false)
{
	// We only need dateline vertices (and intersection copies) for clipping a polygon geometry.
	if (d_is_polygon_graph)
	{
		// Create the four corner vertices of the dateline.
		d_dateline_corner_south_front = d_vertex_node_pool.construct(Vertex(LatLonPoint(-90, 180)));
		d_dateline_corner_north_front = d_vertex_node_pool.construct(Vertex(LatLonPoint(90, 180)));
		d_dateline_corner_north_back = d_vertex_node_pool.construct(Vertex(LatLonPoint(90, -180)));
		d_dateline_corner_south_back = d_vertex_node_pool.construct(Vertex(LatLonPoint(-90, -180)));

		// Add the four vertices to the list of dateline vertices.
		d_dateline_vertices.append(*d_dateline_corner_south_front);
		d_dateline_vertices.append(*d_dateline_corner_north_front);
		d_dateline_vertices.append(*d_dateline_corner_north_back);
		d_dateline_vertices.append(*d_dateline_corner_south_back);
	}
}


void
GPlatesMaths::DateLineWrapper::IntersectionGraph::output_polylines(
		std::vector<LatLonPolyline> &lat_lon_polylines,
		const boost::optional<CentralMeridian> &central_meridian,
		const boost::optional<AngularExtent> &tessellate_threshold)
{
	// There should be one line geometry for a polyline.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_line_geometries.size() == 1,
			GPLATES_ASSERTION_SOURCE);
	const vertex_list_type &polyline_vertices = *d_line_geometries.front().vertices;

	double central_meridian_longitude = 0;
	boost::optional<Rotation> rotate_from_dateline_frame;
	if (central_meridian)
	{
		central_meridian_longitude = central_meridian->longitude;
		rotate_from_dateline_frame = central_meridian->rotate_from_dateline_frame;
	}

	// For polylines we only need to iterate over the polyline vertices and not the dateline vertices.
	//
	// Note: This differs from polygons in that polylines are output by the intersection graph
	// if they intersect or are entirely off the dateline, whereas polygons are only output by the
	// intersection graph if they intersect the dateline.
	vertex_list_type::const_iterator polyline_vertices_iter = polyline_vertices.begin();
	while (polyline_vertices_iter != polyline_vertices.end())
	{
		// Start a new polyline.
		lat_lon_polylines.push_back(LatLonPolyline());
		LatLonPolyline &current_polyline = lat_lon_polylines.back();

		const Vertex &start_polyline_vertex = *polyline_vertices_iter;

		// Each start point of a new polyline (except the first polyline) should be an intersection point.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				start_polyline_vertex.is_intersection ||
					polyline_vertices_iter == polyline_vertices.begin(),
				GPLATES_ASSERTION_SOURCE);

		// Shift from dateline frame back to central meridian frame.
		const PointOnSphere start_polyline_point = rotate_from_dateline_frame
				? rotate_from_dateline_frame.get() * start_polyline_vertex.point
				: start_polyline_vertex.point;
		const LatLonPoint start_polyline_lat_lon_point =
				shift_dateline_frame_lat_lon_point_to_central_meridian_range(
						start_polyline_vertex.lat_lon_point,
						central_meridian_longitude);
		// Add the polyline start point.
		current_polyline.d_line_geometry->add_point(
				start_polyline_lat_lon_point,
				start_polyline_point,
				central_meridian_longitude,
				tessellate_threshold,
				start_polyline_vertex.is_unwrapped_point,
				start_polyline_vertex.segment_interpolation_ratio,
				start_polyline_vertex.segment_index);

		// Add the remaining vertices of the current polyline.
		// The current polyline stops when we hit another intersection point (or reach end of original polyline).
		for (++polyline_vertices_iter;
			polyline_vertices_iter != polyline_vertices.end();
			++polyline_vertices_iter)
		{
			const Vertex &polyline_vertex = *polyline_vertices_iter;

			// Shift from dateline frame back to central meridian frame.
			const PointOnSphere polyline_point = rotate_from_dateline_frame
					? rotate_from_dateline_frame.get() * polyline_vertex.point
					: polyline_vertex.point;
			const LatLonPoint polyline_lat_lon_point =
					shift_dateline_frame_lat_lon_point_to_central_meridian_range(
							polyline_vertex.lat_lon_point,
							central_meridian_longitude);

			current_polyline.d_line_geometry->add_point(
					polyline_lat_lon_point,
					polyline_point,
					central_meridian_longitude,
					tessellate_threshold,
					polyline_vertex.is_unwrapped_point,
					polyline_vertex.segment_interpolation_ratio,
					polyline_vertex.segment_index);

			if (polyline_vertex.is_intersection)
			{
				++polyline_vertices_iter;
				// End the current polyline.
				break;
			}
		}
	}

	// The last polyline added must have at least two points.
	// All prior polylines are guaranteed to have at least two points by the way vertices
	// are added to them in the above loop.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			lat_lon_polylines.back().get_points().size() >= 2,
			GPLATES_ASSERTION_SOURCE);
}


void
GPlatesMaths::DateLineWrapper::IntersectionGraph::output_intersecting_polygons(
		std::vector<LatLonPolygon> &lat_lon_polygons,
		const PolygonOnSphere::non_null_ptr_to_const_type &input_polygon,
		const boost::optional<CentralMeridian> &central_meridian,
		const boost::optional<AngularExtent> &tessellate_threshold)
{
	// There should be a line geometry for the exterior ring and for each interior ring.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_line_geometries.size() == 1 + input_polygon->number_of_interior_rings(),
			GPLATES_ASSERTION_SOURCE);

	//
	// First see if there were any intersections with the dateline.
	// This is determined by counting the number of dateline vertices.
	// If there's only the original four then no intersections were found.
	// This is likely the most common case when there are many polygons covering the globe
	// because only a small portion of input polygons are likely to intersect the dateline.
	//
	vertex_list_type::const_iterator dateline_vertices_iter = d_dateline_vertices.begin();
	for (int n = 0; n < 4; ++n)
	{
		++dateline_vertices_iter;
	}
	if (dateline_vertices_iter == d_dateline_vertices.end())
	{
		// There were no intersections.
		return;
	}

	//
	// Generate flags indicating which intersection vertices enter/exit the geometry polygon.
	//
	generate_entry_exit_flags_for_dateline_polygon(input_polygon);

	//
	// Iterate over the intersection graph and output the polygons.
	//
	output_intersecting_polygons(lat_lon_polygons, central_meridian, tessellate_threshold);
}


void
GPlatesMaths::DateLineWrapper::IntersectionGraph::generate_entry_exit_flags_for_dateline_polygon(
		const PolygonOnSphere::non_null_ptr_to_const_type &input_polygon)
{
	// If the geometry polygon does *not* intersect the north pole then we can accurately determine
	// whether the north pole is inside/outside the geometry polygon.
	if (!d_geometry_intersected_north_pole)
	{
		// See if the north pole is inside or outside the geometry polygon.
		const bool north_pole_is_in_geometry_polygon =
				input_polygon->is_point_in_polygon(
						PointOnSphere(NORTH_POLE),
						PolygonOnSphere::LOW_SPEED_NO_SETUP_NO_MEMORY_USAGE);

		// Generate flags indicating which intersection vertices enter/exit the geometry polygon interior.
		generate_entry_exit_flags_for_dateline_polygon(
				// Arbitrarily choose an original (non-intersection) dateline vertex that maps to the north pole...
				vertex_list_type::iterator(*d_dateline_corner_north_front),
				north_pole_is_in_geometry_polygon);
	}
	// Else if the geometry polygon does *not* intersect the south pole then we can accurately determine
	// whether the south pole is inside/outside the geometry polygon.
	else if (!d_geometry_intersected_south_pole)
	{
		// See if the south pole is inside or outside the geometry polygon.
		const bool south_pole_is_in_geometry_polygon =
				input_polygon->is_point_in_polygon(
						PointOnSphere(SOUTH_POLE),
						PolygonOnSphere::LOW_SPEED_NO_SETUP_NO_MEMORY_USAGE);

		// Generate flags indicating which intersection vertices enter/exit the geometry polygon interior.
		generate_entry_exit_flags_for_dateline_polygon(
				// Arbitrarily choose an original (non-intersection) dateline vertex that maps to the south pole...
				vertex_list_type::iterator(*d_dateline_corner_south_front),
				south_pole_is_in_geometry_polygon);
	}
	else
	{
		// Both the north and south poles are intersected by the geometry polygon.
		// Therefore we cannot easily determine what part of the dateline is inside/outside
		// the geometry polygon. We could walk along the dateline in increments but that would
		// require doing an epsilon test over the geometry polygon edges for each increment to
		// ensure numerical robustness.
		//
		// At this stage let's just randomly choose a result.
		// The geometry polygon is quite large since it intersects both poles so if we get it wrong
		// and treat its inside as its outside then it's not as bad as if the polygon was small.

		// Generate flags indicating which intersection vertices enter/exit the geometry polygon interior.
		generate_entry_exit_flags_for_dateline_polygon(
				// Arbitrarily choose an original (non-intersection) dateline vertex (any will do)...
				vertex_list_type::iterator(*d_dateline_corner_north_front),
				// Arbitrarily choose a point-in-polygon result...
				true);
	}
}


void
GPlatesMaths::DateLineWrapper::IntersectionGraph::generate_entry_exit_flags_for_dateline_polygon(
		const vertex_list_type::iterator initial_dateline_vertex_iter,
		const bool initial_dateline_vertex_is_inside_geometry_polygon)
{
	vertex_list_type::iterator dateline_vertex_iter = initial_dateline_vertex_iter;
	bool exiting_geometry_polygon = initial_dateline_vertex_is_inside_geometry_polygon;

	// Loop through all the dateline vertices (and intersection vertices) until back to starting vertex.
	do
	{
		Vertex &dateline_vertex = *dateline_vertex_iter;

		// Only intersection vertices get entry/exit flags.
		if (dateline_vertex.is_intersection)
		{
			// Record whether entering or leaving the geometry polygon.
			dateline_vertex.exits_other_polygon = exiting_geometry_polygon;

			// Toggle the entry/exit flag.
			exiting_geometry_polygon ^= 1;
		}

		// Move to the next dateline vertex.
		++dateline_vertex_iter;

		// Handle wraparound.
		if (dateline_vertex_iter == d_dateline_vertices.end())
		{
			dateline_vertex_iter = d_dateline_vertices.begin();
		}
	}
	while (dateline_vertex_iter != initial_dateline_vertex_iter);
}


void
GPlatesMaths::DateLineWrapper::IntersectionGraph::output_intersecting_polygons(
		std::vector<LatLonPolygon> &lat_lon_polygons,
		const boost::optional<CentralMeridian> &central_meridian,
		const boost::optional<AngularExtent> &tessellate_threshold)
{
	//
	// NOTE: If we get here then the input polygon intersected the dateline on at least one of its rings.
	// These intersecting polygon rings can be found by traversing the (intersection) dateline vertices.
	// Each intersecting ring (whether it was the input polygon's exterior ring or an interior ring)
	// is considered an exterior ring of a new output polygon. Any rings that don't intersect the dateline
	// are not handled here (they are handled after return from the IntersectionGraph.
	//
	// FIXME: We should convert all self-intersecting polygons into non-self-intersecting polygons
	// before we dateline wrap - this includes ensuring any resultant exterior rings intersect at most
	// at vertices (not along line segments) and that interior rings do not touch exterior ring and that
	// interior rings only touch each other at most at vertices (not along line segments) - this is
	// essentially the Shapefile standard for polygons. This avoids problems where an interior ring
	// (that doesn't intersect dateline) of an input polygon intersects both clipped/wrapped halves
	// of the input exterior ring, and so we don't know which half to assign the interior ring to.
	//

	double central_meridian_longitude = 0;
	boost::optional<Rotation> rotate_from_dateline_frame;
	if (central_meridian)
	{
		central_meridian_longitude = central_meridian->longitude;
		rotate_from_dateline_frame = central_meridian->rotate_from_dateline_frame;
	}

	// Iterate over the dateline vertices that are potential start vertices for the output polygons.
	for (vertex_list_type::iterator output_polygon_start_vertices_iter = d_dateline_vertices.begin();
		output_polygon_start_vertices_iter != d_dateline_vertices.end();
		++output_polygon_start_vertices_iter)
	{
		// The start of an output polygon should be:
		//  (1) an intersection vertex, *and*
		//  (2) an exit vertex, *and*
		//  (3) not already used to start an output polygon.
		//
		// The first condition is because it is possible that not all (or not any) of the original
		// four dateline vertices are used in any output polygons.
		//
		// The second condition is because the first thing the output polygon loop below does is
		// switch over to the geometry vertex list (and this only makes sense if the dateline
		// traversal is exiting the geometry polygon).
		if (!output_polygon_start_vertices_iter->is_intersection ||
			!output_polygon_start_vertices_iter->exits_other_polygon ||
			output_polygon_start_vertices_iter->used_to_output_polygon)
		{
			continue;
		}

		// Start a new polygon.
		lat_lon_polygons.push_back(LatLonPolygon());
		LatLonPolygon &current_output_polygon = lat_lon_polygons.back();

		// These get initialised when the first polygon arc is encountered.
		boost::optional<PointOnSphere> arc_start_point;
		boost::optional<LatLonPoint> arc_start_lat_lon_point;

		// Used to keep track of which vertex list we are traversing as we alternate between
		// the ring geometry and dateline lists to map the path of the current output polygon.
		//
		// We start out traversing the dateline vertices list first.
		vertex_list_type *output_polygon_vertex_list = &d_dateline_vertices;

		// Iterate over the output polygon vertices until we return to the start of the output polygon.
		vertex_list_type::iterator output_polygon_vertices_iter = output_polygon_start_vertices_iter;
		// NOTE: The value doesn't actually matter because we immediately changes lists upon entering loop.
		bool list_traversal_is_forward = true;
		do
		{
			Vertex &output_polygon_vertex = *output_polygon_vertices_iter;

			// Shift from dateline frame back to central meridian frame.
			const PointOnSphere output_polygon_point = rotate_from_dateline_frame
					? rotate_from_dateline_frame.get() * output_polygon_vertex.point
					: output_polygon_vertex.point;
			const LatLonPoint output_polygon_lat_lon_point =
					shift_dateline_frame_lat_lon_point_to_central_meridian_range(
							output_polygon_vertex.lat_lon_point,
							central_meridian_longitude);

			// Add the current vertex to the current output polygon.
			current_output_polygon.d_exterior_ring_line_geometry->add_point(
					output_polygon_lat_lon_point,
					output_polygon_point,
					central_meridian_longitude,
					tessellate_threshold,
					output_polygon_vertex.is_unwrapped_point,
					output_polygon_vertex.segment_interpolation_ratio,
					output_polygon_vertex.segment_index,
					output_polygon_vertex.geometry_part_index,
					// Is the segment ending with the current vertex along the dateline ? ...
					output_polygon_vertex_list == &d_dateline_vertices /*is_dateline_segment*/);
			output_polygon_vertex.used_to_output_polygon = true;

			// At intersection vertices we need to jump lists.
			if (output_polygon_vertex.is_intersection)
			{
				// Switch iteration over to the other vertex list (geometry <-> dateline).
				output_polygon_vertex_list =
						static_cast<vertex_list_type *>(
								output_polygon_vertex.intersection_neighbour_list);
				output_polygon_vertices_iter = vertex_list_type::iterator(
						*static_cast<vertex_list_type::Node *>(
								output_polygon_vertex.intersection_neighbour));

				// There are two intersection vertices that are the same point.
				// One is in the dateline vertices list and the other in the geometry list.
				// Both copies need to be marked as used.
				output_polygon_vertices_iter->used_to_output_polygon = true;

				// Determine the traversal direction of the other polygon vertex list.
				// If normal forward list traversal means exiting other polygon then we need
				// to traverse in the backwards direction instead.
				list_traversal_is_forward = !output_polygon_vertices_iter->exits_other_polygon;
			}

			// Move to the next output polygon vertex.
			if (list_traversal_is_forward)
			{
				++output_polygon_vertices_iter;
				// Handle list wraparound.
				if (output_polygon_vertices_iter == output_polygon_vertex_list->end())
				{
					output_polygon_vertices_iter = output_polygon_vertex_list->begin();
				}
			}
			else // traversing the list backwards...
			{
				// Handle list wraparound.
				if (output_polygon_vertices_iter == output_polygon_vertex_list->begin())
				{
					output_polygon_vertices_iter = output_polygon_vertex_list->end();
				}
				--output_polygon_vertices_iter;
			}
		}
		while (output_polygon_vertices_iter != output_polygon_start_vertices_iter);

		if (tessellate_threshold)
		{
			// It's a polygon (not a polyline) so tessellate the last arc (from last point to start point).
			current_output_polygon.d_exterior_ring_line_geometry->finish_tessellating_polygon_ring(
					central_meridian_longitude,
					tessellate_threshold.get());
		}
	}
}


void
GPlatesMaths::DateLineWrapper::IntersectionGraph::get_line_geometry_intersection_results(
		std::vector<IntersectionResult> &intersection_results) const
{
	intersection_results.reserve(d_line_geometries.size());

	// Determine the intersection status of each line geometry.
	std::vector<LineGeometry>::const_iterator line_geometry_iter = d_line_geometries.begin();
	std::vector<LineGeometry>::const_iterator line_geometry_end = d_line_geometries.end();
	for ( ; line_geometry_iter != line_geometry_end; ++line_geometry_iter)
	{
		const LineGeometry &line_geometry = *line_geometry_iter;

		if (line_geometry.intersects_dateline)
		{
			intersection_results.push_back(INTERSECTS_DATELINE);
		}
		else if (line_geometry.vertices->empty())
		{
			// It's possible the line geometry got consumed by the dateline.
			// This can happen if it is entirely *on* the dateline which is considered to be
			// *outside* the dateline polygon (which covers the entire globe and 'effectively'
			// excludes a very thin area of size epsilon around the dateline arc).
			intersection_results.push_back(ENTIRELY_ON_DATELINE);
		}
		else
		{
			intersection_results.push_back(ENTIRELY_OFF_DATELINE);
		}
	}
}


void
GPlatesMaths::DateLineWrapper::IntersectionGraph::output_non_intersecting_line_polygon_ring(
		LatLonLineGeometry &ring_line_geometry,
		unsigned int ring_geometry_index,
		const boost::optional<CentralMeridian> &central_meridian,
		const boost::optional<AngularExtent> &tessellate_threshold) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			ring_geometry_index < d_line_geometries.size(),
			GPLATES_ASSERTION_SOURCE);

	const LineGeometry &ring_geometry = d_line_geometries[ring_geometry_index];

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			!ring_geometry.intersects_dateline,
			GPLATES_ASSERTION_SOURCE);

	const vertex_list_type &ring_vertices = *ring_geometry.vertices;

	double central_meridian_longitude = 0;
	boost::optional<Rotation> rotate_from_dateline_frame;
	if (central_meridian)
	{
		central_meridian_longitude = central_meridian->longitude;
		rotate_from_dateline_frame = central_meridian->rotate_from_dateline_frame;
	}

	vertex_list_type::const_iterator ring_vertices_iter = ring_vertices.begin();
	vertex_list_type::const_iterator ring_vertices_end = ring_vertices.end();
	for ( ; ring_vertices_iter != ring_vertices_end; ++ring_vertices_iter)
	{
		const Vertex &ring_vertex = *ring_vertices_iter;

		// Each vertex should not be an intersection point.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				!ring_vertex.is_intersection,
				GPLATES_ASSERTION_SOURCE);

		// Shift from dateline frame back to central meridian frame (if necessary).
		const PointOnSphere ring_point = rotate_from_dateline_frame
				? rotate_from_dateline_frame.get() * ring_vertex.point
				: ring_vertex.point;
		const LatLonPoint ring_lat_lon_point =
				shift_dateline_frame_lat_lon_point_to_central_meridian_range(
						ring_vertex.lat_lon_point,
						central_meridian_longitude);

		// Add the ring point.
		ring_line_geometry.add_point(
				ring_lat_lon_point,
				ring_point,
				central_meridian_longitude,
				tessellate_threshold,
				ring_vertex.is_unwrapped_point,
				ring_vertex.segment_interpolation_ratio,
				ring_vertex.segment_index,
				ring_vertex.geometry_part_index,
				// Polygon ring did not intersect dateline so it can't be along the dateline...
				false/*is_dateline_segment*/);
	}

	if (tessellate_threshold)
	{
		// It's a polygon ring (not a polyline) so tessellate the last arc (from last point to start point).
		ring_line_geometry.finish_tessellating_polygon_ring(
				central_meridian_longitude,
				tessellate_threshold.get());
	}
}


template <typename LineSegmentForwardIter>
void
GPlatesMaths::DateLineWrapper::IntersectionGraph::add_line_geometry(
		LineSegmentForwardIter const dateline_frame_line_segments_begin,
		LineSegmentForwardIter const dateline_frame_line_segments_end,
		bool is_polygon_ring)
{
	const unsigned int line_geometry_index = d_line_geometries.size();

	// Add a new line geometry.
	// This could be for a polyline (which only ever adds one line geometry), or
	// for a polygon exterior or interior ring (polygon adds a line geometry for each ring).
	d_line_geometries.push_back(LineGeometry());

	// PolylineOnSphere and PolygonOnSphere ensure at least 1 (and 2) line segments.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			dateline_frame_line_segments_begin != dateline_frame_line_segments_end,
			GPLATES_ASSERTION_SOURCE);

	// Classify the first point.
	const GreatCircleArc &first_line_segment = *dateline_frame_line_segments_begin;
	const VertexClassification first_vertex_classification =
			classify_vertex(first_line_segment.start_point().position_vector());

	if (!is_polygon_ring)
	{
		//
		// The geometry is a polyline so emit the first vertex if it's off the dateline (and poles).
		// We don't need to emit a vertex if it's on the dateline because that'll happen for the next
		// vertex when it goes off the dateline (or if it doesn't then the one after that, etc).
		// And for a polyline we don't need to worry about the previous point because there is none.
		//
		switch (first_vertex_classification)
		{
		case CLASSIFY_FRONT:
		case CLASSIFY_BACK:
		case CLASSIFY_OFF_DATELINE_ARC_ON_PLANE:
			add_vertex(
					first_line_segment.start_point(),
					0.0/*segment_interpolation_ratio*/,
					0/*segment_index*/,
					line_geometry_index/*geometry_part_index*/);
			break;
		case CLASSIFY_ON_DATELINE_ARC:
		case CLASSIFY_ON_NORTH_POLE:
		case CLASSIFY_ON_SOUTH_POLE:
			// Note that we don't add a vertex if it's on the dateline (or its poles).
			break;
		}
	}
	// Note that if the geometry *is* a polygon ring then its last line segment will wrap around
	// back to the start point so the start point will get handled as part of the loop below.

	VertexClassification previous_end_vertex_classification = first_vertex_classification;

	unsigned int line_segment_index = 0;
	for (LineSegmentForwardIter line_segment_iter = dateline_frame_line_segments_begin;
		line_segment_iter != dateline_frame_line_segments_end;
		++line_segment_iter, ++line_segment_index)
	{
		const GreatCircleArc &current_line_segment = *line_segment_iter;

		const VertexClassification current_end_vertex_classification =
				classify_vertex(current_line_segment.end_point().position_vector());

		// Note that the end point of the previous GCA matches the start point of the current GCA.
		add_line_segment(
				current_line_segment,
				line_segment_index,
				line_geometry_index,
				previous_end_vertex_classification,
				current_end_vertex_classification);

		previous_end_vertex_classification = current_end_vertex_classification;
	}
}


void
GPlatesMaths::DateLineWrapper::IntersectionGraph::add_line_segment(
		const GreatCircleArc &line_segment,
		unsigned int line_segment_index,
		unsigned int line_geometry_index,
		VertexClassification line_segment_start_vertex_classification,
		VertexClassification line_segment_end_vertex_classification)
{
	switch (line_segment_start_vertex_classification)
	{
	case CLASSIFY_FRONT:
		switch (line_segment_end_vertex_classification)
		{
		case CLASSIFY_FRONT:
			add_vertex(line_segment.end_point(), 1.0/*segment_interpolation_ratio*/, line_segment_index, line_geometry_index);
			break;
		case CLASSIFY_BACK:
			// NOTE: Front-to-back and back-to-front transitions are the only cases where we do
			// line segment intersection tests (of geometry line segment with dateline).
			{
				IntersectionType intersection_type;
				double segment_interpolation_ratio;
				boost::optional<PointOnSphere> intersection_point =
						intersect_line_segment(intersection_type, segment_interpolation_ratio, line_segment, true);
				// We only emit a vertex on the dateline if there was an intersection.
				if (intersection_point)
				{
					switch (intersection_type)
					{
					case INTERSECTED_DATELINE:
						// Line segment is front-to-back as it crosses the dateline.
						add_intersection_vertex_on_front_dateline(
								intersection_point.get(),
								false/*is_unwrapped_point*/,
								segment_interpolation_ratio,
								line_segment_index,
								line_geometry_index,
								true/*exiting_dateline_polygon*/);
						add_intersection_vertex_on_back_dateline(
								intersection_point.get(),
								false/*is_unwrapped_point*/,
								segment_interpolation_ratio,
								line_segment_index,
								line_geometry_index,
								false/*exiting_dateline_polygon*/);
						break;
					case INTERSECTED_NORTH_POLE:
						// Use longitude of 'start' point as longitude of first intersection point.
						// Use longitude of 'end' point as longitude of second intersection point.
						// This results in meridian lines being vertical lines in rectangular coordinates.
						// Here the two longitudes will be separated by 180 degrees (or very close to).
						add_intersection_vertex_on_north_pole(
								line_segment.start_point(),
								false/*is_unwrapped_point*/,
								segment_interpolation_ratio,
								line_segment_index,
								line_geometry_index,
								true/*exiting_dateline_polygon*/);
						add_intersection_vertex_on_north_pole(
								line_segment.end_point(),
								false/*is_unwrapped_point*/,
								segment_interpolation_ratio,
								line_segment_index,
								line_geometry_index,
								false/*exiting_dateline_polygon*/);
						break;
					case INTERSECTED_SOUTH_POLE:
						// Use longitude of 'start' point as longitude of first intersection point.
						// Use longitude of 'end' point as longitude of second intersection point.
						// This results in meridian lines being vertical lines in rectangular coordinates.
						// Here the two longitudes will be separated by 180 degrees (or very close to).
						add_intersection_vertex_on_south_pole(
								line_segment.start_point(),
								false/*is_unwrapped_point*/,
								segment_interpolation_ratio,
								line_segment_index,
								line_geometry_index,
								true/*exiting_dateline_polygon*/);
						add_intersection_vertex_on_south_pole(
								line_segment.end_point(),
								false/*is_unwrapped_point*/,
								segment_interpolation_ratio,
								line_segment_index,
								line_geometry_index,
								false/*exiting_dateline_polygon*/);
						break;
					default:
						GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
						break;
					}
				}
			}
			add_vertex(line_segment.end_point(), 1.0/*segment_interpolation_ratio*/, line_segment_index, line_geometry_index);
			break;
		case CLASSIFY_OFF_DATELINE_ARC_ON_PLANE:
			add_vertex(line_segment.end_point(), 1.0/*segment_interpolation_ratio*/, line_segment_index, line_geometry_index);
			break;
		case CLASSIFY_ON_DATELINE_ARC:
			// Use latitude of 'end' point as latitude of intersection point.
			add_intersection_vertex_on_front_dateline(
					line_segment.end_point(),
					true/*is_unwrapped_point*/,
					1.0/*segment_interpolation_ratio*/,
					line_segment_index,
					line_geometry_index,
					true/*exiting_dateline_polygon*/);
			break;
		case CLASSIFY_ON_NORTH_POLE:
			// Use longitude of 'start' point as longitude of intersection point.
			// This results in meridian lines being vertical lines in rectangular coordinates.
			add_intersection_vertex_on_north_pole(
					line_segment.start_point(),
					true/*is_unwrapped_point*/,
					1.0/*segment_interpolation_ratio*/,
					line_segment_index,
					line_geometry_index,
					true/*exiting_dateline_polygon*/);
			break;
		case CLASSIFY_ON_SOUTH_POLE:
			// Use longitude of 'start' point as longitude of intersection point.
			// This results in meridian lines being vertical lines in rectangular coordinates.
			add_intersection_vertex_on_south_pole(
					line_segment.start_point(),
					true/*is_unwrapped_point*/,
					1.0/*segment_interpolation_ratio*/,
					line_segment_index,
					line_geometry_index,
					true/*exiting_dateline_polygon*/);
			break;
		}
		break;

	case CLASSIFY_BACK:
		switch (line_segment_end_vertex_classification)
		{
		case CLASSIFY_FRONT:
			// NOTE: Front-to-back and back-to-front transitions are the only cases where we do
			// line segment intersection tests (of geometry line segment with dateline).
			{
				IntersectionType intersection_type;
				double segment_interpolation_ratio;
				boost::optional<PointOnSphere> intersection_point =
						intersect_line_segment(intersection_type, segment_interpolation_ratio, line_segment, false);
				// We only emit a vertex on the dateline if there was an intersection.
				if (intersection_point)
				{
					switch (intersection_type)
					{
					case INTERSECTED_DATELINE:
						// Line segment is back-to-front as it crosses the dateline.
						add_intersection_vertex_on_back_dateline(
								intersection_point.get(),
								false/*is_unwrapped_point*/,
								segment_interpolation_ratio,
								line_segment_index,
								line_geometry_index,
								true/*exiting_dateline_polygon*/);
						add_intersection_vertex_on_front_dateline(
								intersection_point.get(),
								false/*is_unwrapped_point*/,
								segment_interpolation_ratio,
								line_segment_index,
								line_geometry_index,
								false/*exiting_dateline_polygon*/);
						break;
					case INTERSECTED_NORTH_POLE:
						// Use longitude of 'start' point as longitude of first intersection point.
						// Use longitude of 'end' point as longitude of second intersection point.
						// This results in meridian lines being vertical lines in rectangular coordinates.
						// Here the two longitudes will be separated by 180 degrees (or very close to).
						add_intersection_vertex_on_north_pole(
								line_segment.start_point(),
								false/*is_unwrapped_point*/,
								segment_interpolation_ratio,
								line_segment_index,
								line_geometry_index,
								true/*exiting_dateline_polygon*/);
						add_intersection_vertex_on_north_pole(
								line_segment.end_point(),
								false/*is_unwrapped_point*/,
								segment_interpolation_ratio,
								line_segment_index,
								line_geometry_index,
								false/*exiting_dateline_polygon*/);
						break;
					case INTERSECTED_SOUTH_POLE:
						// Use longitude of 'start' point as longitude of first intersection point.
						// Use longitude of 'end' point as longitude of second intersection point.
						// This results in meridian lines being vertical lines in rectangular coordinates.
						// Here the two longitudes will be separated by 180 degrees (or very close to).
						add_intersection_vertex_on_south_pole(
								line_segment.start_point(),
								false/*is_unwrapped_point*/,
								segment_interpolation_ratio,
								line_segment_index,
								line_geometry_index,
								true/*exiting_dateline_polygon*/);
						add_intersection_vertex_on_south_pole(
								line_segment.end_point(),
								false/*is_unwrapped_point*/,
								segment_interpolation_ratio,
								line_segment_index,
								line_geometry_index,
								false/*exiting_dateline_polygon*/);
						break;
					default:
						GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
						break;
					}
				}
			}
			add_vertex(line_segment.end_point(), 1.0/*segment_interpolation_ratio*/, line_segment_index, line_geometry_index);
			break;
		case CLASSIFY_BACK:
			add_vertex(line_segment.end_point(), 1.0/*segment_interpolation_ratio*/, line_segment_index, line_geometry_index);
			break;
		case CLASSIFY_OFF_DATELINE_ARC_ON_PLANE:
			add_vertex(line_segment.end_point(), 1.0/*segment_interpolation_ratio*/, line_segment_index, line_geometry_index);
			break;
		case CLASSIFY_ON_DATELINE_ARC:
			// Use latitude of 'end' point as latitude of intersection point.
			add_intersection_vertex_on_back_dateline(
					line_segment.end_point(),
					true/*is_unwrapped_point*/,
					1.0/*segment_interpolation_ratio*/,
					line_segment_index,
					line_geometry_index,
					true/*exiting_dateline_polygon*/);
			break;
		case CLASSIFY_ON_NORTH_POLE:
			// Use longitude of 'start' point as longitude of intersection point.
			// This results in meridian lines being vertical lines in rectangular coordinates.
			add_intersection_vertex_on_north_pole(
					line_segment.start_point(),
					true/*is_unwrapped_point*/,
					1.0/*segment_interpolation_ratio*/,
					line_segment_index,
					line_geometry_index,
					true/*exiting_dateline_polygon*/);
			break;
		case CLASSIFY_ON_SOUTH_POLE:
			// Use longitude of 'start' point as longitude of intersection point.
			// This results in meridian lines being vertical lines in rectangular coordinates.
			add_intersection_vertex_on_south_pole(
					line_segment.start_point(),
					true/*is_unwrapped_point*/,
					1.0/*segment_interpolation_ratio*/,
					line_segment_index,
					line_geometry_index,
					true/*exiting_dateline_polygon*/);
			break;
		}
		break;

	case CLASSIFY_OFF_DATELINE_ARC_ON_PLANE:
		switch (line_segment_end_vertex_classification)
		{
		case CLASSIFY_FRONT:
			add_vertex(line_segment.end_point(), 1.0/*segment_interpolation_ratio*/, line_segment_index, line_geometry_index);
			break;
		case CLASSIFY_BACK:
			add_vertex(line_segment.end_point(), 1.0/*segment_interpolation_ratio*/, line_segment_index, line_geometry_index);
			break;
		case CLASSIFY_OFF_DATELINE_ARC_ON_PLANE:
			add_vertex(line_segment.end_point(), 1.0/*segment_interpolation_ratio*/, line_segment_index, line_geometry_index);
			break;
		case CLASSIFY_ON_DATELINE_ARC:
			// First we have to decide if the current line segment passed through the north or south pole.
			// Also note that we add the 'start' point, and not the end point, since it's off
			// the dateline and hence its longitude is used for intersection point.
			// The longitude will be very close to zero since both start and end are on the 'thick' plane.
			if (does_line_segment_on_dateline_plane_cross_north_pole(
					line_segment,
					false/*is_line_segment_start_point_on_dateline*/))
			{
				// Calculate the ratio of distance from the North pole to segment start point divided by
				// distance between segment start and end points.
				const double segment_interpolation_ratio = line_segment.is_zero_length()
						? 0.0
						: acos(dot(NORTH_POLE, line_segment.start_point().position_vector())).dval() /
							acos(line_segment.dot_of_endpoints()).dval();

				intersected_north_pole();
				add_intersection_vertex_on_north_pole(
						line_segment.start_point(),
						false/*is_unwrapped_point*/,
						segment_interpolation_ratio,
						line_segment_index,
						line_geometry_index,
						true/*exiting_dateline_polygon*/);
			}
			else
			{
				// Calculate the ratio of distance from the South pole to segment start point divided by
				// distance between segment start and end points.
				const double segment_interpolation_ratio = line_segment.is_zero_length()
						? 0.0
						: acos(dot(SOUTH_POLE, line_segment.start_point().position_vector())).dval() /
							acos(line_segment.dot_of_endpoints()).dval();

				intersected_south_pole();
				add_intersection_vertex_on_south_pole(
						line_segment.start_point(),
						false/*is_unwrapped_point*/,
						segment_interpolation_ratio,
						line_segment_index,
						line_geometry_index,
						true/*exiting_dateline_polygon*/);
			}
			break;
		case CLASSIFY_ON_NORTH_POLE:
			// Use longitude of 'start' point as longitude of intersection point.
			// It'll be very close to zero.
			add_intersection_vertex_on_north_pole(
					line_segment.start_point(),
					true/*is_unwrapped_point*/,
					1.0/*segment_interpolation_ratio*/,
					line_segment_index,
					line_geometry_index,
					true/*exiting_dateline_polygon*/);
			break;
		case CLASSIFY_ON_SOUTH_POLE:
			// Use longitude of 'start' point as longitude of intersection point.
			// It'll be very close to zero.
			add_intersection_vertex_on_south_pole(
					line_segment.start_point(),
					true/*is_unwrapped_point*/,
					1.0/*segment_interpolation_ratio*/,
					line_segment_index,
					line_geometry_index,
					true/*exiting_dateline_polygon*/);
			break;
		}
		break;

	case CLASSIFY_ON_DATELINE_ARC:
		switch (line_segment_end_vertex_classification)
		{
		case CLASSIFY_FRONT:
			// Use latitude of 'start' point as latitude of intersection point.
			add_intersection_vertex_on_front_dateline(
					line_segment.start_point(),
					true/*is_unwrapped_point*/,
					0.0/*segment_interpolation_ratio*/,
					line_segment_index,
					line_geometry_index,
					false/*exiting_dateline_polygon*/);
			add_vertex(line_segment.end_point(), 1.0/*segment_interpolation_ratio*/, line_segment_index, line_geometry_index);
			break;
		case CLASSIFY_BACK:
			// Use latitude of 'start' point as latitude of intersection point.
			add_intersection_vertex_on_back_dateline(
					line_segment.start_point(),
					true/*is_unwrapped_point*/,
					0.0/*segment_interpolation_ratio*/,
					line_segment_index,
					line_geometry_index,
					false/*exiting_dateline_polygon*/);
			add_vertex(line_segment.end_point(), 1.0/*segment_interpolation_ratio*/, line_segment_index, line_geometry_index);
			break;
		case CLASSIFY_OFF_DATELINE_ARC_ON_PLANE:
			// First we have to decide if the current line segment passed through the north or south pole.
			// Also note that we add the 'end' point, and not the start point, since it's off
			// the dateline and hence its longitude is used for intersection point.
			// The longitude will be very close to zero since both start and end are on the 'thick' plane.
			if (does_line_segment_on_dateline_plane_cross_north_pole(
					line_segment,
					true/*is_line_segment_start_point_on_dateline*/))
			{
				// Calculate the ratio of distance from the North pole to segment start point divided by
				// distance between segment start and end points.
				const double segment_interpolation_ratio = line_segment.is_zero_length()
						? 0.0
						: acos(dot(NORTH_POLE, line_segment.start_point().position_vector())).dval() /
							acos(line_segment.dot_of_endpoints()).dval();

				intersected_north_pole();
				add_intersection_vertex_on_north_pole(
						line_segment.end_point(),
						false/*is_unwrapped_point*/,
						segment_interpolation_ratio,
						line_segment_index,
						line_geometry_index,
						false/*exiting_dateline_polygon*/);
			}
			else
			{
				// Calculate the ratio of distance from the South pole to segment start point divided by
				// distance between segment start and end points.
				const double segment_interpolation_ratio = line_segment.is_zero_length()
						? 0.0
						: acos(dot(SOUTH_POLE, line_segment.start_point().position_vector())).dval() /
							acos(line_segment.dot_of_endpoints()).dval();

				intersected_south_pole();
				add_intersection_vertex_on_south_pole(
						line_segment.end_point(),
						false/*is_unwrapped_point*/,
						segment_interpolation_ratio,
						line_segment_index,
						line_geometry_index,
						false/*exiting_dateline_polygon*/);
			}
			add_vertex(line_segment.end_point(), 1.0/*segment_interpolation_ratio*/, line_segment_index, line_geometry_index);
			break;
		case CLASSIFY_ON_DATELINE_ARC:
			// No intersection - as odd as it sounds the current line segment is outside the dateline 'polygon'.
			break;
		case CLASSIFY_ON_NORTH_POLE:
			// No intersection - as odd as it sounds the current line segment is outside the dateline 'polygon'.
			break;
		case CLASSIFY_ON_SOUTH_POLE:
			// No intersection - as odd as it sounds the current line segment is outside the dateline 'polygon'.
			break;
		}
		break;

	case CLASSIFY_ON_NORTH_POLE:
		switch (line_segment_end_vertex_classification)
		{
		case CLASSIFY_FRONT:
			// Use longitude of 'end' point as longitude of intersection point.
			// This results in meridian lines being vertical lines in rectangular coordinates.
			add_intersection_vertex_on_north_pole(
					line_segment.end_point(),
					true/*is_unwrapped_point*/,
					0.0/*segment_interpolation_ratio*/,
					line_segment_index,
					line_geometry_index,
					false/*exiting_dateline_polygon*/);
			add_vertex(line_segment.end_point(), 1.0/*segment_interpolation_ratio*/, line_segment_index, line_geometry_index);
			break;
		case CLASSIFY_BACK:
			// Use longitude of 'end' point as longitude of intersection point.
			// This results in meridian lines being vertical lines in rectangular coordinates.
			add_intersection_vertex_on_north_pole(
					line_segment.end_point(),
					true/*is_unwrapped_point*/,
					0.0/*segment_interpolation_ratio*/,
					line_segment_index,
					line_geometry_index,
					false/*exiting_dateline_polygon*/);
			add_vertex(line_segment.end_point(), 1.0/*segment_interpolation_ratio*/, line_segment_index, line_geometry_index);
			break;
		case CLASSIFY_OFF_DATELINE_ARC_ON_PLANE:
			// Use longitude of 'end' point as longitude of intersection point.
			// It'll be very close to zero.
			add_intersection_vertex_on_north_pole(
					line_segment.end_point(),
					true/*is_unwrapped_point*/,
					0.0/*segment_interpolation_ratio*/,
					line_segment_index,
					line_geometry_index,
					false/*exiting_dateline_polygon*/);
			add_vertex(line_segment.end_point(), 1.0/*segment_interpolation_ratio*/, line_segment_index, line_geometry_index);
			break;
		case CLASSIFY_ON_DATELINE_ARC:
			// No intersection - as odd as it sounds the current line segment is outside the dateline 'polygon'.
			break;
		case CLASSIFY_ON_NORTH_POLE:
			// No intersection - as odd as it sounds the current line segment is outside the dateline 'polygon'.
			break;
		case CLASSIFY_ON_SOUTH_POLE:
			// No intersection - as odd as it sounds the current line segment is outside the dateline 'polygon'.
			break;
		}
		break;

	case CLASSIFY_ON_SOUTH_POLE:
		switch (line_segment_end_vertex_classification)
		{
		case CLASSIFY_FRONT:
			// Use longitude of 'end' point as longitude of intersection point.
			// This results in meridian lines being vertical lines in rectangular coordinates.
			add_intersection_vertex_on_south_pole(
					line_segment.end_point(),
					true/*is_unwrapped_point*/,
					0.0/*segment_interpolation_ratio*/,
					line_segment_index,
					line_geometry_index,
					false/*exiting_dateline_polygon*/);
			add_vertex(line_segment.end_point(), 1.0/*segment_interpolation_ratio*/, line_segment_index, line_geometry_index);
			break;
		case CLASSIFY_BACK:
			// Use longitude of 'end' point as longitude of intersection point.
			// This results in meridian lines being vertical lines in rectangular coordinates.
			add_intersection_vertex_on_south_pole(
					line_segment.end_point(),
					true/*is_unwrapped_point*/,
					0.0/*segment_interpolation_ratio*/,
					line_segment_index,
					line_geometry_index,
					false/*exiting_dateline_polygon*/);
			add_vertex(line_segment.end_point(), 1.0/*segment_interpolation_ratio*/, line_segment_index, line_geometry_index);
			break;
		case CLASSIFY_OFF_DATELINE_ARC_ON_PLANE:
			// Use longitude of 'end' point as longitude of intersection point.
			// It'll be very close to zero.
			add_intersection_vertex_on_south_pole(
					line_segment.end_point(),
					true/*is_unwrapped_point*/,
					0.0/*segment_interpolation_ratio*/,
					line_segment_index,
					line_geometry_index,
					false/*exiting_dateline_polygon*/);
			add_vertex(line_segment.end_point(), 1.0/*segment_interpolation_ratio*/, line_segment_index, line_geometry_index);
			break;
		case CLASSIFY_ON_DATELINE_ARC:
			// No intersection - as odd as it sounds the current line segment is outside the dateline 'polygon'.
			break;
		case CLASSIFY_ON_NORTH_POLE:
			// No intersection - as odd as it sounds the current line segment is outside the dateline 'polygon'.
			break;
		case CLASSIFY_ON_SOUTH_POLE:
			// No intersection - as odd as it sounds the current line segment is outside the dateline 'polygon'.
			break;
		}
		break;

	default:
		// Shouldn't get here.
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}
}


boost::optional<GPlatesMaths::PointOnSphere>
GPlatesMaths::DateLineWrapper::IntersectionGraph::intersect_line_segment(
		IntersectionType &intersection_type,
		double &segment_interpolation_ratio,
		const GreatCircleArc &line_segment,
		bool line_segment_start_point_in_dateline_front_half_space)
{
	// NOTE: The line segment endpoints must be on opposite sides of the 'thick' dateline plane,
	// otherwise the result is not numerically robust.

	// Since the dateline is a full 180 degree arc (endpoints are antipodal to each other)
	// we know that the plane containing the line segment (by line segment is meant great circle arc)
	// will always split the dateline endpoints (the north and south pole) on either side.
	// Any plane passing through the globe centre will do this.
	// So we don't need to test for that like we would for GCA-to-GCA intersection.

	// The line segment should not be zero length since we know the end points are on opposite
	// sides of the fairly 'thick' plane. But if it is zero length then somehow the epsilon
	// used to compare floating point numbers in 'real_t' is unusually large for some reason.
	// If this happens then just return the intersection point as either the start or end point.
	// This is a reasonable thing to do and will be undetectable except for the duplicate point.
	if (line_segment.is_zero_length())
	{
		intersection_type = INTERSECTED_DATELINE;
		segment_interpolation_ratio = 0;
		return line_segment.start_point();
	}
	const UnitVector3D &line_segment_normal = line_segment.rotation_axis();

	// And we already know the line segment endpoints are on opposite sides of the plane containing
	// the dateline (precondition) so we only need to test the following condition:
	//   * the start points of each arc are on different sides of
	//     the other arc's plane (eg, if the line segment start point
	//     is on the *negative* side of the dateline plane then the
	//     dateline start point (south pole) must be on the *positive*
	//     side of the line segment plane).
	// The above condition is required so we don't return an intersection
	// when the arcs are on the opposite sides of the globe (even
	// though the plane of each arc divides the other arc's endpoints).

	const double dot_south_pole_and_line_segment_normal = dot(line_segment_normal, SOUTH_POLE).dval();

	if (dot_south_pole_and_line_segment_normal > EPSILON_THICK_PLANE_SINE)
	{
		if (!line_segment_start_point_in_dateline_front_half_space)
		{
			// Dateline arc start point (south pole) is in *positive* half-space of line segment's plane.
			// Line segment start point is in *negative* half-space of dateline's plane.
			// Intersection detected - although can still get no intersection if line segment endpoints are antipodal.
			intersection_type = INTERSECTED_DATELINE;
			return calculate_intersection(line_segment, segment_interpolation_ratio);
		}
	}
	else if (dot_south_pole_and_line_segment_normal < -EPSILON_THICK_PLANE_SINE)
	{
		if (line_segment_start_point_in_dateline_front_half_space)
		{
			// Dateline arc start point (south pole) is in *negative* half-space of line segment's plane.
			// Line segment start point is in *positive* half-space of dateline's plane.
			// Intersection detected - although can still get no intersection if line segment endpoints are antipodal.
			intersection_type = INTERSECTED_DATELINE;
			return calculate_intersection(line_segment, segment_interpolation_ratio);
		}
	}
	else
	{
		// The south pole (start point of dateline arc) is on the 'thick' plane of the line segment.
		// And hence so is the north pole for that matter (since it's antipodal to the south pole).
		// However we still need to determine which pole the line segment crosses, if any.

		// See if on the south pole...
		if (
			// Is the south pole closer to the line segment start point than the line segment end point is...
			dot(line_segment.start_point().position_vector(), SOUTH_POLE).dval() >= line_segment.dot_of_endpoints().dval() &&
			// Does the south pole lie on the half-circle starting at the line segment start point...
			dot(cross(line_segment.start_point().position_vector(), SOUTH_POLE), line_segment_normal).dval() >= 0)
		{
			intersection_type = INTERSECTED_SOUTH_POLE;

			// Calculate the ratio of distance from the South pole to segment start point divided by
			// distance between segment start and end points.
			segment_interpolation_ratio =
					acos(dot(SOUTH_POLE, line_segment.start_point().position_vector())).dval() /
						acos(line_segment.dot_of_endpoints()).dval();

			intersected_south_pole();
			return PointOnSphere(SOUTH_POLE);
		}

		// See if on the north pole...
		if (
			// Is the north pole closer to the line segment start point than the line segment end point is...
			dot(line_segment.start_point().position_vector(), NORTH_POLE).dval() >= line_segment.dot_of_endpoints().dval() &&
			// Does the north pole lie on the half-circle starting at the line segment start point...
			dot(cross(line_segment.start_point().position_vector(), NORTH_POLE), line_segment_normal).dval() >= 0)
		{
			intersection_type = INTERSECTED_NORTH_POLE;

			// Calculate the ratio of distance from the South pole to segment start point divided by
			// distance between segment start and end points.
			segment_interpolation_ratio =
					acos(dot(NORTH_POLE, line_segment.start_point().position_vector())).dval() /
						acos(line_segment.dot_of_endpoints()).dval();

			intersected_north_pole();
			return PointOnSphere(NORTH_POLE);
		}
	}

	// No intersection detected.
	return boost::none;
}


boost::optional<GPlatesMaths::PointOnSphere>
GPlatesMaths::DateLineWrapper::IntersectionGraph::calculate_intersection(
		const GreatCircleArc &line_segment,
		double &segment_interpolation_ratio) const
{
	// Determine the signed distances of the line segments endpoints from the dateline plane.
	const real_t signed_distance_line_segment_start_point_to_dateline_plane =
			dot(FRONT_HALF_SPACE_NORMAL, line_segment.start_point().position_vector());
	const real_t signed_distance_line_segment_end_point_to_dateline_plane =
			dot(FRONT_HALF_SPACE_NORMAL, line_segment.end_point().position_vector());

	// The denominator of the ratios used to interpolate the line segment endpoints.
	// We're going to normalise the final interpolated vector so normally we wouldn't need this
	// but the signed distances can be so small that the final interpolated vector could be
	// so close to the zero vector that we can't normalise it. To avoid this we need to first
	// make those signed distances larger by dividing by something on the order of their magnitude.
	const real_t denom =
			signed_distance_line_segment_start_point_to_dateline_plane -
					signed_distance_line_segment_end_point_to_dateline_plane;
	if (denom == 0 /*this is a floating-point comparison epsilon test*/)
	{
		// This shouldn't happen since the line segment end points are on opposite sides of
		// the 'thick' plane containing the dateline.
		// It means the line segment end points are both too close to the dateline plane.
		// If this happens then just return no intersection.
		return boost::none;
	}
	const real_t inv_denom = 1.0 / denom;

	// Interpolate the line segment endpoints based on the signed distances.
	const Vector3D interpolated_line_segment =
			(inv_denom * signed_distance_line_segment_start_point_to_dateline_plane) *
					line_segment.end_point().position_vector() -
			(inv_denom * signed_distance_line_segment_end_point_to_dateline_plane) *
					line_segment.start_point().position_vector();

	// Normalise to get a unit vector.
	if (interpolated_line_segment.magSqrd() <= 0 /*this is a floating-point comparison epsilon test*/)
	{
		// This shouldn't happen unless either:
		//  1) The signed distances of the line segment end points are zero, but this shouldn't
		//     happen since a precondition is that they are on opposite sides of the thick dateline plane.
		//  2) The line segment end points are antipodal to each other and 'GreatCircleArc' should
		//     not have allowed this. If the end points are that close to being antipodal then we
		//     can argue that the line segment arc takes an arc path on the other side of the globe
		//     and hence misses the dateline.
		// If this happens then just return no intersection.
		return boost::none;
	}

	const UnitVector3D intersection_point = interpolated_line_segment.get_normalisation();

	// Calculate the ratio of distance from intersection point to segment start point divided by
	// distance between segment start and end points.
	//
	// A precondition of caller is that the line segment is not zero length.
	segment_interpolation_ratio =
			acos(dot(intersection_point, line_segment.start_point().position_vector())).dval() /
				acos(line_segment.dot_of_endpoints()).dval();

	return PointOnSphere(intersection_point);
}


GPlatesMaths::DateLineWrapper::VertexClassification
GPlatesMaths::DateLineWrapper::IntersectionGraph::classify_vertex(
		const UnitVector3D &vertex)
{
	//
	// Test if the vertex is on the thick plane (that the dateline great circle arc lies on).
	//

	const real_t dot_vertex_and_front_half_space_normal = dot(vertex, FRONT_HALF_SPACE_NORMAL);

	// NOTE: 'dval' means bypassing the epsilon test of 'real_t' - we have our own epsilon.
	if (dot_vertex_and_front_half_space_normal.dval() > EPSILON_THICK_PLANE_SINE)
	{
		return CLASSIFY_FRONT;
	}

	// NOTE: 'dval' means bypassing the epsilon test of 'real_t' - we have our own epsilon.
	if (dot_vertex_and_front_half_space_normal.dval() < -EPSILON_THICK_PLANE_SINE)
	{
		return CLASSIFY_BACK;
	}

	//
	// Test for on the north or south pole.
	//
	// Note that we test the small region around each pole before testing for on/off the dateline.
	// This is because the small region around each pole eats into the region tested for on/off dateline.
	//

	// NOTE: 'dval' means bypassing the epsilon test of 'real_t' - we have our own epsilon.
	if (dot(vertex, NORTH_POLE).dval() > EPSILON_THICK_PLANE_COSINE)
	{
		intersected_north_pole();
		return CLASSIFY_ON_NORTH_POLE;
	}

	// NOTE: 'dval' means bypassing the epsilon test of 'real_t' - we have our own epsilon.
	if (dot(vertex, SOUTH_POLE).dval() > EPSILON_THICK_PLANE_COSINE)
	{
		intersected_south_pole();
		return CLASSIFY_ON_SOUTH_POLE;
	}

	//
	// Test for on/off the dateline great circle arc itself.
	//

	// NOTE: 'dval' means bypassing the epsilon test of 'real_t'.
	// No epsilon is used for this test because that's been covered by the poles above.
	if (dot(vertex, DATELINE_HEMISPHERE_NORMAL).dval() < 0)
	{
		return CLASSIFY_OFF_DATELINE_ARC_ON_PLANE;
	}

	return CLASSIFY_ON_DATELINE_ARC;
}


void
GPlatesMaths::DateLineWrapper::IntersectionGraph::add_vertex(
		const PointOnSphere &point,
		const double &segment_interpolation_ratio,
		unsigned int segment_index,
		unsigned int geometry_part_index)
{
	// Convert from cartesian to lat/lon coordinates.
	const LatLonPoint vertex = make_lat_lon_point(point);

	// Create an intersection vertex wrapped in a list node.
	vertex_list_type::Node *vertex_node = d_vertex_node_pool.construct(
			Vertex(
					vertex,
					point,
					true/*is_unwrapped_point*/,
					segment_interpolation_ratio,
					segment_index,
					geometry_part_index));

	// Add to the geometry sequence.
	LineGeometry &line_geometry = d_line_geometries.back();
	line_geometry.vertices->append(*vertex_node);
}


void
GPlatesMaths::DateLineWrapper::IntersectionGraph::add_intersection_vertex_on_front_dateline(
		const PointOnSphere &point,
		bool is_unwrapped_point,
		const double &segment_interpolation_ratio,
		unsigned int segment_index,
		unsigned int geometry_part_index,
		bool exiting_dateline_polygon)
{
	// Override the point's longitude with that of the dateline (from the front which is 180 degrees).
	const LatLonPoint original_vertex = make_lat_lon_point(point);
	const double &original_vertex_latitude = original_vertex.latitude();
	const LatLonPoint intersection_vertex(original_vertex_latitude, 180);

	// Create a copy of the intersection vertex for the geometry list.
	vertex_list_type::Node *geometry_vertex_node = d_vertex_node_pool.construct(
			Vertex(
					intersection_vertex,
					boost::none/*point*/,
					is_unwrapped_point,
					segment_interpolation_ratio,
					segment_index,
					geometry_part_index,
					true/*is_intersection*/,
					exiting_dateline_polygon));

	// Append to the end of to the geometry sequence.
	LineGeometry &line_geometry = d_line_geometries.back();
	line_geometry.vertices->append(*geometry_vertex_node);

	// Mark the line geometry as having intersected the dateline.
	line_geometry.intersects_dateline = true;

	// If we're graphing a polyline then no need to go any further.
	if (!d_is_polygon_graph)
	{
		return;
	}

	// Create another copy of the intersection vertex for the dateline list.
	// NOTE: We determine the 'exits_other_polygon' vertex flag later.
	vertex_list_type::Node *dateline_vertex_node = d_vertex_node_pool.construct(
			Vertex(
					intersection_vertex,
					boost::none/*point*/,
					is_unwrapped_point,
					segment_interpolation_ratio,
					segment_index,
					geometry_part_index,
					true/*is_intersection*/));

	// Insert into the dateline vertices sequence.
	// But we need to insert in the correct location so that the vertices on the dateline follow
	// a continuous loop around the dateline polygon (ie, vertices must be sorted).
	const vertex_list_type::iterator insert_begin(*d_dateline_corner_south_front);
	const vertex_list_type::iterator insert_end(*d_dateline_corner_north_front);
	vertex_list_type::iterator insert_iter = insert_begin;
	for ( ; insert_iter != insert_end; ++insert_iter)
	{
		// Keeping iterating until we find a vertex with a larger latitude.
		// NOTE: This is the reverse comparison to that of the 'back' dateline.
		if (original_vertex_latitude <= insert_iter.get()->element().lat_lon_point.latitude())
		{
			// Insert before the current vertex.
			break;
		}
	}

	// Insert the intersection vertex at the correct location in the dateline vertices list.
	dateline_vertex_node->splice_self_before(*insert_iter.get());

	// Link the two intersection nodes together so we can later jump from one sequence to the other.
	link_intersection_vertices(
			*line_geometry.vertices, geometry_vertex_node,
			d_dateline_vertices, dateline_vertex_node);
}


void
GPlatesMaths::DateLineWrapper::IntersectionGraph::add_intersection_vertex_on_back_dateline(
		const PointOnSphere &point,
		bool is_unwrapped_point,
		const double &segment_interpolation_ratio,
		unsigned int segment_index,
		unsigned int geometry_part_index,
		bool exiting_dateline_polygon)
{
	// Override the point's longitude with that of the dateline (from the back which is -180 degrees).
	const LatLonPoint original_vertex = make_lat_lon_point(point);
	const double &original_vertex_latitude = original_vertex.latitude();
	const LatLonPoint intersection_vertex(original_vertex_latitude, -180);

	// Create a copy of the intersection vertex for the geometry list.
	vertex_list_type::Node *geometry_vertex_node = d_vertex_node_pool.construct(
			Vertex(
					intersection_vertex,
					boost::none/*point*/,
					is_unwrapped_point,
					segment_interpolation_ratio,
					segment_index,
					geometry_part_index,
					true/*is_intersection*/,
					exiting_dateline_polygon));

	// Append to the end of to the geometry sequence.
	LineGeometry &line_geometry = d_line_geometries.back();
	line_geometry.vertices->append(*geometry_vertex_node);

	// Mark the line geometry as having intersected the dateline.
	line_geometry.intersects_dateline = true;

	// If we're graphing a polyline then no need to go any further.
	if (!d_is_polygon_graph)
	{
		return;
	}

	// Create another copy of the intersection vertex for the dateline list.
	// NOTE: We determine the 'exits_other_polygon' vertex flag later.
	vertex_list_type::Node *dateline_vertex_node = d_vertex_node_pool.construct(
			Vertex(
					intersection_vertex,
					boost::none/*point*/,
					is_unwrapped_point,
					segment_interpolation_ratio,
					segment_index,
					geometry_part_index,
					true/*is_intersection*/));

	// Insert into the dateline vertices sequence.
	// But we need to insert in the correct location so that the vertices on the dateline follow
	// a continuous loop around the dateline polygon (ie, vertices must be sorted).
	const vertex_list_type::iterator insert_begin(*d_dateline_corner_north_back);
	const vertex_list_type::iterator insert_end(*d_dateline_corner_south_back);
	vertex_list_type::iterator insert_iter = insert_begin;
	for ( ; insert_iter != insert_end; ++insert_iter)
	{
		// Keeping iterating until we find a vertex with a smaller latitude.
		// NOTE: This is the reverse comparison to that of the 'front' dateline.
		if (original_vertex_latitude >= insert_iter.get()->element().lat_lon_point.latitude())
		{
			// Insert before the current vertex.
			break;
		}
	}

	// Insert the intersection vertex at the correct location in the dateline vertices list.
	dateline_vertex_node->splice_self_before(*insert_iter.get());

	// Link the two intersection nodes together so we can later jump from one sequence to the other.
	link_intersection_vertices(
			*line_geometry.vertices, geometry_vertex_node,
			d_dateline_vertices, dateline_vertex_node);
}


void
GPlatesMaths::DateLineWrapper::IntersectionGraph::add_intersection_vertex_on_north_pole(
		const PointOnSphere &point,
		bool is_unwrapped_point,
		const double &segment_interpolation_ratio,
		unsigned int segment_index,
		unsigned int geometry_part_index,
		bool exiting_dateline_polygon)
{
	// Override the point's latitude with that of the north pole's.
	const LatLonPoint original_vertex = make_lat_lon_point(point);
	const double &original_vertex_longitude = original_vertex.longitude();
	const LatLonPoint intersection_vertex(90, original_vertex_longitude);

	// Create a copy of the intersection vertex for the geometry list.
	vertex_list_type::Node *geometry_vertex_node = d_vertex_node_pool.construct(
			Vertex(
					intersection_vertex,
					boost::none/*point*/,
					is_unwrapped_point,
					segment_interpolation_ratio,
					segment_index,
					geometry_part_index,
					true/*is_intersection*/,
					exiting_dateline_polygon));

	// Append to the end of to the geometry sequence.
	LineGeometry &line_geometry = d_line_geometries.back();
	line_geometry.vertices->append(*geometry_vertex_node);

	// Mark the line geometry as having intersected the dateline.
	line_geometry.intersects_dateline = true;

	// If we're graphing a polyline then no need to go any further.
	if (!d_is_polygon_graph)
	{
		return;
	}

	// Create another copy of the intersection vertex for the dateline list.
	// NOTE: We determine the 'exits_other_polygon' vertex flag later.
	vertex_list_type::Node *dateline_vertex_node = d_vertex_node_pool.construct(
			Vertex(
					intersection_vertex,
					boost::none/*point*/,
					is_unwrapped_point,
					segment_interpolation_ratio,
					segment_index,
					geometry_part_index,
					true/*is_intersection*/));

	// Insert into the dateline vertices sequence.
	// But we need to insert in the correct location so that the vertices on the dateline follow
	// a continuous loop around the dateline polygon (ie, vertices must be sorted).
	const vertex_list_type::iterator insert_begin(*d_dateline_corner_north_front);
	const vertex_list_type::iterator insert_end(*d_dateline_corner_north_back);
	vertex_list_type::iterator insert_iter = insert_begin;
	for ( ; insert_iter != insert_end; ++insert_iter)
	{
		// Keeping iterating until we find a vertex with a smaller longitude.
		// NOTE: This is the reverse comparison to that of the south pole.
		if (original_vertex_longitude >= insert_iter.get()->element().lat_lon_point.longitude())
		{
			// Insert before the current vertex.
			break;
		}
	}

	// Insert the intersection vertex at the correct location in the dateline vertices list.
	dateline_vertex_node->splice_self_before(*insert_iter.get());

	// Link the two intersection nodes together so we can later jump from one sequence to the other.
	link_intersection_vertices(
			*line_geometry.vertices, geometry_vertex_node,
			d_dateline_vertices, dateline_vertex_node);
}


void
GPlatesMaths::DateLineWrapper::IntersectionGraph::add_intersection_vertex_on_south_pole(
		const PointOnSphere &point,
		bool is_unwrapped_point,
		const double &segment_interpolation_ratio,
		unsigned int segment_index,
		unsigned int geometry_part_index,
		bool exiting_dateline_polygon)
{
	// Override the point's latitude with that of the south pole's.
	const LatLonPoint original_vertex = make_lat_lon_point(point);
	const double &original_vertex_longitude = original_vertex.longitude();
	const LatLonPoint intersection_vertex(-90, original_vertex_longitude);

	// Create a copy of the intersection vertex for the geometry list.
	vertex_list_type::Node *geometry_vertex_node = d_vertex_node_pool.construct(
			Vertex(
					intersection_vertex,
					boost::none/*point*/,
					is_unwrapped_point,
					segment_interpolation_ratio,
					segment_index,
					geometry_part_index,
					true/*is_intersection*/,
					exiting_dateline_polygon));

	// Append to the end of to the geometry sequence.
	LineGeometry &line_geometry = d_line_geometries.back();
	line_geometry.vertices->append(*geometry_vertex_node);

	// Mark the line geometry as having intersected the dateline.
	line_geometry.intersects_dateline = true;

	// If we're graphing a polyline then no need to go any further.
	if (!d_is_polygon_graph)
	{
		return;
	}

	// Create another copy of the intersection vertex for the dateline list.
	// NOTE: We determine the 'exits_other_polygon' vertex flag later.
	vertex_list_type::Node *dateline_vertex_node = d_vertex_node_pool.construct(
			Vertex(
					intersection_vertex,
					boost::none/*point*/,
					is_unwrapped_point,
					segment_interpolation_ratio,
					segment_index,
					geometry_part_index,
					true/*is_intersection*/));

	// Insert into the dateline vertices sequence.
	// But we need to insert in the correct location so that the vertices on the dateline follow
	// a continuous loop around the dateline polygon (ie, vertices must be sorted).
	const vertex_list_type::iterator insert_begin(*d_dateline_corner_south_back);
	const vertex_list_type::iterator insert_end = d_dateline_vertices.end();
	vertex_list_type::iterator insert_iter = insert_begin;
	for ( ; insert_iter != insert_end; ++insert_iter)
	{
		// Keeping iterating until we find a vertex with a larger longitude.
		// NOTE: This is the reverse comparison to that of the north pole.
		if (original_vertex_longitude <= insert_iter.get()->element().lat_lon_point.longitude())
		{
			// Insert before the current vertex.
			break;
		}
	}

	// Insert the intersection vertex at the correct location in the dateline vertices list.
	dateline_vertex_node->splice_self_before(*insert_iter.get());

	// Link the two intersection nodes together so we can later jump from one sequence to the other.
	link_intersection_vertices(
			*line_geometry.vertices, geometry_vertex_node,
			d_dateline_vertices, dateline_vertex_node);
}


GPlatesMaths::DateLineWrapper::CentralMeridian::CentralMeridian(
		const double &longitude_) :
	longitude(longitude_),
	rotate_to_dateline_frame(
			// Rotates, about north pole, to move central meridian longitude to zero longitude...
			Rotation::create(
					UnitVector3D::zBasis()/*north pole*/,
					convert_deg_to_rad(-longitude_))),
	rotate_from_dateline_frame(rotate_to_dateline_frame.get_reverse())
{
}
