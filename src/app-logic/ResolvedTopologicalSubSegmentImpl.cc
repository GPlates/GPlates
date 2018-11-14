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

#include <algorithm>
#include <iterator>

#include "ResolvedTopologicalSubSegmentImpl.h"

#include "GeometryUtils.h"
#include "ReconstructedFeatureGeometry.h"
#include "ReconstructionGeometryUtils.h"
#include "ResolvedTopologicalLine.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


namespace GPlatesAppLogic
{
	namespace
	{
		/**
		 * Finds the vertex source info corresponding to the specified intersection along the section polyline.
		 */
		ResolvedVertexSourceInfo::non_null_ptr_to_const_type
		get_intersection_vertex_source_info(
				const ResolvedSubSegmentRangeInSection::Intersection &intersection,
				ReconstructionGeometry::non_null_ptr_to_const_type section_reconstruction_geometry)
		{
			boost::optional<ReconstructedFeatureGeometry::non_null_ptr_to_const_type> section_reconstructed_feature_geometry =
					ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
							ReconstructedFeatureGeometry::non_null_ptr_to_const_type>(section_reconstruction_geometry);
			if (section_reconstructed_feature_geometry)
			{
				// It doesn't matter where the intersection in the section is since all points have the same source info.
				return ResolvedVertexSourceInfo::create(section_reconstructed_feature_geometry.get());
			}
			// else it's a resolved topological *line* ...

			boost::optional<ResolvedTopologicalLine::non_null_ptr_to_const_type> section_resolved_topological_line_opt =
					ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
							ResolvedTopologicalLine::non_null_ptr_to_const_type>(section_reconstruction_geometry);

			// Section reconstruction geometry must either be a ReconstructedFeatureGeometry or a ResolvedTopologicalLine.
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					section_resolved_topological_line_opt,
					GPLATES_ASSERTION_SOURCE);
			ResolvedTopologicalLine::non_null_ptr_to_const_type section_resolved_topological_line =
					section_resolved_topological_line_opt.get();

			const resolved_vertex_source_info_seq_type &section_vertex_source_infos =
					section_resolved_topological_line->get_vertex_source_infos();

			if (intersection.on_segment_start)
			{
				// Since intersection is on start of segment it is also a vertex index.
				const unsigned int vertex_index = intersection.segment_index;

				// Note that this can be the fictitious one-past-the-last *segment* but we can
				// dereference as a *vertex index* since that will be the last *vertex*.
				return section_vertex_source_infos[vertex_index];
			}
			// else intersection is in middle of a segment...

			// Segment's start and end points.
			// Note that the segment's *end* vertex is dereferenceable because we can't be in the middle
			// of the fictitious *one-past-the-last* segment (since already tested not on segment start).
			const unsigned int segment_start_vertex_index = intersection.segment_index;
			const unsigned int segment_end_vertex_index = segment_start_vertex_index + 1;

			// If the segment's start and end points have the same vertex source info then we don't
			// need to interpolate between them.
			//
			// Note that we're comparing ResolvedVertexSourceInfo objects, not 'non_null_intrusive_ptr's.
			if (*section_vertex_source_infos[segment_start_vertex_index] ==
				*section_vertex_source_infos[segment_end_vertex_index])
			{
				return section_vertex_source_infos[segment_start_vertex_index];
			}
			// else vertex source infos are different for start and end points of intersected segment...

			//
			// This situation will be very rare because:
			//  - If topological line consists of points, then the end points of the topological line
			//    will usually be made to match the end points of adjacent topological sections such
			//    that they touch (intersect at points, not in middle of segments).
			//  - If topological line consists of intersecting static lines, then the only segments
			//    along topological line that contain differing vertex source infos for segment start
			//    and end points will be zero-length segments resulting from the intersection of
			//    those static lines (ie, each static line can have a different plate ID, but they
			//    will intersect at a point, which then becomes a zero-length segment with start point
			//    carrying one plate ID and end point carrying the other).
			//
			return ResolvedVertexSourceInfo::create(
					section_vertex_source_infos[segment_start_vertex_index],
					section_vertex_source_infos[segment_end_vertex_index],
					intersection.interpolate_ratio_in_segment);
		}


		/**
		 * Returns the vertex source info at either the start (if @a is_at_start_vertex is true) or end of the section.
		 *
		 * The returned source info is to be used for rubber banding.
		 */
		ResolvedVertexSourceInfo::non_null_ptr_to_const_type
		get_rubber_band_vertex_source_info_at_section_end_point(
				const ReconstructionGeometry::non_null_ptr_to_const_type &section_reconstruction_geometry,
				bool is_at_start_vertex)
		{
			boost::optional<ReconstructedFeatureGeometry::non_null_ptr_to_const_type> section_reconstructed_feature_geometry_opt =
					ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
							ReconstructedFeatureGeometry::non_null_ptr_to_const_type>(section_reconstruction_geometry);
			if (section_reconstructed_feature_geometry_opt)
			{
				ReconstructedFeatureGeometry::non_null_ptr_to_const_type section_reconstructed_feature_geometry =
						section_reconstructed_feature_geometry_opt.get();

				// All section vertices have the same source info.
				ResolvedVertexSourceInfo::non_null_ptr_to_const_type section_source_info =
						ResolvedVertexSourceInfo::create(section_reconstructed_feature_geometry);

				// Get the section end points.
				//
				// Currently we need to convert a polygon exterior to polyline (same as done by TopologicalIntersections).
				// Note that the end vertex of a polygon exterior converted to a polyline (where start == end)
				// is different than the end vertex of the polygon exterior ring (where start != end).
				//
				// FIXME: Make sure we use the same section geometry as used by TopologicalIntersections.
				//        Perhaps by passing in the actual section geometries obtained from TopologicalIntersections.
				//
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type section_geometry =
						section_reconstructed_feature_geometry->reconstructed_geometry();
				boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> section_polyline =
						GeometryUtils::convert_geometry_to_polyline(*section_geometry, false/*exclude_polygons_with_interior_rings*/);
				const std::pair<GPlatesMaths::PointOnSphere, GPlatesMaths::PointOnSphere> section_end_points =
						section_polyline
						? GeometryUtils::get_geometry_exterior_end_points(*section_polyline.get())
						: GeometryUtils::get_geometry_exterior_end_points(*section_geometry);

				// Create a source info that calculates velocity at the section start or end point.
				//
				// Note that we fix the velocity calculation such that it's always calculated *at* the section start/end point.
				// This way when the two source infos (for two adjacent sections) are interpolated to a point
				// midway between the ends of the two sections we will be interpolating velocities *at* the
				// section end points rather than interpolating velocities at the midway point (ie, that are
				// calculated *at* the midway point but using different section plate IDs).
				if (is_at_start_vertex)
				{
					const GPlatesMaths::PointOnSphere &start_vertex_point = section_end_points.first;

					return ResolvedVertexSourceInfo::create(section_source_info, start_vertex_point);
				}
				else
				{
					const GPlatesMaths::PointOnSphere &end_vertex_point = section_end_points.second;

					return ResolvedVertexSourceInfo::create(section_source_info, end_vertex_point);
				}
			}
			// else it's a resolved topological *line* ...

			boost::optional<ResolvedTopologicalLine::non_null_ptr_to_const_type> section_resolved_topological_line_opt =
					ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
							ResolvedTopologicalLine::non_null_ptr_to_const_type>(section_reconstruction_geometry);

			// Section reconstruction geometry must either be a ReconstructedFeatureGeometry or a ResolvedTopologicalLine.
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					section_resolved_topological_line_opt,
					GPLATES_ASSERTION_SOURCE);
			ResolvedTopologicalLine::non_null_ptr_to_const_type section_resolved_topological_line =
					section_resolved_topological_line_opt.get();

			const resolved_vertex_source_info_seq_type &section_vertex_source_infos =
					section_resolved_topological_line->get_vertex_source_infos();

			// Should have at least two vertices (since a resolved line is a polyline).
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					section_vertex_source_infos.size() >= 2,
					GPLATES_ASSERTION_SOURCE);

			// Create a source info that calculates velocity at the section start or end point.
			//
			// Note that we fix the velocity calculation such that it's always calculated *at* the section start/end point.
			// This way when the two source infos (for two adjacent sections) are interpolated to a point
			// midway between the ends of the two sections we will be interpolating velocities *at* the
			// section end points rather than interpolating velocities at the midway point (ie, that are
			// calculated *at* the midway point but using different section plate IDs).
			if (is_at_start_vertex)
			{
				ResolvedVertexSourceInfo::non_null_ptr_to_const_type start_vertex_source_info =
						section_vertex_source_infos.front();
				const GPlatesMaths::PointOnSphere &start_vertex_point =
						section_resolved_topological_line->resolved_topology_line()->start_point();

				return ResolvedVertexSourceInfo::create(start_vertex_source_info, start_vertex_point);
			}
			else
			{
				ResolvedVertexSourceInfo::non_null_ptr_to_const_type end_vertex_source_info =
						section_vertex_source_infos.back();
				const GPlatesMaths::PointOnSphere &end_vertex_point =
						section_resolved_topological_line->resolved_topology_line()->end_point();

				return ResolvedVertexSourceInfo::create(end_vertex_source_info, end_vertex_point);
			}
		}


		/**
		 * Get the vertex source info corresponding to the specified rubber band between section and
		 * either previous or next section.
		 *
		 * This will be an equal blend between one end of current section and one end of previous section.
		 */
		ResolvedVertexSourceInfo::non_null_ptr_to_const_type
		get_rubber_band_vertex_source_info(
				const ResolvedSubSegmentRangeInSection::RubberBand &rubber_band,
				const ReconstructionGeometry::non_null_ptr_to_const_type &section_reconstruction_geometry,
				const boost::optional<ReconstructionGeometry::non_null_ptr_to_const_type> &prev_section_reconstruction_geometry,
				const boost::optional<ReconstructionGeometry::non_null_ptr_to_const_type> &next_section_reconstruction_geometry)
		{
			// Get the source info at the rubber-band end of the current section.
			const ResolvedVertexSourceInfo::non_null_ptr_to_const_type section_vertex_source_info_at_end_point =
					get_rubber_band_vertex_source_info_at_section_end_point(
							section_reconstruction_geometry,
							rubber_band.is_at_start_of_current_section/*is_at_start_vertex*/);

			if (rubber_band.adjacent_is_previous_section)
			{
				// If there's a rubber band between the previous section and current section then
				// there should also be a previous section reconstruction geometry.
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						prev_section_reconstruction_geometry,
						GPLATES_ASSERTION_SOURCE);

				// Get the source info at the rubber-band end of the previous section.
				const ResolvedVertexSourceInfo::non_null_ptr_to_const_type prev_section_vertex_source_info_at_end_point =
						get_rubber_band_vertex_source_info_at_section_end_point(
								prev_section_reconstruction_geometry.get(),
								rubber_band.is_at_start_of_adjacent_section/*is_at_start_vertex*/);

				// Interpolate between the previous section and the current section.
				return ResolvedVertexSourceInfo::create(
						section_vertex_source_info_at_end_point,
						prev_section_vertex_source_info_at_end_point,
						0.5/*interpolate_ratio*/);
			}
			else // adjacent to *next* section ...
			{
				// If there's a rubber band between the next section and current section then
				// there should also be a next section reconstruction geometry.
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						next_section_reconstruction_geometry,
						GPLATES_ASSERTION_SOURCE);

				// Get the source info at the rubber-band end of the next section.
				const ResolvedVertexSourceInfo::non_null_ptr_to_const_type next_section_vertex_source_info_at_end_point =
						get_rubber_band_vertex_source_info_at_section_end_point(
								next_section_reconstruction_geometry.get(),
								rubber_band.is_at_start_of_adjacent_section/*is_at_start_vertex*/);

				// Interpolate between the current section and the next section.
				return ResolvedVertexSourceInfo::create(
						section_vertex_source_info_at_end_point,
						next_section_vertex_source_info_at_end_point,
						0.5/*interpolate_ratio*/);
			}
		}


		/**
		 * Add the source infos for those section vertices contributing to the sub-segment.
		 */
		void
		get_section_vertex_source_info_range(
				resolved_vertex_source_info_seq_type &vertex_source_infos,
				ReconstructionGeometry::non_null_ptr_to_const_type section_reconstruction_geometry,
				unsigned int start_vertex_index,
				unsigned int end_vertex_index)
		{
			// See if the section is a reconstructed feature geometry.
			boost::optional<ReconstructedFeatureGeometry::non_null_ptr_to_const_type> section_reconstructed_feature_geometry =
					ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
							ReconstructedFeatureGeometry::non_null_ptr_to_const_type>(section_reconstruction_geometry);
			if (section_reconstructed_feature_geometry)
			{
				// Share the same source reconstructed feature geometry across all (non-rubber-band) points in this sub-segment.
				const ResolvedVertexSourceInfo::non_null_ptr_to_const_type section_source_info =
						ResolvedVertexSourceInfo::create(section_reconstructed_feature_geometry.get());

				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						start_vertex_index <= end_vertex_index,
						GPLATES_ASSERTION_SOURCE);
				const unsigned int num_vertices_in_range = end_vertex_index - start_vertex_index;

				vertex_source_infos.insert(
						vertex_source_infos.end(),
						num_vertices_in_range,
						section_source_info);
			}
			else // it must be a resolved topological *line* ...
			{
				boost::optional<ResolvedTopologicalLine::non_null_ptr_to_const_type> section_resolved_topological_line_opt =
						ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
								ResolvedTopologicalLine::non_null_ptr_to_const_type>(section_reconstruction_geometry);

				// Section reconstruction geometry must either be a ReconstructedFeatureGeometry or a ResolvedTopologicalLine.
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						section_resolved_topological_line_opt,
						GPLATES_ASSERTION_SOURCE);
				ResolvedTopologicalLine::non_null_ptr_to_const_type section_resolved_topological_line =
						section_resolved_topological_line_opt.get();

				//
				// Determine which vertex sources in the unclipped resolved topological line correspond
				// to the (potentially) clipped sub-segment of the resolved topological line.
				//

				// Vertex sources of points in the unclipped section geometry.
				const resolved_vertex_source_info_seq_type &resolved_vertex_source_infos =
						section_resolved_topological_line->get_vertex_source_infos();

				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						// Can be equal since end index is actually *one-past-the-last* vertex to include...
						end_vertex_index <= resolved_vertex_source_infos.size(),
						GPLATES_ASSERTION_SOURCE);

				// Copy the vertex source infos between the intersections (if any).
				for (unsigned int vertex_index = start_vertex_index; vertex_index < end_vertex_index; ++vertex_index)
				{
					vertex_source_infos.push_back(resolved_vertex_source_infos[vertex_index]);
				}
			}
		}
	}
}


void
GPlatesAppLogic::ResolvedTopologicalSubSegmentImpl::get_sub_segment_vertex_source_infos(
		resolved_vertex_source_info_seq_type &vertex_source_infos,
		const ResolvedSubSegmentRangeInSection &sub_segment,
		ReconstructionGeometry::non_null_ptr_to_const_type section_reconstruction_geometry,
		boost::optional<ReconstructionGeometry::non_null_ptr_to_const_type> prev_section_reconstruction_geometry,
		boost::optional<ReconstructionGeometry::non_null_ptr_to_const_type> next_section_reconstruction_geometry)
{
	// Allocate some space (to avoid re-allocations when adding).
	vertex_source_infos.reserve(vertex_source_infos.size() + sub_segment.get_num_points());

	// Add the start intersection, if one.
	if (const boost::optional<ResolvedSubSegmentRangeInSection::Intersection> &start_intersection =
		sub_segment.get_start_intersection())
	{
		vertex_source_infos.push_back(
				get_intersection_vertex_source_info(
						start_intersection.get(),
						section_reconstruction_geometry));
	}
	// Else add the start rubber band, if one.
	else if (const boost::optional<ResolvedSubSegmentRangeInSection::RubberBand> &start_rubber_band =
			sub_segment.get_start_rubber_band())
	{
		vertex_source_infos.push_back(
				get_rubber_band_vertex_source_info(
						start_rubber_band.get(),
						section_reconstruction_geometry,
						prev_section_reconstruction_geometry,
						next_section_reconstruction_geometry));
	}
	// else no start intersection or start rubber band.

	// Add the source infos for those section vertices contributing to the sub-segment.
	// If there are start/end intersections then these are the vertices after/before those intersections.
	get_section_vertex_source_info_range(
			vertex_source_infos,
			section_reconstruction_geometry,
			sub_segment.get_start_section_vertex_index(),
			sub_segment.get_end_section_vertex_index());

	// Add the end intersection, if one.
	if (const boost::optional<ResolvedSubSegmentRangeInSection::Intersection> &end_intersection =
		sub_segment.get_end_intersection())
	{
		vertex_source_infos.push_back(
				get_intersection_vertex_source_info(
						end_intersection.get(),
						section_reconstruction_geometry));
	}
	// Else add the end rubber band, if one.
	else if (const boost::optional<ResolvedSubSegmentRangeInSection::RubberBand> &end_rubber_band =
			sub_segment.get_end_rubber_band())
	{
		vertex_source_infos.push_back(
				get_rubber_band_vertex_source_info(
						end_rubber_band.get(),
						section_reconstruction_geometry,
						prev_section_reconstruction_geometry,
						next_section_reconstruction_geometry));
	}
	// else no end intersection or end rubber band.
}
