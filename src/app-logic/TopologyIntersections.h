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

#ifndef GPLATES_APP_LOGIC_TOPOLOGYINTERSECTIONS_H
#define GPLATES_APP_LOGIC_TOPOLOGYINTERSECTIONS_H

#include <boost/optional.hpp>
#include <boost/tuple/tuple.hpp>

#include "maths/GeometryOnSphere.h"
#include "maths/PointOnSphere.h"


namespace GPlatesAppLogic
{
	/**
	 * Keeps track of a topological section's intersection results with its
	 * neighbouring sections to assist with determining the partitioned segment.
	 *
	 * We store the full geometry in the head segment to start with leaving the
	 * tail segment empty.
	 * We could have chosen the other way around if we wanted - it's arbitrary.
	 *
	 * When/if this segment gets intersected with a neighbour it will be divided
	 * into a head and tail segment (or one of the two if intersection is a T-junction).
	 * Then when one of those segments is intersected again (with the other neighbour) then
	 * it will be divided into a head and tail segment (or one of the two if
	 * intersection is a T-junction).
	 * This two-step procedure is followed in order to find the middle segment which
	 * is the actual segment used for a resolved topological geometry.
	 */
	class TopologicalIntersections
	{
	public:
		/**
		 * We initialise with the full section geometry.
		 */
		TopologicalIntersections(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &section_geometry);


		/**
		 * Intersects this section with the previous neighbouring topological section and
		 * returns intersection point if there was one.
		 *
		 * NOTE: In order to get meaningful results from 'this' object you need to
		 * call this method on 'this' object and call it on 'this' object's next section.
		 * Ideally this is called on each section in a circular boundary section list ensuring
		 * that each section gets intersected with both its neighbouring sections.
		 *
		 * If there were two or more intersections then only the chosen intersection is
		 * returned - and this is reported as a user error.
		 * See TopologyInternalUtils::intersect_topological_sections() for more details
		 * regarding how the intersection point is chosen/handled.
		 *
		 * @a previous_section_reverse_hint is an optimisation hint that use the reverse
		 * flag of the previous section to minimize the number of intersection tests needed.
		 * This takes advantage of the iteration order in which sections are iterated
		 * (from first section to last section).
		 */
		boost::optional<GPlatesMaths::PointOnSphere>
		intersect_with_previous_section(
				TopologicalIntersections &previous_section,
				const bool previous_section_reverse_hint);


		/**
		 * Intersects this section with the previous neighbouring topological section and
		 * returns one or two intersection points if there were any.
		 *
		 * NOTE: This method should only be called once - this is because there
		 * are exactly two sections in the topology list and they are allowed to
		 * intersect twice - thus this method is designed to handle both intersections
		 * in one call unlike the above method which only handles one intersection per call.
		 *
		 * This is a special case because under these conditions a topology plate polygon can be formed.
		 *
		 * If there were three or more intersections then only two chosen intersections are
		 * returned - and this is reported as a user error.
		 * See TopologyInternalUtils::intersect_topological_sections_allowing_two_intersections()
		 * for more details regarding how the two intersection points are chosen/handled.
		 */
		boost::optional<
				boost::tuple<
						// First intersection
						GPlatesMaths::PointOnSphere,
						// Optional second intersection
						boost::optional<GPlatesMaths::PointOnSphere> > >
		intersect_with_previous_section_allowing_two_intersections(
				TopologicalIntersections &previous_section);


		/**
		 * Returns the reverse flag for this section.
		 *
		 * If this section intersected both its neighbouring sections then
		 * @a reverse_hint will be ignored and a reverse flag determined by
		 * previous intersection processing will be returned.
		 *
		 * If this section did not intersect both its neighbouring sections then
		 * @a reverse_hint will be passed straight back to the caller.
		 * This is because the reverse flag was undetermined by intersection
		 * processed and so the caller's flag is then respected.
		 */
		bool
		get_reverse_flag(
				bool reverse_hint) const;


		/**
		 * Returns the sub-segment that will contribute to a resolved topological geometry.
		 *
		 * The returned segment does not have reversal taken into account (it's the
		 * unreversed geometry).
		 *
		 * The returned data is non-null since T-junctions, V-junctions and cases like
		 * adjacent sections intersecting this section at the same point will all return
		 * a point geometry (intersection point).
		 *
		 * @a reverse_hint is only used if this section has intersected exactly one of
		 * its neighbouring sections and in this case it helps determine whether to
		 * return the head or tail segment partitioned by that intersection.
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		get_unreversed_sub_segment(
				bool reverse_hint) const;


		/**
		 * Returns true if this section only intersects the previous section.
		 */
		bool
		only_intersects_previous_section() const
		{
			return d_num_neighbours_intersected == 1 &&
					d_last_intersected_start_section;
		}


		/**
		 * Returns true if this section only intersects the next section.
		 */
		bool
		only_intersects_next_section() const
		{
			return d_num_neighbours_intersected == 1 &&
					!d_last_intersected_start_section;
		}


		/**
		 * Returns true if this section intersects both its adjacent sections.
		 */
		bool
		intersects_previous_and_next_sections() const
		{
			return d_num_neighbours_intersected == 2;
		}

		unsigned int 
		get_num_neighbours_intersected() const
		{
			return d_num_neighbours_intersected;
		}

	private:
		/**
		 * Keep track of where we are in the processing of intersections.
		 */
		enum State { STATE_INITIALISED, STATE_PROCESSING, STATE_FINISHED} d_state;

		/**
		 * The original section geometry before it was partitioned by intersections.
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type d_section_geometry;

		/**
		 * The head segment resulting from the most recent intersection with one
		 * of this section's neighbours.
		 *
		 * This segment contains (starts with) the head point of the segment that
		 * was intersected to produce the head segment and the tail segment.
		 */
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
				d_head_segment;

		/**
		 * The tail segment resulting from the most recent intersection with one
		 * of this section's neighbours.
		 *
		 * This segment contains (ends with) the tail point of the segment that
		 * was intersected to produce the head segment and the tail segment.
		 */
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> d_tail_segment;

		/**
		 * This is only useful in the context of a second intersection
		 * if there was one - and in that context the question is:
		 * Did the second intersection (with one neighbour) intersect
		 * this section's head or tail segment (from the first
		 * intersection with the other neighbour) ?
		 */
		bool d_last_intersected_head_segment;

		/**
		 * This is only useful in the context of a second intersection
		 * if there was one - and in that context the question is:
		 * Did the second intersection (with one neighbour) intersect
		 * this section's head or tail segment (from the first
		 * intersection with the other neighbour) ?
		 */
		bool d_last_intersected_start_section;

		/**
		 * The number of neighbours that this section intersects.
		 */
		unsigned int d_num_neighbours_intersected;

		/**
		 * The sub-segment that will contribute to the resolved topological geometry.
		 *
		 * This is empty until two intersections have been processed on this section.
		 */
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> d_sub_segment;

		/**
		 * Should the section direction be reversed?
		 *
		 * This flag is only valid when this section has intersected both its neighbours
		 * otherwise the caller will have to provide a reverse flag.
		 */
		boost::optional<bool> d_reverse_section;


		boost::optional<GPlatesMaths::PointOnSphere>
		intersect_with_previous_section(
				TopologicalIntersections &previous_section,
				bool intersect_prev_section_head,
				bool intersect_this_section_head);

		/**
		 * Determines and sets the sub-segment but requires two intersections
		 * to have been processed on this section.
		 */
		void
		set_sub_segment();

		/**
		 * Detect T-junctions and set the head or tail null segment to the intersection point.
		 */
		void
		handle_t_or_v_junction(
				const GPlatesMaths::PointOnSphere &intersection);
	};
}

#endif // GPLATES_APP_LOGIC_TOPOLOGYINTERSECTIONS_H
