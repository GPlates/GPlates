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

#include "ResolvedTopologicalLine.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


void
GPlatesAppLogic::ResolvedTopologicalLine::resolved_topology_geometry_points(
		std::vector<GPlatesMaths::PointOnSphere> &resolved_topology_geometry_points_) const
{
	const resolved_topology_line_ptr_type resolved_topology_line_ = resolved_topology_line();

	// Add the vertices of the resolved topological line to the end of the caller's array.
	resolved_topology_geometry_points_.insert(
			resolved_topology_geometry_points_.end(),
			resolved_topology_line_->vertex_begin(),
			resolved_topology_line_->vertex_end());
}


void
GPlatesAppLogic::ResolvedTopologicalLine::resolved_topology_geometry_point_velocities(
		std::vector<GPlatesMaths::Vector3D> &resolved_topology_geometry_point_velocities_,
		const double &velocity_delta_time,
		VelocityDeltaTime::Type velocity_delta_time_type,
		VelocityUnits::Value velocity_units,
		const double &earth_radius_in_kms) const
{
	const resolved_topology_line_ptr_type resolved_topology_line_ = resolved_topology_line();

	// Get the resolved source infos (one per point in the resolved line).
	const resolved_vertex_source_info_seq_type &resolved_source_infos = get_resolved_topology_geometry_point_source_infos();

	// Number of resolved source infos should match number of points in the resolved line.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			resolved_source_infos.size() == resolved_topology_line_->number_of_vertices(),
			GPLATES_ASSERTION_SOURCE);

	// Iterate over the vertex positions and source infos and calculate velocities.
	auto line_points_iter = resolved_topology_line_->vertex_begin();
	auto line_points_end = resolved_topology_line_->vertex_end();
	auto resolved_source_infos_iter = resolved_source_infos.begin();
	for ( ; line_points_iter != line_points_end; ++line_points_iter, ++resolved_source_infos_iter)
	{
		const GPlatesMaths::PointOnSphere &point = *line_points_iter;
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
GPlatesAppLogic::ResolvedTopologicalLine::get_resolved_topology_geometry_point_source_infos() const
{
	// Cache all vertex source infos on first call.
	if (!d_vertex_source_infos)
	{
		calc_vertex_source_infos();
	}

	return d_vertex_source_infos.get();
}


const std::vector<GPlatesModel::FeatureHandle::weak_ref> &
GPlatesAppLogic::ResolvedTopologicalLine::get_resolved_topology_geometry_point_source_features() const
{
	// Cache all vertex source features on first call.
	if (!d_vertex_source_features)
	{
		calc_vertex_source_features();
	}

	return d_vertex_source_features.get();
}


void
GPlatesAppLogic::ResolvedTopologicalLine::calc_vertex_source_infos() const
{
	d_vertex_source_infos = resolved_vertex_source_info_seq_type();
	resolved_vertex_source_info_seq_type &vertex_source_infos = d_vertex_source_infos.get();

	// Copy source infos from points in each subsegment.
	for (const auto &sub_segment : d_sub_segment_seq)
	{
		// Subsegment should be reversed if that's how it contributed to this resolved topological line...
		sub_segment->get_reversed_sub_segment_point_source_infos(
				vertex_source_infos,
				INCLUDE_SUB_SEGMENT_RUBBER_BAND_POINTS_IN_RESOLVED_LINE/*include_rubber_band_points*/);
	}
}


void
GPlatesAppLogic::ResolvedTopologicalLine::calc_vertex_source_features() const
{
	d_vertex_source_features = std::vector<GPlatesModel::FeatureHandle::weak_ref>();
	std::vector<GPlatesModel::FeatureHandle::weak_ref> &vertex_source_features = d_vertex_source_features.get();

	// Copy source features from points in each subsegment.
	for (const auto &sub_segment : d_sub_segment_seq)
	{
		// Subsegment should be reversed if that's how it contributed to this resolved topological line...
		sub_segment->get_reversed_sub_segment_point_source_features(
				vertex_source_features,
				INCLUDE_SUB_SEGMENT_RUBBER_BAND_POINTS_IN_RESOLVED_LINE/*include_rubber_band_points*/);
	}
}
