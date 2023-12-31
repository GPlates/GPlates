/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2018 The University of Sydney, Australia
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

#include <utility>
#include <vector>
#include <boost/optional.hpp>

#include "ResolvedSubSegmentRangeInSection.h"

#include "GeometryUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "maths/GreatCircleArc.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"


GPlatesAppLogic::ResolvedSubSegmentRangeInSection::ResolvedSubSegmentRangeInSection(
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type section_geometry,
		boost::optional<IntersectionOrRubberBand> start_intersection_or_rubber_band,
		boost::optional<IntersectionOrRubberBand> end_intersection_or_rubber_band) :
	d_section_geometry(section_geometry),
	d_num_points_in_section_geometry(GeometryUtils::get_num_geometry_exterior_points(*section_geometry))
{
	// Section geometry must be a point, multi-point or polyline.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GeometryUtils::get_geometry_type(*d_section_geometry) != GPlatesMaths::GeometryType::POLYGON,
			GPLATES_ASSERTION_SOURCE);

	//
	// Set the start/end intersection/rubber-band.
	//

	if (start_intersection_or_rubber_band)
	{
		// Either an intersection or a rubber band.
		if (boost::optional<const Intersection &> start_intersection =
			start_intersection_or_rubber_band->get_intersection())
		{
			d_start_intersection = start_intersection.get();
		}
		else
		{
			d_start_rubber_band = start_intersection_or_rubber_band->get_rubber_band().get();
		}
	}

	if (end_intersection_or_rubber_band)
	{
		// Either an intersection or a rubber band.
		if (boost::optional<const Intersection &> end_intersection =
			end_intersection_or_rubber_band->get_intersection())
		{
			d_end_intersection = end_intersection.get();
		}
		else
		{
			d_end_rubber_band = end_intersection_or_rubber_band->get_rubber_band().get();
		}
	}

	//
	// Set the vertex index of first and one-past-the-last vertices to include in this sub-segment.
	//
	// This is only affected by intersections (not rubber banding).
	//

	if (d_start_intersection &&
		d_end_intersection)
	{
		// Make sure the caller passed an end intersection that is equal to or greater than the start intersection.
		//
		// Note: This uses an epsilon test of angle (when both intersections on same segment).
		// In other words, a little more forgiving than Intersection::operator<().
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				d_start_intersection->segment_index < d_end_intersection->segment_index ||
					(d_start_intersection->segment_index == d_end_intersection->segment_index &&
						d_start_intersection->angle_in_segment <= d_end_intersection->angle_in_segment),
				GPLATES_ASSERTION_SOURCE);

		// Start at the end point of the segment containing the start intersection
		// (the "+1" increments from start of segment to end of segment, which is also start of next segment).
		//
		// GeometryIntersect guarantees an intersection point will not be recorded at the *end* point
		// of a segment since that would instead be recorded as the *start* point of the *next* segment
		// (which can be the fictitious one-past-the-last segment).
		// So our start intersection point can never replace the end point of the segment it intersects.
		d_start_section_vertex_index = d_start_intersection->segment_index + 1;

		// End at the start point of the segment containing the end intersection
		// (the "+1" increments one-past-the-start of segment).
		d_end_section_vertex_index = d_end_intersection->segment_index + 1;

		// GeometryIntersect guarantees an intersection point will not be recorded at the *end* point
		// of a segment since that would instead be recorded as the *start* point of the *next* segment
		// (which can be the fictitious one-past-the-last segment). But it can be recorded at the
		// *start* point of a segment. So our end intersection point *can* replace the start point
		// of the segment it intersects (this differs from the start intersection which can *never*
		// replace the end point of the segment it intersects).
		// If the end intersection does coincide with the start of a segment then we need to copy
		// one less point from the section geometry (that point being the start of the segment).
		//
		// If the end intersection point *is* at the start point of segment.
		if (d_end_intersection->on_segment_start)
		{
			// Only copy one less point from section geometry if we have at least one to copy.
			// It's possible the end intersection is on the same segment as the start intersection
			// (in which case both intersections must coincide with that segment's start point) and
			// so there are no section geometry points left to remove. In this case we'll just
			// be outputting the two coincident intersection points.
			if (d_end_section_vertex_index > d_start_section_vertex_index)
			{
				--d_end_section_vertex_index;
			}
		}
	}
	else if (!d_start_intersection &&
		!d_end_intersection)
	{
		// If there is a start and end rubber band and they're both at the start or both at the end of the
		// current section then handle this as a special case by excluding all vertices of the section geometry.
		if (d_start_rubber_band &&
			d_end_rubber_band &&
			d_start_rubber_band->is_at_start_of_current_section == d_end_rubber_band->is_at_start_of_current_section)
		{
			// No section geometry vertices. Just the two rubber band points (both at start or both at end).
			d_start_section_vertex_index = d_num_points_in_section_geometry;
			d_end_section_vertex_index = d_num_points_in_section_geometry;
		}
		else
		{
			// There are no intersections.
			// And if there is a start and end rubber band then they're on opposite ends of the section geometry.

			// However, if we have a point section geometry with no rubber bands (and no intersections) then
			// create a start and an end intersection to ensure that this sub-segment will have two points
			// (because we guarantee that a sub-segment geometry is a polyline, which requires two points,
			// noting that the sub-segment geometry returned by 'get_geometry()' includes rubber band points).
			//
			// Note that this particular case should only occur during the building of a topology, such
			// as adding the first point to a topology in the build tool (in which case the sole point will
			// not yet have any neighbours to rubber band with). However once a topology is built, any point
			// sections should have at least one rubber band (because they'll have at least one neighbour)
			// which, along with the sole section point, will be two points for the section's sub-segment.
			if (d_num_points_in_section_geometry == 1 &&
				!d_start_rubber_band &&
				!d_end_rubber_band)
			{
				d_start_intersection = Intersection::create_at_section_start_or_end(*section_geometry, true/*at_start*/);
				d_end_intersection = Intersection::create_at_section_start_or_end(*section_geometry, false/*at_start*/);

				// No section geometry vertices. Just the two new intersection points.
				d_start_section_vertex_index = d_num_points_in_section_geometry;
				d_end_section_vertex_index = d_num_points_in_section_geometry;

				// Note that we could have instead created only a single intersection and included the one section point
				// (to give us two sub-segment points), however it's a bit arbitrary whether to create intersection
				// at start or end, so we just make it more symmetrical with two intersection points.
			}
			else
			{
				// If there is a start and end rubber band then they're on opposite ends of the section geometry,
				// so just set the full section geometry as the sub-segment. And there will be enough sub-segment
				// points in the sub-segment geometry (ie, at least two) to form a polyline.
				d_start_section_vertex_index = 0;
				d_end_section_vertex_index = d_num_points_in_section_geometry;
			}
		}
	}
	else if (d_start_intersection)
	{
		const unsigned int num_vertices_in_section = d_num_points_in_section_geometry;
		const unsigned int num_segments_in_section = num_vertices_in_section - 1;

		// There's no end intersection so end at the end of the section.
		d_end_section_vertex_index = num_vertices_in_section;

		// If start intersection is *on* the last vertex of the section geometry then the intersection is a T-junction.
		//
		// Note that we are testing for the fictitious one-past-the-last segment which is equivalent
		// to the last vertex of the section geometry (end point of last segment).
		if (d_start_intersection->segment_index == num_segments_in_section &&
			d_start_intersection->on_segment_start/*not really necessary for fictitious one-past-the-last segment*/)
		{
			// We guarantee that a sub-segment geometry is a polyline, which requires two points.
			// However if we don't have an end rubber band then we will only have one point
			// (the start intersection right at the end of the section geometry). In this case we'll
			// create an end intersection also right at the end of the section geometry.
			if (!d_end_rubber_band)
			{
				d_end_intersection = Intersection::create_at_section_start_or_end(*section_geometry, false/*at_start*/);
			}

			// No section vertices contribute, only the original start intersection point and either
			// the existing end rubber band point or our new end intersection point.
			d_start_section_vertex_index = num_vertices_in_section;
		}
		else
		{
			// Start at the end point of the segment containing the intersection
			// (the "+1" increments from start of segment to end of segment, which is also start of next segment).
			d_start_section_vertex_index = d_start_intersection->segment_index + 1;
		}
	}
	else // d_end_intersection ...
	{
		// There's no start intersection so start at the start of the section.
		d_start_section_vertex_index = 0;

		// If end intersection is *on* the first vertex of the section geometry then the intersection is a T-junction.
		if (d_end_intersection->segment_index == 0 &&
			d_end_intersection->on_segment_start)
		{
			// We guarantee that a sub-segment geometry is a polyline, which requires two points.
			// However if we don't have a start rubber band then we will only have one point
			// (the end intersection right at the start of the section geometry). In this case we'll
			// create a start intersection also right at the start of the section geometry.
			if (!d_start_rubber_band)
			{
				d_start_intersection = Intersection::create_at_section_start_or_end(*section_geometry, true/*at_start*/);
			}

			// No section vertices contribute, only the original end intersection point and either
			// the existing start rubber band point or our new start intersection point.
			d_end_section_vertex_index = 0;
		}
		else
		{
			// End at the start point of the segment containing the intersection
			// (the "+1" increments one-past-the-start of segment).
			d_end_section_vertex_index = d_end_intersection->segment_index + 1;

			// If the end intersection does coincide with the start of a segment then we need to copy
			// one less point from the section geometry (that point being the start of the segment).
			//
			// If the intersection point *is* at the start point of a segment.
			if (d_end_intersection->on_segment_start)
			{
				// Note that the end vertex index must currently (before decrement) be at least 2 because
				// we cannot be on the first segment (on its start point).
				--d_end_section_vertex_index;
			}
		}
	}
}


GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::ResolvedSubSegmentRangeInSection::get_geometry() const
{
	// Create and cache sub-segment geometry if we haven't already done so.
	if (!d_sub_segment_geometry)
	{
		// The points that will form the sub-segment polyline.
		std::vector<GPlatesMaths::PointOnSphere> sub_segment_points;

		// Note that we always include rubber band points to avoid retrieving no points.
		//
		// This can happen when a sub-sub-segment of a resolved line sub-segment is entirely within the
		// start or end rubber band region of the sub-sub-segment (and hence the sub-sub-segment geometry
		// is only made up of two rubber band points which, if excluded, would result in no points).
		// Note that this only applies when both rubber band points are on the same side of the section geometry.
		get_geometry_points(sub_segment_points, true/*include_rubber_band_points*/);

		// We should have at least two points when rubber band points are included.
		// This is because the constructor ensures this.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				sub_segment_points.size() >= 2,
				GPLATES_ASSERTION_SOURCE);

		// We have enough points from section geometry and intersections to create a polyline (ie, at least two points).
		d_sub_segment_geometry = GPlatesMaths::PolylineOnSphere::create(sub_segment_points);
	}

	return d_sub_segment_geometry.get();
}


void
GPlatesAppLogic::ResolvedSubSegmentRangeInSection::get_geometry_points(
		std::vector<GPlatesMaths::PointOnSphere> &geometry_points,
		bool include_rubber_band_points) const
{
	// Add the start intersection or rubber band.
	if (d_start_intersection)
	{
		geometry_points.push_back(d_start_intersection->position);
	}
	else if (include_rubber_band_points &&
			d_start_rubber_band)
	{
		geometry_points.push_back(d_start_rubber_band->position);
	}

	// Copy the points of the sub-segment range (within the entire section).
	// Note that it's possible for none of the section points to contribute (if we have an intersection).
	GeometryUtils::get_geometry_exterior_points_range(
			*d_section_geometry,
			geometry_points,
			d_start_section_vertex_index,
			d_end_section_vertex_index);

	// Add the end intersection or rubber band.
	if (d_end_intersection)
	{
		geometry_points.push_back(d_end_intersection->position);
	}
	else if (include_rubber_band_points &&
			d_end_rubber_band)
	{
		geometry_points.push_back(d_end_rubber_band->position);
	}
}


void
GPlatesAppLogic::ResolvedSubSegmentRangeInSection::get_reversed_geometry_points(
		std::vector<GPlatesMaths::PointOnSphere> &geometry_points,
		bool use_reverse,
		bool include_rubber_band_points) const
{
	// If need to reverse then add points in reverse order compared to 'get_geometry_points()'.
	if (use_reverse)
	{
		// Add the end intersection or rubber band.
		if (d_end_intersection)
		{
			geometry_points.push_back(d_end_intersection->position);
		}
		else if (include_rubber_band_points &&
				d_end_rubber_band)
		{
			geometry_points.push_back(d_end_rubber_band->position);
		}

		// Copy the points of the sub-segment range (within the entire section).
		// Note that it's possible for none of the section points to contribute (if we have an intersection).
		GeometryUtils::get_geometry_exterior_points_range(
				*d_section_geometry,
				geometry_points,
				d_start_section_vertex_index,
				d_end_section_vertex_index,
				true/*reverse_points*/);

		// Add the start intersection or rubber band.
		if (d_start_intersection)
		{
			geometry_points.push_back(d_start_intersection->position);
		}
		else if (include_rubber_band_points &&
				d_start_rubber_band)
		{
			geometry_points.push_back(d_start_rubber_band->position);
		}
	}
	else // not reversed...
	{
		get_geometry_points(geometry_points, include_rubber_band_points);
	}
}


std::pair<
		GPlatesMaths::PointOnSphere/*start point*/,
		GPlatesMaths::PointOnSphere/*end point*/>
GPlatesAppLogic::ResolvedSubSegmentRangeInSection::get_end_points(
		bool include_rubber_band_points) const
{
	boost::optional<GPlatesMaths::PointOnSphere> start_point;
	boost::optional<GPlatesMaths::PointOnSphere> end_point;

	// Add the start intersection or rubber band, if any.
	if (d_start_intersection)
	{
		start_point = d_start_intersection->position;
	}
	else if (include_rubber_band_points &&
			d_start_rubber_band)
	{
		start_point = d_start_rubber_band->position;
	}

	// Add the end intersection or rubber band, if any.
	if (d_end_intersection)
	{
		end_point = d_end_intersection->position;
	}
	else if (include_rubber_band_points &&
			d_end_rubber_band)
	{
		end_point = d_end_rubber_band->position;
	}

	if (!start_point ||
		!end_point)
	{
		// Get the start and end point of section geometry.
		const std::pair<
				GPlatesMaths::PointOnSphere/*start point*/,
				GPlatesMaths::PointOnSphere/*end point*/>
						section_start_and_end_points =
								GeometryUtils::get_geometry_exterior_end_points(*d_section_geometry);

		if (!start_point)
		{
			start_point = section_start_and_end_points.first;
		}
		if (!end_point)
		{
			end_point = section_start_and_end_points.second;
		}
	}

	return std::make_pair(start_point.get(), end_point.get());
}


std::pair<
		GPlatesMaths::PointOnSphere/*start point*/,
		GPlatesMaths::PointOnSphere/*end point*/>
GPlatesAppLogic::ResolvedSubSegmentRangeInSection::get_reversed_end_points(
		bool use_reverse,
		bool include_rubber_band_points) const
{
	// If need to reverse.
	if (use_reverse)
	{
		const std::pair<GPlatesMaths::PointOnSphere, GPlatesMaths::PointOnSphere> end_points =
				get_end_points(include_rubber_band_points);

		// Reverse the end points.
		return std::make_pair(end_points.second, end_points.first);
	}
	else
	{
		return get_end_points(include_rubber_band_points);
	}
}


unsigned int
GPlatesAppLogic::ResolvedSubSegmentRangeInSection::get_num_points(
		bool include_rubber_band_points) const
{
	unsigned int num_points = d_end_section_vertex_index - d_start_section_vertex_index;

	if (d_start_intersection ||
		(include_rubber_band_points && d_start_rubber_band))
	{
		++num_points;
	}
	if (d_end_intersection ||
		(include_rubber_band_points && d_end_rubber_band))
	{
		++num_points;
	}

	return num_points;
}


boost::optional<GPlatesAppLogic::ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand>
GPlatesAppLogic::ResolvedSubSegmentRangeInSection::get_start_intersection_or_rubber_band() const
{
	if (d_start_intersection)
	{
		return IntersectionOrRubberBand(d_start_intersection.get());
	}
	else if (d_start_rubber_band)
	{
		return IntersectionOrRubberBand(d_start_rubber_band.get());
	}
	else
	{
		return boost::none;
	}
}


boost::optional<GPlatesAppLogic::ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand>
GPlatesAppLogic::ResolvedSubSegmentRangeInSection::get_end_intersection_or_rubber_band() const
{
	if (d_end_intersection)
	{
		return IntersectionOrRubberBand(d_end_intersection.get());
	}
	else if (d_end_rubber_band)
	{
		return IntersectionOrRubberBand(d_end_rubber_band.get());
	}
	else
	{
		return boost::none;
	}
}


GPlatesAppLogic::ResolvedSubSegmentRangeInSection::Intersection
GPlatesAppLogic::ResolvedSubSegmentRangeInSection::Intersection::create(
		const GPlatesMaths::GeometryIntersect::Intersection &intersection,
		const GPlatesMaths::PolylineOnSphere &section_polyline,
		bool section_polyline_is_first_geometry)
{
	const unsigned int segment_index = section_polyline_is_first_geometry
			? intersection.segment_index1
			: intersection.segment_index2;
	const bool on_segment_start = section_polyline_is_first_geometry
			? intersection.is_on_segment1_start()
			: intersection.is_on_segment2_start();
	const GPlatesMaths::AngularDistance &angle_in_segment =
			section_polyline_is_first_geometry
			? intersection.angle_in_segment1
			: intersection.angle_in_segment2;

	//
	// Note: We delay the calculation of interpolation ratio until needed since it's relatively
	// expensive (with two 'acos()' calls and a division).
	//

	return Intersection(
			intersection.position,
			segment_index,
			on_segment_start,
			angle_in_segment);
}


GPlatesAppLogic::ResolvedSubSegmentRangeInSection::Intersection
GPlatesAppLogic::ResolvedSubSegmentRangeInSection::Intersection::create_at_section_start_or_end(
		const GPlatesMaths::GeometryOnSphere &section_geometry,
		bool at_start)
{
	//
	// Section geometry can be a point, multi-point or polyline. But not a polygon.
	//
	if (boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> section_polyline_opt =
		GeometryUtils::get_polyline_on_sphere(section_geometry))
	{
		const GPlatesMaths::PolylineOnSphere &section_polyline = *section_polyline_opt.get();

		return Intersection(
				at_start ? section_polyline.start_point() : section_polyline.end_point(),
				at_start ? 0 : section_polyline.number_of_segments()/*segment_index*/,
				true/*on_segment_start*/,
				GPlatesMaths::AngularDistance::ZERO/*angle_in_segment*/,
				0.0/*interpolate_ratio_in_segment*/);
	}
	else if (boost::optional<const GPlatesMaths::PointOnSphere &> section_point_opt =
			GeometryUtils::get_point_on_sphere(section_geometry))
	{
		const GPlatesMaths::PointOnSphere &section_point = section_point_opt.get();

		return Intersection(
				section_point,
				0/*segment_index*/,
				true/*on_segment_start*/,
				GPlatesMaths::AngularDistance::ZERO/*angle_in_segment*/,
				0.0/*interpolate_ratio_in_segment*/);
	}
	else
	{
		boost::optional<GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type> section_multi_point_opt =
				GeometryUtils::get_multi_point_on_sphere(section_geometry);

		// Section geometry must be a multi-point (or point, polyline). But not a polygon.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				section_multi_point_opt,
				GPLATES_ASSERTION_SOURCE);

		const GPlatesMaths::MultiPointOnSphere &section_multi_point = *section_multi_point_opt.get();

		return Intersection(
				at_start ? section_multi_point.start_point() : section_multi_point.end_point(),
				// If we consider the multi-point to be consecutively connected with GCA segments
				// between its points then it has one less segment than number of points, and
				// this represents the fictitious one-past-the-last segment...
				at_start ? 0 : section_multi_point.number_of_points() - 1/*segment_index*/,
				true/*on_segment_start*/,
				GPlatesMaths::AngularDistance::ZERO/*angle_in_segment*/,
				0.0/*interpolate_ratio_in_segment*/);
	}
}


GPlatesAppLogic::ResolvedSubSegmentRangeInSection::Intersection
GPlatesAppLogic::ResolvedSubSegmentRangeInSection::Intersection::create_from_inner_segment(
		const GPlatesMaths::PointOnSphere &intersection_position,
		const GPlatesMaths::GeometryOnSphere &section_geometry,
		unsigned int segment_index,
		const double &start_inner_segment_interpolate_ratio_in_segment,
		const double &end_inner_segment_interpolate_ratio_in_segment,
		const double &interpolate_ratio_in_inner_segment)
{
	boost::optional<GPlatesMaths::GreatCircleArc> segment = get_segment(section_geometry, segment_index);
	if (!segment)
	{
		// Segment index represents the fictitious one-past-the-last segment.
		// So must represent the last vertex of the section geometry.
		return Intersection(
				intersection_position,
				segment_index,
				true/*on_segment_start*/,
				GPlatesMaths::AngularDistance::ZERO/*angle_in_segment*/,
				0.0/*interpolate_ratio_in_segment*/);
	}

	// Determine if intersection position is "on" segment *end* point
	// using the same threshold used for geometry intersections.
	const bool on_segment_end = dot(intersection_position.position_vector(), segment->end_point().position_vector()).dval() >=
			GPlatesMaths::GeometryIntersect::Intersection::get_on_segment_start_threshold_cosine();
	if (on_segment_end)
	{
		// Increment the segment index so that the intersection position is "on" the segment *start* point
		// (not *end* point) since we're keeping to the same standard as the GeometryIntersect code and
		// only allowing intersection to lie on the *start* of a segment (not the *end*).
		//
		// Note that incrementing may take us to the fictitious one-past-the-last segment.
		return Intersection(
				intersection_position,
				segment_index + 1,
				true/*on_segment_start*/,
				GPlatesMaths::AngularDistance::ZERO/*angle_in_segment*/,
				0.0/*interpolate_ratio_in_segment*/);
	}

	const GPlatesMaths::Real dot_intersection_and_segment_start =
			dot(intersection_position.position_vector(), segment->start_point().position_vector());
	const GPlatesMaths::AngularDistance angle_in_segment =
			GPlatesMaths::AngularDistance::create_from_cosine(dot_intersection_and_segment_start);

	// Determine if intersection position is "on" segment start point
	// using the same threshold used for geometry intersections.
	const bool on_segment_start = dot_intersection_and_segment_start.dval() >=
			GPlatesMaths::GeometryIntersect::Intersection::get_on_segment_start_threshold_cosine();

	// Interpolate the interpolate ratios (of the inner segment) to get the final interpolation ratio
	// in the outer segment.
	const double interpolate_ratio_in_segment =
			start_inner_segment_interpolate_ratio_in_segment + interpolate_ratio_in_inner_segment *
				(end_inner_segment_interpolate_ratio_in_segment - start_inner_segment_interpolate_ratio_in_segment);

	return Intersection(
			intersection_position,
			segment_index,
			on_segment_start,
			angle_in_segment,
			interpolate_ratio_in_segment);
}


boost::optional<GPlatesMaths::GreatCircleArc>
GPlatesAppLogic::ResolvedSubSegmentRangeInSection::Intersection::get_segment(
		const GPlatesMaths::GeometryOnSphere &section_geometry,
		unsigned int segment_index)
{
	//
	// Section geometry can be a point, multi-point or polyline. But not a polygon.
	//
	if (boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> section_polyline_opt =
		GeometryUtils::get_polyline_on_sphere(section_geometry))
	{
		const GPlatesMaths::PolylineOnSphere &section_polyline = *section_polyline_opt.get();

		if (segment_index == section_polyline.number_of_segments())
		{
			// Fictitious one-past-the-last segment.
			return boost::none;
		}

		return section_polyline.get_segment(segment_index);
	}
	else if (boost::optional<const GPlatesMaths::PointOnSphere &> section_point_opt =
			GeometryUtils::get_point_on_sphere(section_geometry))
	{
		// Only one point, so segment index must be zero.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				segment_index == 0,
				GPLATES_ASSERTION_SOURCE);

		// Only one point, not enough for a GCA segment.
		return boost::none;
	}
	else
	{
		boost::optional<GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type> section_multi_point_opt =
				GeometryUtils::get_multi_point_on_sphere(section_geometry);

		// Section geometry must be a multi-point (or point, polyline). But not a polygon.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				section_multi_point_opt,
				GPLATES_ASSERTION_SOURCE);

		const GPlatesMaths::MultiPointOnSphere &section_multi_point = *section_multi_point_opt.get();

		// If we consider the multi-point to be consecutively connected with GCA segments between its
		// points then it has one less segment than number of points.
		const unsigned int number_of_segments = section_multi_point.number_of_points() - 1;

		if (segment_index == number_of_segments)
		{
			// Fictitious one-past-the-last segment.
			// Also handles the case of only one point in multi-point.
			return boost::none;
		}

		// Note that we create a GCA segment between adjacent points (starting at 'segment_index')
		// even though the section geometry is a multi-point.
		return GPlatesMaths::GreatCircleArc::create(
				section_multi_point.get_point(segment_index),
				section_multi_point.get_point(segment_index + 1));
	}
}


double
GPlatesAppLogic::ResolvedSubSegmentRangeInSection::Intersection::get_interpolate_ratio_in_segment(
		const GPlatesMaths::GeometryOnSphere &section_geometry) const
{
	if (!d_interpolate_ratio_in_segment)
	{
		d_interpolate_ratio_in_segment = 0.0;

		if (!on_segment_start)
		{
			boost::optional<GPlatesMaths::GreatCircleArc> segment = get_segment(section_geometry, segment_index);
			if (segment)
			{
				if (!segment->is_zero_length())
				{
					// Calculate the ratio of distance from intersection point to segment start point divided by
					// distance between segment start and end points.
					d_interpolate_ratio_in_segment =
							acos(dot(position.position_vector(), segment->start_point().position_vector())).dval() /
								segment->arc_length().dval();
				}
			}
			else // fictitious one-past-the-last segment ...
			{
				// Don't dereference the fictitious one-past-the-last segment, it represents the last vertex.
				// And don't decrement the segment index to make it valid as this will mess things up when
				// retrieving sub-segment ranges (which can result in sub-segments missing a couple of vertices).
				// We can just leave the interpolate ratio as 0.0 since 'on_segment_start' should be true and
				// the last vertex is essentially the start of the fictitious one-past-the-last segment.
			}
		}
	}

	return d_interpolate_ratio_in_segment.get();
}


GPlatesAppLogic::ResolvedSubSegmentRangeInSection::RubberBand
GPlatesAppLogic::ResolvedSubSegmentRangeInSection::RubberBand::create(
		const GPlatesMaths::PointOnSphere &current_section_position,
		const GPlatesMaths::PointOnSphere &adjacent_section_position,
		bool is_at_start_of_current_section,
		bool is_at_start_of_adjacent_section,
		const ReconstructionGeometry::non_null_ptr_to_const_type &current_section_reconstruction_geometry,
		const ReconstructionGeometry::non_null_ptr_to_const_type &adjacent_section_reconstruction_geometry)
{
	// Rubber band point is the mid-point between the start/end of the current section and
	// start/end of the adjacent section.
	const GPlatesMaths::Vector3D mid_point =
			GPlatesMaths::Vector3D(current_section_position.position_vector()) +
			GPlatesMaths::Vector3D(adjacent_section_position.position_vector());
	const GPlatesMaths::UnitVector3D rubber_band_position = mid_point.is_zero_magnitude()
			// Current and adjacent section positions are antipodal, so just generate any point midway between them...
			? generate_perpendicular(current_section_position.position_vector())
			: mid_point.get_normalisation();

	return RubberBand(
		GPlatesMaths::PointOnSphere(rubber_band_position),
		0.5/*interpolate_ratio*/,
		current_section_position,
		adjacent_section_position,
		is_at_start_of_current_section,
		is_at_start_of_adjacent_section,
		current_section_reconstruction_geometry,
		adjacent_section_reconstruction_geometry);
}


GPlatesAppLogic::ResolvedSubSegmentRangeInSection::RubberBand
GPlatesAppLogic::ResolvedSubSegmentRangeInSection::RubberBand::create_from_intersected_rubber_band(
		const RubberBand &rubber_band,
		const GPlatesMaths::PointOnSphere &intersection_position)
{
	//
	// Intersected rubber band position at 'interpolate_ratio' along the great circle arc segment
	// between the start/end of the rubber band's current section and start/end of its adjacent section.
	//

	const GPlatesMaths::GreatCircleArc rubber_band_segment =
			GPlatesMaths::GreatCircleArc::create(
					rubber_band.current_section_position,
					rubber_band.adjacent_section_position);
	if (rubber_band_segment.is_zero_length())
	{
		// The current and adjacent section positions are (close to) equal.
		return RubberBand(
				// Choose the start of rubber band segment - we're assuming intersection position is (close to) the same...
				rubber_band_segment.start_point()/*rubber band position*/,
				0.0/*interpolate_ratio*/,
				rubber_band.current_section_position,
				rubber_band.adjacent_section_position,
				rubber_band.is_at_start_of_current_section,
				rubber_band.is_at_start_of_adjacent_section,
				rubber_band.current_section_reconstruction_geometry,
				rubber_band.adjacent_section_reconstruction_geometry);
	}

	// Calculate the ratio of distance from intersection point to rubber band segment's start point
	// divided by distance between rubber band segment's start and end points.
	const double interpolate_ratio_in_rubber_segment =
			acos(dot(intersection_position.position_vector(), rubber_band_segment.start_point().position_vector())).dval() /
				rubber_band_segment.arc_length().dval();

	return RubberBand(
			intersection_position,
			interpolate_ratio_in_rubber_segment,
			rubber_band.current_section_position,
			rubber_band.adjacent_section_position,
			rubber_band.is_at_start_of_current_section,
			rubber_band.is_at_start_of_adjacent_section,
			rubber_band.current_section_reconstruction_geometry,
			rubber_band.adjacent_section_reconstruction_geometry);
}
