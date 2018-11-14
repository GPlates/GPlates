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

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


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
		// There are no intersections so just set the full section geometry as the sub-segment.
		d_start_section_vertex_index = 0;
		d_end_section_vertex_index = d_num_points_in_section_geometry;
	}
	else if (d_start_intersection)
	{
		const unsigned int num_vertices_in_section = d_num_points_in_section_geometry;
		const unsigned int num_segments_in_section = num_vertices_in_section - 1;

		// There's no end intersection so end at the end of the section.
		d_end_section_vertex_index = num_vertices_in_section;

		// If start intersection is *on* the last vertex of the section geometry then the intersection
		// is a T-junction, so return sub-segment as a point geometry (the intersection point).
		//
		// Note that we are testing for the fictitious one-past-the-last segment which is equivalent
		// to the last vertex of the section geometry (end point of last segment).
		if (d_start_intersection->segment_index == num_segments_in_section &&
			d_start_intersection->on_segment_start/*not really necessary for fictitious one-past-the-last segment*/)
		{
			// No section vertices contribute, only the single intersection point.
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

		// If end intersection is *on* the first vertex of the section geometry then the intersection
		// is a T-junction, so return sub-segment as a point geometry (the intersection point).
		if (d_end_intersection->segment_index == 0 &&
			d_end_intersection->on_segment_start)
		{
			// No section vertices contribute, only the single intersection point.
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


GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::ResolvedSubSegmentRangeInSection::get_geometry(
		bool include_rubber_band_points) const
{
	// If no intersections or rubber bands.
	if (!d_start_intersection &&
		!d_end_intersection &&
		!d_start_rubber_band &&
		!d_end_rubber_band)
	{
		// Just return the entire section geometry (which could be a single point, or a multi-point).
		return d_section_geometry;
	}

	//
	// We have at least one intersection or rubber band point.
	//

	// The points that will form the sub-segment polyline.
	std::vector<GPlatesMaths::PointOnSphere> sub_segment_points;
	get_geometry_points(sub_segment_points, include_rubber_band_points);

	// If we don't have enough points for a polyline then just return a point geometry.
	// We should at least have a point (if section geometry was a point).
	if (sub_segment_points.size() == 1)
	{
		return sub_segment_points.front().get_non_null_pointer();
	}

	// We have enough points from section geometry and intersections to create a polyline (ie, at least two points).
	return GPlatesMaths::PolylineOnSphere::create_on_heap(sub_segment_points);
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


GPlatesAppLogic::ResolvedSubSegmentRangeInSection::Intersection::Intersection(
		const GPlatesMaths::PointOnSphere &position_,
		unsigned int segment_index_,
		bool on_segment_start_,
		GPlatesMaths::AngularDistance angle_in_segment_,
		const GPlatesMaths::PolylineOnSphere &section_polyline) :
	position(position_),
	segment_index(segment_index_),
	on_segment_start(on_segment_start_),
	angle_in_segment(angle_in_segment_),
	interpolate_ratio_in_segment(0.0)
{
	if (segment_index == section_polyline.number_of_segments())
	{
		// Don't dereference the fictitious one-past-the-last segment, it represents the last vertex.
		// And don't decrement the segment index to make it valid as this will mess things up when
		// retrieving sub-segment ranges (which can result in sub-segments missing a couple of vertices).
		// We can just leave the interpolate ratio as 0.0 since 'on_segment_start' should be true and
		// the last vertex is essentially the start of the fictitious one-past-the-last segment.
	}
	else
	{
		const GPlatesMaths::GreatCircleArc &segment = section_polyline.get_segment(segment_index);

		if (!on_segment_start &&
			!segment.is_zero_length())
		{
			// Calculate the ratio of distance from intersection point to segment start point divided by
			// distance between segment start and end points.
			interpolate_ratio_in_segment =
					acos(dot(position.position_vector(), segment.start_point().position_vector())).dval() /
						acos(segment.dot_of_endpoints()).dval();
		}
	}
}


GPlatesAppLogic::ResolvedSubSegmentRangeInSection::Intersection::Intersection(
		const GPlatesMaths::GeometryOnSphere &section_geometry,
		bool at_start) :
	position(
			at_start
			? GeometryUtils::get_geometry_exterior_end_points(section_geometry).first
			: GeometryUtils::get_geometry_exterior_end_points(section_geometry).second),
	segment_index(
			at_start
			? 0
			// For polylines this is the same as the fictitous one-past-the-last segment...
			: GeometryUtils::get_num_geometry_exterior_points(section_geometry) - 1),
	on_segment_start(true),
	angle_in_segment(GPlatesMaths::AngularDistance::ZERO),
	interpolate_ratio_in_segment(0.0)
{
}
