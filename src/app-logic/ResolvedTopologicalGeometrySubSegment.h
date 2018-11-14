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

#ifndef GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALGEOMETRYSUBSEGMENT_H
#define GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALGEOMETRYSUBSEGMENT_H

#include <vector>
#include <boost/optional.hpp>

#include "ReconstructionGeometry.h"
#include "ResolvedSubSegmentRangeInSection.h"
#include "ResolvedVertexSourceInfo.h"

#include "maths/GeometryOnSphere.h"
#include "maths/PointOnSphere.h"

#include "model/FeatureHandle.h"

#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	/**
	 * Records the reconstructed geometry, and any other relevant information, of a subsegment.
	 *
	 * A subsegment can come from a reconstructed feature geometry or a resolved topological *line*.
	 *
	 * A subsegment is the subset of a reconstructed topological section's vertices that are used to form
	 * part of the geometry of a resolved topological polygon/polyline or boundary of a topological network.
	 */
	class ResolvedTopologicalGeometrySubSegment :
			public GPlatesUtils::ReferenceCount<ResolvedTopologicalGeometrySubSegment>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<ResolvedTopologicalGeometrySubSegment> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ResolvedTopologicalGeometrySubSegment> non_null_ptr_to_const_type;


		/**
		 * Create a subsegment using specified subsegment range (in section) and reconstruction geometry that it came from.
		 *
		 * If @a segment_reconstruction_geometry is a reconstructed feature geometry then all points in the
		 * subsegment geometry will share that same source reconstructed feature geometry.
		 *
		 * If @a segment_reconstruction_geometry is a resolved topological line then each point in the
		 * subsegment geometry will come from a subsegment of that resolved topological line
		 * (where those subsegments, in turn, are reconstructed feature geometries).
		 */
		static
		non_null_ptr_type
		create(
				const ResolvedSubSegmentRangeInSection &sub_segment,
				bool use_reverse,
				const GPlatesModel::FeatureHandle::const_weak_ref &segment_feature_ref,
				const ReconstructionGeometry::non_null_ptr_to_const_type &segment_reconstruction_geometry,
				boost::optional<ReconstructionGeometry::non_null_ptr_to_const_type> prev_segment_reconstruction_geometry,
				boost::optional<ReconstructionGeometry::non_null_ptr_to_const_type> next_segment_reconstruction_geometry)
		{
			return non_null_ptr_type(
					new ResolvedTopologicalGeometrySubSegment(
							sub_segment,
							use_reverse,
							segment_feature_ref,
							segment_reconstruction_geometry,
							prev_segment_reconstruction_geometry,
							next_segment_reconstruction_geometry));
		}


		//! Reference to the feature referenced by the topological section.
		const GPlatesModel::FeatureHandle::const_weak_ref &
		get_feature_ref() const
		{
			return d_segment_feature_ref;
		}

		/**
		 * The reconstruction geometry that the sub-segment was obtained from.
		 *
		 * This can be either a reconstructed feature geometry or a resolved topological *line*.
		 */
		const ReconstructionGeometry::non_null_ptr_to_const_type &
		get_reconstruction_geometry() const
		{
			return d_segment_reconstruction_geometry;
		}

		//! The reconstruction geometry of the previous sub-segment.
		boost::optional<ReconstructionGeometry::non_null_ptr_to_const_type>
		get_prev_reconstruction_geometry() const
		{
			return d_prev_segment_reconstruction_geometry;
		}

		//! The reconstruction geometry of the next sub-segment.
		boost::optional<ReconstructionGeometry::non_null_ptr_to_const_type>
		get_next_reconstruction_geometry() const
		{
			return d_next_segment_reconstruction_geometry;
		}

		/**
		 * Returns the full (un-clipped) section geometry.
		 *
		 * It will be a point, multi-point or polyline (a polygon exterior ring is converted to polyline).
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		get_section_geometry() const
		{
			return d_sub_segment.get_section_geometry();
		}


		/**
		 * The sub-segment range with the entire topological section geometry.
		 */
		const ResolvedSubSegmentRangeInSection &
		get_sub_segment() const
		{
			return d_sub_segment;
		}


		/**
		 * If true then the geometry returned by @a get_sub_segment_geometry had its points reversed in order
		 * before contributing to the final resolved topological geometry.
		 */
		bool
		get_use_reverse() const
		{
			return d_use_reverse;
		}

		/**
		 * The subset of vertices of topological section used in resolved topology geometry.
		 *
		 * NOTE: These are the un-reversed vertices of the original geometry that contributed this
		 * sub-segment - the actual order of vertices (as contributed to the final resolved
		 * topological geometry along with other sub-segments) depends on this un-reversed geometry
		 * and the reversal flag returned by @a get_use_reverse.
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		get_sub_segment_geometry() const
		{
			return d_sub_segment.get_geometry();
		}

		/**
		 * Returns the (unreversed) sub-segment points.
		 *
		 * Does not clear @a geometry_points - just appends points.
		 *
		 * NOTE: These are the un-reversed vertices of the original geometry that contributed this
		 * sub-segment - the actual order of vertices (as contributed to the final resolved
		 * topological geometry along with other sub-segments) depends on this un-reversed geometry
		 * and the reversal flag returned by @a get_use_reverse.
		 */
		void
		get_sub_segment_points(
				std::vector<GPlatesMaths::PointOnSphere> &geometry_points) const
		{
			d_sub_segment.get_geometry_points(geometry_points);
		}

		/**
		 * Returns the sub-segment points as they contribute to the resolved topology.
		 *
		 * These are @a get_sub_segment_points if @a get_use_reverse is false,
		 * otherwise they are a reversed version of @a get_sub_segment_points.
		 *
		 * Does not clear @a geometry_points - just appends points.
		 */
		void
		get_reversed_sub_segment_points(
				std::vector<GPlatesMaths::PointOnSphere> &geometry_points) const
		{
			d_sub_segment.get_reversed_geometry_points(geometry_points, d_use_reverse);
		}


		/**
		 * Returns the (unreversed) per-point source reconstructed feature geometries.
		 *
		 * Each point in @a get_sub_segment_points references a source reconstructed feature geometry.
		 * This method returns the same number of point sources as points returned by @a get_sub_segment_points.
		 *
		 * Does not clear @a point_source_infos - just appends point sources.
		 *
		 * @throws PreconditionViolationError if the section reconstruction geometry passed into @a create
		 * is neither a @a ReconstructedFeatureGeometry nor a @a ResolvedTopologicalLine.
		 */
		void
		get_sub_segment_point_source_infos(
				resolved_vertex_source_info_seq_type &point_source_infos) const;

		/**
		 * Same as @a get_sub_segment_point_source_infos but reverses them if necessary such that they are in the
		 * same order as @a get_reversed_sub_segment_points.
		 *
		 * These are @a get_sub_segment_point_source_infos if @a get_use_reverse is false,
		 * otherwise they are a reversed version of @a get_sub_segment_point_source_infos.
		 */
		void
		get_reversed_sub_segment_point_source_infos(
				resolved_vertex_source_info_seq_type &point_source_infos) const;

	private:

		//! The sub-segment.
		ResolvedSubSegmentRangeInSection d_sub_segment;

		//! Indicates if geometry (sub-segment) direction was reversed when assembling topology.
		bool d_use_reverse;

		//! Reference to the source feature handle of the topological section.
		GPlatesModel::FeatureHandle::const_weak_ref d_segment_feature_ref;

		/**
		 * The section reconstruction geometry.
		 *
		 * This is either a reconstructed feature geometry or a resolved topological *line*.
		 */
		ReconstructionGeometry::non_null_ptr_to_const_type d_segment_reconstruction_geometry;

		//! The reconstruction geometry of the previous section (if any).
		boost::optional<ReconstructionGeometry::non_null_ptr_to_const_type> d_prev_segment_reconstruction_geometry;
		//! The reconstruction geometry of the next section (if any).
		boost::optional<ReconstructionGeometry::non_null_ptr_to_const_type> d_next_segment_reconstruction_geometry;


		/**
		 * Each point in the subsegment geometry can potentially reference a different
		 * source reconstructed feature geometry.
		 *
		 * Note: All points can share the same point source info (if this subsegment came from a
		 * reconstructed feature geometry), but there is still one pointer for each point.
		 * However this does not use much extra memory, 8 bytes per point compared to the
		 * 32 bytes per PointOnSphere in the geometry.
		 *
		 * As an optimisation, this is only created when first requested.
		 */
		mutable boost::optional<resolved_vertex_source_info_seq_type> d_point_source_infos;


		ResolvedTopologicalGeometrySubSegment(
				const ResolvedSubSegmentRangeInSection &sub_segment,
				bool use_reverse,
				const GPlatesModel::FeatureHandle::const_weak_ref &segment_feature_ref,
				const ReconstructionGeometry::non_null_ptr_to_const_type &segment_reconstruction_geometry,
				boost::optional<ReconstructionGeometry::non_null_ptr_to_const_type> prev_segment_reconstruction_geometry,
				boost::optional<ReconstructionGeometry::non_null_ptr_to_const_type> next_segment_reconstruction_geometry) :
			d_sub_segment(sub_segment),
			d_use_reverse(use_reverse),
			d_segment_feature_ref(segment_feature_ref),
			d_segment_reconstruction_geometry(segment_reconstruction_geometry),
			d_prev_segment_reconstruction_geometry(prev_segment_reconstruction_geometry),
			d_next_segment_reconstruction_geometry(next_segment_reconstruction_geometry)
		{  }
	};


	//! Typedef for a sequence of @a ResolvedTopologicalGeometrySubSegment objects.
	typedef std::vector<ResolvedTopologicalGeometrySubSegment::non_null_ptr_type> sub_segment_seq_type;
}

#endif // GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALGEOMETRYSUBSEGMENT_H
