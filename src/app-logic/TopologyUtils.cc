/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
 * Copyright (C) 2013 California Institute of Technology
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
#include <cstddef> // For std::size_t
#include <utility>
#include <vector>
#include <boost/cast.hpp>
#include <boost/optional.hpp>

#include "AppLogicUtils.h"
#include "GeometryUtils.h"
#include "Reconstruction.h"
#include "ReconstructionGeometryUtils.h"
#include "ResolvedTopologicalGeometry.h"
#include "ResolvedTopologicalNetwork.h"
#include "TopologyGeometryResolver.h"
#include "TopologyInternalUtils.h"
#include "TopologyNetworkResolver.h"
#include "TopologyUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/PointInPolygon.h"
#include "maths/PolygonIntersections.h"

#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlTopologicalLine.h"

#include "utils/UnicodeStringUtils.h"


#if 0
	#define DEBUG_POINT_IN_POLYGON
#endif


namespace GPlatesAppLogic
{
	namespace TopologyUtils
	{
		struct PlateIdLessThanComparison
		{
			typedef std::pair<GPlatesModel::integer_plate_id_type,
					const GPlatesAppLogic::ResolvedTopologicalGeometry *> plate_id_and_boundary_type;

			PlateIdLessThanComparison()
			{  }

			bool
			operator()(
					const plate_id_and_boundary_type &a,
					const plate_id_and_boundary_type &b) const
			{
				return (a.first < b.first);
			}
		};
	}
}


bool
GPlatesAppLogic::TopologyUtils::is_topological_geometry_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature)
{
	// Iterate over the feature properties.
	GPlatesModel::FeatureHandle::const_iterator iter = feature->begin();
	GPlatesModel::FeatureHandle::const_iterator end = feature->end();
	for ( ; iter != end; ++iter) 
	{
		// If the current property is a topological geometry then we have a topological feature.
		if (TopologyInternalUtils::get_topology_geometry_property_value_type(iter))
		{
			return true;
		}
	}

	return false;
}


bool
GPlatesAppLogic::TopologyUtils::has_topological_geometry_features(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	GPlatesModel::FeatureCollectionHandle::const_iterator feature_collection_iter = feature_collection->begin();
	GPlatesModel::FeatureCollectionHandle::const_iterator feature_collection_end = feature_collection->end();
	for ( ; feature_collection_iter != feature_collection_end; ++feature_collection_iter)
	{
		GPlatesModel::FeatureHandle::const_weak_ref feature_ref = (*feature_collection_iter)->reference();

		if (is_topological_geometry_feature(feature_ref))
		{ 
			return true; 
		}
	}

	return false;
}


bool
GPlatesAppLogic::TopologyUtils::is_topological_line_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature)
{
	// Iterate over the feature properties.
	GPlatesModel::FeatureHandle::const_iterator iter = feature->begin();
	GPlatesModel::FeatureHandle::const_iterator end = feature->end();
	for ( ; iter != end; ++iter) 
	{
		static const GPlatesPropertyValues::StructuralType GPML_TOPOLOGICAL_LINE =
				GPlatesPropertyValues::StructuralType::create_gpml("TopologicalLine");

		if (TopologyInternalUtils::get_topology_geometry_property_value_type(iter) == GPML_TOPOLOGICAL_LINE)
		{
			return true;
		}
	}

	return false;
}


bool
GPlatesAppLogic::TopologyUtils::has_topological_line_features(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	GPlatesModel::FeatureCollectionHandle::const_iterator feature_collection_iter = feature_collection->begin();
	GPlatesModel::FeatureCollectionHandle::const_iterator feature_collection_end = feature_collection->end();
	for ( ; feature_collection_iter != feature_collection_end; ++feature_collection_iter)
	{
		GPlatesModel::FeatureHandle::const_weak_ref feature_ref = (*feature_collection_iter)->reference();

		if (is_topological_line_feature(feature_ref))
		{ 
			return true; 
		}
	}

	return false;
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::TopologyUtils::resolve_topological_lines(
		std::vector<resolved_topological_geometry_non_null_ptr_type> &resolved_topological_lines,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &topological_line_features_collection,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const reconstruction_tree_non_null_ptr_to_const_type &reconstruction_tree,
		boost::optional<const std::vector<ReconstructHandle::type> &> topological_sections_reconstruct_handles)
{
	// Get the next global reconstruct handle - it'll be stored in each RTG.
	const ReconstructHandle::type reconstruct_handle = ReconstructHandle::get_next_reconstruct_handle();

	// Resolve topological lines (not boundaries).
	TopologyGeometryResolver::resolve_geometry_flags_type resolve_geometry_flags;
	resolve_geometry_flags.set(TopologyGeometryResolver::RESOLVE_LINE);

	// Visit topological line features.
	TopologyGeometryResolver topology_line_resolver(
			resolved_topological_lines,
			resolve_geometry_flags,
			reconstruct_handle,
			reconstruction_tree_creator,
			reconstruction_tree,
			topological_sections_reconstruct_handles);

	AppLogicUtils::visit_feature_collections(
			topological_line_features_collection.begin(),
			topological_line_features_collection.end(),
			topology_line_resolver);

	return reconstruct_handle;
}


bool
GPlatesAppLogic::TopologyUtils::is_topological_boundary_feature(
	const GPlatesModel::FeatureHandle::const_weak_ref &feature)
{
	// Iterate over the feature properties.
	GPlatesModel::FeatureHandle::const_iterator iter = feature->begin();
	GPlatesModel::FeatureHandle::const_iterator end = feature->end();
	for ( ; iter != end; ++iter) 
	{
		static const GPlatesPropertyValues::StructuralType GPML_TOPOLOGICAL_POLYGON =
				GPlatesPropertyValues::StructuralType::create_gpml("TopologicalPolygon");

		if (TopologyInternalUtils::get_topology_geometry_property_value_type(iter) == GPML_TOPOLOGICAL_POLYGON)
		{
			return true;
		}
	}

	return false;
}


bool
GPlatesAppLogic::TopologyUtils::has_topological_boundary_features(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	GPlatesModel::FeatureCollectionHandle::const_iterator feature_collection_iter = feature_collection->begin();
	GPlatesModel::FeatureCollectionHandle::const_iterator feature_collection_end = feature_collection->end();
	for ( ; feature_collection_iter != feature_collection_end; ++feature_collection_iter)
	{
		const GPlatesModel::FeatureHandle::const_weak_ref feature_ref = (*feature_collection_iter)->reference();

		if (is_topological_boundary_feature(feature_ref))
		{ 
			return true; 
		}
	}

	return false;
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::TopologyUtils::resolve_topological_boundaries(
		std::vector<resolved_topological_geometry_non_null_ptr_type> &resolved_topological_boundaries,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &topological_closed_plate_polygon_features_collection,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const reconstruction_tree_non_null_ptr_to_const_type &reconstruction_tree,
		boost::optional<const std::vector<ReconstructHandle::type> &> topological_sections_reconstruct_handles)
{
	// Get the next global reconstruct handle - it'll be stored in each RTG.
	const ReconstructHandle::type reconstruct_handle = ReconstructHandle::get_next_reconstruct_handle();

	// Resolve topological boundaries (not lines).
	TopologyGeometryResolver::resolve_geometry_flags_type resolve_geometry_flags;
	resolve_geometry_flags.set(TopologyGeometryResolver::RESOLVE_BOUNDARY);

	// Visit topological boundary features.
	TopologyGeometryResolver topology_boundary_resolver(
			resolved_topological_boundaries,
			resolve_geometry_flags,
			reconstruct_handle,
			reconstruction_tree_creator,
			reconstruction_tree,
			topological_sections_reconstruct_handles);

	AppLogicUtils::visit_feature_collections(
			topological_closed_plate_polygon_features_collection.begin(),
			topological_closed_plate_polygon_features_collection.end(),
			topology_boundary_resolver);

	return reconstruct_handle;
}



GPlatesAppLogic::TopologyUtils::ResolvedBoundariesForGeometryPartitioning::ResolvedBoundariesForGeometryPartitioning(
		const std::vector<resolved_topological_geometry_non_null_ptr_type> &resolved_topological_boundaries)
{
	// Reserve memory.
	d_resolved_boundaries.reserve(resolved_topological_boundaries.size());

	// Iterate through the resolved topological boundaries and generate polygons for geometry partitioning.
	std::vector<resolved_topological_geometry_non_null_ptr_type>::const_iterator rtb_iter =
			resolved_topological_boundaries.begin();
	std::vector<resolved_topological_geometry_non_null_ptr_type>::const_iterator rtb_end =
			resolved_topological_boundaries.end();
	for ( ; rtb_iter != rtb_end; ++rtb_iter)
	{
		const ResolvedTopologicalGeometry::non_null_ptr_type &rtb = *rtb_iter;

		boost::optional<ResolvedTopologicalGeometry::resolved_topology_boundary_ptr_type>
				resolved_topology_boundary_polygon = rtb->resolved_topology_boundary();
		if (resolved_topology_boundary_polygon)
		{
			const GeometryPartitioning resolved_boundary_for_geometry_partitioning(
					rtb.get(),
					GPlatesMaths::PolygonIntersections::create(resolved_topology_boundary_polygon.get()));

			d_resolved_boundaries.push_back(resolved_boundary_for_geometry_partitioning);
		}
	}
}


bool
GPlatesAppLogic::TopologyUtils::ResolvedBoundariesForGeometryPartitioning::find_resolved_topology_boundaries_containing_point(
		resolved_topological_boundary_seq_type &resolved_topological_boundaries_containing_point,
		const GPlatesMaths::PointOnSphere &point) const
{
	bool was_point_contained_by_any_polygons = false;

	// Iterate through the resolved geometries and see which ones (polygon) contain the point.
	ResolvedBoundariesForGeometryPartitioning::resolved_boundary_seq_type::const_iterator rtb_iter =
			d_resolved_boundaries.begin();
	ResolvedBoundariesForGeometryPartitioning::resolved_boundary_seq_type::const_iterator rtb_end =
			d_resolved_boundaries.end();
	for ( ; rtb_iter != rtb_end; ++rtb_iter)
	{
		const GeometryPartitioning &rtb = *rtb_iter;

#ifdef DEBUG_POINT_IN_POLYGON
		// Get reconstructionPlateId property value
		static const GPlatesModel::PropertyName property_name =
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

		const GPlatesPropertyValues::GpmlPlateId *recon_plate_id;

		if ( GPlatesFeatureVisitors::get_property_value(
			rtb.resolved_topological_boundary->get_feature_ref(), property_name, recon_plate_id ) )
		{
			std::cout 
				<< " TopologyUtils::find_resolved_topology_boundaries_containing_point(): " 
				<< " reconstructionPlateId = " << recon_plate_id->value() 
				<< " has " << rtb.resolved_topological_boundary->resolved_topology_geometry()
						->number_of_segments() + 1 << " vertices." 
				<< std::endl;
		}
#endif

		// Test the point against the resolved topology polygon.
		const GPlatesMaths::PolygonIntersections::Result point_in_poly_result =
				rtb.polygon_intersections->partition_point(point);

		if (point_in_poly_result != GPlatesMaths::PolygonIntersections::GEOMETRY_OUTSIDE)
		{
			was_point_contained_by_any_polygons = true;

			resolved_topological_boundaries_containing_point.push_back(rtb.resolved_topological_boundary);
		}
	}

	return was_point_contained_by_any_polygons;
}


bool
GPlatesAppLogic::TopologyUtils::ResolvedBoundariesForGeometryPartitioning::partition_geometry(
		resolved_boundary_partitioned_geometries_seq_type &resolved_boundary_partitioned_geometries_seq,
		partitioned_geometry_seq_type &partitioned_outside_geometries,
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry) const
{
	bool was_geometry_partitioned_into_any_resolved_boundaries = false;

	// Keeps track of the geometries that are outside the resolved boundary
	// being processed (and outside all resolved boundaries processed so far).
	partitioned_geometry_seq_type partitioned_outside_geometries1;
	partitioned_geometry_seq_type partitioned_outside_geometries2;
	partitioned_geometry_seq_type *current_resolved_boundary_partitioned_outside_geometries =
			&partitioned_outside_geometries1;
	partitioned_geometry_seq_type *next_resolved_boundary_partitioned_outside_geometries =
			&partitioned_outside_geometries2;

	// Add the input geometry to the current list of outside geometries
	// to start off the processing chain.
	current_resolved_boundary_partitioned_outside_geometries->push_back(geometry);

	// Iterate through the resolved boundaries.
	ResolvedBoundariesForGeometryPartitioning::resolved_boundary_seq_type::const_iterator rtb_iter =
			d_resolved_boundaries.begin();
	ResolvedBoundariesForGeometryPartitioning::resolved_boundary_seq_type::const_iterator rtb_end =
			d_resolved_boundaries.end();
	for ( ; rtb_iter != rtb_end; ++rtb_iter)
	{
		const GeometryPartitioning &rtb = *rtb_iter;

		// Geometries partitioned inside the current resolved boundary are stored here.
		ResolvedBoundaryPartitionedGeometries resolved_boundary_partitioned_polylines(
				rtb.resolved_topological_boundary);

		// Clear the next list before we start filling it up.
		next_resolved_boundary_partitioned_outside_geometries->clear();

		// Iterate over the current sequence of outside geometries and partition them
		// using the current resolved boundary.
		partitioned_geometry_seq_type::const_iterator outside_geometry_iter =
				current_resolved_boundary_partitioned_outside_geometries->begin();
		partitioned_geometry_seq_type::const_iterator outside_geometry_end =
				current_resolved_boundary_partitioned_outside_geometries->end();
		for ( ; outside_geometry_iter != outside_geometry_end; ++outside_geometry_iter)
		{
			const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &outside_geometry =
					*outside_geometry_iter;

			// Partition the current outside geometry against the resolved topology boundary
			// polygon. Geometry partitioned outside the current resolved boundary get stored
			// in the sequence of outside geometries used for the next resolved boundary.
			rtb.polygon_intersections->partition_geometry(
					outside_geometry,
					resolved_boundary_partitioned_polylines.partitioned_inside_geometries/*inside*/,
					*next_resolved_boundary_partitioned_outside_geometries/*outside*/);
		}

		// Add the partitioned geometries to the caller's list if any were partitioned
		// into the current resolved boundary.
		if (!resolved_boundary_partitioned_polylines.partitioned_inside_geometries.empty())
		{
			resolved_boundary_partitioned_geometries_seq.push_back(
					resolved_boundary_partitioned_polylines);

			was_geometry_partitioned_into_any_resolved_boundaries = true;
		}

		// Swap the pointers to the current and next list of outside geometries.
		std::swap(
				current_resolved_boundary_partitioned_outside_geometries,
				next_resolved_boundary_partitioned_outside_geometries);

	}

	// Pass any remaining partitioned outside geometries to the caller.
	// These are not inside any of the resolved boundaries.
	partitioned_outside_geometries.splice(
			partitioned_outside_geometries.end(),
			*current_resolved_boundary_partitioned_outside_geometries);

	return was_geometry_partitioned_into_any_resolved_boundaries;
}


boost::optional< std::pair<
		GPlatesModel::integer_plate_id_type,
		const GPlatesAppLogic::ResolvedTopologicalGeometry * > >
GPlatesAppLogic::TopologyUtils::find_reconstruction_plate_id_furthest_from_anchor_in_plate_circuit(
		const resolved_topological_boundary_seq_type &resolved_boundaries)
{
	typedef std::pair<GPlatesModel::integer_plate_id_type,
			const ResolvedTopologicalGeometry *> plate_id_and_boundary_type;

	std::vector<plate_id_and_boundary_type> reconstruction_plate_ids;
	reconstruction_plate_ids.reserve(resolved_boundaries.size());

	// Loop over the resolved topological boundaries.
	GPlatesAppLogic::TopologyUtils::resolved_topological_boundary_seq_type::const_iterator rtb_iter;
	for (rtb_iter = resolved_boundaries.begin();
		rtb_iter != resolved_boundaries.end();
		++rtb_iter)
	{
		const ResolvedTopologicalGeometry *rtb = *rtb_iter;

		if (rtb->plate_id())
		{
			// The resolved boundary feature has a reconstruction plate ID.
			reconstruction_plate_ids.push_back(
					std::make_pair(*rtb->plate_id(), rtb));
		}
	}

	if (reconstruction_plate_ids.empty()) {
		return boost::none;
	}

	// Sort the plate ids.
	std::sort(reconstruction_plate_ids.begin(), reconstruction_plate_ids.end(),
			PlateIdLessThanComparison());

	// Get the highest numeric plate id - which is at the back of the sorted sequence.
	return reconstruction_plate_ids.back();
}


bool
GPlatesAppLogic::TopologyUtils::is_topological_network_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature)
{
	// Iterate over the feature properties.
	GPlatesModel::FeatureHandle::const_iterator iter = feature->begin();
	GPlatesModel::FeatureHandle::const_iterator end = feature->end();
	for ( ; iter != end; ++iter) 
	{
		static const GPlatesPropertyValues::StructuralType GPML_TOPOLOGICAL_NETWORK =
				GPlatesPropertyValues::StructuralType::create_gpml("TopologicalNetwork");

		if (TopologyInternalUtils::get_topology_geometry_property_value_type(iter) == GPML_TOPOLOGICAL_NETWORK)
		{
			return true;
		}
	}

	return false;
}


bool
GPlatesAppLogic::TopologyUtils::has_topological_network_features(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	GPlatesModel::FeatureCollectionHandle::const_iterator feature_collection_iter =
			feature_collection->begin();
	GPlatesModel::FeatureCollectionHandle::const_iterator feature_collection_end =
			feature_collection->end();
	for ( ; feature_collection_iter != feature_collection_end; ++feature_collection_iter)
	{
		const GPlatesModel::FeatureHandle::const_weak_ref feature_ref = (*feature_collection_iter)->reference();

		if (is_topological_network_feature(feature_ref))
		{ 
			return true; 
		}
	}

	return false;
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::TopologyUtils::resolve_topological_networks(
		std::vector<resolved_topological_network_non_null_ptr_type> &resolved_topological_networks,
		const double &reconstruction_time,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &topological_network_features_collection,
		boost::optional<const std::vector<ReconstructHandle::type> &> topological_geometry_reconstruct_handles)
{
	// Get the next global reconstruct handle - it'll be stored in each RTN.
	const ReconstructHandle::type reconstruct_handle = ReconstructHandle::get_next_reconstruct_handle();

	// Visit topological network features.
	TopologyNetworkResolver topology_network_resolver(
			resolved_topological_networks,
			reconstruction_time,
			reconstruct_handle,
			topological_geometry_reconstruct_handles);

	AppLogicUtils::visit_feature_collections(
			topological_network_features_collection.begin(),
			topological_network_features_collection.end(),
			topology_network_resolver);

	return reconstruct_handle;
}
