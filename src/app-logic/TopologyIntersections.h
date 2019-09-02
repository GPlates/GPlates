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

#include <utility>
#include <boost/enable_shared_from_this.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/weak_ptr.hpp>

#include "ReconstructionGeometry.h"
#include "ResolvedSubSegmentRangeInSection.h"

#include "maths/GeometryIntersect.h"
#include "maths/GeometryOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"


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
	class TopologicalIntersections :
			public boost::enable_shared_from_this<TopologicalIntersections>
	{
	public:
		typedef boost::shared_ptr<TopologicalIntersections> shared_ptr_type;
		typedef boost::shared_ptr<const TopologicalIntersections> shared_ptr_to_const_type;

		typedef boost::weak_ptr<TopologicalIntersections> weak_ptr_type;
		typedef boost::weak_ptr<const TopologicalIntersections> weak_ptr_to_const_type;


		/**
		 * We initialise with the full section geometry.
		 *
		 * If a polygon, then only the exterior ring is used (as a polyline).
		 *
		 * If this section intersects both its neighbouring sections then @a reverse_hint will be ignored
		 * (and a reverse flag determined by intersection processing will be used). If this section does
		 * *not* intersect both its neighbouring sections then @a reverse_hint will be used.
		 */
		static
		const shared_ptr_type
		create(
				const ReconstructionGeometry::non_null_ptr_to_const_type &section_reconstruction_geometry,
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &section_geometry,
				bool reverse_hint)
		{
			return shared_ptr_type(new TopologicalIntersections(section_reconstruction_geometry, section_geometry, reverse_hint));
		}


		/**
		 * Set the reverse hint (if it cannot be set in the constructor, or if it needs to be changed).
		 *
		 * If this section intersects both its neighbouring sections then @a reverse_hint will be ignored
		 * (and a reverse flag determined by intersection processing will be used). If this section does
		 * *not* intersect both its neighbouring sections then @a reverse_hint will be used.
		 */
		void
		set_reverse_hint(
				bool reverse_hint)
		{
			d_reverse_hint = reverse_hint;
		}


		/**
		 * Returns the original reconstruction geometry that the section geometry came from.
		 *
		 * This is the reconstruction geometry passed into the constructor.
		 */
		ReconstructionGeometry::non_null_ptr_to_const_type
		get_section_reconstruction_geometry() const
		{
			return d_section_reconstruction_geometry;
		}


		/**
		 * Returns the section geometry.
		 *
		 * This is the geometry passed into the constructor, except for polygons where the geometry
		 * is the exterior ring in the form of a polyline.
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		get_section_geometry() const
		{
			return d_section_geometry;
		}


		/**
		 * Intersects this section with the previous neighbouring topological section and
		 * returns intersection point if there was one.
		 *
		 * NOTE: In order to get meaningful results from 'this' object you need to
		 * call this method on 'this' object and call it on 'this' object's next section.
		 * Ideally this is called on each section in a circular boundary section list ensuring
		 * that each section gets intersected with both its neighbouring sections.
		 *
		 * If there were two or more intersections then only one is chosen.
		 */
		boost::optional<GPlatesMaths::PointOnSphere>
		intersect_with_previous_section(
				const shared_ptr_type &previous_section);


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
				const shared_ptr_type &previous_section);


		/**
		 * Returns the reverse flag for this section.
		 *
		 * If this section intersected both its neighbouring sections then the reverse hint
		 * (passed in contructor) will be ignored and a reverse flag determined by
		 * previous intersection processing will be returned.
		 *
		 * If this section did not intersect both its neighbouring sections then the reverse hint
		 * (passed in contructor) will be passed straight back to the caller.
		 * This is because the reverse flag was undetermined by intersection
		 * processing and so the reverse hint is then respected.
		 */
		bool
		get_reverse_flag() const;


		/**
		 * Returns the sub-segment range (including optional start/end intersections) of the entire
		 * section geometry that will contribute to a resolved topological geometry.
		 */
		ResolvedSubSegmentRangeInSection
		get_sub_segment_range_in_section() const;


		//! Delegate to equivalent method in @a ResolvedSubSegmentRangeInSection.
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		get_sub_segment_geometry() const
		{
			return get_sub_segment_range_in_section().get_geometry();
		}

		//! Delegate to equivalent method in @a ResolvedSubSegmentRangeInSection.
		void
		get_sub_segment_points(
				std::vector<GPlatesMaths::PointOnSphere> &geometry_points,
				bool include_rubber_band_points = true) const
		{
			get_sub_segment_range_in_section().get_geometry_points(geometry_points, include_rubber_band_points);
		}

		//! Delegate to equivalent method in @a ResolvedSubSegmentRangeInSection.
		void
		get_reversed_sub_segment_points(
				std::vector<GPlatesMaths::PointOnSphere> &geometry_points,
				bool include_rubber_band_points = true) const
		{
			get_sub_segment_range_in_section()
					.get_reversed_geometry_points(geometry_points, get_reverse_flag(), include_rubber_band_points);
		}

		//! Delegate to equivalent method in @a ResolvedSubSegmentRangeInSection.
		std::pair<GPlatesMaths::PointOnSphere/*start point*/, GPlatesMaths::PointOnSphere/*end point*/>
		get_sub_segment_end_points(
				bool include_rubber_band_points = true) const
		{
			return get_sub_segment_range_in_section().get_end_points(include_rubber_band_points);
		}

		//! Delegate to equivalent method in @a ResolvedSubSegmentRangeInSection.
		std::pair<GPlatesMaths::PointOnSphere/*start point*/, GPlatesMaths::PointOnSphere/*end point*/>
		get_reversed_sub_segment_end_points(
				bool include_rubber_band_points = true) const
		{
			return get_sub_segment_range_in_section()
					.get_reversed_end_points(get_reverse_flag(), include_rubber_band_points);
		}


		/**
		 * Returns true if this section only intersects the previous section.
		 */
		bool
		only_intersects_previous_section() const
		{
			return d_prev_intersection && !d_next_intersection;
		}

		/**
		 * Returns true if this section only intersects the next section.
		 */
		bool
		only_intersects_next_section() const
		{
			return !d_prev_intersection && d_next_intersection;
		}

		/**
		 * Returns true if this section intersects both its adjacent sections.
		 */
		bool
		intersects_previous_and_next_sections() const
		{
			return d_prev_intersection && d_next_intersection;
		}

		/**
		 * Returns true if this section does not intersect either of its adjacent sections.
		 */
		bool 
		does_not_intersect_previous_or_next_section() const
		{
			return !d_prev_intersection && !d_next_intersection;
		}

	private:

		/**
		 * Type to emulate *segments* used in prior implementations.
		 *
		 * This is only used when two adjacent sections intersect at more than one position, in which case
		 * it's used to choose the same intersection position as prior implementations.
		 * The current implementation doesn't need *segments* (other than for backward compatibility).
		 */
		typedef std::pair<
				boost::optional<ResolvedSubSegmentRangeInSection::Intersection>,
				boost::optional<ResolvedSubSegmentRangeInSection::Intersection> >
						backward_compatible_segment_type;


		/**
		 * The original reconstruction geometry that the section geometry came from.
		 */
		ReconstructionGeometry::non_null_ptr_to_const_type d_section_reconstruction_geometry;

		/**
		 * The original section geometry before it was partitioned by intersections.
		 *
		 * Note: For polygons this is actually the exterior ring of the polygon (in the form of a polyline).
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type d_section_geometry;

		/**
		 * If this section intersects both its neighbouring sections then @a reverse_hint will be ignored
		 * (and a reverse flag determined by intersection processing will be used). If this section does
		 * *not* intersect both its neighbouring sections then @a reverse_hint will be used.
		 */
		bool d_reverse_hint;

		/**
		 * The section geometry as an intersectable polyline.
		 *
		 * This is none for points and multipoints; and the exterior ring for polygons.
		 */
		boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> d_intersectable_section_polyline;

		/**
		 * The previous section that we were tested for intersection with.
		 */
		boost::optional<weak_ptr_type> d_prev_section;

		/**
		 * The next section that we were tested for intersection with.
		 */
		boost::optional<weak_ptr_type> d_next_section;

		/**
		 * Intersection with previous section, if any.
		 */
		boost::optional<ResolvedSubSegmentRangeInSection::Intersection> d_prev_intersection;

		/**
		 * Intersection with next section, if any.
		 */
		boost::optional<ResolvedSubSegmentRangeInSection::Intersection> d_next_intersection;


		TopologicalIntersections(
				const ReconstructionGeometry::non_null_ptr_to_const_type &section_reconstruction_geometry,
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &section_geometry,
				bool reverse_hint);

		boost::optional<GPlatesMaths::PointOnSphere>
		backward_compatible_multiple_intersections_with_previous_section(
				const shared_ptr_type &previous_section,
				const GPlatesMaths::GeometryIntersect::Graph &intersection_graph);

		boost::optional<GPlatesMaths::PointOnSphere>
		backward_compatible_multiple_intersections_between_segments(
				const shared_ptr_type &previous_section,
				const GPlatesMaths::GeometryIntersect::Graph &intersection_graph,
				const backward_compatible_segment_type &previous_segment,
				const backward_compatible_segment_type &current_segment);


		boost::optional<ResolvedSubSegmentRangeInSection::RubberBand>
		get_rubber_band(
				const boost::optional<weak_ptr_type> &adjacent_section,
				bool adjacent_is_previous_section) const;


		GPlatesMaths::PointOnSphere
		set_intersection_with_previous_section(
				const shared_ptr_type &previous_section,
				const GPlatesMaths::GeometryIntersect::Intersection &intersection,
				const ResolvedSubSegmentRangeInSection::Intersection &intersection_in_previous,
				const ResolvedSubSegmentRangeInSection::Intersection &intersection_in_current)
		{
			previous_section->d_next_intersection = intersection_in_previous;
			d_prev_intersection = intersection_in_current;

			return intersection.position;
		}

		GPlatesMaths::PointOnSphere
		set_intersection_with_previous_section(
				const shared_ptr_type &previous_section,
				const GPlatesMaths::GeometryIntersect::Intersection &intersection);
	};
}

#endif // GPLATES_APP_LOGIC_TOPOLOGYINTERSECTIONS_H
