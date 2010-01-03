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
 
#ifndef GPLATES_APP_LOGIC_TOPOLOGYUTILS_H
#define GPLATES_APP_LOGIC_TOPOLOGYUTILS_H

#include <list>
#include <vector>
#include <boost/any.hpp>
#include <boost/function.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/PolygonIntersections.h"


#include "model/FeatureCollectionHandle.h"


namespace GPlatesModel
{
	class Reconstruction;
	class ResolvedTopologicalBoundary;
	class ResolvedTopologicalNetwork;
	class ResolvedTopologicalNetworkImpl;
}

namespace GPlatesAppLogic
{
	/**
	 * This namespace contains utilities that clients of topology-related functionality use.
	 */
	namespace TopologyUtils
	{
		/**
		 * Finds all topological closed plate boundary features and network features
		 * in @a topological_features_collection that exist at time @a reconstruction_time and
		 * creates @a ResolvedTopologicalBoundary objects and @a ResolvedTopologicalNetwork
		 * objects respectively for each one found and stores them in @a reconstruction.
		 *
		 * @pre the features referenced by any of these topological features
		 *      must have already been reconstructed and currently exist in @a reconstruction.
		 */
		void
		resolve_topologies(
				const double &reconstruction_time,
				GPlatesModel::Reconstruction &reconstruction,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
						topological_features_collection);

		//
		// Topological boundaries
		//

		//! Typedef for a sequence of resolved topological boundaries.
		typedef std::vector<const GPlatesModel::ResolvedTopologicalBoundary *>
				resolved_topological_boundary_seq_type;

		//! Typedef for a sequence of 'GeometryOnSphere::non_null_ptr_to_const_type'.
		typedef GPlatesMaths::PolygonIntersections::partitioned_geometry_seq_type
				partitioned_geometry_seq_type;


		class ResolvedBoundaryPartitionedGeometries
		{
		public:
			ResolvedBoundaryPartitionedGeometries(
					const GPlatesModel::ResolvedTopologicalBoundary *resolved_topological_boundary);

			//! The resolved topological boundary that partitioned the polylines.
			const GPlatesModel::ResolvedTopologicalBoundary *resolved_topological_boundary;

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
		 * Returns true if @a reconstruction contains resolved topological boundaries.
		 *
		 * These are required for any cookie-cutting operations.
		 */
		bool
		has_resolved_topological_boundaries(
				GPlatesModel::Reconstruction &reconstruction);


		/**
		 * Finds all @a ResolvedTopologicalBoundary objects in @a reconstruction
		 * and returns a structure for partitioning geometry using the polygon
		 * of each @a ResolvedTopologicalBoundary.
		 *
		 * The returned structure can tested tested like a bool - it's true
		 * if any @a ResolvedTopologicalBoundary objects are found in @a reconstruction.
		 */
		resolved_boundaries_for_geometry_partitioning_query_type
		query_resolved_topologies_for_geometry_partitioning(
				GPlatesModel::Reconstruction &reconstruction);


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
		boost::optional<GPlatesModel::integer_plate_id_type>
		find_reconstruction_plate_id_furthest_from_anchor_in_plate_circuit(
				const resolved_topological_boundary_seq_type &resolved_boundaries);


		//
		// Topological networks
		//

		//! Typedef for a sequence of resolved topological networks.
		typedef std::vector<GPlatesModel::ResolvedTopologicalNetwork *>
				resolved_topological_network_seq_type;

		class ResolvedNetworkForInterpolationQuery;
		/**
		 * Typedef for an interpolation query of a single resolved topology network.
		 */
		typedef boost::shared_ptr<ResolvedNetworkForInterpolationQuery>
				resolved_network_for_interpolation_query_type;

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
		 * the first argument and a @a resolved_network_for_interpolation_query_type as
		 * the second argument.
		 */
		typedef void network_interpolation_query_callback_signature(
				GPlatesModel::ResolvedTopologicalNetworkImpl *,
				const resolved_network_for_interpolation_query_type &);
		typedef boost::function<network_interpolation_query_callback_signature>
				network_interpolation_query_callback_type;


		/**
		 * Finds all @a ResolvedTopologicalNetwork objects in @a reconstruction
		 * and generates a mapping from each point in each network to a tuple of scalars
		 * using the function @a map_point_to_scalars_function.
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
		 * @a resolved_network_for_interpolation_query_type as a second argument and
		 * can be used to let the caller know about the individual network queries.
		 *
		 * The returned structure can be tested like a bool - it's true
		 * if any @a ResolvedTopologicalNetwork objects are found in @a reconstruction.
		 *
		 * For example, to calculate 3D velocity vectors for each point in each network
		 * you would pass a function that returned three scalars and you would set
		 * @a num_mapped_scalars_per_point to three (for the x, y and z components).
		 */
		resolved_networks_for_interpolation_query_type
		query_resolved_topology_networks_for_interpolation(
				GPlatesModel::Reconstruction &reconstruction,
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
				const resolved_network_for_interpolation_query_type &resolved_network_query,
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
				const resolved_network_for_interpolation_query_type &resolved_network_query,
				const GPlatesMaths::PointOnSphere &point);


		//! Typedef for a sequence of resolved topological networks.
		typedef std::vector<GPlatesModel::ResolvedTopologicalNetworkImpl *>
				resolved_topological_network_impl_seq_type;

		/**
		 * Finds all @a ResolvedTopologicalNetworkImpl objects in @a reconstruction.
		 *
		 * NOTE: These are the full topological networks - currently
		 * @a ResolvedTopologicalNetwork only refers to a part of the network and
		 * multiple @a ResolvedTopologicalNetwork objects can refer to/share the same
		 * @a ResolvedTopologicalNetworkImpl object.
		 */
		void
		find_resolved_topological_network_impls(
				resolved_topological_network_impl_seq_type &,
				const GPlatesModel::Reconstruction &reconstruction);
	}
}

#endif // GPLATES_APP_LOGIC_TOPOLOGYUTILS_H
