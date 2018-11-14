/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include <vector>

#include "TopologyIntersections.h"

#include "GeometryUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


namespace GPlatesAppLogic
{
	namespace
	{
		/**
		 * Returns the section geometry as an intersectable polyline.
		 *
		 * Returns polyline unchanged for a polyline.
		 * Returns the exterior ring for a polygon.
		 * Returns none for a point or multipoint.
		 */
		boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>
		get_intersectable_section_polyline(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &section_geometry)
		{
			const GPlatesMaths::GeometryType::Value geometry_type =
					GeometryUtils::get_geometry_type(*section_geometry);
			if (geometry_type == GPlatesMaths::GeometryType::POLYLINE)
			{
				return GeometryUtils::get_polyline_on_sphere(*section_geometry);
			}
			else if (geometry_type == GPlatesMaths::GeometryType::POLYGON)
			{
				const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type section_polygon =
						GeometryUtils::get_polygon_on_sphere(*section_geometry).get();

				// Treat the exterior ring as a *polyline*.
				// In other words, we iterate over one extra vertex compared to the usual polygon
				// ring vertex iterator so that the last vertex is the end point of the last ring segment
				// (which is also the first vertex of ring).
				return GPlatesMaths::PolylineOnSphere::create_on_heap(
						section_polygon->exterior_polyline_vertex_begin(),
						section_polygon->exterior_polyline_vertex_end());
			}
			else
			{
				// Points and multi-points are not intersectable, so return none.
				return boost::none;
			}
		}
	}
}


GPlatesAppLogic::TopologicalIntersections::TopologicalIntersections(
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &section_geometry,
		bool reverse_hint) :
	d_section_geometry(section_geometry),
	d_reverse_hint(reverse_hint),
	d_intersectable_section_polyline(get_intersectable_section_polyline(section_geometry))
{
	// Make sure the section geometry matches what we are using as an intersectable section polyline.
	// For polygon sections, this means the section geometry is the exterior ring (in the form of a polyline).
	if (d_intersectable_section_polyline)
	{
		d_section_geometry = d_intersectable_section_polyline.get();
	}
}


boost::optional<GPlatesMaths::PointOnSphere>
GPlatesAppLogic::TopologicalIntersections::intersect_with_previous_section(
		const shared_ptr_type &previous_section)
{
	// Must not have already been tested for intersection with a previous section
	// (which also means previous section not been tested with a next section).
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			!d_prev_section && !previous_section->d_next_section,
			GPLATES_ASSERTION_SOURCE);

	// Assign our previous section and its next section.
	d_prev_section = weak_ptr_type(previous_section);
	previous_section->d_next_section = weak_ptr_type(shared_from_this());

	// If the two geometries (from previous and current sections) are not intersectable
	// (ie, are points or multi-points) then return false.
	if (!previous_section->d_intersectable_section_polyline ||
		!d_intersectable_section_polyline)
	{
		return boost::none;
	}

	// Intersect the two section polylines.
	// If there were no intersections then return false.
	GPlatesMaths::GeometryIntersect::Graph intersection_graph;
	if (!GPlatesMaths::GeometryIntersect::intersect(
			intersection_graph,
			*previous_section->d_intersectable_section_polyline.get(),
			*d_intersectable_section_polyline.get()))
	{
		return boost::none;
	}

	//
	// We have at least one intersection - ideally we're only expecting one intersection.
	//

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!intersection_graph.unordered_intersections.empty(),
			GPLATES_ASSERTION_SOURCE);

	boost::optional<GPlatesMaths::PointOnSphere> intersection_position;

	if (intersection_graph.unordered_intersections.size() == 1)
	{
		// There's a single intersection.
		//
		// If the topology data has been built correctly then there should be a single intersection
		// (and in some cases no intersection is also fine, eg, if data builder did not intend sections to intersect).
		const GPlatesMaths::GeometryIntersect::Intersection &intersection =
				intersection_graph.unordered_intersections[0];

		intersection_position = set_intersection_with_previous_section(previous_section, intersection);
	}
	else // handle multiple intersections...
	{
		// Handle multiple intersections in the same way as prior implementations. In which case we want
		// to choose the same intersection as before so that users' topologies don't suddenly look different.
		// Although ideally the topologies should be re-built so that adjacent sections only intersect once.
		intersection_position = backward_compatible_multiple_intersections_with_previous_section(
				previous_section,
				intersection_graph);
	}

	// Return intersection position.
	return intersection_position;
}


boost::optional<GPlatesMaths::PointOnSphere>
GPlatesAppLogic::TopologicalIntersections::backward_compatible_multiple_intersections_with_previous_section(
		const shared_ptr_type &previous_section,
		const GPlatesMaths::GeometryIntersect::Graph &intersection_graph)
{
// These errors never really get fixed in the topology datasets so might as well stop spamming the log.
// Better to write a pyGPlates script to detect these types of errors as a post-process.
#if 0
	// Produce a warning message since there is more than one intersection.
	qWarning() << "TopologicalIntersections::intersect_with_previous_section: ";
	qWarning() << "	num_intersect=" << intersection_graph.unordered_intersections.size();
	qWarning() << "	Boundary feature intersection relations may not be correct!";
	qWarning() << "	Make sure boundary feature's only intersect once.";
#endif

	//
	// Handle multiple intersections.
	//
	// We are emulating the prior implementation that independently tested against a head and
	// tail segment for each section (initially each section had a head segment and then, after the
	// first intersection, also had a tail segment). In our current implementation we test the entire
	// section in one go.
	//
	// The prior implementation consisted of the following notes:
	//
	// Test all four possible combinations of intersections of the head/tail segments
	// of the previous section with the head/tail segments of the current section.
	//
	// We'll just accept the first intersection we find.
	// Ideally two adjacent topology sections should only intersect once (and the
	// user who built the plate polygons should make sure of this) but if they intersect
	// more than once then we need to handle this.
	// There are two cases where two adjacent sections can intersect more than once:
	// 1) A head or tail segment of one section intersects both the head and tail
	//    segment of the other section.
	//    We handle this below by only considering the first intersection we
	//    happen to come across first. This introduces some randomness in the results
	//    but at least it gives a result (besides it's up to the user to not build
	//    plate polygons that intersect like this).
	// 2) A head or tail segment of one section intersects either the head or tail
	//    segment of the other section at more than one point.
	//    We handle this by considering the first intersection point only and dividing the
	//    two intersected segments each into a head and tail segment (neglecting the
	//    fact that there are other intersection points). This also introduces some
	//    randomness (but once again it's up to the user to not build
	//    plate polygons that intersect like this).
	//

	//
	// In prior implementations, all sections initially had a head segment and gained a tail segment
	// upon the first intersection.
	//
	// Note that we're currently testing for an intersection of current section with previous section,
	// so if previous section already has an intersection then it must have been with its previous section,
	// and similarly if current section already has an intersection then it must have been with its next section.
	//

	backward_compatible_segment_type previous_head_segment;
	boost::optional<backward_compatible_segment_type> previous_tail_segment;
	if (previous_section->d_prev_intersection)
	{
		previous_head_segment.second = previous_section->d_prev_intersection.get();

		previous_tail_segment = backward_compatible_segment_type();
		previous_tail_segment->first = previous_section->d_prev_intersection.get();
	}

	backward_compatible_segment_type current_head_segment;
	boost::optional<backward_compatible_segment_type> current_tail_segment;
	if (d_next_intersection)
	{
		current_head_segment.second = d_next_intersection.get();

		current_tail_segment = backward_compatible_segment_type();
		current_tail_segment->first = d_next_intersection.get();
	}

	//
	// Note that the following code layout pretty much matches prior implementations.
	//
	// The previous-section-reverse-hint is no longer really needed, however it is currently
	// retained only to give the same results as previous implementations when the current and
	// previous sections intersect at *multiple* points (in which case we want to choose the same
	// intersection point as before so that users' topologies don't suddenly look different).
	// Ideally the topologies should be re-built so that adjacent sections only intersect once.
	//

	if (previous_section->d_reverse_hint)
	{
		if (true/*current_head_segment*/)
		{
			// Try intersection with previous section's head segment first.
			if (true/*previous_head_segment*/)
			{
				boost::optional<GPlatesMaths::PointOnSphere> intersection =
						backward_compatible_multiple_intersections_between_segments(
								previous_section,
								intersection_graph,
								previous_head_segment,
								current_head_segment);
				if (intersection)
				{
					return intersection;
				}
			}
			if (previous_tail_segment)
			{
				boost::optional<GPlatesMaths::PointOnSphere> intersection =
						backward_compatible_multiple_intersections_between_segments(
								previous_section,
								intersection_graph,
								previous_tail_segment.get(),
								current_head_segment);
				if (intersection)
				{
					return intersection;
				}
			}
		}

		if (current_tail_segment)
		{
			// Try intersection with previous section's head segment first.
			if (true/*previous_head_segment*/)
			{
				boost::optional<GPlatesMaths::PointOnSphere> intersection =
						backward_compatible_multiple_intersections_between_segments(
								previous_section,
								intersection_graph,
								previous_head_segment,
								current_tail_segment.get());
				if (intersection)
				{
					return intersection;
				}
			}
			if (previous_tail_segment)
			{
				boost::optional<GPlatesMaths::PointOnSphere> intersection =
						backward_compatible_multiple_intersections_between_segments(
								previous_section,
								intersection_graph,
								previous_tail_segment.get(),
								current_tail_segment.get());
				if (intersection)
				{
					return intersection;
				}
			}
		}
	}
	else
	{
		if (true/*current_head_segment*/)
		{
			// Try intersection with previous section's tail segment first.
			if (previous_tail_segment)
			{
				boost::optional<GPlatesMaths::PointOnSphere> intersection =
						backward_compatible_multiple_intersections_between_segments(
								previous_section,
								intersection_graph,
								previous_tail_segment.get(),
								current_head_segment);
				if (intersection)
				{
					return intersection;
				}
			}
			if (true/*previous_head_segment*/)
			{
				boost::optional<GPlatesMaths::PointOnSphere> intersection =
						backward_compatible_multiple_intersections_between_segments(
								previous_section,
								intersection_graph,
								previous_head_segment,
								current_head_segment);
				if (intersection)
				{
					return intersection;
				}
			}
		}

		if (current_tail_segment)
		{
			// Try intersection with previous section's tail segment first.
			if (previous_tail_segment)
			{
				boost::optional<GPlatesMaths::PointOnSphere> intersection =
						backward_compatible_multiple_intersections_between_segments(
								previous_section,
								intersection_graph,
								previous_tail_segment.get(),
								current_tail_segment.get());
				if (intersection)
				{
					return intersection;
				}
			}
			if (true/*previous_head_segment*/)
			{
				boost::optional<GPlatesMaths::PointOnSphere> intersection =
						backward_compatible_multiple_intersections_between_segments(
								previous_section,
								intersection_graph,
								previous_head_segment,
								current_tail_segment.get());
				if (intersection)
				{
					return intersection;
				}
			}
		}
	}

	// Shouldn't really be able to get here since we know there are intersections so at least one
	// combination of segment/segment tests above should have succeeded.
	// However it creates no problem in the calling code if we do get here.
	return boost::none;
}


boost::optional<GPlatesMaths::PointOnSphere>
GPlatesAppLogic::TopologicalIntersections::backward_compatible_multiple_intersections_between_segments(
		const shared_ptr_type &previous_section,
		const GPlatesMaths::GeometryIntersect::Graph &intersection_graph,
		const backward_compatible_segment_type &previous_segment,
		const backward_compatible_segment_type &current_segment)
{
	const unsigned int num_intersections = intersection_graph.unordered_intersections.size();

	// Iterate through all intersections until/if we find one that is within both previous and current segments.
	for (unsigned int i = 0; i < num_intersections; ++i)
	{
		// Prior implementations just considered the first intersection along segment belonging to *first* geometry.
		//
		// Prior implementation used the first *unordered* intersection but, for the prior implementation,
		// this happened to be ordered along the first geometry. Whereas our current (GeometryIntersect)
		// implementation is truly unordered, so we now need to be explicit about our ordering.
		const GPlatesMaths::GeometryIntersect::Intersection &intersection =
				intersection_graph.unordered_intersections[
						intersection_graph.geometry1_ordered_intersections[i]];

		// If we have an intersection then we must have had intersectable polylines in the first place.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				d_intersectable_section_polyline &&
						previous_section->d_intersectable_section_polyline,
				GPLATES_ASSERTION_SOURCE);
		const GPlatesMaths::PolylineOnSphere &section_geometry1 = *previous_section->d_intersectable_section_polyline.get();
		const GPlatesMaths::PolylineOnSphere &section_geometry2 = *d_intersectable_section_polyline.get();

		const ResolvedSubSegmentRangeInSection::Intersection intersection_in_previous(
				intersection.position,
				intersection.segment_index1,
				intersection.is_on_segment1_start(),
				intersection.angle_in_segment1,
				section_geometry1);
		const ResolvedSubSegmentRangeInSection::Intersection intersection_in_current(
				intersection.position,
				intersection.segment_index2,
				intersection.is_on_segment2_start(),
				intersection.angle_in_segment2,
				section_geometry2);

		//
		// Note that in each segment at most one of its two end points can be non-none.
		// A none end point just means the start or end of the entire *section*.
		// However if both end points are none this means the segment *is* the entire section.
		//

		if (previous_segment.first)  // previous tail segment
		{
			// See if intersection is within the previous tail segment.
			if (previous_segment.first.get() <= intersection_in_previous)
			{
				if (current_segment.first)  // current tail segment
				{
					// See if intersection is within the current tail segment.
					if (current_segment.first.get() <= intersection_in_current)
					{
						return set_intersection_with_previous_section(
							previous_section, intersection, intersection_in_previous, intersection_in_current);
					}
				}
				else if (current_segment.second)  // current head segment
				{
					// See if intersection is within the current head segment.
					if (current_segment.second.get() >= intersection_in_current)
					{
						return set_intersection_with_previous_section(
							previous_section, intersection, intersection_in_previous, intersection_in_current);
					}
				}
				else  // current complete section
				{
					// Current segment is complete section, so intersection must lie within it.

					return set_intersection_with_previous_section(
						previous_section, intersection, intersection_in_previous, intersection_in_current);
				}
			}
		}
		else if (previous_segment.second)  // previous head segment
		{
			// See if intersection is within the previous head segment.
			if (previous_segment.second.get() >= intersection_in_previous)
			{
				if (current_segment.first)  // current tail segment
				{
					// See if intersection is within the current tail segment.
					if (current_segment.first.get() <= intersection_in_current)
					{
						return set_intersection_with_previous_section(
							previous_section, intersection, intersection_in_previous, intersection_in_current);
					}
				}
				else if (current_segment.second)  // current head segment
				{
					// See if intersection is within the current head segment.
					if (current_segment.second.get() >= intersection_in_current)
					{
						return set_intersection_with_previous_section(
							previous_section, intersection, intersection_in_previous, intersection_in_current);
					}
				}
				else  // current complete section
				{
					// Current segment is complete section, so intersection must lie within it.

					return set_intersection_with_previous_section(
						previous_section, intersection, intersection_in_previous, intersection_in_current);
				}
			}
		}
		else  // previous complete section
		{
			// Previous segment is complete section, so intersection must lie within it.

			if (current_segment.first)  // current tail segment
			{
				// See if intersection is within the current tail segment.
				if (current_segment.first.get() <= intersection_in_current)
				{
					return set_intersection_with_previous_section(
						previous_section, intersection, intersection_in_previous, intersection_in_current);
				}
			}
			else if (current_segment.second)  // current head segment
			{
					// See if intersection is within the current head segment.
					if (current_segment.second.get() >= intersection_in_current)
					{
						return set_intersection_with_previous_section(
							previous_section, intersection, intersection_in_previous, intersection_in_current);
					}
			}
			else  // current complete section
			{
				// Current segment is complete section, so intersection must lie within it.

				return set_intersection_with_previous_section(
					previous_section, intersection, intersection_in_previous, intersection_in_current);
			}
		}
	}

	// None of the intersections were within both the previous segment and the current segment.
	return boost::none;
}


boost::optional<
		boost::tuple<
				// First intersection
				GPlatesMaths::PointOnSphere,
				// Optional second intersection
				boost::optional<GPlatesMaths::PointOnSphere> > >
GPlatesAppLogic::TopologicalIntersections::intersect_with_previous_section_allowing_two_intersections(
		const shared_ptr_type &previous_section)
{
	// We're expecting two sections that have not yet been tested for intersection with previous or next.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			!d_prev_section && !d_next_section &&
				!previous_section->d_prev_section && !previous_section->d_next_section,
			GPLATES_ASSERTION_SOURCE);

	// Assign our previous and next sections, and its previous and next sections.
	d_prev_section = d_next_section = weak_ptr_type(previous_section);
	previous_section->d_prev_section = previous_section->d_next_section = weak_ptr_type(shared_from_this());

	// If the two geometries (from previous and current sections) are not intersectable
	// (ie, are points or multi-points) then return false.
	if (!previous_section->d_intersectable_section_polyline ||
		!d_intersectable_section_polyline)
	{
		return boost::none;
	}

	// Intersect the two section polylines.
	// If there were no intersections then return false.
	GPlatesMaths::GeometryIntersect::Graph intersection_graph;
	if (!GPlatesMaths::GeometryIntersect::intersect(
			intersection_graph,
			*previous_section->d_intersectable_section_polyline.get(),
			*d_intersectable_section_polyline.get()))
	{
		return boost::none;
	}

	//
	// We have at least one intersection - ideally we're expecting two intersections.
	//

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!intersection_graph.unordered_intersections.empty(),
			GPLATES_ASSERTION_SOURCE);

	if (intersection_graph.unordered_intersections.size() == 1)
	{
		// We have a first intersection, but not a second. So no middle segments.
		const GPlatesMaths::GeometryIntersect::Intersection &first_intersection =
				intersection_graph.unordered_intersections[0];

		const GPlatesMaths::PointOnSphere first_intersection_position =
				set_intersection_with_previous_section(previous_section, first_intersection);

		return boost::make_tuple(
				first_intersection_position,
				boost::optional<GPlatesMaths::PointOnSphere>()/*second_intersection_position*/);
	}
	else // handle two or more intersections...
	{
// These errors never really get fixed in the topology datasets so might as well stop spamming the log.
// Better to write a pyGPlates script to detect these types of errors as a post-process.
#if 0
		// Produce a warning message if there is more than two intersections.
		if (intersection_graph.unordered_intersections.size() > 2)
		{
			qWarning() << "TopologyInternalUtils::intersect_topological_sections_allowing_two_intersections: ";
			qWarning() << "	num_intersect=" << intersection_graph.unordered_intersections.size();
			qWarning() << "	Boundary feature intersection relations may not be correct!";
			qWarning() << "	Make sure a boundary with exactly two sections only intersects twice.";
		}
#endif

		// Get the first and second intersections (if there are more then only the first two are considered).
		//
		// Prior implementation used the first and second *unordered* intersections but, for the prior
		// implementation, this happened to be ordered along the first geometry. Whereas our current
		// (GeometryIntersect) implementation is truly unordered, so we now need to be explicit about our ordering.
		//
		// NOTE: We could instead use the first and *last* intersections (since that might be more desirable)
		// but we are remaining backward compatible with the previous implementation (although we may not
		// be backward compatible with the *orientation* of the cycle, but at least the two intersections
		// points should be the same as previous implementation).
		const GPlatesMaths::GeometryIntersect::Intersection &first_intersection =
				intersection_graph.unordered_intersections[
						intersection_graph.geometry1_ordered_intersections[0]];
		const GPlatesMaths::GeometryIntersect::Intersection &second_intersection =
				intersection_graph.unordered_intersections[
						intersection_graph.geometry1_ordered_intersections[1]];

		const GPlatesMaths::PolylineOnSphere &section_geometry1 = *previous_section->d_intersectable_section_polyline.get();
		const GPlatesMaths::PolylineOnSphere &section_geometry2 = *d_intersectable_section_polyline.get();

		const ResolvedSubSegmentRangeInSection::Intersection first_intersection_in_previous(
				first_intersection.position,
				first_intersection.segment_index1,
				first_intersection.is_on_segment1_start(),
				first_intersection.angle_in_segment1,
				section_geometry1);
		const ResolvedSubSegmentRangeInSection::Intersection first_intersection_in_current(
				first_intersection.position,
				first_intersection.segment_index2,
				first_intersection.is_on_segment2_start(),
				first_intersection.angle_in_segment2,
				section_geometry2);
		const ResolvedSubSegmentRangeInSection::Intersection second_intersection_in_previous(
				second_intersection.position,
				second_intersection.segment_index1,
				second_intersection.is_on_segment1_start(),
				second_intersection.angle_in_segment1,
				section_geometry1);
		const ResolvedSubSegmentRangeInSection::Intersection second_intersection_in_current(
				second_intersection.position,
				second_intersection.segment_index2,
				second_intersection.is_on_segment2_start(),
				second_intersection.angle_in_segment2,
				section_geometry2);

		const GPlatesMaths::PointOnSphere first_intersection_position =
				set_intersection_with_previous_section(
						previous_section,
						first_intersection,
						first_intersection_in_previous,
						first_intersection_in_current);
		const GPlatesMaths::PointOnSphere second_intersection_position =
				previous_section->set_intersection_with_previous_section(
						shared_from_this(),
						second_intersection,
						// Note that these two are swapped since we are setting the intersection on
						// our *previous* section, and its current section is our previous section and
						// its previous section is our current section...
						second_intersection_in_current,
						second_intersection_in_previous);

		return boost::make_tuple(
				first_intersection_position,
				boost::optional<GPlatesMaths::PointOnSphere>(second_intersection_position));
	}
}


bool
GPlatesAppLogic::TopologicalIntersections::get_reverse_flag() const
{
	// If we intersected both the previous and next sections then we've effectively already
	// determined the reverse flag.
	if (d_prev_intersection &&
		d_next_intersection)
	{
		// If the intersection with the next section is closer to our section start point than
		// the intersection with the previous section then our section will need reversing.
		return d_next_intersection.get() < d_prev_intersection.get();
	}
	else
	{
		// Return the reverse hint (passed in constructor).
		return d_reverse_hint;
	}
}


GPlatesAppLogic::ResolvedSubSegmentRangeInSection
GPlatesAppLogic::TopologicalIntersections::get_sub_segment_range_in_section() const
{
	// If we intersected both the previous and next sections then we've effectively already determined the reverse flag.
	if (d_prev_intersection &&
		d_next_intersection)
	{
		// If the intersection with the next section is closer to our section start point than the
		// intersection with the previous section then our section will need reversing to be un-reversed.
		const bool reversed = d_next_intersection.get() < d_prev_intersection.get();
		const ResolvedSubSegmentRangeInSection::Intersection &start_intersection = reversed
				? d_next_intersection.get()
				: d_prev_intersection.get();
		const ResolvedSubSegmentRangeInSection::Intersection &end_intersection = reversed
				? d_prev_intersection.get()
				: d_next_intersection.get();

		return ResolvedSubSegmentRangeInSection(
				d_section_geometry,
				ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(start_intersection),
				ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(end_intersection));
	}

	if (!d_prev_intersection &&
		!d_next_intersection)
	{
		// If the current section did not intersect either of its neighbours then
		// just set the full section geometry as the sub-segment.
		//
		// And trust the reverse flag generated by the plate polygon build tool.
		// This is because the user would have made sure all topology sections intersected
		// (at the reconstruction time used for building the plate polygon) and since
		// all sections intersected each other then the correct reverse flags would
		// have automatically been generated. So we should trust those reverse flags.
		//
		// And if the user generated the plate polygon using the old version of the
		// build tool (where the user had to explicitly specify the reverse flag
		// rather than them being auto-generated by the code) then we can still trust
		// the reverse flags because the user would have changed them until the
		// topology looked correct (ie, head1->tail1->head2->tail2->head3 etc).
		// However, back then it was possible for the user to incorrectly specify the
		// reverse flag for one section in the topology and still have it look correct
		// provided all the sections intersected each other - but as soon as the user changed
		// the reconstruction time, in the build tool, to a time when not all sections
		// intersected then the error was visible - so if the user did not correct this,
		// by explicitly changing the reverse flag, then we will actually be trusting an
		// incorrect reverse flag here (but then the old version of the topology geometry
		// resolver would also generate an incorrect plate polygon). So this is something
		// the user should fix with the new build tool simply by selecting the topological
		// plate polygon and then selecting the edit tool (this will automatically generate
		// the correct reverse flags for all sections if all sections are intersecting)
		// and then selecting the 'Apply/Creating' button to save the new reverse flags.

		boost::optional<ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand> start;
		boost::optional<ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand> end;

		// Get the previous rubber band.
		boost::optional<ResolvedSubSegmentRangeInSection::RubberBand> prev_rubber_band =
				get_rubber_band(d_prev_section, true/*adjacent_is_previous_section*/);
		if (prev_rubber_band)
		{
			if (d_reverse_hint)
			{
				end = ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(prev_rubber_band.get());
			}
			else
			{
				start = ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(prev_rubber_band.get());
			}
		}

		// Get the next rubber band.
		boost::optional<ResolvedSubSegmentRangeInSection::RubberBand> next_rubber_band =
				get_rubber_band(d_next_section, false/*adjacent_is_previous_section*/);
		if (next_rubber_band)
		{
			if (d_reverse_hint)
			{
				start = ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(next_rubber_band.get());
			}
			else
			{
				end = ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(next_rubber_band.get());
			}
		}

		return ResolvedSubSegmentRangeInSection(d_section_geometry, start, end);
	}

	//
	// If we get here then we intersected either the previous section or next section (but not both).
	//

	//
	// The current section only intersected one of its neighbours.
	// In this case we want to trust the reverse flag set by the topology build tool
	// (for an explanation see the comment for the case for zero intersections above).
	// All we need to do is pick the head segment or tail segment from the single intersection.
	// We do this by considering the geometry of the head and tail segments
	// after they have been reversed (if the reverse flag is set).
	// If the single intersection was with the start (previous) neighbour then we
	// want the start point of this section's reversed geometry to touch the
	// end point of the previous neighbour's sub-segment (ie, the intersection point).
	// This means choosing the tail segment of the current segment if the geometry
	// is not reversed or the head segment if it is reversed.
	// Similar logic follows if the single intersection was with the end (next) neighbour.
	//
	if (d_prev_intersection)
	{
		const ResolvedSubSegmentRangeInSection::Intersection &intersection = d_prev_intersection.get();

		if (d_reverse_hint)
		{
			// Get the start rubber band.
			boost::optional<ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand> start;
			boost::optional<ResolvedSubSegmentRangeInSection::RubberBand> start_rubber_band =
					get_rubber_band(d_next_section, false/*adjacent_is_previous_section*/);
			if (start_rubber_band)
			{
				start = ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(start_rubber_band.get());
			}

			// Use head segment.
			return ResolvedSubSegmentRangeInSection(
					d_section_geometry,
					start,
					ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(intersection)/*end*/);
		}
		else // don't reverse ...
		{
			// Get the end rubber band.
			boost::optional<ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand> end;
			boost::optional<ResolvedSubSegmentRangeInSection::RubberBand> end_rubber_band =
					get_rubber_band(d_next_section, false/*adjacent_is_previous_section*/);
			if (end_rubber_band)
			{
				end = ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(end_rubber_band.get());
			}

			// Use tail segment.
			return ResolvedSubSegmentRangeInSection(
					d_section_geometry,
					ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(intersection)/*start*/,
					end);
		}
	}
	else // next intersection ...
	{
		const ResolvedSubSegmentRangeInSection::Intersection &intersection = d_next_intersection.get();

		if (d_reverse_hint)
		{
			// Get the end rubber band.
			boost::optional<ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand> end;
			boost::optional<ResolvedSubSegmentRangeInSection::RubberBand> end_rubber_band =
					get_rubber_band(d_prev_section, true/*adjacent_is_previous_section*/);
			if (end_rubber_band)
			{
				end = ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(end_rubber_band.get());
			}

			// Use tail segment.
			return ResolvedSubSegmentRangeInSection(
					d_section_geometry,
					ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(intersection)/*start*/,
					end);
		}
		else // don't reverse ...
		{
			// Get the start rubber band.
			boost::optional<ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand> start;
			boost::optional<ResolvedSubSegmentRangeInSection::RubberBand> start_rubber_band =
					get_rubber_band(d_prev_section, true/*adjacent_is_previous_section*/);
			if (start_rubber_band)
			{
				start = ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(start_rubber_band.get());
			}

			// Use head segment.
			return ResolvedSubSegmentRangeInSection(
					d_section_geometry,
					start,
					ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(intersection)/*end*/);
		}
	}
}

boost::optional<GPlatesAppLogic::ResolvedSubSegmentRangeInSection::RubberBand>
GPlatesAppLogic::TopologicalIntersections::get_rubber_band(
		const boost::optional<weak_ptr_type> &adjacent_section_weak_ptr,
		bool adjacent_is_previous_section) const
{
	// We should always have adjacent sections unless they were not tested for intersection by our client
	// (this can happen when topologies are resolving while a user is building a new topology and adding
	// the same section feature more than once to the same topology - there can be a time during building
	// when the same section feature is adajcent to itself and hence cannot be intersected with itself).
	//
	// If there's no adjacent section then we'll get no rubber banding to it.
	if (adjacent_section_weak_ptr)
	{
		shared_ptr_type adjacent_section = adjacent_section_weak_ptr->lock();

		if (adjacent_section)
		{
			const bool is_at_start_of_current_section = d_reverse_hint ^ adjacent_is_previous_section;

			const std::pair<GPlatesMaths::PointOnSphere, GPlatesMaths::PointOnSphere> curr_section_end_points =
					GeometryUtils::get_geometry_exterior_end_points(*get_section_geometry());
			const GPlatesMaths::PointOnSphere &curr_section_rubber_band = is_at_start_of_current_section
					? curr_section_end_points.first
					: curr_section_end_points.second;

			const bool is_at_start_of_adjacent_section = adjacent_section->d_reverse_hint ^ !adjacent_is_previous_section;

			const std::pair<GPlatesMaths::PointOnSphere, GPlatesMaths::PointOnSphere> adjacent_section_end_points =
					GeometryUtils::get_geometry_exterior_end_points(*adjacent_section->get_section_geometry());
			const GPlatesMaths::PointOnSphere &adjacent_section_rubber_band = is_at_start_of_adjacent_section
					? adjacent_section_end_points.first
					: adjacent_section_end_points.second;

			// Rubber band point is the mid-point between the start/end of the current section and
			// start/end of the adjacent section.
			const GPlatesMaths::Vector3D rubber_band_point =
					GPlatesMaths::Vector3D(curr_section_rubber_band.position_vector()) +
					GPlatesMaths::Vector3D(adjacent_section_rubber_band.position_vector());

			if (!rubber_band_point.is_zero_magnitude())
			{
				return ResolvedSubSegmentRangeInSection::RubberBand(
						GPlatesMaths::PointOnSphere(rubber_band_point.get_normalisation()),
						is_at_start_of_current_section,
						is_at_start_of_adjacent_section,
						adjacent_is_previous_section);
			}
		}
	}

	return boost::none;
}


GPlatesMaths::PointOnSphere
GPlatesAppLogic::TopologicalIntersections::set_intersection_with_previous_section(
		const shared_ptr_type &previous_section,
		const GPlatesMaths::GeometryIntersect::Intersection &intersection)
{
	// If we have an intersection then we must have had intersectable polylines in the first place.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_intersectable_section_polyline &&
					previous_section->d_intersectable_section_polyline,
			GPLATES_ASSERTION_SOURCE);
	const GPlatesMaths::PolylineOnSphere &section_geometry1 = *previous_section->d_intersectable_section_polyline.get();
	const GPlatesMaths::PolylineOnSphere &section_geometry2 = *d_intersectable_section_polyline.get();

	const ResolvedSubSegmentRangeInSection::Intersection intersection_in_previous(
			intersection.position,
			intersection.segment_index1,
			intersection.is_on_segment1_start(),
			intersection.angle_in_segment1,
			section_geometry1);
	const ResolvedSubSegmentRangeInSection::Intersection intersection_in_current(
			intersection.position,
			intersection.segment_index2,
			intersection.is_on_segment2_start(),
			intersection.angle_in_segment2,
			section_geometry2);

	return set_intersection_with_previous_section(
			previous_section,
			intersection,
			intersection_in_previous,
			intersection_in_current);
}
