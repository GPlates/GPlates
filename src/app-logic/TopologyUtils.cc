/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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
#include <boost/shared_ptr.hpp>

#include "AppLogicUtils.h"
#include "CgalUtils.h"
#include "GeometryUtils.h"
#include "Reconstruction.h"
#include "ReconstructionGeometryUtils.h"
#include "ResolvedTopologicalBoundary.h"
#include "ResolvedTopologicalNetwork.h"
#include "TopologyBoundaryResolver.h"
#include "TopologyInternalUtils.h"
#include "TopologyNetworkResolver.h"
#include "TopologyUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/PointInPolygon.h"
#include "maths/PolygonIntersections.h"

#include "utils/UnicodeStringUtils.h"


#if 0
	#define DEBUG_POINT_IN_POLYGON
#endif


namespace GPlatesAppLogic
{
	namespace TopologyUtils
	{
		/**
		 * Associates a @a ResolvedTopologicalBoundary with its polygon structure
		 * for geometry partitioning.
		 */
		class ResolvedBoundaryForGeometryPartitioning
		{
		public:
			ResolvedBoundaryForGeometryPartitioning(
					const ResolvedTopologicalBoundary *resolved_topological_boundary_,
					const GPlatesMaths::PolygonIntersections::non_null_ptr_type &polygon_intersections_) :
				resolved_topological_boundary(resolved_topological_boundary_),
				polygon_intersections(polygon_intersections_)
			{  }


			const ResolvedTopologicalBoundary *resolved_topological_boundary;
			GPlatesMaths::PolygonIntersections::non_null_ptr_type polygon_intersections;
		};
		/**
		 * A sequence of @a ResolvedBoundaryForGeometryPartitioning objects.
		 */
		class ResolvedBoundariesForGeometryPartitioning
		{
		public:
			typedef std::vector<ResolvedBoundaryForGeometryPartitioning> resolved_boundary_seq_type;

			resolved_boundary_seq_type resolved_boundaries;
		};


		/**
		 * Used to interpolate a network.
		 */
		class ResolvedNetworkForInterpolationQuery
		{
		public:
			ResolvedNetworkForInterpolationQuery(
					ResolvedTopologicalNetwork *network_) :
				network(network_)
			{  }

			typedef boost::shared_ptr<CgalUtils::cgal_map_point_to_value_type> scalar_map_ptr_type;
			typedef std::vector<scalar_map_ptr_type> scalar_map_seq_type;

			ResolvedTopologicalNetwork *network;
			scalar_map_seq_type scalar_map_seq;
		};
		/**
		 * A sequence of @a ResolvedNetworkForInterpolationQuery objects.
		 */
		class ResolvedNetworksForInterpolationQuery
		{
		public:
			typedef std::vector<resolved_network_for_interpolation_query_type> resolved_network_seq_type;
			resolved_network_seq_type resolved_networks;
		};


		/**
		 * Interpolates the scalars in a resolved network and returns them.
		 */
		boost::optional< std::vector<double> >
		interpolate_resolved_topology_network(
				const resolved_network_for_interpolation_query_type &resolved_network_query,
				const CgalUtils::cgal_point_2_type &cgal_point_2)
		{
			const ResolvedNetworkForInterpolationQuery &resolved_network = *resolved_network_query;

			boost::optional<CgalUtils::interpolate_triangulation_query_type> interpolation_query =
					CgalUtils::query_interpolate_triangulation(
							cgal_point_2,
							resolved_network.network->get_cgal_triangulation());

			// Return false if the point is not in the current network.
			if (!interpolation_query)
			{
				return boost::none;
			}

			std::vector<double> interpolated_scalars;
			interpolated_scalars.reserve(resolved_network.scalar_map_seq.size());

			// Iterate through the scalar maps and interpolate for each scalar.
			ResolvedNetworkForInterpolationQuery::scalar_map_seq_type::const_iterator scalar_maps_iter =
					resolved_network.scalar_map_seq.begin();
			ResolvedNetworkForInterpolationQuery::scalar_map_seq_type::const_iterator scalar_maps_end =
					resolved_network.scalar_map_seq.end();
			for ( ; scalar_maps_iter != scalar_maps_end; ++scalar_maps_iter)
			{
				// Get the map for the current scalar.
				const CgalUtils::cgal_map_point_to_value_type &scalar_map = **scalar_maps_iter;

				// Interpolate the mapped values.
				interpolated_scalars.push_back(
						CgalUtils::interpolate_triangulation(
								*interpolation_query,
								scalar_map));
			}

			// We are currently just returning scalars interpolated from the first network
			// that the point is found inside.
			// This will probably need to be changed if the topological networks can overlap.
			return interpolated_scalars;
		}
	}
}


bool
GPlatesAppLogic::TopologyUtils::is_topological_closed_plate_boundary_feature(
		const GPlatesModel::FeatureHandle &feature)
{
	// FIXME: Do this check based on feature properties rather than feature type.
	// So if something looks like a TCPB (because it has a topology polygon property)
	// then treat it like one. For this to happen we first need TopologicalNetwork to
	// use a property type different than TopologicalPolygon.
	//
	static const QString type("TopologicalClosedPlateBoundary");

	return type == GPlatesUtils::make_qstring_from_icu_string(feature.feature_type().get_name());
}


bool
GPlatesAppLogic::TopologyUtils::has_topological_closed_plate_boundary_features(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	GPlatesModel::FeatureCollectionHandle::const_iterator feature_collection_iter = feature_collection->begin();
	GPlatesModel::FeatureCollectionHandle::const_iterator feature_collection_end = feature_collection->end();
	for ( ; feature_collection_iter != feature_collection_end; ++feature_collection_iter)
	{
		const GPlatesModel::FeatureHandle &feature_handle = **feature_collection_iter;

		if (is_topological_closed_plate_boundary_feature(feature_handle))
		{ 
			return true; 
		}
	}

	return false;
}


GPlatesAppLogic::ReconstructionGeometryCollection::non_null_ptr_type
GPlatesAppLogic::TopologyUtils::resolve_topological_boundaries(
		ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
				topological_boundary_features_collection)
{
	ReconstructionGeometryCollection::non_null_ptr_type reconstruction_geom_collection =
			ReconstructionGeometryCollection::create(reconstruction_tree);

	// Visit topological boundary features.
	TopologyBoundaryResolver topology_boundary_resolver(*reconstruction_geom_collection);

	AppLogicUtils::visit_feature_collections(
		topological_boundary_features_collection.begin(),
		topological_boundary_features_collection.end(),
		topology_boundary_resolver);

	return reconstruction_geom_collection;
}


GPlatesAppLogic::TopologyUtils::resolved_boundaries_for_geometry_partitioning_query_type
GPlatesAppLogic::TopologyUtils::query_resolved_topologies_for_geometry_partitioning(
		const ReconstructionGeometryCollection &reconstruction_geometry_collection)
{
	// First get all ResolvedTopologicalBoundary objects in the reconstruction.
	resolved_topological_boundary_seq_type all_resolved_boundaries;
	ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
				reconstruction_geometry_collection.begin(),
				reconstruction_geometry_collection.end(),
				all_resolved_boundaries);

	// Query structure to return to the caller.
	resolved_boundaries_for_geometry_partitioning_query_type resolved_boundaries_query;

	if (all_resolved_boundaries.empty())
	{
		// Return empty query.
		return resolved_boundaries_query;
	}

	// Create a new query to be filled.
	resolved_boundaries_query.reset(new ResolvedBoundariesForGeometryPartitioning());

	// Reserve memory.
	resolved_boundaries_query->resolved_boundaries.reserve(all_resolved_boundaries.size());

	// Iterate through the resolved topological boundaries and
	// generate polygons for geometry partitioning.
	resolved_topological_boundary_seq_type::const_iterator rtb_iter = all_resolved_boundaries.begin();
	resolved_topological_boundary_seq_type::const_iterator rtb_end = all_resolved_boundaries.end();
	for ( ; rtb_iter != rtb_end; ++rtb_iter)
	{
		const ResolvedTopologicalBoundary *rtb = *rtb_iter;

		const ResolvedBoundaryForGeometryPartitioning resolved_boundary_for_geometry_partitioning(
				rtb,
				GPlatesMaths::PolygonIntersections::create(rtb->resolved_topology_geometry()));

		resolved_boundaries_query->resolved_boundaries.push_back(
				resolved_boundary_for_geometry_partitioning);
	}

	return resolved_boundaries_query;
}


bool
GPlatesAppLogic::TopologyUtils::find_resolved_topology_boundaries_containing_point(
		resolved_topological_boundary_seq_type &resolved_topological_boundaries_containing_point,
		const GPlatesMaths::PointOnSphere &point,
		const resolved_boundaries_for_geometry_partitioning_query_type &resolved_boundaries_query)
{
	// Return early if query does not exist.
	if (!resolved_boundaries_query)
	{
		return false;
	}

	bool was_point_contained_by_any_polygons = false;

	// Iterate through the resolved geometries and see which ones (polygon) contain the point.
	ResolvedBoundariesForGeometryPartitioning::resolved_boundary_seq_type::const_iterator rtb_iter =
			resolved_boundaries_query->resolved_boundaries.begin();
	ResolvedBoundariesForGeometryPartitioning::resolved_boundary_seq_type::const_iterator rtb_end =
			resolved_boundaries_query->resolved_boundaries.end();
	for ( ; rtb_iter != rtb_end; ++rtb_iter)
	{
		const ResolvedBoundaryForGeometryPartitioning &rtb = *rtb_iter;

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
GPlatesAppLogic::TopologyUtils::partition_geometry_using_resolved_topology_boundaries(
		resolved_boundary_partitioned_geometries_seq_type &resolved_boundary_partitioned_geometries_seq,
		partitioned_geometry_seq_type &partitioned_outside_geometries,
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
		const resolved_boundaries_for_geometry_partitioning_query_type &resolved_boundaries_query)
{
	// Return early if query does not exist.
	if (!resolved_boundaries_query)
	{
		return false;
	}

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
			resolved_boundaries_query->resolved_boundaries.begin();
	ResolvedBoundariesForGeometryPartitioning::resolved_boundary_seq_type::const_iterator rtb_end =
			resolved_boundaries_query->resolved_boundaries.end();
	for ( ; rtb_iter != rtb_end; ++rtb_iter)
	{
		const ResolvedBoundaryForGeometryPartitioning &rtb = *rtb_iter;

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


boost::optional<GPlatesModel::integer_plate_id_type>
GPlatesAppLogic::TopologyUtils::find_reconstruction_plate_id_furthest_from_anchor_in_plate_circuit(
		const resolved_topological_boundary_seq_type &resolved_boundaries)
{
	std::vector<GPlatesModel::integer_plate_id_type> reconstruction_plate_ids;
	reconstruction_plate_ids.reserve(resolved_boundaries.size());

	// Loop over the resolved topological boundaries.
	GPlatesAppLogic::TopologyUtils::resolved_topological_boundary_seq_type::const_iterator rtb_iter;
	for (rtb_iter = resolved_boundaries.begin();
		rtb_iter != resolved_boundaries.end();
		++rtb_iter)
	{
		const ResolvedTopologicalBoundary *rtb = *rtb_iter;

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


GPlatesAppLogic::TopologyUtils::ResolvedBoundaryPartitionedGeometries::ResolvedBoundaryPartitionedGeometries(
		const ResolvedTopologicalBoundary *resolved_topological_boundary_) :
	resolved_topological_boundary(resolved_topological_boundary_)
{
}


bool
GPlatesAppLogic::TopologyUtils::is_topological_network_feature(
		const GPlatesModel::FeatureHandle &feature)
{
	// FIXME: Do this check based on feature properties rather than feature type.
	// So if something looks like a TCPB (because it has a topology polygon property)
	// then treat it like one. For this to happen we first need TopologicalNetwork to
	// use a property type different than TopologicalPolygon.
	//
	static const QString type("TopologicalNetwork");

	return type == GPlatesUtils::make_qstring_from_icu_string(feature.feature_type().get_name());
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
		const GPlatesModel::FeatureHandle &feature_handle = **feature_collection_iter;

		if (is_topological_network_feature(feature_handle))
		{ 
			return true; 
		}
	}

	return false;
}


GPlatesAppLogic::ReconstructionGeometryCollection::non_null_ptr_type
GPlatesAppLogic::TopologyUtils::resolve_topological_networks(
		ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
				topological_network_features_collection)
{
	ReconstructionGeometryCollection::non_null_ptr_type reconstruction_geom_collection =
			ReconstructionGeometryCollection::create(reconstruction_tree);

	// Visit topological network features.
	TopologyNetworkResolver topology_network_resolver(*reconstruction_geom_collection);

	AppLogicUtils::visit_feature_collections(
		topological_network_features_collection.begin(),
		topological_network_features_collection.end(),
		topology_network_resolver);

	return reconstruction_geom_collection;
}


GPlatesAppLogic::TopologyUtils::resolved_networks_for_interpolation_query_type
GPlatesAppLogic::TopologyUtils::query_resolved_topology_networks_for_interpolation(
		ReconstructionGeometryCollection &reconstruction_geometry_collection,
		const map_point_to_scalars_function_type &map_point_to_scalars_function,
		const unsigned int num_mapped_scalars_per_point,
		const boost::any &map_point_to_scalars_user_data,
		const network_interpolation_query_callback_type &
				network_interpolation_query_callback)
{
	// Get all ResolvedTopologicalNetwork objects in the reconstruction.
	resolved_topological_network_seq_type resolved_networks;
	ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
				reconstruction_geometry_collection.begin(),
				reconstruction_geometry_collection.end(),
				resolved_networks);

	if (resolved_networks.empty())
	{
		// Return empty query.
		return resolved_networks_for_interpolation_query_type();
	}

	// Query structure to return to the caller.
	resolved_networks_for_interpolation_query_type resolved_networks_query;

	// Create a new query to be filled.
	resolved_networks_query.reset(new ResolvedNetworksForInterpolationQuery());

	// Iterate through the resolved topological networks and add the network points to
	// a CGAL delaunay triangulation.
	resolved_topological_network_seq_type::const_iterator network_iter = resolved_networks.begin();
	resolved_topological_network_seq_type::const_iterator network_end = resolved_networks.end();
	for ( ; network_iter != network_end; ++network_iter)
	{
		ResolvedTopologicalNetwork *network = *network_iter;

		resolved_network_for_interpolation_query_type resolved_network_query(
				new ResolvedNetworkForInterpolationQuery(network));

		// Create new CGAL network point-to-scalar mappings for the current network.
		// Each of these mappings will map network points to a component of the
		// tuple returned by the caller's 'map_point_to_scalars_function' function.
		// For example, 3D velocity would have three scalars per network point to represent
		// the velocity at each network point.
		resolved_network_query->scalar_map_seq.reserve(num_mapped_scalars_per_point);
		for (unsigned int scalar_index = 0;
			scalar_index < num_mapped_scalars_per_point;
			++scalar_index)
		{
			resolved_network_query->scalar_map_seq.push_back(
					ResolvedNetworkForInterpolationQuery::scalar_map_ptr_type(
							new CgalUtils::cgal_map_point_to_value_type()));
		}

		// Iterate through the nodes of the current topological network.
		ResolvedTopologicalNetwork::node_const_iterator nodes_iter = network->nodes_begin();
		ResolvedTopologicalNetwork::node_const_iterator nodes_end = network->nodes_end();
		for ( ; nodes_iter != nodes_end; ++nodes_iter)
		{
			const ResolvedTopologicalNetwork::Node &node = *nodes_iter;

			// Get the node's geometry points.
			std::vector<GPlatesMaths::PointOnSphere> node_points;
			GPlatesAppLogic::GeometryUtils::get_geometry_points(
					*node.get_geometry(), node_points);

			// Get the feature referenced by the node.
			const GPlatesModel::FeatureHandle::const_weak_ref &node_feature_ref =
					node.get_feature_ref();

			// Iterate through the points of the current node.
			std::vector<GPlatesMaths::PointOnSphere>::const_iterator node_points_iter =
					node_points.begin();
			std::vector<GPlatesMaths::PointOnSphere>::const_iterator node_points_end =
					node_points.end();
			for ( ; node_points_iter != node_points_end; ++node_points_iter)
			{
				const GPlatesMaths::PointOnSphere &node_point = *node_points_iter;

				// Call the caller's function to map the node point to one or more scalars.
				// We pass in the node's feature reference since the function might
				// do calculations specific to that feature.
				const std::vector<double> scalars = map_point_to_scalars_function(
						node_point,
						node_feature_ref,
						map_point_to_scalars_user_data);

				// The caller's function should always return the number of scalars
				// that it says it will.
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						scalars.size() == num_mapped_scalars_per_point,
						GPLATES_ASSERTION_SOURCE);

				const CgalUtils::cgal_point_2_type cgal_node_point_2 =
						CgalUtils::convert_point_to_cgal(node_point);

				// Iterate over the scalars and insert each one into its own map.
				for (unsigned int scalar_index = 0; scalar_index < scalars.size(); ++scalar_index)
				{
					const double scalar = scalars[scalar_index];

					// Insert the scalar into the map associated with the current scalar slot.
					resolved_network_query->scalar_map_seq[scalar_index]->insert(
							std::make_pair(cgal_node_point_2, scalar));
				}
			}
		}

		// Store the query for the current network in a global query for all networks.
		resolved_networks_query->resolved_networks.push_back(resolved_network_query);

		// Tell the caller about the current network query and which network it's for.
		if (network_interpolation_query_callback)
		{
			network_interpolation_query_callback(network, resolved_network_query);
		}
	}

	return resolved_networks_query;
}


void
GPlatesAppLogic::TopologyUtils::get_resolved_topology_network_scalars(
		const resolved_network_for_interpolation_query_type &resolved_network_query,
		std::vector<GPlatesMaths::PointOnSphere> &network_points,
		std::vector< std::vector<double> > &network_scalar_tuple_sequence)
{
	// Return early if query is not set.
	if (!resolved_network_query)
	{
		return;
	}

	const ResolvedNetworkForInterpolationQuery &resolved_network = *resolved_network_query;

	const std::size_t num_scalars = resolved_network.scalar_map_seq.size();
	if (num_scalars == 0)
	{
		return;
	}

	// Each scalar map should contain 'num_network_points' elements so just need to
	// look at the first to determine it.
	const std::size_t num_network_points = resolved_network.scalar_map_seq[0]->size();
	if (num_network_points == 0)
	{
		return;
	}

	// Reserve memory in the caller's arrays.
	network_points.reserve(num_network_points);
	network_scalar_tuple_sequence.reserve(num_network_points);

	// Keep track of the scalar map iterators as we will be processing them
	// in sync with each other.
	typedef CgalUtils::cgal_map_point_to_value_type::const_iterator scalar_map_iter_type;
	std::vector<scalar_map_iter_type> scalar_map_iters;
	scalar_map_iters.reserve(num_scalars);
	for (std::size_t scalar_map_iter_index = 0;
		scalar_map_iter_index < num_scalars;
		++scalar_map_iter_index)
	{
		// Each scalar map should contain 'num_network_points' elements.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				resolved_network.scalar_map_seq[scalar_map_iter_index]->size() == num_network_points,
				GPLATES_ASSERTION_SOURCE);

		scalar_map_iters.push_back(
				resolved_network.scalar_map_seq[scalar_map_iter_index]->begin());
	}

	// Progress through the network points and gather the scalar_tuple from each scalar map.
	for (std::size_t network_point_index = 0;
		network_point_index < num_network_points;
		++network_point_index)
	{
		std::vector<double> scalar_tuple;
		scalar_tuple.reserve(num_scalars);

		// All the scalar maps should have the same sequence of keys (which is the
		// network point) so just get it from the first scalar map.
		const CgalUtils::cgal_point_2_type &cgal_network_point_2 = scalar_map_iters[0]->first;
		const GPlatesMaths::PointOnSphere network_point =
				CgalUtils::convert_point_from_cgal(cgal_network_point_2);

		// Iterate through the scalar maps and collect each mapped scalar.
		for (std::size_t scalar_map_iter_index = 0;
			scalar_map_iter_index < num_scalars;
			++scalar_map_iter_index)
		{
			scalar_map_iter_type &scalar_map_iter = scalar_map_iters[scalar_map_iter_index];

			const double &scalar = scalar_map_iter->second;
			scalar_tuple.push_back(scalar);

			// Move the scalar map iterator to the next in the sequence.
			++scalar_map_iter;
		}

		// Add the network point and corresponding scalar tuple to the caller's arrays.
		network_points.push_back(network_point);
		network_scalar_tuple_sequence.push_back(scalar_tuple);
	}
}


boost::optional< std::vector<double> >
GPlatesAppLogic::TopologyUtils::interpolate_resolved_topology_networks(
		const resolved_networks_for_interpolation_query_type &resolved_networks_query,
		const GPlatesMaths::PointOnSphere &point)
{
	// Return early if query does not exist.
	if (!resolved_networks_query)
	{
		return boost::none;
	}

	const CgalUtils::cgal_point_2_type cgal_point_2 = CgalUtils::convert_point_to_cgal(point);

	// Iterate through the resolved networks.
	ResolvedNetworksForInterpolationQuery::resolved_network_seq_type::const_iterator rtn_iter =
			resolved_networks_query->resolved_networks.begin();
	ResolvedNetworksForInterpolationQuery::resolved_network_seq_type::const_iterator rtn_end =
			resolved_networks_query->resolved_networks.end();
	for ( ; rtn_iter != rtn_end; ++rtn_iter)
	{
		const resolved_network_for_interpolation_query_type &resolved_network_query = *rtn_iter;

		const boost::optional< std::vector<double> > interpolated_scalars =
				interpolate_resolved_topology_network(resolved_network_query, cgal_point_2);

		// Continue to the next network if the point is not in the current network.
		if (interpolated_scalars)
		{
			return interpolated_scalars;
		}
	}

	// Point was not in any topology networks.
	return boost::none;
}


boost::optional< std::vector<double> >
GPlatesAppLogic::TopologyUtils::interpolate_resolved_topology_network(
		const resolved_network_for_interpolation_query_type &resolved_network_query,
		const GPlatesMaths::PointOnSphere &point)
{
	const CgalUtils::cgal_point_2_type cgal_point_2 = CgalUtils::convert_point_to_cgal(point);

	return interpolate_resolved_topology_network(resolved_network_query, cgal_point_2);
}
