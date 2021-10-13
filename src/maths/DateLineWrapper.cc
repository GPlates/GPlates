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
#include <boost/bind.hpp>

#include "DateLineWrapper.h"

#include "GreatCircleArc.h"
#include "SmallCircleBounds.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


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
	}
}


void
GPlatesMaths::DateLineWrapper::wrap_to_dateline(
		const PolylineOnSphere::non_null_ptr_to_const_type &input_polyline,
		std::vector<lat_lon_polyline_type> &output_polylines)
{
	if (!possibly_intersects_dateline(input_polyline))
	{
		// No intersection with the dateline so just convert entire geometry to lat/lon coordinates.
		lat_lon_polygon_type lat_lon_polyline(new lat_lon_points_seq_type());
		lat_lon_polyline->reserve(input_polyline->number_of_vertices());

		std::transform(
				input_polyline->vertex_begin(),
				input_polyline->vertex_end(),
				std::back_inserter(*lat_lon_polyline),
				&make_lat_lon_point);

		output_polylines.push_back(lat_lon_polyline);

		return;
	}

	IntersectionGraph graph(false/*is_polygon_graph*/);
	generate_intersection_graph(
			graph,
			input_polyline->begin(),
			input_polyline->end(),
			false/*is_polygon*/);

	graph.generate_polylines(output_polylines);
}


void
GPlatesMaths::DateLineWrapper::wrap_to_dateline(
		const PolygonOnSphere::non_null_ptr_to_const_type &input_polygon,
		std::vector<lat_lon_polygon_type> &output_polygons)
{
	if (!possibly_intersects_dateline(input_polygon))
	{
		// No intersection with the dateline so just convert entire geometry to lat/lon coordinates.
		lat_lon_polygon_type lat_lon_polygon(new lat_lon_points_seq_type());
		lat_lon_polygon->reserve(input_polygon->number_of_vertices());

		std::transform(
				input_polygon->vertex_begin(),
				input_polygon->vertex_end(),
				std::back_inserter(*lat_lon_polygon),
				&make_lat_lon_point);

		output_polygons.push_back(lat_lon_polygon);

		return;
	}

	IntersectionGraph graph(true/*is_polygon_graph*/);
	generate_intersection_graph(
			graph,
			input_polygon->begin(),
			input_polygon->end(),
			true/*is_polygon*/);

	graph.generate_polygons(output_polygons, input_polygon);
}


bool
GPlatesMaths::DateLineWrapper::intersects_dateline(
		const BoundingSmallCircle &geometry_bounding_small_circle) const
{
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
		const real_t dot_centroid_and_closest_of_front_or_back_half_space_normal =
				// NOTE: 'dval' means not using epsilon test here...
				(dot_centroid_and_front_half_space_normal.dval() > 0)
				? dot_centroid_and_front_half_space_normal
				: -dot_centroid_and_front_half_space_normal;

		// We only used 'real_t' to take advantage of range testing in 'sqrt'.
		const double sine_angle_from_geometry_centroid_to_dateline_arc_normal =
				sqrt(1 - dot_centroid_and_closest_of_front_or_back_half_space_normal *
						dot_centroid_and_closest_of_front_or_back_half_space_normal).dval();

		// cosine(angle_from_geometry_centroid_to_front_half_space_normal)...
		const double &cosine_angle_from_geometry_centroid_to_dateline_arc_normal =
				dot_centroid_and_closest_of_front_or_back_half_space_normal.dval();

		// NOTE: No epsilon testing here...
		return 0 >=
				geometry_bounding_small_circle.get_small_circle_boundary_cosine() *
					cosine_angle_from_geometry_centroid_to_dateline_arc_normal -
				geometry_bounding_small_circle.get_small_circle_boundary_sine() *
					sine_angle_from_geometry_centroid_to_dateline_arc_normal;
	}
	else
	{
		// Only need to test distance of geometry centroid to north or south pole.
		return geometry_bounding_small_circle.test(NORTH_POLE) != BoundingSmallCircle::OUTSIDE_BOUNDS ||
			geometry_bounding_small_circle.test(SOUTH_POLE) != BoundingSmallCircle::OUTSIDE_BOUNDS;
	}
}


bool
GPlatesMaths::DateLineWrapper::possibly_intersects_dateline(
		const PolylineOnSphere::non_null_ptr_to_const_type &polyline) const
{
	return intersects_dateline(polyline->get_bounding_small_circle());
}


bool
GPlatesMaths::DateLineWrapper::possibly_intersects_dateline(
		const PolygonOnSphere::non_null_ptr_to_const_type &polygon) const
{
	return intersects_dateline(polygon->get_bounding_small_circle());
}


template <typename LineSegmentForwardIter>
void
GPlatesMaths::DateLineWrapper::generate_intersection_graph(
		IntersectionGraph &graph,
		LineSegmentForwardIter const line_segments_begin,
		LineSegmentForwardIter const line_segments_end,
		bool is_polygon)
{
	// PolylineOnSphere and PolygonOnSphere ensure at least 1 (and 2) line segments.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			line_segments_begin != line_segments_end,
			GPLATES_ASSERTION_SOURCE);

	// Classify the first point.
	const GreatCircleArc &first_line_segment = *line_segments_begin;
	const VertexClassification first_vertex_classification =
			classify_vertex(first_line_segment.start_point().position_vector(), graph);

	if (!is_polygon)
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
			graph.add_vertex(first_line_segment.start_point());
			break;
		case CLASSIFY_ON_DATELINE_ARC:
		case CLASSIFY_ON_NORTH_POLE:
		case CLASSIFY_ON_SOUTH_POLE:
			// Note that we don't add a vertex if it's on the dateline (or its poles).
			break;
		}
	}
	// Note that if the geometry *is* a polygon then its last line segment will wrap around
	// back to the start point so the start point will get handled as part of the loop below.

	VertexClassification previous_end_vertex_classification = first_vertex_classification;

	for (LineSegmentForwardIter line_segment_iter = line_segments_begin;
		line_segment_iter != line_segments_end;
		++line_segment_iter)
	{
		const GreatCircleArc &current_line_segment = *line_segment_iter;

		const VertexClassification current_end_vertex_classification =
				classify_vertex(current_line_segment.end_point().position_vector(), graph);

		// Note that the end point of the previous GCA matches the start point of the current GCA.
		add_line_segment_to_intersection_graph(
				graph,
				current_line_segment,
				previous_end_vertex_classification,
				current_end_vertex_classification);

		previous_end_vertex_classification = current_end_vertex_classification;
	}
}


void
GPlatesMaths::DateLineWrapper::add_line_segment_to_intersection_graph(
		IntersectionGraph &graph,
		const GreatCircleArc &line_segment,
		VertexClassification line_segment_start_vertex_classification,
		VertexClassification line_segment_end_vertex_classification)
{
	switch (line_segment_start_vertex_classification)
	{
	case CLASSIFY_FRONT:
		switch (line_segment_end_vertex_classification)
		{
		case CLASSIFY_FRONT:
			graph.add_vertex(line_segment.end_point());
			break;
		case CLASSIFY_BACK:
			// NOTE: Front-to-back and back-to-front transitions are the only cases where we do
			// line segment intersection tests (of geometry line segment with dateline).
			{
				IntersectionType intersection_type;
				boost::optional<PointOnSphere> intersection_point =
						intersect_line_segment(intersection_type, line_segment, true, graph);
				// We only emit a vertex on the dateline if there was an intersection.
				if (intersection_point)
				{
					switch (intersection_type)
					{
					case INTERSECTED_DATELINE:
						// Line segment is front-to-back as it crosses the dateline.
						graph.add_intersection_vertex_on_front_dateline(intersection_point.get(), true);
						graph.add_intersection_vertex_on_back_dateline(intersection_point.get(), false);
						break;
					case INTERSECTED_NORTH_POLE:
						// Use longitude of 'start' point as longitude of first intersection point.
						// Use longitude of 'end' point as longitude of second intersection point.
						// This results in meridian lines being vertical lines in rectangular coordinates.
						// Here the two longitudes will be separated by 180 degrees (or very close to).
						graph.add_intersection_vertex_on_north_pole(line_segment.start_point(), true);
						graph.add_intersection_vertex_on_north_pole(line_segment.end_point(), false);
						break;
					case INTERSECTED_SOUTH_POLE:
						// Use longitude of 'start' point as longitude of first intersection point.
						// Use longitude of 'end' point as longitude of second intersection point.
						// This results in meridian lines being vertical lines in rectangular coordinates.
						// Here the two longitudes will be separated by 180 degrees (or very close to).
						graph.add_intersection_vertex_on_south_pole(line_segment.start_point(), true);
						graph.add_intersection_vertex_on_south_pole(line_segment.end_point(), false);
						break;
					default:
						GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
						break;
					}
				}
			}
			graph.add_vertex(line_segment.end_point());
			break;
		case CLASSIFY_OFF_DATELINE_ARC_ON_PLANE:
			graph.add_vertex(line_segment.end_point());
			break;
		case CLASSIFY_ON_DATELINE_ARC:
			// Use latitude of 'end' point as latitude of intersection point.
			graph.add_intersection_vertex_on_front_dateline(line_segment.end_point(), true);
			break;
		case CLASSIFY_ON_NORTH_POLE:
			// Use longitude of 'start' point as longitude of intersection point.
			// This results in meridian lines being vertical lines in rectangular coordinates.
			graph.add_intersection_vertex_on_north_pole(line_segment.start_point(), true);
			break;
		case CLASSIFY_ON_SOUTH_POLE:
			// Use longitude of 'start' point as longitude of intersection point.
			// This results in meridian lines being vertical lines in rectangular coordinates.
			graph.add_intersection_vertex_on_south_pole(line_segment.start_point(), true);
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
				boost::optional<PointOnSphere> intersection_point =
						intersect_line_segment(intersection_type, line_segment, false, graph);
				// We only emit a vertex on the dateline if there was an intersection.
				if (intersection_point)
				{
					switch (intersection_type)
					{
					case INTERSECTED_DATELINE:
						// Line segment is back-to-front as it crosses the dateline.
						graph.add_intersection_vertex_on_back_dateline(intersection_point.get(), true);
						graph.add_intersection_vertex_on_front_dateline(intersection_point.get(), false);
						break;
					case INTERSECTED_NORTH_POLE:
						// Use longitude of 'start' point as longitude of first intersection point.
						// Use longitude of 'end' point as longitude of second intersection point.
						// This results in meridian lines being vertical lines in rectangular coordinates.
						// Here the two longitudes will be separated by 180 degrees (or very close to).
						graph.add_intersection_vertex_on_north_pole(line_segment.start_point(), true);
						graph.add_intersection_vertex_on_north_pole(line_segment.end_point(), false);
						break;
					case INTERSECTED_SOUTH_POLE:
						// Use longitude of 'start' point as longitude of first intersection point.
						// Use longitude of 'end' point as longitude of second intersection point.
						// This results in meridian lines being vertical lines in rectangular coordinates.
						// Here the two longitudes will be separated by 180 degrees (or very close to).
						graph.add_intersection_vertex_on_south_pole(line_segment.start_point(), true);
						graph.add_intersection_vertex_on_south_pole(line_segment.end_point(), false);
						break;
					default:
						GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
						break;
					}
				}
			}
			graph.add_vertex(line_segment.end_point());
			break;
		case CLASSIFY_BACK:
			graph.add_vertex(line_segment.end_point());
			break;
		case CLASSIFY_OFF_DATELINE_ARC_ON_PLANE:
			graph.add_vertex(line_segment.end_point());
			break;
		case CLASSIFY_ON_DATELINE_ARC:
			// Use latitude of 'end' point as latitude of intersection point.
			graph.add_intersection_vertex_on_back_dateline(line_segment.end_point(), true);
			break;
		case CLASSIFY_ON_NORTH_POLE:
			// Use longitude of 'start' point as longitude of intersection point.
			// This results in meridian lines being vertical lines in rectangular coordinates.
			graph.add_intersection_vertex_on_north_pole(line_segment.start_point(), true);
			break;
		case CLASSIFY_ON_SOUTH_POLE:
			// Use longitude of 'start' point as longitude of intersection point.
			// This results in meridian lines being vertical lines in rectangular coordinates.
			graph.add_intersection_vertex_on_south_pole(line_segment.start_point(), true);
			break;
		}
		break;

	case CLASSIFY_OFF_DATELINE_ARC_ON_PLANE:
		switch (line_segment_end_vertex_classification)
		{
		case CLASSIFY_FRONT:
			graph.add_vertex(line_segment.end_point());
			break;
		case CLASSIFY_BACK:
			graph.add_vertex(line_segment.end_point());
			break;
		case CLASSIFY_OFF_DATELINE_ARC_ON_PLANE:
			graph.add_vertex(line_segment.end_point());
			break;
		case CLASSIFY_ON_DATELINE_ARC:
			// First we have to decide if the current line segment passed through the north or south pole.
			// Also note that we add the 'start' point, and not the end point, since it's off
			// the dateline and hence it's longitude is used for intersection point.
			// The longitude will be very close to zero since both start and end are on the 'thick' plane.
			if (does_line_segment_on_dateline_plane_cross_north_pole(
					line_segment,
					false/*is_line_segment_start_point_on_dateline*/))
			{
				graph.add_intersection_vertex_on_north_pole(line_segment.start_point(), true);
			}
			else
			{
				graph.add_intersection_vertex_on_south_pole(line_segment.start_point(), true);
			}
			break;
		case CLASSIFY_ON_NORTH_POLE:
			// Use longitude of 'start' point as longitude of intersection point.
			// It'll be very close to zero.
			graph.add_intersection_vertex_on_north_pole(line_segment.start_point(), true);
			break;
		case CLASSIFY_ON_SOUTH_POLE:
			// Use longitude of 'start' point as longitude of intersection point.
			// It'll be very close to zero.
			graph.add_intersection_vertex_on_south_pole(line_segment.start_point(), true);
			break;
		}
		break;

	case CLASSIFY_ON_DATELINE_ARC:
		switch (line_segment_end_vertex_classification)
		{
		case CLASSIFY_FRONT:
			// Use latitude of 'start' point as latitude of intersection point.
			graph.add_intersection_vertex_on_front_dateline(line_segment.start_point(), false);
			graph.add_vertex(line_segment.end_point());
			break;
		case CLASSIFY_BACK:
			// Use latitude of 'start' point as latitude of intersection point.
			graph.add_intersection_vertex_on_back_dateline(line_segment.start_point(), false);
			graph.add_vertex(line_segment.end_point());
			break;
		case CLASSIFY_OFF_DATELINE_ARC_ON_PLANE:
			// First we have to decide if the current line segment passed through the north or south pole.
			// Also note that we add the 'end' point, and not the start point, since it's off
			// the dateline and hence it's longitude is used for intersection point.
			// The longitude will be very close to zero since both start and end are on the 'thick' plane.
			if (does_line_segment_on_dateline_plane_cross_north_pole(
					line_segment,
					true/*is_line_segment_start_point_on_dateline*/))
			{
				graph.add_intersection_vertex_on_north_pole(line_segment.end_point(), false);
			}
			else
			{
				graph.add_intersection_vertex_on_south_pole(line_segment.end_point(), false);
			}
			graph.add_vertex(line_segment.end_point());
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
			graph.add_intersection_vertex_on_north_pole(line_segment.end_point(), false);
			graph.add_vertex(line_segment.end_point());
			break;
		case CLASSIFY_BACK:
			// Use longitude of 'end' point as longitude of intersection point.
			// This results in meridian lines being vertical lines in rectangular coordinates.
			graph.add_intersection_vertex_on_north_pole(line_segment.end_point(), false);
			graph.add_vertex(line_segment.end_point());
			break;
		case CLASSIFY_OFF_DATELINE_ARC_ON_PLANE:
			// Use longitude of 'end' point as longitude of intersection point.
			// It'll be very close to zero.
			graph.add_intersection_vertex_on_north_pole(line_segment.end_point(), false);
			graph.add_vertex(line_segment.end_point());
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
			graph.add_intersection_vertex_on_south_pole(line_segment.end_point(), false);
			graph.add_vertex(line_segment.end_point());
			break;
		case CLASSIFY_BACK:
			// Use longitude of 'end' point as longitude of intersection point.
			// This results in meridian lines being vertical lines in rectangular coordinates.
			graph.add_intersection_vertex_on_south_pole(line_segment.end_point(), false);
			graph.add_vertex(line_segment.end_point());
			break;
		case CLASSIFY_OFF_DATELINE_ARC_ON_PLANE:
			// Use longitude of 'end' point as longitude of intersection point.
			// It'll be very close to zero.
			graph.add_intersection_vertex_on_south_pole(line_segment.end_point(), false);
			graph.add_vertex(line_segment.end_point());
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
GPlatesMaths::DateLineWrapper::intersect_line_segment(
		IntersectionType &intersection_type,
		const GreatCircleArc &line_segment,
		bool line_segment_start_point_in_dateline_front_half_space,
		IntersectionGraph &graph) const
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
		return line_segment.end_point();
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
			return calculate_intersection(line_segment);
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
			return calculate_intersection(line_segment);
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
			graph.intersected_south_pole();
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
			graph.intersected_north_pole();
			return PointOnSphere(NORTH_POLE);
		}
	}

	// No intersection detected.
	return boost::none;
}


boost::optional<GPlatesMaths::PointOnSphere>
GPlatesMaths::DateLineWrapper::calculate_intersection(
		const GreatCircleArc &line_segment) const
{
	// Determine the signed distances of the line segments endpoints from the dateline plane.
	const real_t signed_distance_line_segment_start_point_to_dateline_plane =
			dot(FRONT_HALF_SPACE_NORMAL, line_segment.start_point().position_vector());
	const real_t signed_distance_line_segment_end_point_to_dateline_plane =
			dot(FRONT_HALF_SPACE_NORMAL, line_segment.end_point().position_vector());

	// The denominator of the ratios used to interpolate the line segment endpoints.
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
			signed_distance_line_segment_start_point_to_dateline_plane * inv_denom *
					line_segment.end_point().position_vector() -
			signed_distance_line_segment_end_point_to_dateline_plane * inv_denom *
					line_segment.start_point().position_vector();

	// Normalise to get a unit vector.
	if (interpolated_line_segment.magSqrd() <= 0 /*this is a floating-point comparison epsilon test*/)
	{
		// This shouldn't happen unless the line segment end points are antipodal to each other
		// and 'GreatCircleArc' should not have allowed this.
		// If the end points are that close to being antipodal then we can argue that the line segment
		// arc takes an arc path on the other side of the globe and hence misses the dateline.
		// If this happens then just return no intersection.
		return boost::none;
	}

	return PointOnSphere(interpolated_line_segment.get_normalisation());
}


GPlatesMaths::DateLineWrapper::VertexClassification
GPlatesMaths::DateLineWrapper::classify_vertex(
		const UnitVector3D &vertex,
		IntersectionGraph &graph) const
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
		graph.intersected_north_pole();
		return CLASSIFY_ON_NORTH_POLE;
	}

	// NOTE: 'dval' means bypassing the epsilon test of 'real_t' - we have our own epsilon.
	if (dot(vertex, SOUTH_POLE).dval() > EPSILON_THICK_PLANE_COSINE)
	{
		graph.intersected_south_pole();
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


// Note that the value doesn't matter - it's just used when constructing list sentinel nodes.
const GPlatesMaths::DateLineWrapper::Vertex
GPlatesMaths::DateLineWrapper::IntersectionGraph::LISTS_SENTINEL(LatLonPoint(0, 0));


GPlatesMaths::DateLineWrapper::IntersectionGraph::IntersectionGraph(
		bool is_polygon_graph_) :
	d_geometry_vertices(LISTS_SENTINEL),
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
GPlatesMaths::DateLineWrapper::IntersectionGraph::generate_polylines(
		std::vector<lat_lon_polyline_type> &lat_lon_polylines)
{
	// For polylines we only need to iterate over the geometry vertices and not the dateline vertices.
	vertex_list_type::const_iterator geometry_vertices_iter = d_geometry_vertices.begin();
	while (geometry_vertices_iter != d_geometry_vertices.end())
	{
		// Start a new polyline.
		lat_lon_polyline_type current_polyline(new lat_lon_points_seq_type());
		lat_lon_polylines.push_back(current_polyline);

		const Vertex &start_polyline_vertex = *geometry_vertices_iter;

		// Each start point of a new polyline (except the first polyline) should be an intersection point.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				start_polyline_vertex.is_intersection ||
					geometry_vertices_iter == d_geometry_vertices.begin(),
				GPLATES_ASSERTION_SOURCE);

		// At the polyline start point.
		current_polyline->push_back(start_polyline_vertex.point);

		// Add the remaining vertices of the current polyline.
		// The current polyline stops when we hit another intersection point (or reach end of original polyline).
		for (++geometry_vertices_iter;
			geometry_vertices_iter != d_geometry_vertices.end();
			++geometry_vertices_iter)
		{
			const Vertex &geometry_vertex = *geometry_vertices_iter;

			current_polyline->push_back(geometry_vertex.point);

			if (geometry_vertex.is_intersection)
			{
				++geometry_vertices_iter;
				// End the current polyline.
				break;
			}
		}
	}

	// Note that it is possible that all the original polyline line segments got swallowed by the dateline.
	// This can happen if the original polyline is entirely *on* the dateline which is considered
	// to be *outside* the dateline polygon (which covers the entire globe and 'effectively' excludes
	// a very thin area of size epsilon around the dateline arc).
	if (d_geometry_vertices.begin() != d_geometry_vertices.end())
	{
		// The last polyline added must have at least two points.
		// All prior polylines are guaranteed to have at least two points by the way vertices
		// are added to them in the above loop.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				lat_lon_polylines.back()->size() >= 2,
				GPLATES_ASSERTION_SOURCE);
	}
}


void
GPlatesMaths::DateLineWrapper::IntersectionGraph::generate_polygons(
		std::vector<lat_lon_polygon_type> &lat_lon_polygons,
		const PolygonOnSphere::non_null_ptr_to_const_type &input_polygon)
{
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
		// There were no intersections so just output the single non-intersected input polygon.

		// Start a polygon.
		lat_lon_polygon_type polygon(new lat_lon_points_seq_type());
		lat_lon_polygons.push_back(polygon);

		// Copy the geometry vertex positions into the polygon.
		std::transform(
				d_geometry_vertices.begin(),
				d_geometry_vertices.end(),
				std::back_inserter(*polygon),
				boost::bind(&Vertex::point, _1));

		return;
	}

	//
	// Generate flags indicating which intersection vertices enter/exit the geometry polygon interior.
	//
	generate_entry_exit_flags_for_dateline_polygon(input_polygon);

	//
	// Iterate over the intersection graph and output the polygons.
	//
	output_intersecting_polygons(lat_lon_polygons);
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
		const PointInPolygon::Result north_pole_in_geometry_polygon_result =
				input_polygon->is_point_in_polygon(
						PointOnSphere(NORTH_POLE),
						PolygonOnSphere::LOW_SPEED_NO_SETUP_NO_MEMORY_USAGE);

		// Generate flags indicating which intersection vertices enter/exit the geometry polygon interior.
		generate_entry_exit_flags_for_dateline_polygon(
				// Arbitrarily choose an original (non-intersection) dateline vertex that maps to the north pole...
				vertex_list_type::iterator(*d_dateline_corner_north_front),
				north_pole_in_geometry_polygon_result == PointInPolygon::POINT_INSIDE_POLYGON);
	}
	// Else if the geometry polygon does *not* intersect the south pole then we can accurately determine
	// whether the south pole is inside/outside the geometry polygon.
	else if (!d_geometry_intersected_south_pole)
	{
		// See if the south pole is inside or outside the geometry polygon.
		const PointInPolygon::Result south_pole_in_geometry_polygon_result =
				input_polygon->is_point_in_polygon(
						PointOnSphere(SOUTH_POLE),
						PolygonOnSphere::LOW_SPEED_NO_SETUP_NO_MEMORY_USAGE);

		// Generate flags indicating which intersection vertices enter/exit the geometry polygon interior.
		generate_entry_exit_flags_for_dateline_polygon(
				// Arbitrarily choose an original (non-intersection) dateline vertex that maps to the south pole...
				vertex_list_type::iterator(*d_dateline_corner_south_front),
				south_pole_in_geometry_polygon_result == PointInPolygon::POINT_INSIDE_POLYGON);
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
		std::vector<lat_lon_polygon_type> &lat_lon_polygons)
{
	//
	// NOTE: If we get here then the input polygon intersected the dateline and so all output
	// polygons can be found by traversing the (intersection) dateline vertices.
	// So it's not possible to have any output polygons that *only* exist in 'd_geometry_vertices'.
	// Hence we can find all output polygons by searching through 'd_dateline_vertices'.
	//

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
		lat_lon_polygon_type current_output_polygon(new lat_lon_points_seq_type());
		lat_lon_polygons.push_back(current_output_polygon);

		// Used to keep track of which vertex list we are traversing as we alternate between
		// geometry and dateline lists to map the path of the current output polygon.
		vertex_list_type *vertex_lists[2] =
		{
			&d_dateline_vertices,
			&d_geometry_vertices
		};

		// We start out traversing the dateline vertices list first.
		unsigned int current_vertex_list = 0;

		// Iterate over the output polygon vertices until we return to the start of the output polygon.
		vertex_list_type::iterator output_polygon_vertices_iter = output_polygon_start_vertices_iter;
		// NOTE: The value doesn't actually matter because we immediately changes lists upon entering loop.
		bool list_traversal_is_forward = true;
		do
		{
			Vertex &output_polygon_vertex = *output_polygon_vertices_iter;

			// Add the current vertex to the current output polygon.
			current_output_polygon->push_back(output_polygon_vertex.point);
			output_polygon_vertex.used_to_output_polygon = true;

			// At intersection vertices we need to jump lists.
			if (output_polygon_vertex.is_intersection)
			{
				// The matching intersection vertex node in the other vertex list.
				vertex_list_type::Node *intersection_neighbour =
						static_cast<vertex_list_type::Node *>(
								output_polygon_vertex.intersection_neighbour);

				// Switch iteration over to the other vertex list (geometry <-> dateline).
				output_polygon_vertices_iter = vertex_list_type::iterator(*intersection_neighbour);

				// There are two intersection vertices that are the same point.
				// One is in the dateline vertices list and the other in the geometry list.
				// Both copies need to be marked as used.
				output_polygon_vertices_iter->used_to_output_polygon = true;

				// Determine the traversal direction of the other polygon vertex list.
				// If normal forward list traversal means exiting other polygon then we need
				// to traverse in the backwards direction instead.
				list_traversal_is_forward = !output_polygon_vertices_iter->exits_other_polygon;

				// Toggle the vertex list we're currently traversing.
				current_vertex_list ^= 1;
			}

			// Move to the next output polygon vertex.
			if (list_traversal_is_forward)
			{
				++output_polygon_vertices_iter;
				// Handle list wraparound.
				if (output_polygon_vertices_iter == vertex_lists[current_vertex_list]->end())
				{
					output_polygon_vertices_iter = vertex_lists[current_vertex_list]->begin();
				}
			}
			else // traversing the list backwards...
			{
				// Handle list wraparound.
				if (output_polygon_vertices_iter == vertex_lists[current_vertex_list]->begin())
				{
					output_polygon_vertices_iter = vertex_lists[current_vertex_list]->end();
				}
				--output_polygon_vertices_iter;
			}
		}
		while (output_polygon_vertices_iter != output_polygon_start_vertices_iter);
	}
}


void
GPlatesMaths::DateLineWrapper::IntersectionGraph::add_vertex(
			const PointOnSphere &point)
{
	// Convert from cartesian to lat/lon coordinates.
	const LatLonPoint vertex = make_lat_lon_point(point);

	// Create an intersection vertex wrapped in a list node.
	vertex_list_type::Node *vertex_node = d_vertex_node_pool.construct(Vertex(vertex));

	// Add to the geometry sequence.
	d_geometry_vertices.append(*vertex_node);
}


void
GPlatesMaths::DateLineWrapper::IntersectionGraph::add_intersection_vertex_on_front_dateline(
		const PointOnSphere &point,
		bool exiting_dateline_polygon)
{
	// Override the point's longitude with that of the dateline (from the front which is 180 degrees).
	const LatLonPoint original_vertex = make_lat_lon_point(point);
	const double &original_vertex_latitude = original_vertex.latitude();
	const LatLonPoint intersection_vertex(original_vertex_latitude, 180);

	// Create a copy of the intersection vertex for the geometry list.
	vertex_list_type::Node *geometry_vertex_node = d_vertex_node_pool.construct(
			Vertex(intersection_vertex, true/*is_intersection*/, exiting_dateline_polygon));

	// Append to the end of to the geometry sequence.
	d_geometry_vertices.append(*geometry_vertex_node);

	// If we're graphing a polyline then no need to go any further.
	if (!d_is_polygon_graph)
	{
		return;
	}

	// Create another copy of the intersection vertex for the dateline list.
	// NOTE: We determine the 'exits_other_polygon' vertex flag later.
	vertex_list_type::Node *dateline_vertex_node = d_vertex_node_pool.construct(
			Vertex(intersection_vertex, true/*is_intersection*/));

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
		if (original_vertex_latitude <= insert_iter.get()->element().point.latitude())
		{
			// Insert before the current vertex.
			break;
		}
	}

	// Insert the intersection vertex at the correct location in the dateline vertices list.
	dateline_vertex_node->splice_self_before(*insert_iter.get());

	// Link the two intersection nodes together so we can later jump from one sequence to the other.
	link_intersection_vertices(geometry_vertex_node, dateline_vertex_node);
}


void
GPlatesMaths::DateLineWrapper::IntersectionGraph::add_intersection_vertex_on_back_dateline(
		const PointOnSphere &point,
		bool exiting_dateline_polygon)
{
	// Override the point's longitude with that of the dateline (from the back which is -180 degrees).
	const LatLonPoint original_vertex = make_lat_lon_point(point);
	const double &original_vertex_latitude = original_vertex.latitude();
	const LatLonPoint intersection_vertex(original_vertex_latitude, -180);

	// Create a copy of the intersection vertex for the geometry list.
	vertex_list_type::Node *geometry_vertex_node = d_vertex_node_pool.construct(
			Vertex(intersection_vertex, true/*is_intersection*/, exiting_dateline_polygon));

	// Append to the end of to the geometry sequence.
	d_geometry_vertices.append(*geometry_vertex_node);

	// If we're graphing a polyline then no need to go any further.
	if (!d_is_polygon_graph)
	{
		return;
	}

	// Create another copy of the intersection vertex for the dateline list.
	// NOTE: We determine the 'exits_other_polygon' vertex flag later.
	vertex_list_type::Node *dateline_vertex_node = d_vertex_node_pool.construct(
			Vertex(intersection_vertex, true/*is_intersection*/));

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
		if (original_vertex_latitude >= insert_iter.get()->element().point.latitude())
		{
			// Insert before the current vertex.
			break;
		}
	}

	// Insert the intersection vertex at the correct location in the dateline vertices list.
	dateline_vertex_node->splice_self_before(*insert_iter.get());

	// Link the two intersection nodes together so we can later jump from one sequence to the other.
	link_intersection_vertices(geometry_vertex_node, dateline_vertex_node);
}


void
GPlatesMaths::DateLineWrapper::IntersectionGraph::add_intersection_vertex_on_north_pole(
		const PointOnSphere &point,
		bool exiting_dateline_polygon)
{
	// Override the point's latitude with that of the north pole's.
	const LatLonPoint original_vertex = make_lat_lon_point(point);
	const double &original_vertex_longitude = original_vertex.longitude();
	const LatLonPoint intersection_vertex(90, original_vertex_longitude);

	// Create a copy of the intersection vertex for the geometry list.
	vertex_list_type::Node *geometry_vertex_node = d_vertex_node_pool.construct(
			Vertex(intersection_vertex, true/*is_intersection*/, exiting_dateline_polygon));

	// Append to the end of to the geometry sequence.
	d_geometry_vertices.append(*geometry_vertex_node);

	// If we're graphing a polyline then no need to go any further.
	if (!d_is_polygon_graph)
	{
		return;
	}

	// Create another copy of the intersection vertex for the dateline list.
	// NOTE: We determine the 'exits_other_polygon' vertex flag later.
	vertex_list_type::Node *dateline_vertex_node = d_vertex_node_pool.construct(
			Vertex(intersection_vertex, true/*is_intersection*/));

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
		if (original_vertex_longitude >= insert_iter.get()->element().point.longitude())
		{
			// Insert before the current vertex.
			break;
		}
	}

	// Insert the intersection vertex at the correct location in the dateline vertices list.
	dateline_vertex_node->splice_self_before(*insert_iter.get());

	// Link the two intersection nodes together so we can later jump from one sequence to the other.
	link_intersection_vertices(geometry_vertex_node, dateline_vertex_node);
}


void
GPlatesMaths::DateLineWrapper::IntersectionGraph::add_intersection_vertex_on_south_pole(
		const PointOnSphere &point,
		bool exiting_dateline_polygon)
{
	// Override the point's latitude with that of the south pole's.
	const LatLonPoint original_vertex = make_lat_lon_point(point);
	const double &original_vertex_longitude = original_vertex.longitude();
	const LatLonPoint intersection_vertex(-90, original_vertex_longitude);

	// Create a copy of the intersection vertex for the geometry list.
	vertex_list_type::Node *geometry_vertex_node = d_vertex_node_pool.construct(
			Vertex(intersection_vertex, true/*is_intersection*/, exiting_dateline_polygon));

	// Append to the end of to the geometry sequence.
	d_geometry_vertices.append(*geometry_vertex_node);

	// If we're graphing a polyline then no need to go any further.
	if (!d_is_polygon_graph)
	{
		return;
	}

	// Create another copy of the intersection vertex for the dateline list.
	// NOTE: We determine the 'exits_other_polygon' vertex flag later.
	vertex_list_type::Node *dateline_vertex_node = d_vertex_node_pool.construct(
			Vertex(intersection_vertex, true/*is_intersection*/));

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
		if (original_vertex_longitude <= insert_iter.get()->element().point.longitude())
		{
			// Insert before the current vertex.
			break;
		}
	}

	// Insert the intersection vertex at the correct location in the dateline vertices list.
	dateline_vertex_node->splice_self_before(*insert_iter.get());

	// Link the two intersection nodes together so we can later jump from one sequence to the other.
	link_intersection_vertices(geometry_vertex_node, dateline_vertex_node);
}
