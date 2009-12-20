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
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/PolygonIntersections.h"

#include "model/FeatureCollectionHandle.h"


namespace GPlatesMaths
{
	class PointOnSphere;
	class PolygonOnSphere;
	class PolylineOnSphere;
}

namespace GPlatesModel
{
	class Reconstruction;
	class ResolvedTopologicalGeometry;
}

namespace GPlatesAppLogic
{
	/**
	 * This namespace contains utilities that clients of topology-related functionality use.
	 */
	namespace TopologyUtils
	{
		//! Typedef for a sequence of resolved topological geometries.
		typedef std::vector<const GPlatesModel::ResolvedTopologicalGeometry *>
				resolved_topological_geometry_seq_type;

		//! Typedef for a sequence of 'PolylineOnSphere::non_null_ptr_to_const_type'.
		typedef GPlatesMaths::PolygonIntersections::partitioned_polyline_seq_type
				partitioned_polyline_seq_type;


		class ResolvedBoundaryPartitionedPolylines
		{
		public:
			ResolvedBoundaryPartitionedPolylines(
					const GPlatesModel::ResolvedTopologicalGeometry *resolved_topological_boundary);

			//! The resolved topological boundary that partitioned the polylines.
			const GPlatesModel::ResolvedTopologicalGeometry *resolved_topological_boundary;

			//! The partitioned polylines.
			partitioned_polyline_seq_type partitioned_inside_polylines;
		};


		/**
		 * Typedef for a sequence of associations of partitioned polylines and
		 * the @a ResolvedTopologicalGeometry that they were partitioned into.
		 */
		typedef std::list<const ResolvedBoundaryPartitionedPolylines>
				resolved_boundary_partitioned_polylines_seq_type;


		class ResolvedGeometriesForGeometryPartitioning;
		/**
		 * Typedef for the results of finding resolved topologies (and creating
		 * a structure optimised for point-in-polygon tests).
		 */
		typedef boost::shared_ptr<ResolvedGeometriesForGeometryPartitioning>
				resolved_geometries_for_geometry_partitioning_query_type;


		/**
		 * Finds all topological closed plate boundary features in @a topological_features_collection
		 * that exist at time @a reconstruction_time and creates @a ResolvedTopologicalGeometry objects
		 * for each one and stores them in @a reconstruction.
		 *
		 * @pre the features referenced by any of these topological closed plate boundary features
		 *      must have already been reconstructed and exist in @a reconstruction.
		 */
		void
		resolve_topologies(
				const double &reconstruction_time,
				GPlatesModel::Reconstruction &reconstruction,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
						topological_features_collection);


		/**
		 * Returns true if @a reconstruction contains resolved topological boundaries.
		 *
		 * These are required for any cookie-cutting operations.
		 */
		bool
		has_resolved_topological_boundaries(
				GPlatesModel::Reconstruction &reconstruction);


		/**
		 * Finds all @a ResolvedTopologicalGeometry objects in @a reconstruction
		 * and returns a structure for partitioning geometry using the polygon
		 * of each @a ResolvedTopologicalGeometry.
		 *
		 * The returned structure can tested tested like a bool - it's true
		 * if any @a ResolvedTopologicalGeometry objects are found in @a reconstruction.
		 */
		resolved_geometries_for_geometry_partitioning_query_type
		query_resolved_topologies_for_geometry_partitioning(
				GPlatesModel::Reconstruction &reconstruction);


		/**
		 * Searches all @a ResolvedTopologicalGeometry objects in @a resolved_geoms_query
		 * (returned from a call to @a query_resolved_topologies_for_geometry_partitioning)
		 * to see which ones contain @a point and returns any found in
		 * @a resolved_topological_geom_seq.
		 *
		 * Returns true if any are found.
		 */
		bool
		find_resolved_topologies_containing_point(
				resolved_topological_geometry_seq_type &resolved_topological_geom_seq,
				const GPlatesMaths::PointOnSphere &point,
				const resolved_geometries_for_geometry_partitioning_query_type &resolved_geoms_query);


		/**
		 * Partition @a polyline into the resolved topological boundaries.
		 *
		 * The boundaries are assumed to not overlap each other - if they do then
		 * which overlapping resolved boundary it is partitioned into is undefined.
		 *
		 * On returning @a partitioned_outside_polylines contains any partitioned
		 * polylines that are not inside any resolved boundaries.
		 *
		 * Returns true if @a polyline is inside any resolved boundaries in which
		 * case elements were appended to @a resolved_boundary_partitioned_polylines_seq.
		 */
		bool
		partition_polyline_using_resolved_topology_boundaries(
				resolved_boundary_partitioned_polylines_seq_type &
						resolved_boundary_partitioned_polylines_seq,
				partitioned_polyline_seq_type &partitioned_outside_polylines,
				const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type &polyline,
				const resolved_geometries_for_geometry_partitioning_query_type &resolved_geoms_query);


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
				const resolved_topological_geometry_seq_type &resolved_boundaries);
	}
}

#endif // GPLATES_APP_LOGIC_TOPOLOGYUTILS_H
