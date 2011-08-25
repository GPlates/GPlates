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
 
#ifndef GPLATES_APP_LOGIC_TOPOLOGYUTILS_H
#define GPLATES_APP_LOGIC_TOPOLOGYUTILS_H

#include <list>
#include <vector>
#include <boost/any.hpp>
#include <boost/function.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "AppLogicFwd.h"
#include "ReconstructHandle.h"

#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/PolygonIntersections.h"

#include "model/FeatureCollectionHandle.h"


namespace GPlatesAppLogic
{
	/**
	 * This namespace contains utilities that clients of topology-related functionality use.
	 */
	namespace TopologyUtils
	{
		//
		// Topological boundaries
		//

		//! Typedef for a sequence of resolved topological boundaries.
		typedef std::vector<const ResolvedTopologicalBoundary *>
				resolved_topological_boundary_seq_type;

		//! Typedef for a sequence of 'GeometryOnSphere::non_null_ptr_to_const_type'.
		typedef GPlatesMaths::PolygonIntersections::partitioned_geometry_seq_type
				partitioned_geometry_seq_type;


		class ResolvedBoundaryPartitionedGeometries
		{
		public:
			ResolvedBoundaryPartitionedGeometries(
					const ResolvedTopologicalBoundary *resolved_topological_boundary);

			//! The resolved topological boundary that partitioned the polylines.
			const ResolvedTopologicalBoundary *resolved_topological_boundary;

			//! The partitioned geometries.
			partitioned_geometry_seq_type partitioned_inside_geometries;
		};


		/**
		 * Typedef for a sequence of associations of partitioned geometries and
		 * the @a ResolvedTopologicalBoundary that they were partitioned into.
		 */
		typedef std::list<ResolvedBoundaryPartitionedGeometries>
				resolved_boundary_partitioned_geometries_seq_type;

		class ResolvedBoundariesForGeometryPartitioning;
		/**
		 * Typedef for the results of finding resolved topology boundaries for
		 * geometry partitioning.
		 */
		typedef boost::shared_ptr<ResolvedBoundariesForGeometryPartitioning>
				resolved_boundaries_for_geometry_partitioning_query_type;


		/**
		 * Returns true if @a feature is a topological closed plate boundary feature.
		 */
		bool
		is_topological_closed_plate_boundary_feature(
				const GPlatesModel::FeatureHandle &feature);


		/**
		 * Returns true if @a feature_collection contains topological closed plate boundary features.
		 */
		bool
		has_topological_closed_plate_boundary_features(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection);


		/**
		 * Create and return a sequence of @a ResolvedTopologicalBoundary objects by resolving
		 * topological closed plate boundaries in @a topological_closed_plate_polygon_features_collection.
		 *
		 * The boundaries are resolved by referencing already reconstructed topological boundary
		 * section features in @a reconstructed_topological_boundary_sections.
		 *
		 * @param reconstruction_tree is associated with the output resolved topological boundaries.
		 * @param topological_sections_reconstruct_handles is a list of reconstruct handles that
		 *        identifies the subset, of all RFGs observing the topological section features,
		 *        that should be searched when resolving the topological boundaries.
		 *        This is useful to avoid outdated RFGs still in existence among other scenarios.
		 */
		void
		resolve_topological_boundaries(
				std::vector<resolved_topological_boundary_non_null_ptr_type> &resolved_topological_boundaries,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &topological_closed_plate_polygon_features_collection,
				const reconstruction_tree_non_null_ptr_to_const_type &reconstruction_tree,
				boost::optional<const std::vector<ReconstructHandle::type> &> topological_sections_reconstruct_handles = boost::none);


		/**
		 * Returns a structure for partitioning geometry using the polygon
		 * of each @a ResolvedTopologicalBoundary in @a resolved_topological_boundaries.
		 *
		 * The returned structure can tested tested like a bool - it's true if
		 * @a resolved_topological_boundaries is not empty.
		 */
		resolved_boundaries_for_geometry_partitioning_query_type
		query_resolved_topologies_for_geometry_partitioning(
				const std::vector<resolved_topological_boundary_non_null_ptr_type> &resolved_topological_boundaries);


		/**
		 * Searches all @a ResolvedTopologicalBoundary objects in @a resolved_geoms_query
		 * (returned from a call to @a query_resolved_topologies_for_geometry_partitioning)
		 * to see which ones contain @a point and returns any found in
		 * @a resolved_topological_boundary_seq.
		 *
		 * Returns true if any are found.
		 */
		bool
		find_resolved_topology_boundaries_containing_point(
				resolved_topological_boundary_seq_type &resolved_topological_boundary_seq,
				const GPlatesMaths::PointOnSphere &point,
				const resolved_boundaries_for_geometry_partitioning_query_type &resolved_boundaries_query);


		/**
		 * Partition @a geometry into the resolved topological boundaries.
		 *
		 * The boundaries are assumed to not overlap each other - if they do then
		 * which overlapping resolved boundary it is partitioned into is undefined.
		 *
		 * On returning @a partitioned_outside_geometries contains any partitioned
		 * geometries that are not inside any resolved boundaries.
		 *
		 * Returns true if @a geometry is inside any resolved boundaries (even partially)
		 * in which case elements were appended to
		 * @a resolved_boundary_partitioned_geometries_seq.
		 */
		bool
		partition_geometry_using_resolved_topology_boundaries(
				resolved_boundary_partitioned_geometries_seq_type &
						resolved_boundary_partitioned_geometries_seq,
				partitioned_geometry_seq_type &partitioned_outside_geometries,
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
				const resolved_boundaries_for_geometry_partitioning_query_type &resolved_boundaries_query);


		/**
		 * Searches through @a resolved_boundaries and finds the 'reconstructionPlateId'
		 * belonging to the plate that is furthest from the anchor (or root) of the rotation tree.
		 *
		 * This function is usually called after @a find_resolved_topologies_containing_point.
		 *
		 * NOTE: Currently this test is done by looking for the highest plate id number - this
		 * may need to change in future if a new plate id scheme is used.
		 *
		 * The reason for choosing the plate that is furthest from the anchor is, I think,
		 * because it provides more detailed rotations since plates further down the plate
		 * circuit (or tree) are relative to plates higher up the tree and hence provide
		 * extra detail.
		 *
		 * Ideally the plate boundaries shouldn't overlap at all but it is possible to construct
		 * a set that do overlap.
		 *
		 * Returns false if none of the resolved boundaries have a 'reconstructionPlateId' -
		 * they all should unless the TopologicalClosedPlateBoundary feature they reference
		 * does not have one for some reason.
		 */
		boost::optional< std::pair<
				GPlatesModel::integer_plate_id_type,
				const ResolvedTopologicalBoundary * > >
		find_reconstruction_plate_id_furthest_from_anchor_in_plate_circuit(
				const resolved_topological_boundary_seq_type &resolved_boundaries);


		//
		// Topological networks
		//

		//! Typedef for a sequence of resolved topological networks.
		typedef std::vector<const ResolvedTopologicalNetwork *>
				resolved_topological_network_seq_type;

		class ResolvedNetworkForInterpolationQuery;
		/**
		 * Typedef for an interpolation query of a single resolved topology network.
		 */
		typedef boost::shared_ptr<ResolvedNetworkForInterpolationQuery>
				resolved_network_for_interpolation_query_shared_ptr_type;

		class ResolvedNetworksForInterpolationQuery;
		/**
		 * Typedef for interpolation queries of resolved topology networks.
		 */
		typedef boost::shared_ptr<ResolvedNetworksForInterpolationQuery>
				resolved_networks_for_interpolation_query_type;

		/**
		 * A function that accepts a point and a feature reference and returns one or more
		 * floating-point scalars.
		 *
		 * There is also a boost::any parameter that is passed directly from the
		 * @a query_resolved_topology_networks_for_interpolation function (that
		 * invokes this function).
		 *
		 * For example, a function to calculate a 3D velocity vector would
		 * return three numbers (for the x, y and z components).
		 */
		typedef std::vector<double> map_point_to_scalars_function_signature(
				const GPlatesMaths::PointOnSphere &,
				const GPlatesModel::FeatureHandle::const_weak_ref &,
				const boost::any &);
		typedef boost::function<map_point_to_scalars_function_signature>
				map_point_to_scalars_function_type;

		/**
		 * A function that accepts a ResolvedTopologicalNetworkImpl pointer as
		 * the first argument and a @a resolved_network_for_interpolation_query_shared_ptr_type as
		 * the second argument.
		 */
		typedef void network_interpolation_query_callback_signature(
				const ResolvedTopologicalNetwork *,
				const resolved_network_for_interpolation_query_shared_ptr_type &);
		typedef boost::function<network_interpolation_query_callback_signature>
				network_interpolation_query_callback_type;


		/**
		 * Returns true if @a feature is a topological network feature.
		 */
		bool
		is_topological_network_feature(
				const GPlatesModel::FeatureHandle &feature);


		/**
		 * Returns true if @a feature_collection contains topological network features.
		 */
		bool
		has_topological_network_features(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection);


		/**
		 * Create and return a sequence of @a ResolvedTopologicalNetwork objects by resolving
		 * topological networks in @a topological_network_features_collection.
		 *
		 * The sections are resolved by referencing already reconstructed topological
		 * section features in @a reconstructed_topological_sections.
		 *
		 * @param reconstruction_tree is associated with the output resolved topological networks.
		 * @param topological_sections_reconstruct_handles is a list of reconstruct handles that
		 *        identifies the subset, of all RFGs observing the topological section features,
		 *        that should be searched when resolving the topological boundaries.
		 *        This is useful to avoid outdated RFGs still in existence among other scenarios.
		 */
		void
		resolve_topological_networks(
				std::vector<resolved_topological_network_non_null_ptr_type> &resolved_topological_networks,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &topological_network_features_collection,
				const reconstruction_tree_non_null_ptr_to_const_type &reconstruction_tree,
				boost::optional<const std::vector<ReconstructHandle::type> &> topological_sections_reconstruct_handles);


		/**
		 * Generates a mapping from each point in each network in @a resolved_topological_networks
		 * to a tuple of scalars using the function @a map_point_to_scalars_function.
		 *
		 * The returned structure can be passed to @a interpolate_resolved_topology_networks.
		 *
		 * @a num_mapped_scalars_per_point should be the number of scalars returned by the
		 * function @a map_point_to_scalars_function.
		 *
		 * @a map_point_to_scalars_user_data is arbitrary data (decided by the caller) that
		 * is passed directly to the @a map_point_to_scalars_function function whenever
		 * it is invoked internally. This is effectively any information needed by that
		 * function that it cannot obtain from the feature reference that is also passed to it.
		 *
		 * @a network_interpolation_query_callback is an optional function that takes
		 * a ResolvedTopologicalNetworkImpl pointer as the first argument and a
		 * @a resolved_network_for_interpolation_query_shared_ptr_type as a second argument and
		 * can be used to let the caller know about the individual network queries.
		 *
		 * The returned structure can be tested like a bool - it's true if 
		 * @a resolved_topological_networks is not empty.
		 *
		 * For example, to calculate 3D velocity vectors for each point in each network
		 * you would pass a function that returned three scalars and you would set
		 * @a num_mapped_scalars_per_point to three (for the x, y and z components).
		 */
		resolved_networks_for_interpolation_query_type
		query_resolved_topology_networks_for_interpolation(
				const std::vector<resolved_topological_network_non_null_ptr_type> &resolved_topological_networks,
				const map_point_to_scalars_function_type &map_point_to_scalars_function,
				const unsigned int num_mapped_scalars_per_point,
				const boost::any &map_point_to_scalars_user_data,
				const network_interpolation_query_callback_type &network_interpolation_query_callback =
						network_interpolation_query_callback_type());


		/**
		 * Returns the scalars filled in by @a query_resolved_topology_networks_for_interpolation
		 * for a specific resolved topological network identified by @a resolved_network_query.
		 *
		 * @post The network points and the scalar tuples at each point are returned in
		 *       @a network_points and @a network_scalar_tuple_sequence and both these
		 *       sequences are in the same order and have the same size.
		 */
		void
		get_resolved_topology_network_scalars(
				const resolved_network_for_interpolation_query_shared_ptr_type &resolved_network_query,
				std::vector<GPlatesMaths::PointOnSphere> &network_points,
				std::vector< std::vector<double> > &network_scalar_tuple_sequence);


		/**
		 * Iterates through all resolved networks in @a resolved_networks_query
		 * and finds the first network that contains @a point.
		 *
		 * For this network it interpolates the network points nearest @a point in that network
		 * and uses that to interpolate the mapped scalars for those points.
		 *
		 * The interpolated scalars are returned and the number of scalars is the
		 * 'num_mapped_scalars_per_point' parameters passed to
		 * @a query_resolved_topology_networks_for_interpolation.
		 *
		 * Returns false if @a point is not in any of the resolved networks.
		 *
		 * For example, if interpolating 3D velocity vectors this function would
		 * return three scalars (for the x, y and z velocity components).
		 */
		boost::optional< std::vector<double> >
		interpolate_resolved_topology_networks(
				const resolved_networks_for_interpolation_query_type &resolved_networks_query,
				const GPlatesMaths::PointOnSphere &point);

		// as above , using the 2D+C triangulation for inteprolation
		boost::optional< std::vector<double> >
		interpolate_resolved_topology_networks_constrained(
				const resolved_networks_for_interpolation_query_type &resolved_networks_query,
				const GPlatesMaths::PointOnSphere &point);


		/**
		 * For this network it interpolates the network points nearest @a point in that network
		 * and uses that to interpolate the mapped scalars for those points.
		 *
		 * The interpolated scalars are returned and the number of scalars is the
		 * 'num_mapped_scalars_per_point' parameters passed to
		 * @a query_resolved_topology_networks_for_interpolation.
		 *
		 * Returns false if @a point is in the resolved network.
		 *
		 * For example, if interpolating 3D velocity vectors this function would
		 * return three scalars (for the x, y and z velocity components).
		 */
		boost::optional< std::vector<double> >
		interpolate_resolved_topology_network(
				const resolved_network_for_interpolation_query_shared_ptr_type &resolved_network_query,
				const GPlatesMaths::PointOnSphere &point);
	}
}

#endif // GPLATES_APP_LOGIC_TOPOLOGYUTILS_H
