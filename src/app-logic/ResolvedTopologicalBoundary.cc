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

#include "ResolvedTopologicalBoundary.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


void
GPlatesAppLogic::ResolvedTopologicalBoundary::resolved_topology_geometry_points(
		std::vector<GPlatesMaths::PointOnSphere> &resolved_topology_geometry_points_) const
{
	const resolved_topology_boundary_ptr_type resolved_topology_boundary_ = resolved_topology_boundary();

	// Add the vertices of the resolved topological boundary to the end of the caller's array.
	//
	// Note: The boundary polygon will not have any interior holes unlike a resolved topological *network*
	//       which can have interior rigid blocks.
	resolved_topology_geometry_points_.insert(
			resolved_topology_geometry_points_.end(),
			resolved_topology_boundary_->vertex_begin(),
			resolved_topology_boundary_->vertex_end());
}


void
GPlatesAppLogic::ResolvedTopologicalBoundary::resolved_topology_geometry_point_velocities(
		std::vector<GPlatesMaths::Vector3D> &resolved_topology_geometry_point_velocities_,
		const double &velocity_delta_time,
		VelocityDeltaTime::Type velocity_delta_time_type,
		VelocityUnits::Value velocity_units,
		const double &earth_radius_in_kms) const
{
	const resolved_topology_boundary_ptr_type resolved_topology_boundary_ = resolved_topology_boundary();

	// Get the resolved source infos (one per point in the resolved boundary).
	const resolved_vertex_source_info_seq_type &resolved_source_infos = get_vertex_source_infos();

	// Number of resolved source infos should match number of points in the resolved boundary.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			resolved_source_infos.size() == resolved_topology_boundary_->number_of_vertices(),
			GPLATES_ASSERTION_SOURCE);

	// Iterate over the vertex positions and source infos and calculate velocities.
	auto boundary_points_iter = resolved_topology_boundary_->vertex_begin();
	auto boundary_points_end = resolved_topology_boundary_->vertex_end();
	auto resolved_source_infos_iter = resolved_source_infos.begin();
	for ( ; boundary_points_iter != boundary_points_end; ++boundary_points_iter, ++resolved_source_infos_iter)
	{
		const GPlatesMaths::PointOnSphere &point = *boundary_points_iter;
		const auto resolved_source_info = *resolved_source_infos_iter;

		resolved_topology_geometry_point_velocities_.push_back(
				resolved_source_info->get_velocity_vector(
						point,
						get_reconstruction_time(),
						velocity_delta_time,
						velocity_delta_time_type,
						velocity_units,
						earth_radius_in_kms));
	}
}


const GPlatesAppLogic::resolved_vertex_source_info_seq_type &
GPlatesAppLogic::ResolvedTopologicalBoundary::get_vertex_source_infos() const
{
	// Cache all vertex source infos on first call.
	if (!d_vertex_source_infos)
	{
		calc_vertex_source_infos();
	}

	return d_vertex_source_infos.get();
}


void
GPlatesAppLogic::ResolvedTopologicalBoundary::calc_vertex_source_infos() const
{
	d_vertex_source_infos = resolved_vertex_source_info_seq_type();
	resolved_vertex_source_info_seq_type &vertex_source_infos = d_vertex_source_infos.get();

	// Copy source infos from points in each subsegment.
	sub_segment_seq_type::const_iterator sub_segments_iter = d_sub_segment_seq.begin();
	sub_segment_seq_type::const_iterator sub_segments_end = d_sub_segment_seq.end();
	for ( ; sub_segments_iter != sub_segments_end; ++sub_segments_iter)
	{
		const ResolvedTopologicalGeometrySubSegment::non_null_ptr_type &sub_segment = *sub_segments_iter;
		// Subsegment should be reversed if that's how it contributed to this resolved topological boundary...
		sub_segment->get_reversed_sub_segment_point_source_infos(
				vertex_source_infos,
				INCLUDE_SUB_SEGMENT_RUBBER_BAND_POINTS_IN_RESOLVED_BOUNDARY/*include_rubber_band_points*/);
	}
}
