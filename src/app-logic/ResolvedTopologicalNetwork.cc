/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
 * Copyright (C) 2012, 2013 California Institute of Technology
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


// FIXME: does not work ; need to fix references to triangulation
// NOTE: use with caution: this can cause the Log window to lag during resize events.
// #define DEBUG_FILE


#include <QDebug>
#include <QFile>
#include <QString>
#include <QTextStream>

#include "ResolvedTopologicalNetwork.h"

#include "ApplicationState.h"
#include "GeometryUtils.h"
#include "ReconstructionGeometryVisitor.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/NotYetImplementedException.h"

#include "maths/AzimuthalEqualAreaProjection.h"

#include "model/PropertyName.h"
#include "model/WeakObserverVisitor.h"

#include "property-values/XsString.h"

#include "utils/GeometryCreationUtils.h"
#include "utils/UnicodeStringUtils.h"


void
GPlatesAppLogic::ResolvedTopologicalNetwork::boundary_polygon_points(
		std::vector<GPlatesMaths::PointOnSphere> &boundary_points_,
		bool include_rigid_blocks_as_interior_holes) const
{
	const boundary_polygon_ptr_type boundary_polygon_ = boundary_polygon(include_rigid_blocks_as_interior_holes);

	// Add the vertices of the resolved topological boundary to the end of the caller's array.
	//
	// Note: We include vertices from all rings (exterior and interiors) in the polygon.
	//       If the polygon has interior rings they'll get added (if it doesn't then they won't).
	boundary_points_.insert(
			boundary_points_.end(),
			boundary_polygon_->vertex_begin(),
			boundary_polygon_->vertex_end());
}


void
GPlatesAppLogic::ResolvedTopologicalNetwork::boundary_polygon_point_velocities(
		std::vector<GPlatesMaths::Vector3D> &resolved_topology_geometry_point_velocities_,
		bool include_rigid_blocks_as_interior_holes,
		const double &velocity_delta_time,
		VelocityDeltaTime::Type velocity_delta_time_type,
		VelocityUnits::Value velocity_units,
		const double &earth_radius_in_kms) const
{
	const boundary_polygon_ptr_type boundary_polygon_ = boundary_polygon(include_rigid_blocks_as_interior_holes);

	// Get the resolved source infos (one per point in the resolved boundary).
	const resolved_vertex_source_info_seq_type &resolved_source_infos =
			boundary_polygon_point_source_infos(include_rigid_blocks_as_interior_holes);

	// Number of resolved source infos should match number of points in the resolved boundary.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			resolved_source_infos.size() == boundary_polygon_->number_of_vertices(),
			GPLATES_ASSERTION_SOURCE);

	// Iterate over the vertex positions and source infos and calculate velocities.
	auto boundary_points_iter = boundary_polygon_->vertex_begin();
	auto boundary_points_end = boundary_polygon_->vertex_end();
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


const GPlatesModel::FeatureHandle::weak_ref
GPlatesAppLogic::ResolvedTopologicalNetwork::get_feature_ref() const
{
	if (is_valid()) {
		return feature_handle_ptr()->reference();
	} else {
		return GPlatesModel::FeatureHandle::weak_ref();
	}
}


const GPlatesAppLogic::resolved_vertex_source_info_seq_type &
GPlatesAppLogic::ResolvedTopologicalNetwork::boundary_polygon_point_source_infos(
		bool include_rigid_blocks_as_interior_holes) const
{
	if (include_rigid_blocks_as_interior_holes)
	{
		if (!d_boundary_with_rigid_blocks_vertex_source_infos)
		{
			calc_boundary_vertex_source_infos(include_rigid_blocks_as_interior_holes);
		}

		return d_boundary_with_rigid_blocks_vertex_source_infos.get();
	}
	else
	{
		// Cache all vertex source infos on first call.
		if (!d_boundary_vertex_source_infos)
		{
			calc_boundary_vertex_source_infos(include_rigid_blocks_as_interior_holes);
		}

		return d_boundary_vertex_source_infos.get();
	}
}


const std::vector<GPlatesModel::FeatureHandle::weak_ref> &
GPlatesAppLogic::ResolvedTopologicalNetwork::boundary_polygon_point_source_features(
		bool include_rigid_blocks_as_interior_holes) const
{
	if (include_rigid_blocks_as_interior_holes)
	{
		if (!d_boundary_with_rigid_blocks_vertex_source_features)
		{
			calc_boundary_vertex_source_features(include_rigid_blocks_as_interior_holes);
		}

		return d_boundary_with_rigid_blocks_vertex_source_features.get();
	}
	else
	{
		// Cache all vertex source features on first call.
		if (!d_boundary_vertex_source_features)
		{
			calc_boundary_vertex_source_features(include_rigid_blocks_as_interior_holes);
		}

		return d_boundary_vertex_source_features.get();
	}
}


void
GPlatesAppLogic::ResolvedTopologicalNetwork::calc_boundary_vertex_source_infos(
		bool include_rigid_blocks_as_interior_holes) const
{
	boost::optional<resolved_vertex_source_info_seq_type &> resolved_vertex_source_info_seq;
	if (include_rigid_blocks_as_interior_holes)
	{
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				!d_boundary_with_rigid_blocks_vertex_source_infos,
				GPLATES_ASSERTION_SOURCE);

		d_boundary_with_rigid_blocks_vertex_source_infos = resolved_vertex_source_info_seq_type();
		resolved_vertex_source_info_seq = d_boundary_with_rigid_blocks_vertex_source_infos.get();
	}
	else
	{
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				!d_boundary_vertex_source_infos,
				GPLATES_ASSERTION_SOURCE);

		d_boundary_vertex_source_infos = resolved_vertex_source_info_seq_type();
		resolved_vertex_source_info_seq = d_boundary_vertex_source_infos.get();
	}

	// Copy source infos from points in each boundary sub-segment.
	for (const auto &boundary_sub_segment : d_boundary_sub_segment_seq)
	{
		// Sub-segment should be reversed if that's how it contributed to the resolved topological network.
		boundary_sub_segment->get_reversed_sub_segment_point_source_infos(
				resolved_vertex_source_info_seq.get(),
				INCLUDE_SUB_SEGMENT_RUBBER_BAND_POINTS_IN_RESOLVED_NETWORK_BOUNDARY/*include_rubber_band_points*/);
	}

	// Also copy source infos from interior rigid blocks (if requested).
	//
	// Note: We do this *after* copying the exterior boundary vertices because we need the source infos to
	//       be in the same order as the vertices in the boundary polygon (returned by 'boundary_polygon()').
	//       And PolygonOnSphere stores the exterior ring first, followed by the interior rings (in the order
	//       they were specified to PolygonOnSphere). And the interior rings were ordered by the order of
	//       the rigid blocks. So we'll add source infos in that order here too.
	if (include_rigid_blocks_as_interior_holes)
	{
		for (const auto &rigid_block : get_triangulation_network().get_rigid_blocks())
		{
			const ReconstructedFeatureGeometry::non_null_ptr_type rigid_block_rfg =
					rigid_block.get_reconstructed_feature_geometry();

			// All points in the rigid block polygon are reconstructed the same way.
			const ResolvedVertexSourceInfo::non_null_ptr_to_const_type rigid_block_source_info =
					ResolvedVertexSourceInfo::create(rigid_block_rfg);

			// Rigid blocks have *polygon* geometries.
			boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> rigid_block_polygon =
					GeometryUtils::get_polygon_on_sphere(*rigid_block_rfg->reconstructed_geometry());
			if (!rigid_block_polygon)
			{
				// Shouldn't be able to get here because only static polygons are added to networks as rigid blocks.
				continue;
			}

			// Rigid blocks should only have an exterior ring (and no interior rings).
			// Well, at least only the exterior ring vertices are added to the network's Delaunay triangulation.
			// So we'll also only add a source info for each *exterior* ring vertex.
			resolved_vertex_source_info_seq->insert(
					resolved_vertex_source_info_seq->end(),
					rigid_block_polygon.get()->number_of_vertices_in_exterior_ring(),
					rigid_block_source_info);
		}
	}
}


void
GPlatesAppLogic::ResolvedTopologicalNetwork::calc_boundary_vertex_source_features(
		bool include_rigid_blocks_as_interior_holes) const
{
	boost::optional<std::vector<GPlatesModel::FeatureHandle::weak_ref> &> resolved_vertex_source_feature_seq;
	if (include_rigid_blocks_as_interior_holes)
	{
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				!d_boundary_with_rigid_blocks_vertex_source_features,
				GPLATES_ASSERTION_SOURCE);

		d_boundary_with_rigid_blocks_vertex_source_features = std::vector<GPlatesModel::FeatureHandle::weak_ref>();
		resolved_vertex_source_feature_seq = d_boundary_with_rigid_blocks_vertex_source_features.get();
	}
	else
	{
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				!d_boundary_vertex_source_features,
				GPLATES_ASSERTION_SOURCE);

		d_boundary_vertex_source_features = std::vector<GPlatesModel::FeatureHandle::weak_ref>();
		resolved_vertex_source_feature_seq = d_boundary_vertex_source_features.get();
	}

	// Copy source features from points in each boundary sub-segment.
	for (const auto &boundary_sub_segment : d_boundary_sub_segment_seq)
	{
		// Sub-segment should be reversed if that's how it contributed to the resolved topological network.
		boundary_sub_segment->get_reversed_sub_segment_point_source_features(
				resolved_vertex_source_feature_seq.get(),
				INCLUDE_SUB_SEGMENT_RUBBER_BAND_POINTS_IN_RESOLVED_NETWORK_BOUNDARY/*include_rubber_band_points*/);
	}

	// Also copy source features from interior rigid blocks (if requested).
	//
	// Note: We do this *after* copying the exterior boundary vertices because we need the source features to
	//       be in the same order as the vertices in the boundary polygon (returned by 'boundary_polygon()').
	//       And PolygonOnSphere stores the exterior ring first, followed by the interior rings (in the order
	//       they were specified to PolygonOnSphere). And the interior rings were ordered by the order of
	//       the rigid blocks. So we'll add source features in that order here too.
	if (include_rigid_blocks_as_interior_holes)
	{
		for (const auto &rigid_block : get_triangulation_network().get_rigid_blocks())
		{
			const ReconstructedFeatureGeometry::non_null_ptr_type rigid_block_rfg =
					rigid_block.get_reconstructed_feature_geometry();

			// All points in the rigid block polygon are reconstructed the same way.
			const GPlatesModel::FeatureHandle::weak_ref rigid_block_source_feature =
					rigid_block_rfg->get_feature_ref();

			// Rigid blocks have *polygon* geometries.
			boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> rigid_block_polygon =
					GeometryUtils::get_polygon_on_sphere(*rigid_block_rfg->reconstructed_geometry());
			if (!rigid_block_polygon)
			{
				// Shouldn't be able to get here because only static polygons are added to networks as rigid blocks.
				continue;
			}

			// Rigid blocks should only have an exterior ring (and no interior rings).
			// Well, at least only the exterior ring vertices are added to the network's Delaunay triangulation.
			// So we'll also only add a source feature for each *exterior* ring vertex.
			resolved_vertex_source_feature_seq->insert(
					resolved_vertex_source_feature_seq->end(),
					rigid_block_polygon.get()->number_of_vertices_in_exterior_ring(),
					rigid_block_source_feature);
		}
	}
}


void
GPlatesAppLogic::ResolvedTopologicalNetwork::accept_visitor(
		ConstReconstructionGeometryVisitor &visitor) const
{
	visitor.visit(this->get_non_null_pointer_to_const());
}


void
GPlatesAppLogic::ResolvedTopologicalNetwork::accept_visitor(
		ReconstructionGeometryVisitor &visitor)
{
	visitor.visit(this->get_non_null_pointer());
}

void
GPlatesAppLogic::ResolvedTopologicalNetwork::accept_weak_observer_visitor(
		GPlatesModel::WeakObserverVisitor<GPlatesModel::FeatureHandle> &visitor)
{
	visitor.visit_resolved_topological_network(*this);
}
