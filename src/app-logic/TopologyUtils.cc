/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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
#include <utility>
#include <vector>

#include "TopologyUtils.h"

#include "ReconstructionGeometryUtils.h"
#include "TopologyResolver.h"

#include "AppLogicUtils.h"

#include "maths/PolygonIntersections.h"

#include "model/Reconstruction.h"

#include "utils/Profile.h"

#if 0
	#define DEBUG_POINT_IN_POLYGON
#endif


namespace GPlatesAppLogic
{
	namespace TopologyUtils
	{
		/**
		 * Associates a @a ResolvedTopologicalGeometry with its polygon structure
		 * used for geometry partitioning.
		 */
		struct ResolvedGeometryForGeometryPartitioning
		{
			ResolvedGeometryForGeometryPartitioning(
					const GPlatesModel::ResolvedTopologicalGeometry *resolved_topological_geometry_,
					const GPlatesMaths::PolygonIntersections::non_null_ptr_type &polygon_intersections_) :
				resolved_topological_geometry(resolved_topological_geometry_),
				polygon_intersections(polygon_intersections_)
			{  }


			const GPlatesModel::ResolvedTopologicalGeometry *resolved_topological_geometry;
			GPlatesMaths::PolygonIntersections::non_null_ptr_type polygon_intersections;
		};


		/**
		 * A sequence of @a ResolvedGeometryForGeometryPartitioning objects.
		 */
		class ResolvedGeometriesForGeometryPartitioning
		{
		public:
			typedef std::vector<ResolvedGeometryForGeometryPartitioning> resolved_geom_seq_type;

			resolved_geom_seq_type d_resolved_geoms;
		};
	}
}


void
GPlatesAppLogic::TopologyUtils::resolve_topologies(
		const double &reconstruction_time,
		GPlatesModel::Reconstruction &reconstruction,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
				topological_features_collection)
{
	PROFILE_FUNC();

	TopologyResolver topology_resolver(
			reconstruction_time, reconstruction);

	AppLogicUtils::visit_feature_collections(
		topological_features_collection.begin(),
		topological_features_collection.end(),
		topology_resolver);
}


bool
GPlatesAppLogic::TopologyUtils::has_resolved_topological_boundaries(
		GPlatesModel::Reconstruction &reconstruction)
{
	// Iterate through the reconstruction geometries and see if any are
	// resolved topological boundaries.
	GPlatesModel::Reconstruction::geometry_collection_type::const_iterator rg_iter =
			reconstruction.geometries().begin();
	GPlatesModel::Reconstruction::geometry_collection_type::const_iterator rg_end =
			reconstruction.geometries().end();
	for ( ; rg_iter != rg_end; ++rg_iter)
	{
		const GPlatesModel::ReconstructionGeometry::non_null_ptr_type &rg = *rg_iter;

		GPlatesModel::ResolvedTopologicalGeometry *rtg = NULL;
		if (GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type(rg, rtg))
		{
			return true;
		}
	}

	return false;
}


GPlatesAppLogic::TopologyUtils::resolved_geometries_for_geometry_partitioning_query_type
GPlatesAppLogic::TopologyUtils::query_resolved_topologies_for_geometry_partitioning(
		GPlatesModel::Reconstruction &reconstruction)
{
	// First get all ResolvedTopologicalGeometry objects in the reconstruction.
	resolved_topological_geometry_seq_type all_resolved_geoms;
	ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
				reconstruction.geometries().begin(),
				reconstruction.geometries().end(),
				all_resolved_geoms);

	// Query structure to return to the caller.
	resolved_geometries_for_geometry_partitioning_query_type resolved_geometries_query;

	if (all_resolved_geoms.empty())
	{
		// Return empty query.
		return resolved_geometries_query;
	}

	// Create a new query to be filled.
	resolved_geometries_query.reset(new ResolvedGeometriesForGeometryPartitioning());

	// Reserve memory.
	resolved_geometries_query->d_resolved_geoms.reserve(all_resolved_geoms.size());

	// Iterate through the resolved topological geometries and
	// generate polygons for geometry partitioning.
	resolved_topological_geometry_seq_type::const_iterator rtg_iter = all_resolved_geoms.begin();
	resolved_topological_geometry_seq_type::const_iterator rtg_end = all_resolved_geoms.end();
	for ( ; rtg_iter != rtg_end; ++rtg_iter)
	{
		const GPlatesModel::ResolvedTopologicalGeometry *rtg = *rtg_iter;

		const ResolvedGeometryForGeometryPartitioning resolved_geom_for_geometry_partitioning(
				rtg,
				GPlatesMaths::PolygonIntersections::create(rtg->resolved_topology_geometry()));

		resolved_geometries_query->d_resolved_geoms.push_back(
				resolved_geom_for_geometry_partitioning);
	}

	return resolved_geometries_query;
}


bool
GPlatesAppLogic::TopologyUtils::find_resolved_topologies_containing_point(
		resolved_topological_geometry_seq_type &resolved_topological_geoms_containing_point,
		const GPlatesMaths::PointOnSphere &point,
		const resolved_geometries_for_geometry_partitioning_query_type &resolved_geoms_query)
{
	// Return early if query does not exist.
	if (!resolved_geoms_query)
	{
		return false;
	}

	bool was_point_contained_by_any_polygons = false;

	// Iterate through the resolved geometries and see which ones (polygon) contain the point.
	ResolvedGeometriesForGeometryPartitioning::resolved_geom_seq_type::const_iterator rtg_iter =
			resolved_geoms_query->d_resolved_geoms.begin();
	ResolvedGeometriesForGeometryPartitioning::resolved_geom_seq_type::const_iterator rtg_end =
			resolved_geoms_query->d_resolved_geoms.end();
	for ( ; rtg_iter != rtg_end; ++rtg_iter)
	{
		const ResolvedGeometryForGeometryPartitioning &rtg = *rtg_iter;

#ifdef DEBUG_POINT_IN_POLYGON
		// Get reconstructionPlateId property value
		static const GPlatesModel::PropertyName property_name =
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

		const GPlatesPropertyValues::GpmlPlateId *recon_plate_id;

		if ( GPlatesFeatureVisitors::get_property_value(
			rtg.resolved_topological_geometry->get_feature_ref(), property_name, recon_plate_id ) )
		{
			std::cout 
				<< " TopologyUtils::find_resolved_topologies_containing_point(): " 
				<< " reconstructionPlateId = " << recon_plate_id->value() 
				<< " has " << rtg.resolved_topological_geometry->resolved_topology_geometry()
						->number_of_segments() + 1 << " vertices." 
				<< std::endl;
		}
#endif

		// Test the point against the resolved topology polygon.
		const GPlatesMaths::PolygonIntersections::Result point_in_poly_result =
				rtg.polygon_intersections->partition_point(point);

		if (point_in_poly_result != GPlatesMaths::PolygonIntersections::GEOMETRY_OUTSIDE)
		{
			was_point_contained_by_any_polygons = true;

			resolved_topological_geoms_containing_point.push_back(rtg.resolved_topological_geometry);
		}
	}

	return was_point_contained_by_any_polygons;
}


bool
GPlatesAppLogic::TopologyUtils::partition_polyline_using_resolved_topology_boundaries(
		resolved_boundary_partitioned_polylines_seq_type &resolved_boundary_partitioned_polylines_seq,
		partitioned_polyline_seq_type &partitioned_outside_polylines,
		const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type &polyline,
		const resolved_geometries_for_geometry_partitioning_query_type &resolved_geoms_query)
{
	// Return early if query does not exist.
	if (!resolved_geoms_query)
	{
		return false;
	}

	bool was_polyline_partitioned_into_any_resolved_boundaries = false;

	// Keeps track of the polylines that are outside the resolved boundary
	// being processed (and outside all resolved boundaries processed so far).
	partitioned_polyline_seq_type partitioned_outside_polylines1;
	partitioned_polyline_seq_type partitioned_outside_polylines2;
	partitioned_polyline_seq_type *current_resolved_boundary_partitioned_outside_polylines =
			&partitioned_outside_polylines1;
	partitioned_polyline_seq_type *next_resolved_boundary_partitioned_outside_polylines =
			&partitioned_outside_polylines2;

	// Add the input polyline to the current list of outside polylines
	// to start off the processing chain.
	current_resolved_boundary_partitioned_outside_polylines->push_back(polyline);

	// Iterate through the resolved boundaries.
	ResolvedGeometriesForGeometryPartitioning::resolved_geom_seq_type::const_iterator rtg_iter =
			resolved_geoms_query->d_resolved_geoms.begin();
	ResolvedGeometriesForGeometryPartitioning::resolved_geom_seq_type::const_iterator rtg_end =
			resolved_geoms_query->d_resolved_geoms.end();
	for ( ; rtg_iter != rtg_end; ++rtg_iter)
	{
		const ResolvedGeometryForGeometryPartitioning &rtg = *rtg_iter;

		// Polylines partitioned inside the current resolved boundary are stored here.
		ResolvedBoundaryPartitionedPolylines resolved_boundary_partitioned_polylines(
				rtg.resolved_topological_geometry);

		// Clear the next list before we start filling it up.
		next_resolved_boundary_partitioned_outside_polylines->clear();

		// Iterate over the current sequence of outside polylines and partition them
		// using the current resolved boundary.
		partitioned_polyline_seq_type::const_iterator outside_polyline_iter =
				current_resolved_boundary_partitioned_outside_polylines->begin();
		partitioned_polyline_seq_type::const_iterator outside_polyline_end =
				current_resolved_boundary_partitioned_outside_polylines->end();
		for ( ; outside_polyline_iter != outside_polyline_end; ++outside_polyline_iter)
		{
			const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type &outside_polyline =
					*outside_polyline_iter;

			// Partition the current outside polyline against the resolved topology boundary
			// polygon. Polylines partitioned outside the current resolved boundary get stored
			// in the sequence of outside polylines used for the next resolved boundary.
			rtg.polygon_intersections->partition_polyline(
					outside_polyline,
					resolved_boundary_partitioned_polylines.partitioned_inside_polylines/*inside*/,
					*next_resolved_boundary_partitioned_outside_polylines/*outside*/);
		}

		// Add the partitioned polylines to the caller's list if any were partitioned
		// into the current resolved boundary.
		if (!resolved_boundary_partitioned_polylines.partitioned_inside_polylines.empty())
		{
			resolved_boundary_partitioned_polylines_seq.push_back(
					resolved_boundary_partitioned_polylines);

			was_polyline_partitioned_into_any_resolved_boundaries = true;
		}

		// Swap the pointers to the current and next list of outside polylines.
		std::swap(
				current_resolved_boundary_partitioned_outside_polylines,
				next_resolved_boundary_partitioned_outside_polylines);

	}

	// Pass any remaining partitioned outside polylines to the caller.
	// These are not inside any of the resolved boundaries.
	partitioned_outside_polylines.splice(
			partitioned_outside_polylines.end(),
			*current_resolved_boundary_partitioned_outside_polylines);

	return was_polyline_partitioned_into_any_resolved_boundaries;
}


boost::optional<GPlatesModel::integer_plate_id_type>
GPlatesAppLogic::TopologyUtils::find_reconstruction_plate_id_furthest_from_anchor_in_plate_circuit(
		const resolved_topological_geometry_seq_type &resolved_boundaries)
{
	std::vector<GPlatesModel::integer_plate_id_type> reconstruction_plate_ids;
	reconstruction_plate_ids.reserve(resolved_boundaries.size());

	// Loop over the resolved topological boundaries.
	GPlatesAppLogic::TopologyUtils::resolved_topological_geometry_seq_type::const_iterator rtb_iter;
	for (rtb_iter = resolved_boundaries.begin();
		rtb_iter != resolved_boundaries.end();
		++rtb_iter)
	{
		const GPlatesModel::ResolvedTopologicalGeometry *rtb = *rtb_iter;

		if (rtb->plate_id())
		{
			// The resolved boundary feature has a reconstruction plate ID.
			reconstruction_plate_ids.push_back(*rtb->plate_id());
		}
	}

	if (reconstruction_plate_ids.empty())
	{
		return boost::none;
	}

	// Sort the plate ids.
	std::sort(reconstruction_plate_ids.begin(), reconstruction_plate_ids.end());

	// Get the highest numeric plate id - which is at the back of the sorted sequence.
	return reconstruction_plate_ids.back();
}


GPlatesAppLogic::TopologyUtils::ResolvedBoundaryPartitionedPolylines::ResolvedBoundaryPartitionedPolylines(
		const GPlatesModel::ResolvedTopologicalGeometry *resolved_topological_boundary_) :
	resolved_topological_boundary(resolved_topological_boundary_)
{
}
