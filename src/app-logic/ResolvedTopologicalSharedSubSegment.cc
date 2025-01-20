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

#include "ResolvedTopologicalSharedSubSegment.h"

#include "TopologyInternalUtils.h"
#include "ResolvedTopologicalSubSegmentImpl.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


void
GPlatesAppLogic::ResolvedTopologicalSharedSubSegment::get_shared_sub_segment_point_velocities(
		std::vector<GPlatesMaths::Vector3D> &geometry_point_velocities,
		bool include_rubber_band_points,
		const double &velocity_delta_time,
		VelocityDeltaTime::Type velocity_delta_time_type,
		VelocityUnits::Value velocity_units,
		const double &earth_radius_in_kms) const
{
	// Get the points in the shared sub-segment.
	std::vector<GPlatesMaths::PointOnSphere> geometry_points;
	get_shared_sub_segment_points(geometry_points, include_rubber_band_points);

	// Get the resolved source infos (one per point in the shared sub-segment).
	resolved_vertex_source_info_seq_type geometry_point_source_infos;
	get_shared_sub_segment_point_source_infos(geometry_point_source_infos, include_rubber_band_points);

	// Number of resolved source infos should match number of points in the shared sub-segment.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			geometry_point_source_infos.size() == geometry_points.size(),
			GPLATES_ASSERTION_SOURCE);

	// Iterate over the vertex positions and source infos and calculate velocities.
	auto geometry_points_iter = geometry_points.begin();
	auto geometry_points_end = geometry_points.end();
	auto geometry_point_source_infos_iter = geometry_point_source_infos.begin();
	for ( ; geometry_points_iter != geometry_points_end; ++geometry_points_iter, ++geometry_point_source_infos_iter)
	{
		const GPlatesMaths::PointOnSphere &geometry_point = *geometry_points_iter;
		const auto geometry_point_source_info = *geometry_point_source_infos_iter;

		geometry_point_velocities.push_back(
				geometry_point_source_info->get_velocity_vector(
						geometry_point,
						d_shared_segment_reconstruction_geometry->get_reconstruction_time(),
						velocity_delta_time,
						velocity_delta_time_type,
						velocity_units,
						earth_radius_in_kms));
	}
}


void
GPlatesAppLogic::ResolvedTopologicalSharedSubSegment::get_reversed_shared_sub_segment_point_velocities(
		std::vector<GPlatesMaths::Vector3D> &geometry_point_velocities,
		bool use_reverse,
		bool include_rubber_band_points,
		const double &velocity_delta_time,
		VelocityDeltaTime::Type velocity_delta_time_type,
		VelocityUnits::Value velocity_units,
		const double &earth_radius_in_kms) const
{
	// Get the points in the shared sub-segment.
	std::vector<GPlatesMaths::PointOnSphere> geometry_points;
	get_reversed_shared_sub_segment_points(geometry_points, include_rubber_band_points);

	// Get the resolved source infos (one per point in the shared sub-segment).
	resolved_vertex_source_info_seq_type geometry_point_source_infos;
	get_reversed_shared_sub_segment_point_source_infos(geometry_point_source_infos, include_rubber_band_points);

	// Number of resolved source infos should match number of points in the shared sub-segment.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			geometry_point_source_infos.size() == geometry_points.size(),
			GPLATES_ASSERTION_SOURCE);

	// Iterate over the vertex positions and source infos and calculate velocities.
	auto geometry_points_iter = geometry_points.begin();
	auto geometry_points_end = geometry_points.end();
	auto geometry_point_source_infos_iter = geometry_point_source_infos.begin();
	for ( ; geometry_points_iter != geometry_points_end; ++geometry_points_iter, ++geometry_point_source_infos_iter)
	{
		const GPlatesMaths::PointOnSphere &geometry_point = *geometry_points_iter;
		const auto geometry_point_source_info = *geometry_point_source_infos_iter;

		geometry_point_velocities.push_back(
				geometry_point_source_info->get_velocity_vector(
						geometry_point,
						d_shared_segment_reconstruction_geometry->get_reconstruction_time(),
						velocity_delta_time,
						velocity_delta_time_type,
						velocity_units,
						earth_radius_in_kms));
	}
}


void
GPlatesAppLogic::ResolvedTopologicalSharedSubSegment::get_shared_sub_segment_point_source_infos(
		resolved_vertex_source_info_seq_type &point_source_infos,
		bool include_rubber_band_points) const
{
	if (!d_point_source_infos)
	{
		d_point_source_infos = resolved_vertex_source_info_seq_type();

		// Get the point source infos (including at the optional rubber band points).
		ResolvedTopologicalSubSegmentImpl::get_sub_segment_vertex_source_infos(
				d_point_source_infos.get(),
				d_shared_sub_segment,
				d_shared_segment_reconstruction_geometry,
				true/*include_rubber_band_points*/);
	}

	// Copy to caller's sequence.
	//
	// If the caller does not want rubber band points then avoid copying them (if they exist).
	resolved_vertex_source_info_seq_type::const_iterator src_point_source_infos_begin = d_point_source_infos->begin();
	resolved_vertex_source_info_seq_type::const_iterator src_point_source_infos_end = d_point_source_infos->end();
	if (!include_rubber_band_points)
	{
		if (d_shared_sub_segment.get_start_rubber_band())
		{
			++src_point_source_infos_begin;
		}
		if (d_shared_sub_segment.get_end_rubber_band())
		{
			--src_point_source_infos_end;
		}
	}

	std::copy(
			src_point_source_infos_begin,
			src_point_source_infos_end,
			std::back_inserter(point_source_infos));
}


void
GPlatesAppLogic::ResolvedTopologicalSharedSubSegment::get_reversed_shared_sub_segment_point_source_infos(
		resolved_vertex_source_info_seq_type &point_source_infos,
		bool use_reverse,
		bool include_rubber_band_points) const
{
	if (!d_point_source_infos)
	{
		d_point_source_infos = resolved_vertex_source_info_seq_type();

		// Get the point source infos (including at the optional rubber band points).
		ResolvedTopologicalSubSegmentImpl::get_sub_segment_vertex_source_infos(
				d_point_source_infos.get(),
				d_shared_sub_segment,
				d_shared_segment_reconstruction_geometry,
				true/*include_rubber_band_points*/);
	}

	// Copy to caller's sequence.
	//
	// If the caller does not want rubber band points then avoid copying them (if they exist).
	resolved_vertex_source_info_seq_type::const_iterator src_point_source_infos_begin = d_point_source_infos->begin();
	resolved_vertex_source_info_seq_type::const_iterator src_point_source_infos_end = d_point_source_infos->end();
	if (!include_rubber_band_points)
	{
		if (d_shared_sub_segment.get_start_rubber_band())
		{
			++src_point_source_infos_begin;
		}
		if (d_shared_sub_segment.get_end_rubber_band())
		{
			--src_point_source_infos_end;
		}
	}

	if (use_reverse)
	{
		std::reverse_copy(
				src_point_source_infos_begin,
				src_point_source_infos_end,
				std::back_inserter(point_source_infos));
	}
	else
	{
		std::copy(
				src_point_source_infos_begin,
				src_point_source_infos_end,
				std::back_inserter(point_source_infos));
	}
}


void
GPlatesAppLogic::ResolvedTopologicalSharedSubSegment::get_shared_sub_segment_point_source_features(
		std::vector<GPlatesModel::FeatureHandle::weak_ref> &point_source_features,
		bool include_rubber_band_points) const
{
	if (!d_point_source_features)
	{
		d_point_source_features = std::vector<GPlatesModel::FeatureHandle::weak_ref>();

		// Get the point source features (including at the optional rubber band points).
		ResolvedTopologicalSubSegmentImpl::get_sub_segment_vertex_source_features(
				d_point_source_features.get(),
				d_shared_sub_segment,
				d_shared_segment_reconstruction_geometry,
				true/*include_rubber_band_points*/);
	}

	// Copy to caller's sequence.
	//
	// If the caller does not want rubber band points then avoid copying them (if they exist).
	std::vector<GPlatesModel::FeatureHandle::weak_ref>::const_iterator src_point_source_infos_begin = d_point_source_features->begin();
	std::vector<GPlatesModel::FeatureHandle::weak_ref>::const_iterator src_point_source_infos_end = d_point_source_features->end();
	if (!include_rubber_band_points)
	{
		if (d_shared_sub_segment.get_start_rubber_band())
		{
			++src_point_source_infos_begin;
		}
		if (d_shared_sub_segment.get_end_rubber_band())
		{
			--src_point_source_infos_end;
		}
	}

	std::copy(
			src_point_source_infos_begin,
			src_point_source_infos_end,
			std::back_inserter(point_source_features));
}


void
GPlatesAppLogic::ResolvedTopologicalSharedSubSegment::get_reversed_shared_sub_segment_point_source_features(
		std::vector<GPlatesModel::FeatureHandle::weak_ref> &point_source_features,
		bool use_reverse,
		bool include_rubber_band_points) const
{
	if (!d_point_source_features)
	{
		d_point_source_features = std::vector<GPlatesModel::FeatureHandle::weak_ref>();

		// Get the point source features (including at the optional rubber band points).
		ResolvedTopologicalSubSegmentImpl::get_sub_segment_vertex_source_features(
				d_point_source_features.get(),
				d_shared_sub_segment,
				d_shared_segment_reconstruction_geometry,
				true/*include_rubber_band_points*/);
	}

	// Copy to caller's sequence.
	//
	// If the caller does not want rubber band points then avoid copying them (if they exist).
	std::vector<GPlatesModel::FeatureHandle::weak_ref>::const_iterator src_point_source_infos_begin = d_point_source_features->begin();
	std::vector<GPlatesModel::FeatureHandle::weak_ref>::const_iterator src_point_source_infos_end = d_point_source_features->end();
	if (!include_rubber_band_points)
	{
		if (d_shared_sub_segment.get_start_rubber_band())
		{
			++src_point_source_infos_begin;
		}
		if (d_shared_sub_segment.get_end_rubber_band())
		{
			--src_point_source_infos_end;
		}
	}

	if (use_reverse)
	{
		std::reverse_copy(
				src_point_source_infos_begin,
				src_point_source_infos_end,
				std::back_inserter(point_source_features));
	}
	else
	{
		std::copy(
				src_point_source_infos_begin,
				src_point_source_infos_end,
				std::back_inserter(point_source_features));
	}
}


const boost::optional<GPlatesAppLogic::sub_segment_seq_type> &
GPlatesAppLogic::ResolvedTopologicalSharedSubSegment::get_sub_sub_segments() const
{
	if (!d_calculated_sub_sub_segments)
	{
		ResolvedTopologicalSubSegmentImpl::get_sub_sub_segments(
			d_sub_sub_segments,
			d_shared_sub_segment,
			d_shared_segment_reconstruction_geometry);

		d_calculated_sub_sub_segments = true;
	}

	return d_sub_sub_segments;
}


bool
GPlatesAppLogic::ResolvedTopologicalSharedSubSegment::ResolvedTopologyInfo::is_resolved_topology_on_left() const
{
	return TopologyInternalUtils::is_resolved_topology_on_left_of_boundary_sub_segment(
			resolved_topology,
			is_sub_segment_geometry_reversed);
}
