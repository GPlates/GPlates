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

#include "TopologyBoundaryIntersections.h"

#include "TopologyInternalUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


GPlatesAppLogic::TopologicalBoundaryIntersections::TopologicalBoundaryIntersections(
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &section_geometry) :
	d_state(STATE_INITIALISED),
	d_section_geometry(section_geometry),
	d_head_segment(section_geometry),
	d_last_intersected_head_segment(false),
	d_last_intersected_start_section(false),
	d_num_neighbours_intersected(0)
{
}


boost::optional<GPlatesMaths::PointOnSphere>
GPlatesAppLogic::TopologicalBoundaryIntersections::intersect_with_previous_section(
		TopologicalBoundaryIntersections &previous_section,
		const bool previous_section_reverse_hint)
{
	// Probably should throw exception on pre-condition violation instead of assert.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			previous_section.d_state != STATE_FINISHED && d_state != STATE_FINISHED,
			GPLATES_ASSERTION_SOURCE);

	previous_section.d_state = STATE_PROCESSING;
	d_state = STATE_PROCESSING;

	// All sections added to the boundary list should have at least one of their
	// head or tail segments initialised.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			previous_section.d_head_segment || previous_section.d_tail_segment,
			GPLATES_ASSERTION_SOURCE);
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_head_segment || d_tail_segment,
			GPLATES_ASSERTION_SOURCE);

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
	//    We handle this in "TopologyInternalUtils::intersect_topological_sections()"
	//    by considering the first intersection point only and dividing the
	//    two intersected segments each into a head and tail segment (neglecting the
	//    fact that there are other intersection points). This also introduces some
	//    randomness (but once again it's up to the user to not build
	//    plate polygons that intersect like this).
	//

#define INTERSECT_WITH_PREVIOUS_SECTION( \
		intersect_prev_section_head, \
		intersect_this_section_head) \
			{ \
				boost::optional<GPlatesMaths::PointOnSphere> intersection = \
						intersect_with_previous_section( \
								previous_section, \
								intersect_prev_section_head, \
								intersect_this_section_head); \
				if (intersection) \
				{ \
					return intersection; \
				} \
			}

	if (d_head_segment)
	{
		// Optimisation: try to minimize the number of intersection tests
		// by picking the most likely pair that will intersect.
		if (previous_section_reverse_hint)
		{
			// Try intersection with previous section's head segment first.
			if (previous_section.d_head_segment)
			{
				INTERSECT_WITH_PREVIOUS_SECTION(true, true)
			}
			if (previous_section.d_tail_segment)
			{
				INTERSECT_WITH_PREVIOUS_SECTION(false, true)
			}
		}
		else
		{
			// Try intersection with previous section's tail segment first.
			if (previous_section.d_tail_segment)
			{
				INTERSECT_WITH_PREVIOUS_SECTION(false, true)
			}
			if (previous_section.d_head_segment)
			{
				INTERSECT_WITH_PREVIOUS_SECTION(true, true)
			}
		}
	}

	if (d_tail_segment)
	{
		// Optimisation: try to minimize the number of intersection tests
		// by picking the most likely pair that will intersect.
		if (previous_section_reverse_hint)
		{
			// Try intersection with previous section's head segment first.
			if (previous_section.d_head_segment)
			{
				INTERSECT_WITH_PREVIOUS_SECTION(true, false)
			}
			if (previous_section.d_tail_segment)
			{
				INTERSECT_WITH_PREVIOUS_SECTION(false, false)
			}
		}
		else
		{
			// Try intersection with previous section's tail segment first.
			if (previous_section.d_tail_segment)
			{
				INTERSECT_WITH_PREVIOUS_SECTION(false, false)
			}
			if (previous_section.d_head_segment)
			{
				INTERSECT_WITH_PREVIOUS_SECTION(true, false)
			}
		}
	}

	return boost::none;
}


boost::optional<GPlatesMaths::PointOnSphere>
GPlatesAppLogic::TopologicalBoundaryIntersections::intersect_with_previous_section(
		TopologicalBoundaryIntersections &previous_section,
		bool intersect_prev_section_head,
		bool intersect_this_section_head)
{
	/*
	 * Typedef for a type returned by the overload of
	 * TopologyInternalUtils::intersect_topological_sections()
	 * allowing one intersection.
	 */
	typedef boost::optional<
			boost::tuple<
				/*intersection point*/
				GPlatesMaths::PointOnSphere, // 0
				/*head_first_section*/
				boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>, // 1
				/*tail_first_section*/
				boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>, // 2
				/*head_second_section*/
				boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>, // 3
				/*tail_second_section*/
				boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> > > // 4
						intersected_segments_type;

	const intersected_segments_type intersected_segments =
			TopologyInternalUtils::intersect_topological_sections(
					intersect_prev_section_head
							? previous_section.d_head_segment.get()
							: previous_section.d_tail_segment.get(),
					intersect_this_section_head
							? d_head_segment.get()
							: d_tail_segment.get());

	// Return early if there was no intersection.
	if (!intersected_segments)
	{
		return boost::none;
	}

	// Intersection point.
	const GPlatesMaths::PointOnSphere &intersection = boost::get<0>(*intersected_segments);

	// Extract the intersected segments.
	previous_section.d_head_segment = boost::get<1>(*intersected_segments);
	previous_section.d_tail_segment = boost::get<2>(*intersected_segments);
	d_head_segment = boost::get<3>(*intersected_segments);
	d_tail_segment = boost::get<4>(*intersected_segments);

	// Detect T-junctions and set the null segment to the intersection point.
	// This ensures all boundary segments will be able to return a geometry even
	// if it's just a point.
	previous_section.handle_t_or_v_junction(intersection);
	handle_t_or_v_junction(intersection);

	previous_section.d_last_intersected_head_segment = intersect_prev_section_head;
	d_last_intersected_head_segment = intersect_this_section_head;

	// The previous section does not have the current section as its start intersection.
	previous_section.d_last_intersected_start_section = false;
	// The current section does have the previous section as its start intersection.
	d_last_intersected_start_section = true;

	// If we have processed two intersections on either section then we can
	// set its boundary segment.
	if (++previous_section.d_num_neighbours_intersected == 2)
	{
		previous_section.set_boundary_segment();
	}
	if (++d_num_neighbours_intersected == 2)
	{
		set_boundary_segment();
	}

	// Intersection detected.
	return intersection;
}


boost::optional<
		boost::tuple<
				// First intersection
				GPlatesMaths::PointOnSphere,
				// Optional second intersection
				boost::optional<GPlatesMaths::PointOnSphere> > >
GPlatesAppLogic::TopologicalBoundaryIntersections::intersect_with_previous_section_allowing_two_intersections(
		TopologicalBoundaryIntersections &previous_section)
{
	// We're expecting two sections that have not yet been intersected.
	// Probably should throw exception on pre-condition violation instead of assert.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			previous_section.d_state == STATE_INITIALISED && d_state == STATE_INITIALISED,
			GPLATES_ASSERTION_SOURCE);

	// This method should only get called once regardless of whether one or two
	// intersections were detected.
	// So set the state to finished in case this method returns early somewhere.
	previous_section.d_state = STATE_FINISHED;
	d_state = STATE_FINISHED;

	/*
	 * Typedef for a type returned by the overload of
	 * TopologyInternalUtils::intersect_topological_sections_allowing_two_intersections()
	 * allowing two intersections.
	 */
	typedef boost::optional<
			boost::tuple<
				/*first intersection point*/
				GPlatesMaths::PointOnSphere, // 0
				/*optional second intersection point*/
				boost::optional<GPlatesMaths::PointOnSphere>, // 1
				/*optional info if two intersections and middle segments form a cycle*/
				boost::optional<bool>, // 2
				/*optional head_first_section*/
				boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>, // 3
				/*optional middle_first_section*/
				boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>, // 4
				/*optional tail_first_section*/
				boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>, // 5
				/*optional head_second_section*/
				boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>, // 6
				/*optional middle_second_section*/
				boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>, // 7
				/*optional tail_second_section*/
				boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> > > // 8
						intersected_segments_type;

	const intersected_segments_type intersected_segments =
			TopologyInternalUtils::intersect_topological_sections_allowing_two_intersections(
					previous_section.d_head_segment.get(),
					d_head_segment.get());

	// Return early if there were no intersections.
	if (!intersected_segments)
	{
		return boost::none;
	}

	const GPlatesMaths::PointOnSphere &first_intersection =
			boost::get<0>(*intersected_segments);
	const boost::optional<GPlatesMaths::PointOnSphere> &second_intersection =
			boost::get<1>(*intersected_segments);

	// If there was only one intersection then there were no middle segments.
	if (!second_intersection)
	{
		previous_section.d_head_segment = boost::get<3>(*intersected_segments);
		// No middle segment.
		previous_section.d_tail_segment = boost::get<5>(*intersected_segments);

		d_head_segment = boost::get<6>(*intersected_segments);
		// No middle segment.
		d_tail_segment = boost::get<8>(*intersected_segments);

		// Detect T-junctions and set the null segment to the intersection point.
		// This ensures all boundary segments will be able to return a geometry even
		// if it's just a point.
		previous_section.handle_t_or_v_junction(first_intersection);
		handle_t_or_v_junction(first_intersection);

		previous_section.d_last_intersected_head_segment = true;
		d_last_intersected_head_segment = true;

		// The previous section does not have the current section as its start intersection.
		previous_section.d_last_intersected_start_section = false;
		// The current section does have the previous section as its start intersection.
		d_last_intersected_start_section = true;

		previous_section.d_num_neighbours_intersected = 1;
		d_num_neighbours_intersected = 1;

		// One intersection detected - haven't finished intersection processing yet.
		return boost::make_tuple(first_intersection, second_intersection);
	}

	// The middle segments are the boundary segments.
	// These middle segments will always be non-null since there's two intersections
	// which cannot be the same intersection.
	previous_section.d_boundary_segment = boost::get<4>(*intersected_segments);
	d_boundary_segment = boost::get<7>(*intersected_segments);

	previous_section.d_num_neighbours_intersected = 2;
	d_num_neighbours_intersected = 2;

	// If the middle segments form a cycle then we don't need to reverse either section.
	// We can dereference the boost optional because we know we have two intersections.
	const bool sections_form_cycle = *boost::get<2>(*intersected_segments);
	// Reverse either of the sections if the two sections don't form a cycle.
	// We arbitrarily choose the current section.
	previous_section.d_reverse_section = false;
	d_reverse_section = !sections_form_cycle;

	// All the other data members don't matter because we're finished.

	// Two intersections detected.
	return boost::make_tuple(first_intersection, second_intersection);
}


bool
GPlatesAppLogic::TopologicalBoundaryIntersections::get_reverse_flag(
		bool reverse_hint) const
{
	// If we've already determined the reverse flag then return it
	// otherwise return the caller's reverse flag.
	if (d_reverse_section)
	{
		return *d_reverse_section;
	}

	return reverse_hint;
}


GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::TopologicalBoundaryIntersections::get_unreversed_boundary_segment(
		bool reverse_hint) const
{
	// If we've already determined the boundary segment then return it.
	if (d_boundary_segment)
	{
		return *d_boundary_segment;
	}

	if (d_num_neighbours_intersected == 0)
	{
		// If the current section did not intersect either of its neighbours then
		// just set the full section geometry as the boundary segment.
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
		// incorrect reverse flag here (but then the old version of the topology boundary
		// resolver would also generate an incorrect plate polygon). So this is something
		// the user should fix with the new build tool simply by selecting the topological
		// plate polygon and then selecting the edit tool (this will automatically generate
		// the correct reverse flags for all sections if all sections are intersecting)
		// and then selecting the 'Apply/Creating' button to save the new reverse flags.
		return d_section_geometry;
	}

	// If we get here then the number of intersections must be one since zero
	// intersections was covered above and two intersections is always covered by
	// the 'd_boundary_segment' case above because non-null geometry is always
	// returned even if the two adjacent sections intersect us at the same point.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_num_neighbours_intersected == 1,
			GPLATES_ASSERTION_SOURCE);

	// The current section only intersected one of its neighbours.
	// In this case we want to trust the reverse flag set by the topology build tool
	// (for an explanation see the comment for the case for zero intersections above).
	// All we need to do is pick the head segment or tail segment
	// from the single intersection.
	// We do this by considering the geometry of the head and tail segments
	// after they have been reversed (if the reverse flag is set).
	// If the single intersection was with the start (previous) neighbour then we
	// want the start point of this section's reversed geometry to touch the
	// end point of the previous neighbour's boundary segment (ie, the intersection point).
	// This means choosing the tail segment of the current segment if the geometry
	// is not reversed or the head segment if it is reversed.
	// Similar logic follows if the single intersection was with the end (next) neighbour.
	// This amounts to the exclusive-or relationship...
	const bool use_tail_segment = d_last_intersected_start_section ^ reverse_hint;

	// We can dereference these optionals because we have already made sure that
	// non-null geometry is stored in both head and tail segments when an intersection
	// happens.
	return use_tail_segment ? *d_tail_segment : *d_head_segment;
}


void
GPlatesAppLogic::TopologicalBoundaryIntersections::set_boundary_segment()
{
	// We should have exactly two intersections.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_num_neighbours_intersected == 2,
			GPLATES_ASSERTION_SOURCE);

	if (d_last_intersected_head_segment)
	{
		// The current section's head segment (from the first intersection) has been
		// intersected (by the second intersection) into a new head and tail segment.
		// This means its boundary segment is the new tail segment from this intersection.
		//
		// Note that it's possible for the second intersection to occur at the end point
		// of the current section's head segment (from the first intersection)
		// which means there is no new tail segment from this intersection.
		// This is ok though since it means the boundary segment is zero length
		// and doesn't contribute to the plate polygon boundary.

		// If the current section just intersected (second intersection) its
		// previous neighbour (a start intersection) then we do not need
		// to reverse the boundary segment - otherwise we do need to reverse.
		//
		// This is because the head segment (from the first intersection) was
		// intersected (by the second intersection), and not the tail segment,
		// meaning that the second intersection was closer to the section's
		// start point than the first intersection. And since the second intersection
		// is with the previous section (a start intersection) it means the
		// order of the boundary segments relative to each other is the same as
		// the order of points along the boundary segments of the current section.
		//
		// And we have enough information that we can override the reverse flag.
		// The initial reverse flag, obtained from the GPML source, is only needed
		// when the section in question does not intersect both its neighbours.
		d_boundary_segment = d_tail_segment;

		d_reverse_section = !d_last_intersected_start_section;
	}
	else
	{
		//
		// The same reasoning that applied above applies here but in reverse.
		//
		d_boundary_segment = d_head_segment;

		d_reverse_section = d_last_intersected_start_section;
	}

	d_state = STATE_FINISHED;
}


void
GPlatesAppLogic::TopologicalBoundaryIntersections::handle_t_or_v_junction(
		const GPlatesMaths::PointOnSphere &intersection)
{
	// Only one of the head and tail segment can be null.
	if (!d_head_segment)
	{
		d_head_segment = intersection.clone_as_geometry();
	}
	else if (!d_tail_segment)
	{
		d_tail_segment = intersection.clone_as_geometry();
	}
}
