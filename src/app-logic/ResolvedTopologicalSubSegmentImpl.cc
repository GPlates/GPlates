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
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type section_geometry,
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
					intersection.get_interpolate_ratio_in_segment(*section_geometry));
		}


		/**
		 * Returns the vertex source info at either the start (if @a is_at_start_vertex is true) or end of the section.
		 *
		 * The returned source info is to be used for rubber banding.
		 */
		ResolvedVertexSourceInfo::non_null_ptr_to_const_type
		get_rubber_band_vertex_source_info_at_section_end_point(
				const ReconstructionGeometry::non_null_ptr_to_const_type &section_reconstruction_geometry,
				const GPlatesMaths::PointOnSphere &section_end_point,
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

				// Create a source info that calculates velocity at the section start or end point.
				//
				// Note that we fix the velocity calculation such that it's always calculated *at* the section start/end point.
				// This way when the two source infos (for two adjacent sections) are interpolated to a point
				// midway between the ends of the two sections, we will be interpolating velocities *at* the
				// section end points rather than interpolating velocities at the midway point (ie, that are
				// calculated *at* the midway point but using different section plate IDs).
				return ResolvedVertexSourceInfo::create(section_source_info, section_end_point);
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
		 * Get the vertex source info corresponding to the specified rubber band between section and adjacent section.
		 *
		 * This will be an equal blend between one end of current section and one end of adjacent section.
		 */
		ResolvedVertexSourceInfo::non_null_ptr_to_const_type
		get_rubber_band_vertex_source_info(
				const ResolvedSubSegmentRangeInSection::RubberBand &rubber_band)
		{
			// Get the source info at the rubber-band end of the current section.
			const ResolvedVertexSourceInfo::non_null_ptr_to_const_type section_vertex_source_info_at_end_point =
					get_rubber_band_vertex_source_info_at_section_end_point(
							rubber_band.current_section_reconstruction_geometry,
							rubber_band.current_section_position,
							rubber_band.is_at_start_of_current_section/*is_at_start_vertex*/);

			// Get the source info at the rubber-band end of the adjacent section.
			const ResolvedVertexSourceInfo::non_null_ptr_to_const_type adjacent_section_vertex_source_info_at_end_point =
					get_rubber_band_vertex_source_info_at_section_end_point(
							rubber_band.adjacent_section_reconstruction_geometry,
							rubber_band.adjacent_section_position,
							rubber_band.is_at_start_of_adjacent_section/*is_at_start_vertex*/);

			// Interpolate between the adjacent section and the current section.
			return ResolvedVertexSourceInfo::create(
					section_vertex_source_info_at_end_point,
					adjacent_section_vertex_source_info_at_end_point,
					rubber_band.interpolate_ratio);
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


		/**
		 * Returns a new sub-sub-segment matching @a sub_sub_segment except for differing range (of section).
		 */
		ResolvedTopologicalGeometrySubSegment::non_null_ptr_type
		create_sub_sub_segment_with_new_range(
				const ResolvedTopologicalGeometrySubSegment &sub_sub_segment,
				boost::optional<ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand> start_of_sub_sub_segment,
				boost::optional<ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand> end_of_sub_sub_segment)
		{
			return ResolvedTopologicalGeometrySubSegment::create(
					ResolvedSubSegmentRangeInSection(
							sub_sub_segment.get_section_geometry(),
							start_of_sub_sub_segment,
							end_of_sub_sub_segment),
					sub_sub_segment.get_use_reverse(),
					sub_sub_segment.get_feature_ref(),
					sub_sub_segment.get_reconstruction_geometry());
		}


		/**
		 * Find the first/last unclipped sub-sub-segment containing the specified intersection and replace it with
		 * a clipped version of that sub-sub-segment that contributes to the sub-segment (ie, clipped resolved line).
		 *
		 * Returns index to clipped sub-sub-segment (if start of sub-segment) or one-past clipped sub-sub-segment
		 * (if end of sub-segment).
		 */
		sub_segment_seq_type::size_type
		replace_intersected_sub_sub_segment(
				const sub_segment_seq_type &unclipped_sub_sub_segments,
				sub_segment_seq_type &clipped_sub_sub_segments,		// Same size as 'unclipped_sub_sub_segments'
				const ResolvedSubSegmentRangeInSection &sub_segment_range,
				const ResolvedSubSegmentRangeInSection::Intersection &intersection,
				bool intersection_is_at_start_of_sub_segment)
		{
			// Find the sub-sub-segment containing the intersection.
			unsigned int sub_sub_segment_start_vertex_index = 0;
			unsigned int sub_sub_segment_end_vertex_index = 0;
			const sub_segment_seq_type::size_type end_sub_sub_segments_index = unclipped_sub_sub_segments.size();
			sub_segment_seq_type::size_type sub_sub_segments_index = 0;
			for ( ; sub_sub_segments_index != end_sub_sub_segments_index; ++sub_sub_segments_index)
			{
				const ResolvedTopologicalGeometrySubSegment::non_null_ptr_type &unclipped_sub_sub_segment =
						unclipped_sub_sub_segments[sub_sub_segments_index];

				sub_sub_segment_start_vertex_index = sub_sub_segment_end_vertex_index;
				sub_sub_segment_end_vertex_index += unclipped_sub_sub_segment->get_num_points_in_sub_segment();

				// Break loop if intersected GCA segment is prior to end of current sub-sub-segment.
				if (intersection.segment_index < sub_sub_segment_end_vertex_index - 1)
				{
					break;
				}
			}

			if (sub_sub_segments_index == end_sub_sub_segments_index)
			{
				// Intersection must be *on* the start of the fictitious *one-past-the-last* GCA segment
				// which means on the last *vertex* of the last sub-sub-segment.
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						intersection.on_segment_start &&
							intersection.segment_index == sub_sub_segment_end_vertex_index - 1,
						GPLATES_ASSERTION_SOURCE);

				// The intersection did not split an existing sub-sub-segment, so normally we would not
				// need to create a new sub-sub-segment, and hence just return 'end_sub_sub_segments_index'.
				//
				// However it's possible to have the sub-segment (of resolved line) start at start
				// rubber band of sub-segment and end at the first vertex of resolved line, or start
				// at last vertex of resolved line and end at end rubber band of sub-segment
				// (in this code block/scope we are concerned with the latter, ie, starting at last vertex).
				//
				// But in these cases the first and last sub-sub-segments (of resolved line) do not have
				// rubber bands corresponding to the start and end of resolved line (because the
				// sub-sub-segments of a resolved line only have rubber banding *between* them).
				//
				// In this case we should not exclude the first or last sub-sub-segment.
				// Instead, in this code block, we decrement the sub-sub-segment index to refer to the
				// last sub-sub-segment. Then further below (in this function), the last vertex of the (reversed)
				// last sub-sub-segment will get changed to 'intersection' - and then later on (not in this function)
				// the end rubber band of that sub-sub-segment will get set to the end rubber band of the
				// resolved line sub-segment, thus completing the picture of the resolved line
				// sub-segment starting at last vertex of resolved line and ending at the end rubber band.
				//
				// This is probably the most subtle point of this function.
				//
				--sub_sub_segments_index;
			}
			// Else if the intersection is at the start of an existing sub-sub-segment then it does not
			// split the sub-sub-segment...
			else if (intersection.segment_index == sub_sub_segment_start_vertex_index &&
				intersection.on_segment_start)
			{
				if (sub_sub_segments_index != 0)
				{
					// Sub-sub-segment does not need to be split.
					//
					// So return index to first sub-sub-segment (if intersection on *start* of resolved line sub-segment)
					// or one-past-last sub-sub-segment (if intersection on *end*). Same index applies in both situations.
					return sub_sub_segments_index;
				}
				// Else the intersection is *on* the first vertex of the resolved line (because
				// 'sub_sub_segments_index == 0 && sub_sub_segment_start_vertex_index == 0 && intersection.on_segment_start').
				// In this case, which is similar to the case above where the intersection is *on* the last vertex
				// of resolved line, it's possible to have the sub-segment (of resolved line) start at start rubber band
				// of sub-segment and end at the first vertex of resolved line.
				// In this case we should not exclude the first sub-sub-segment (for similar reasons noted above).
				// So we don't return early. Note that, at the end of this function, if 'intersection' is an *end*
				// intersection then 'sub_sub_segments_index + 1' is returned instead of 'sub_sub_segments_index'
				// which means the sub-sub-segment (at index 'sub_sub_segments_index') is not excluded whereas it
				// would have been if we had returned 'sub_sub_segments_index' right here.
			}
			// Else if intersection lies on segment joining end of last sub-sub-segment with start of current sub-sub-segment
			// then that segment is a zero-length GCA segment that is essentially between the two adjacent sub-sub-segments...
			else if (intersection.segment_index == sub_sub_segment_start_vertex_index - 1)
			{
				// It's a zero-length GCA segment, so intersection must be on start (and end) of GCA segment.
				//
				// NOTE: We won't actually assert this since the numerical tolerance in the intersection code might
				// be such that an intersection could slip *between* the start and end of GCA segment since a
				// zero-length GCA segment itself is only required to be zero length within a numerical tolerance.
				// Currently the numerical tolerance in the intersection code is designed to prevent this
				// but that could change in future.
				//
				//GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				//		intersection.on_segment_start,
				//		GPLATES_ASSERTION_SOURCE);
				
				// Return index to first sub-sub-segment (if intersection on *start* of resolved line sub-segment)
				// or one-past-last sub-sub-segment (if intersection on *end*). Same index applies in both situations.
				return sub_sub_segments_index;
			}

			// Use the *unclipped* sub-sub-segment range when transferring 'intersection' from
			// resolved line to the clipped sub-sub-segment since 'intersection' is relative to a
			// GCA segment within the *unclipped* sub-sub-segment.
			const ResolvedSubSegmentRangeInSection &unclipped_sub_sub_segment_range =
					unclipped_sub_sub_segments[sub_sub_segments_index]->get_sub_segment();

			// Use the *clipped* sub-sub-segment range for everything else since it might have been modified already
			// (eg, by start intersection/rubber-band of resolved line, if we're processing end intersection).
			const ResolvedTopologicalGeometrySubSegment &clipped_sub_sub_segment =
					*clipped_sub_sub_segments[sub_sub_segments_index];
			const ResolvedSubSegmentRangeInSection &clipped_sub_sub_segment_range =
					clipped_sub_sub_segment.get_sub_segment();

			// For these parameters it doesn't matter whether query unclipped or clipped sub-sub-segment.
			const bool sub_sub_segment_reversed = clipped_sub_sub_segment.get_use_reverse();

			// Which side of the sub-sub-segment to retain depends on sub-sub-segment reversal and
			// whether sub-sub-segment is at start or end of sub-segment.
			boost::optional<ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand> start_of_clipped_sub_sub_segment;
			boost::optional<ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand> end_of_clipped_sub_sub_segment;
			boost::optional<ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand> *new_start_or_end_of_clipped_sub_sub_segment;
			if (sub_sub_segment_reversed ^ intersection_is_at_start_of_sub_segment)
			{
				// Later we'll change the start to match the intersection with resolved line (containing the sub-sub-segments).
				// However the end remains unchanged.
				new_start_or_end_of_clipped_sub_sub_segment = &start_of_clipped_sub_sub_segment;
				end_of_clipped_sub_sub_segment = clipped_sub_sub_segment_range.get_end_intersection_or_rubber_band();
			}
			else
			{
				// Later we'll change the end to match the intersection with resolved line (containing the sub-sub-segments).
				// However the start remains unchanged.
				start_of_clipped_sub_sub_segment = clipped_sub_sub_segment_range.get_start_intersection_or_rubber_band();
				new_start_or_end_of_clipped_sub_sub_segment = &end_of_clipped_sub_sub_segment;
			}

			// See if intersection is in start rubber band GCA segment (joining rubber band point and start of section geometry).
			if (const boost::optional<ResolvedSubSegmentRangeInSection::RubberBand> &
				unclipped_sub_sub_segment_start_rubber_band = unclipped_sub_sub_segment_range.get_start_rubber_band())
			{
				if (sub_sub_segment_reversed)
				{
					if (intersection.segment_index == sub_sub_segment_end_vertex_index - 2)
					{
						// Create the rubber-band position in the sub-sub-segment. This essentially transfers the intersection
						// from the resolved line sub-segment to a new rubber-band of one of its sub-sub-segments.
						*new_start_or_end_of_clipped_sub_sub_segment = ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(
								ResolvedSubSegmentRangeInSection::RubberBand::create_from_intersected_rubber_band(
										unclipped_sub_sub_segment_start_rubber_band.get(),
										intersection.position));
					}
				}
				else
				{
					if (intersection.segment_index == sub_sub_segment_start_vertex_index)
					{
						// Create the rubber-band position in the sub-sub-segment. This essentially transfers the intersection
						// from the resolved line sub-segment to a new rubber-band of one of its sub-sub-segments.
						*new_start_or_end_of_clipped_sub_sub_segment = ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(
								ResolvedSubSegmentRangeInSection::RubberBand::create_from_intersected_rubber_band(
										unclipped_sub_sub_segment_start_rubber_band.get(),
										intersection.position));
					}
				}
			}

			// See if intersection is in end rubber band GCA segment (joining end of section geometry and rubber band point).
			if (const boost::optional<ResolvedSubSegmentRangeInSection::RubberBand> &
				unclipped_sub_sub_segment_end_rubber_band = unclipped_sub_sub_segment_range.get_end_rubber_band())
			{
				if (sub_sub_segment_reversed)
				{
					if (intersection.segment_index == sub_sub_segment_start_vertex_index)
					{
						// Create the rubber-band position in the sub-sub-segment. This essentially transfers the intersection
						// from the resolved line sub-segment to a new rubber-band of one of its sub-sub-segments.
						*new_start_or_end_of_clipped_sub_sub_segment = ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(
								ResolvedSubSegmentRangeInSection::RubberBand::create_from_intersected_rubber_band(
										unclipped_sub_sub_segment_end_rubber_band.get(),
										intersection.position));
					}
				}
				else
				{
					if (intersection.segment_index == sub_sub_segment_end_vertex_index - 2)
					{
						// Create the rubber-band position in the sub-sub-segment. This essentially transfers the intersection
						// from the resolved line sub-segment to a new rubber-band of one of its sub-sub-segments.
						*new_start_or_end_of_clipped_sub_sub_segment = ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(
								ResolvedSubSegmentRangeInSection::RubberBand::create_from_intersected_rubber_band(
										unclipped_sub_sub_segment_end_rubber_band.get(),
										intersection.position));
					}
				}
			}

			// If we haven't intersected the start/end rubber band of a sub-sub-segment (if any) then
			// we must be intersecting the actual section geometry of the sub-sub-segment.
			if (!*new_start_or_end_of_clipped_sub_sub_segment)
			{
				const boost::optional<ResolvedSubSegmentRangeInSection::Intersection> &
						unclipped_sub_sub_segment_start_intersection = unclipped_sub_sub_segment_range.get_start_intersection();
				const boost::optional<ResolvedSubSegmentRangeInSection::Intersection> &
						unclipped_sub_sub_segment_end_intersection = unclipped_sub_sub_segment_range.get_end_intersection();

				unsigned int segment_index_in_sub_sub_segment_geometry;
				double start_inner_segment_interpolate_ratio_in_segment;
				double end_inner_segment_interpolate_ratio_in_segment;

				if (sub_sub_segment_reversed)
				{
					start_inner_segment_interpolate_ratio_in_segment = 1.0;
					end_inner_segment_interpolate_ratio_in_segment = 0.0;

					// The index of the GCA segment within the sub-sub-segment.
					// Note: This is not the segment index within the *unclipped* section geometry (of the sub-sub-segment).
					segment_index_in_sub_sub_segment_geometry =
							// -1 to convert num vertices to num segments, and -1 to convert num segments to segment index...
							sub_sub_segment_end_vertex_index - 2 - intersection.segment_index;
				}
				else // not reversed ...
				{
					start_inner_segment_interpolate_ratio_in_segment = 0.0;
					end_inner_segment_interpolate_ratio_in_segment = 1.0;

					// The index of the GCA segment within the sub-sub-segment.
					// Note: This is not the segment index within the *unclipped* section geometry (of the sub-sub-segment).
					segment_index_in_sub_sub_segment_geometry = intersection.segment_index - sub_sub_segment_start_vertex_index;
				}

				// If has a start rubber band then first GCA segment belongs to that (not the actual section geometry).
				if (unclipped_sub_sub_segment_range.get_start_rubber_band())
				{
					// Note that we've already checked above that intersection is not within the start rubber band GCA segment.
					// If it was we wouldn't be able to get here due to 'if (!*new_start_or_end_of_clipped_sub_sub_segment)' test.
					// So this decrement shouldn't result in a negative index.
					--segment_index_in_sub_sub_segment_geometry;
				}

				// The index of the GCA segment within the section geometry.
				unsigned int segment_index_in_sub_sub_segment_section_geometry = segment_index_in_sub_sub_segment_geometry;

				// If intersection is within the first GCA segment of sub-sub-segment's section geometry then
				// this will affect the start or end interpolation ratio (ie, it won't just be 0.0 or 1.0).
				if (unclipped_sub_sub_segment_start_intersection)
				{
					segment_index_in_sub_sub_segment_section_geometry += unclipped_sub_sub_segment_start_intersection->segment_index;

					if (segment_index_in_sub_sub_segment_geometry == 0)
					{
						if (sub_sub_segment_reversed)
						{
							end_inner_segment_interpolate_ratio_in_segment =
									unclipped_sub_sub_segment_start_intersection->get_interpolate_ratio_in_segment(
											*unclipped_sub_sub_segment_range.get_section_geometry());
						}
						else
						{
							start_inner_segment_interpolate_ratio_in_segment =
									unclipped_sub_sub_segment_start_intersection->get_interpolate_ratio_in_segment(
											*unclipped_sub_sub_segment_range.get_section_geometry());
						}
					}
				}

				// If intersection is within the last GCA segment of sub-sub-segment's section geometry then
				// this will affect the start or end interpolation ratio (ie, it won't just be 0.0 or 1.0).
				if (unclipped_sub_sub_segment_end_intersection)
				{
					if (segment_index_in_sub_sub_segment_geometry ==
						unclipped_sub_sub_segment_range.get_num_points_in_section_geometry() - 2)
					{
						if (sub_sub_segment_reversed)
						{
							start_inner_segment_interpolate_ratio_in_segment =
									unclipped_sub_sub_segment_start_intersection->get_interpolate_ratio_in_segment(
											*unclipped_sub_sub_segment_range.get_section_geometry());
						}
						else
						{
							end_inner_segment_interpolate_ratio_in_segment =
									unclipped_sub_sub_segment_start_intersection->get_interpolate_ratio_in_segment(
											*unclipped_sub_sub_segment_range.get_section_geometry());
						}
					}
				}

				// Create the intersection in the sub-sub-segment.
				// This essentially transfers the intersection from the resolved line sub-segment to
				// one of its sub-sub-segments.
				*new_start_or_end_of_clipped_sub_sub_segment = ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(
						ResolvedSubSegmentRangeInSection::Intersection::create_from_inner_segment(
								intersection.position,
								*clipped_sub_sub_segment_range.get_section_geometry(),
								segment_index_in_sub_sub_segment_section_geometry,
								start_inner_segment_interpolate_ratio_in_segment,
								end_inner_segment_interpolate_ratio_in_segment,
								intersection.get_interpolate_ratio_in_segment(
										*sub_segment_range.get_section_geometry())));
			}

			// Replace the clipped sub-sub-segment with a new range (of the section of sub-sub-segment).
			clipped_sub_sub_segments[sub_sub_segments_index] = create_sub_sub_segment_with_new_range(
					clipped_sub_sub_segment,
					start_of_clipped_sub_sub_segment,
					end_of_clipped_sub_sub_segment);

			return intersection_is_at_start_of_sub_segment
					? sub_sub_segments_index
					// End of resolved line sub-segment returns one-past-the-last sub-sub-segment...
					: sub_sub_segments_index + 1;
		}


		/**
		 * The first (or last) sub-sub-segment essentially gets replaced by a rubber-band version of that sub-sub-segment.
		 *
		 * The resolved line (associated with the sub-segment) does not have a rubber band on its first and last
		 * sub-sub-segments (because it's a line not a polygon) so we ignore the first (or last) sub-sub-segments and
		 * create a new version of that with the specified rubber band (that comes from using the resolved line, in turn,
		 * as a topological section in a boundary topological polygon).
		 */
		void
		replace_rubber_banded_sub_sub_segment(
				sub_segment_seq_type &sub_sub_segments,
				const ResolvedSubSegmentRangeInSection::RubberBand &sub_segment_rubber_band,
				bool rubber_band_is_at_start_of_sub_segment)
		{
			// One end of the sub-sub-segment retains its own intersection/rubber-band while the other end
			// (ie, representing the end of the parent resolved line sub-segment) uses the
			// intersection/rubber-band from the parent sub-segment.
			// 
			// Note that we use the rubber-band of the parent (sub-segment) even though it references the parent's
			// reconstruction geometry. There's no real need to create a rubber-band that references the
			// sub-sub-segment reconstruction geometry, and it's difficult to obtain one for the adjacent
			// sub-sub-segment, if there is one - it could just be a sub-segment (ie, have not sub-sub-segments)
			// if it's not a resolved line (ie, is a reconstructed feature geometry).
			boost::optional<ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand> start_of_sub_sub_segment;
			boost::optional<ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand> end_of_sub_sub_segment;

			// Sub-sub-segment is either first or last one.
			const sub_segment_seq_type::size_type sub_sub_segment_index =
					rubber_band_is_at_start_of_sub_segment ? 0 : sub_sub_segments.size() - 1;
			const ResolvedTopologicalGeometrySubSegment &sub_sub_segment = *sub_sub_segments[sub_sub_segment_index];

			// Which side of the sub-sub-segment to retain depends on sub-sub-segment reversal and
			// whether sub-sub-segment is at start or end of sub-segment.
			if (sub_sub_segment.get_use_reverse() ^ rubber_band_is_at_start_of_sub_segment)
			{
				start_of_sub_sub_segment = ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(sub_segment_rubber_band);
				end_of_sub_sub_segment = sub_sub_segment.get_sub_segment().get_end_intersection_or_rubber_band();
			}
			else
			{
				start_of_sub_sub_segment = sub_sub_segment.get_sub_segment().get_start_intersection_or_rubber_band();
				end_of_sub_sub_segment = ResolvedSubSegmentRangeInSection::IntersectionOrRubberBand(sub_segment_rubber_band);
			}

			// Replace the start or end sub-sub-segment with the new rubber-banded version.
			sub_sub_segments[sub_sub_segment_index] =
					create_sub_sub_segment_with_new_range(
							sub_sub_segment,
							start_of_sub_sub_segment,
							end_of_sub_sub_segment);
		}
	}
}


void
GPlatesAppLogic::ResolvedTopologicalSubSegmentImpl::get_sub_segment_vertex_source_infos(
		resolved_vertex_source_info_seq_type &vertex_source_infos,
		const ResolvedSubSegmentRangeInSection &sub_segment_range,
		ReconstructionGeometry::non_null_ptr_to_const_type section_reconstruction_geometry)
{
	// Allocate some space (to avoid re-allocations when adding).
	vertex_source_infos.reserve(vertex_source_infos.size() + sub_segment_range.get_num_points());

	// Add the start intersection, if one.
	if (const boost::optional<ResolvedSubSegmentRangeInSection::Intersection> &start_intersection =
		sub_segment_range.get_start_intersection())
	{
		vertex_source_infos.push_back(
				get_intersection_vertex_source_info(
						start_intersection.get(),
						sub_segment_range.get_section_geometry(),
						section_reconstruction_geometry));
	}
	// Else add the start rubber band, if one.
	else if (const boost::optional<ResolvedSubSegmentRangeInSection::RubberBand> &start_rubber_band =
			sub_segment_range.get_start_rubber_band())
	{
		vertex_source_infos.push_back(
				get_rubber_band_vertex_source_info(start_rubber_band.get()));
	}
	// else no start intersection or start rubber band.

	// Add the source infos for those section vertices contributing to the sub-segment.
	// If there are start/end intersections then these are the vertices after/before those intersections.
	get_section_vertex_source_info_range(
			vertex_source_infos,
			section_reconstruction_geometry,
			sub_segment_range.get_start_section_vertex_index(),
			sub_segment_range.get_end_section_vertex_index());

	// Add the end intersection, if one.
	if (const boost::optional<ResolvedSubSegmentRangeInSection::Intersection> &end_intersection =
		sub_segment_range.get_end_intersection())
	{
		vertex_source_infos.push_back(
				get_intersection_vertex_source_info(
						end_intersection.get(),
						sub_segment_range.get_section_geometry(),
						section_reconstruction_geometry));
	}
	// Else add the end rubber band, if one.
	else if (const boost::optional<ResolvedSubSegmentRangeInSection::RubberBand> &end_rubber_band =
			sub_segment_range.get_end_rubber_band())
	{
		vertex_source_infos.push_back(
				get_rubber_band_vertex_source_info(end_rubber_band.get()));
	}
	// else no end intersection or end rubber band.
}


void
GPlatesAppLogic::ResolvedTopologicalSubSegmentImpl::get_sub_sub_segments(
		boost::optional<sub_segment_seq_type> &sub_sub_segments,
		const ResolvedSubSegmentRangeInSection &sub_segment_range,
		ReconstructionGeometry::non_null_ptr_to_const_type section_reconstruction_geometry)
{
	// Section reconstruction geometry must either be a ReconstructedFeatureGeometry or a ResolvedTopologicalLine.
	//
	// If it's a ReconstructedFeatureGeometry then it has no sub-segments, so return early (and set 'sub_sub_segments' to none).
	// Otherwise it's a ResolvedTopologicalLine with sub-segments.
	boost::optional<ResolvedTopologicalLine::non_null_ptr_to_const_type> section_resolved_topological_line_opt =
			ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
					ResolvedTopologicalLine::non_null_ptr_to_const_type>(section_reconstruction_geometry);
	if (!section_resolved_topological_line_opt)
	{
		sub_sub_segments = boost::none;
		return;
	}
	ResolvedTopologicalLine::non_null_ptr_to_const_type section_resolved_topological_line =
			section_resolved_topological_line_opt.get();

	const sub_segment_seq_type &unclipped_sub_sub_segments = section_resolved_topological_line->get_sub_segment_sequence();

	//
	// Determine which sub-sub-segments in the (unclipped) resolved topological line correspond
	// to the (potentially) clipped sub-segment of the resolved topological line.
	//

	// A copy of the (unclipped) sub-sub-segments that we might modify.
	// For example, replacing an (unclipped) sub-sub-segment at resolved line start/end intersection with a clipped version.
	//
	// The main reason for modifying a copy is it's possible that the same (unclipped) sub-sub-segment will
	// contain the start and end intersections and so we want the second (end) intersection to modify the
	// already modified (clipped) sub-sub-segment resulting from the first (start) intersection.
	// Similar reasoning applies to start and end rubber-bands when there's only one sub-sub-segment in the resolved
	// line and hence will contain both start and end rubber bands (or to a mixture of intersection and rubber-band).
	sub_segment_seq_type clipped_sub_sub_segments(unclipped_sub_sub_segments);

	// By default we start with the entire range of sub-sub-segments and reduce range if there's a start/end intersection.
	sub_segment_seq_type::size_type begin_sub_sub_segment_index = 0;
	sub_segment_seq_type::size_type end_sub_sub_segment_index = clipped_sub_sub_segments.size();

	// Get the sub-sub-segment (of resolved line) containing the *start* intersection, if one.
	if (const boost::optional<ResolvedSubSegmentRangeInSection::Intersection> &start_intersection =
		sub_segment_range.get_start_intersection())
	{
		begin_sub_sub_segment_index = replace_intersected_sub_sub_segment(
				unclipped_sub_sub_segments,
				clipped_sub_sub_segments,
				sub_segment_range,
				start_intersection.get(),
				true/*intersection_is_at_start_of_sub_segment*/);
	}
	// Else add the start rubber band, if one.
	else if (const boost::optional<ResolvedSubSegmentRangeInSection::RubberBand> &start_rubber_band =
			sub_segment_range.get_start_rubber_band())
	{
		replace_rubber_banded_sub_sub_segment(
				clipped_sub_sub_segments,
				start_rubber_band.get(),
				true/*rubber_band_is_at_start_of_sub_segment*/);
	}
	// else no start intersection or start rubber band.

	// Get the sub-sub-segment (of resolved line) containing the *end* intersection, if one.
	if (const boost::optional<ResolvedSubSegmentRangeInSection::Intersection> &end_intersection =
		sub_segment_range.get_end_intersection())
	{
		end_sub_sub_segment_index = replace_intersected_sub_sub_segment(
				unclipped_sub_sub_segments,
				clipped_sub_sub_segments,
				sub_segment_range,
				end_intersection.get(),
				false/*intersection_is_at_start_of_sub_segment*/);
	}
	// Else add the end rubber band, if one.
	else if (const boost::optional<ResolvedSubSegmentRangeInSection::RubberBand> &end_rubber_band =
			sub_segment_range.get_end_rubber_band())
	{
		replace_rubber_banded_sub_sub_segment(
				// Use the *clipped* sub-sub-segments since might have already been modified
				// (eg, by start intersection/rubber-band of resolved line, handled above)...
				clipped_sub_sub_segments,
				end_rubber_band.get(),
				false/*rubber_band_is_at_start_of_sub_segment*/);
	}
	// else no end intersection or end rubber band.

	//
	// Copy the clipped range of sub-sub-segments associated with the sub-segment of the resolved line.
	//

	// Start with an empty sequence of sub-sub-segments.
	sub_sub_segments = sub_segment_seq_type();

	for (sub_segment_seq_type::size_type sub_sub_segments_index = begin_sub_sub_segment_index;
		sub_sub_segments_index != end_sub_sub_segment_index;
		++sub_sub_segments_index)
	{
		sub_sub_segments->push_back(
				clipped_sub_sub_segments[sub_sub_segments_index]);
	}
}
