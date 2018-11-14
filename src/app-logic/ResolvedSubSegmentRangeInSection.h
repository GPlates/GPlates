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

#ifndef GPLATES_APP_LOGIC_RESOLVEDSUBSEGMENTRANGEINSECTION_H
#define GPLATES_APP_LOGIC_RESOLVEDSUBSEGMENTRANGEINSECTION_H

#include <utility>
#include <vector>
#include <boost/operators.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include "maths/AngularDistance.h"
#include "maths/GeometryOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"


namespace GPlatesAppLogic
{
	/**
	 * The sub-segment range of an entire topological section geometry that contributes to a
	 * resolved topological geometry.
	 *
	 * The sub-segment is the result of intersecting a section with its two adjacent sections
	 * (such as in a topological boundary) which usually results in some vertices from the
	 * section bounded by a start and an end intersection, or in some cases only a single intersection,
	 * or even no intersections (resulting in the sub-segment being the entire section geometry).
	 *
	 * This class keeps track of the range of vertex *indices* so that any quantities associated with
	 * the section vertices can be tracked (such as vertex plate IDs and velocities).
	 */
	class ResolvedSubSegmentRangeInSection
	{
	public:

		/**
		 * Location of intersection within a specific section (eg, current or previous sections).
		 *
		 * Intersections only apply to section polylines, or polygons (treated as exterior ring polylines).
		 */
		class Intersection :
				public boost::less_than_comparable<Intersection>
		{
		public:

			/**
			 * Construct a general intersection.
			 */
			Intersection(
					const GPlatesMaths::PointOnSphere &position_,
					unsigned int segment_index_,
					bool on_segment_start_,
					GPlatesMaths::AngularDistance angle_in_segment_,
					const GPlatesMaths::PolylineOnSphere &section_polyline);

			/**
			 * Construct intersection *at* first vertex (if @a at_start is true) or
			 * last vertex (if @a at_start is false) of section geometry.
			 */
			Intersection(
					const GPlatesMaths::GeometryOnSphere &section_geometry,
					bool at_start);

			/**
			 * Less than operator - all other operators (<=, >, >=) provided by boost::less_than_comparable.
			 */
			bool
			operator<(
					const Intersection &rhs) const
			{
				return segment_index < rhs.segment_index ||
					(segment_index == rhs.segment_index &&
						angle_in_segment.is_precisely_less_than(rhs.angle_in_segment));
			}


			//! Intersection position.
			GPlatesMaths::PointOnSphere position;

			/**
			 * Index into the segments (great circle arcs) of the section polyline.
			 *
			 * NOTE: A segment index can be equal to the number of segments in the section polyline.
			 *       This represents an intersection with the *last* vertex in the section polyline.
			 *       
			 *       In other words, the segment index can be the fictitious *one-past-the-last* segment.
			 *       So, in this case, care should be taken to not dereference (look up segment in
			 *       section polyline). In this case 'on_segment_start' will be true, and this will
			 *       represent an intersection with the start of the fictitious *one-past-the-last* segment
			 *       which is the same as the end of the last segment (ie, last vertex in polyline).
			 *       In this case the segment index can be thought of as the vertex index.
			 */
			unsigned int segment_index;

			//! Whether intersection is *on* the start of the segment indexed by @a segment_index.
			bool on_segment_start;

			/**
			 * Angle (radians) from segment start point to intersection along segment.
			 *
			 * If @a on_segment_start is true then this will be AngularDistance::ZERO.
			 */
			GPlatesMaths::AngularDistance angle_in_segment;

			/**
			 * Value in range [0, 1] where 0 represents the segment start point and 1 the segment
			 * end point - so to interpolate quantities use the formula:
			 *
			 *   quantity_lerp = quantity_at_start_point +
			 *			interpolate_ratio * (quantity_at_end_point - quantity_at_start_point)
			 */
			double interpolate_ratio_in_segment;
		};


		/**
		 * Location and information of rubber banding with an adjacent section.
		 *
		 * Rubber banding occurs when there is not intersection with an adjacent section.
		 */
		class RubberBand
		{
		public:

			RubberBand(
					const GPlatesMaths::PointOnSphere &position_,
					bool is_at_start_of_current_section_,
					bool is_at_start_of_adjacent_section_,
					bool is_previous_section_adjacent_) :
				position(position_),
				is_at_start_of_current_section(is_at_start_of_current_section_),
				is_at_start_of_adjacent_section(is_at_start_of_adjacent_section_),
				adjacent_is_previous_section(is_previous_section_adjacent_)
			{  }


			/**
			 * The rubber band position halfway between the adjacent reversed sub-segment and
			 * the current reversed sub-segment.
			 */
			GPlatesMaths::PointOnSphere position;

			/**
			 * Whether the start vertex of the current unreversed section is used to determine rubber-band position.
			 */
			bool is_at_start_of_current_section;

			/**
			 * Whether the start vertex of the adjacent unreversed section is used to determine rubber-band position.
			 */
			bool is_at_start_of_adjacent_section;

			/**
			 * Whether the adjacent section is the previous section, if false then adjacent to next section.
			 */
			bool adjacent_is_previous_section;
		};


		/**
		 * Can have an @a Intersection or a @a RubberBand (but not both) at start or end of a section.
		 */
		class IntersectionOrRubberBand
		{
		public:
			explicit
			IntersectionOrRubberBand(
					const Intersection &intersection) :
				d_intersection_or_rubber_band(intersection)
			{  }

			explicit
			IntersectionOrRubberBand(
					const RubberBand &rubber_band) :
				d_intersection_or_rubber_band(rubber_band)
			{  }

			//! If returns none then @a get_rubber_band will return non-none.
			boost::optional<const Intersection &>
			get_intersection() const
			{
				if (const Intersection *intersection = boost::get<Intersection>(&d_intersection_or_rubber_band))
				{
					return *intersection;
				}

				return boost::none;
			}

			//! If returns none then @a get_intersection will return non-none.
			boost::optional<const RubberBand &>
			get_rubber_band() const
			{
				if (const RubberBand *rubber_band = boost::get<RubberBand>(&d_intersection_or_rubber_band))
				{
					return *rubber_band;
				}

				return boost::none;
			}

		private:
			boost::variant<Intersection, RubberBand> d_intersection_or_rubber_band;
		};


		/**
		 * If no start intersection or rubber band then sub-segment starts at beginning of section.
		 * If no end intersection or rubber band then sub-segment ends at end of section.
		 *
		 * A start/end rubber band is an extra point that is not on the main section geometry.
		 * It is halfway between the adjacent (reversed) sub-segment and this (reversed) sub-segment.
		 *
		 * Note that @a section_geometry must be a point, multi-point or polyline.
		 *
		 * @throws PreconditionViolationError if @a section_geometry is a polygon.
		 */
		explicit
		ResolvedSubSegmentRangeInSection(
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type section_geometry,
				boost::optional<IntersectionOrRubberBand> start_intersection_or_rubber_band = boost::none,
				boost::optional<IntersectionOrRubberBand> end_intersection_or_rubber_band = boost::none);


		/**
		 * Returns the section geometry.
		 *
		 * This is the geometry passed into the constructor.
		 * It will be a point, multi-point or polyline (a polygon exterior ring is converted to polyline).
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		get_section_geometry() const
		{
			return d_section_geometry;
		}

		/**
		 * Returns the number of points in @a get_section_geoemtry.
		 */
		unsigned int
		get_num_points_in_section_geometry() const
		{
			return d_num_points_in_section_geometry;
		}


		/**
		 * Return the (unreversed) sub-segment geometry.
		 *
		 * The returned data is non-null since T-junctions, V-junctions and cases like
		 * adjacent sections intersecting this section at the same point will all return
		 * a point geometry (intersection point).
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		get_geometry(
				bool include_rubber_band_points = true) const;


		/**
		 * Returns the (unreversed) geometry points.
		 *
		 * Does not clear @a geometry_points - just appends points.
		 */
		void
		get_geometry_points(
				std::vector<GPlatesMaths::PointOnSphere> &geometry_points,
				bool include_rubber_band_points = true) const;

		/**
		 * Returns the geometry points as they contribute to the resolved topology.
		 *
		 * These are @a get_geometry_points if @a use_reverse is false,
		 * otherwise they are a reversed version of @a get_geometry_points.
		 *
		 * Does not clear @a geometry_points - just appends points.
		 */
		void
		get_reversed_geometry_points(
				std::vector<GPlatesMaths::PointOnSphere> &geometry_points,
				bool use_reverse,
				bool include_rubber_band_points = true) const;


		/**
		 * Return the start and end points of the sub-segment range in the section.
		 *
		 * If there are start and/or end intersections or rubber bands then these will be start and/or end points.
		 */
		std::pair<GPlatesMaths::PointOnSphere/*start point*/, GPlatesMaths::PointOnSphere/*end point*/>
		get_end_points(
				bool include_rubber_band_points = true) const;

		/**
		 * Return the start and end points of sub-segment range in section as contributed to resolved topology.
		 *
		 * If there are start and/or end intersections or rubber bands then these will be start and/or end points.
		 *
		 * These are @a get_end_points if @a use_reverse is false,
		 * otherwise they are a reversed version of @a get_end_points.
		 */
		std::pair<GPlatesMaths::PointOnSphere/*start point*/, GPlatesMaths::PointOnSphere/*end point*/>
		get_reversed_end_points(
				bool use_reverse,
				bool include_rubber_band_points = true) const;


		/**
		 * Return the number of points in the sub-segment (including optional intersection or rubber band points).
		 */
		unsigned int
		get_num_points(
				bool include_rubber_band_points = true) const
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


		/**
		 * Index of first vertex of section geometry that contributes to sub-segment.
		 *
		 * If zero then sub-segment start matches start of section.
		 */
		unsigned int
		get_start_section_vertex_index() const
		{
			return d_start_section_vertex_index;
		}

		/**
		 * Index of *one-past-the-last* vertex of section geometry that contributes to sub-segment.
		 *
		 * If equal to the number of vertices in section then sub-segment end matches end of section.
		 *
		 * NOTE: This index is *one-past-the-last* index and so should be used like begin/end iterators.
		 */
		unsigned int
		get_end_section_vertex_index() const
		{
			return d_end_section_vertex_index;
		}


		/**
		 * Optional intersection or rubber band signifying start of sub-segment.
		 *
		 * Note that there cannot be both a start intersection and a start rubber band.
		 *
		 * If no start intersection (or rubber band) then sub-segment start matches start of section.
		 *
		 * NOTE: This could be an intersection with the previous or next section.
		 */
		boost::optional<IntersectionOrRubberBand>
		get_start_intersection_or_rubber_band() const;

		/**
		 * Optional intersection signifying start of sub-segment.
		 *
		 * Note that there cannot be both a start intersection and a start rubber band.
		 *
		 * If no start intersection (or rubber band) then sub-segment start matches start of section.
		 *
		 * NOTE: This could be an intersection with the previous or next section.
		 */
		const boost::optional<Intersection> &
		get_start_intersection() const
		{
			return d_start_intersection;
		}

		/**
		 * Optional rubber band signifying start of sub-segment.
		 *
		 * Note that there cannot be both a start rubber band and a start intersection.
		 *
		 * If no start rubber band (or intersection) then sub-segment start matches start of section.
		 *
		 * NOTE: This could be a rubber band with the previous *or* next section.
		 */
		const boost::optional<RubberBand> &
		get_start_rubber_band() const
		{
			return d_start_rubber_band;
		}


		/**
		 * Optional intersection or rubber band signifying end of sub-segment.
		 *
		 * Note that there cannot be both a end intersection and a end rubber band.
		 *
		 * If no end intersection (or rubber band) then sub-segment end matches end of section.
		 *
		 * NOTE: This could be an intersection with the previous or next section.
		 */
		boost::optional<IntersectionOrRubberBand>
		get_end_intersection_or_rubber_band() const;

		/**
		 * Optional intersection signifying end of sub-segment.
		 *
		 * Note that there cannot be both an end intersection and an end rubber band.
		 *
		 * If no end intersection (or rubber band) then sub-segment end matches end of section.
		 *
		 * NOTE: This could be an intersection with the previous or next section.
		 */
		const boost::optional<Intersection> &
		get_end_intersection() const
		{
			return d_end_intersection;
		}

		/**
		 * Optional intersection signifying end of sub-segment.
		 *
		 * Note that there cannot be both an end rubber band and an end intersection.
		 *
		 * If no end rubber band (or intersection) then sub-segment end matches end of section.
		 *
		 * NOTE: This could be a rubber band with the previous *or* next section.
		 */
		const boost::optional<RubberBand> &
		get_end_rubber_band() const
		{
			return d_end_rubber_band;
		}

	private:
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type d_section_geometry;
		unsigned int d_num_points_in_section_geometry;

		unsigned int d_start_section_vertex_index;
		unsigned int d_end_section_vertex_index;

		boost::optional<Intersection> d_start_intersection;
		boost::optional<Intersection> d_end_intersection;

		boost::optional<RubberBand> d_start_rubber_band;
		boost::optional<RubberBand> d_end_rubber_band;
	};
}

#endif // GPLATES_APP_LOGIC_RESOLVEDSUBSEGMENTRANGEINSECTION_H
