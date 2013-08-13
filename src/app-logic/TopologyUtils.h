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
 
#ifndef GPLATES_APP_LOGIC_TOPOLOGYUTILS_H
#define GPLATES_APP_LOGIC_TOPOLOGYUTILS_H

#include <list>
#include <utility>
#include <vector>
#include <boost/optional.hpp>

#include "AppLogicFwd.h"
#include "ReconstructHandle.h"
#include "ReconstructionTreeCreator.h"
#include "TopologyGeometryType.h"

#include "maths/LatLonPoint.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/PolygonIntersections.h"
#include "maths/ProjectionUtils.h"

#include "model/FeatureCollectionHandle.h"

#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	class ResolvedTopologicalGeometry;

	/**
	 * This namespace contains utilities that clients of topology-related functionality use.
	 */
	namespace TopologyUtils
	{
		//
		// Topological boundaries
		//


		/**
		 * Returns true if @a feature contains any topological geometry.
		 *
		 * Currently this includes a topological boundary (polygon), line (polyline) or network.
		 */
		bool
		is_topological_geometry_feature(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature);

		/**
		 * Returns true if @a feature feature_collection any topological geometry.
		 *
		 * Currently this includes a topological boundary (polygon), line (polyline) or network.
		 */
		bool
		has_topological_geometry_features(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection);


		/**
		 * Returns true if @a feature contains a topological line geometry.
		 */
		bool
		is_topological_line_feature(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature);

		/**
		 * Returns true if @a feature_collection contains topological line features.
		 */
		bool
		has_topological_line_features(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection);


		/**
		 * Create and return a sequence of @a ResolvedTopologicalGeometry objects by resolving
		 * topological lines in @a topological_line_features_collection.
		 *
		 * NOTE: The sections are resolved by referencing already reconstructed topological section
		 * features which in turn must have already have been reconstructed.
		 *
		 * @param reconstruction_tree is associated with the output resolved topological boundaries.
		 * @param topological_sections_reconstruct_handles is a list of reconstruct handles that
		 *        identifies the subset, of all RFGs observing the topological section features,
		 *        that should be searched when resolving the topological lines.
		 *        This is useful to avoid outdated RFGs still in existence (among other scenarios).
		 *
		 * The returned reconstruct handle can be used to identify the resolved topological lines
		 * when resolving topological *boundaries* (since they can reference resolved *lines*).
		 */
		ReconstructHandle::type
		resolve_topological_lines(
				std::vector<resolved_topological_geometry_non_null_ptr_type> &resolved_topological_lines,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &topological_line_features_collection,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const reconstruction_tree_non_null_ptr_to_const_type &reconstruction_tree,
				boost::optional<const std::vector<ReconstructHandle::type> &> topological_sections_reconstruct_handles = boost::none);


		/**
		 * Returns true if @a feature contains a topological polygon geometry.
		 */
		bool
		is_topological_boundary_feature(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature);

		/**
		 * Returns true if @a feature_collection contains topological polygon features.
		 */
		bool
		has_topological_boundary_features(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection);


		/**
		 * Create and return a sequence of @a ResolvedTopologicalGeometry objects by resolving
		 * topological closed plate boundaries in @a topological_closed_plate_polygon_features_collection.
		 *
		 * NOTE: The sections are resolved by referencing already reconstructed topological section
		 * features which in turn must have already have been reconstructed.
		 * This includes any resolved topological lines that form sections.
		 *
		 * @param reconstruction_tree is associated with the output resolved topological boundaries.
		 * @param topological_sections_reconstruct_handles is a list of reconstruct handles that
		 *        identifies the subset, of all RFGs observing the topological section features,
		 *        and all resolved topological lines (@a ResolvedTopologicalGeometry containing polylines)
		 *        observing the topological section features,
		 *        that should be searched when resolving the topological boundaries.
		 *        This is useful to avoid outdated RFGs and RTGS still in existence (among other scenarios).
		 *
		 * The returned reconstruct handle can be used to identify the resolved topological boundaries.
		 * This is not currently used though.
		 */
		ReconstructHandle::type
		resolve_topological_boundaries(
				std::vector<resolved_topological_geometry_non_null_ptr_type> &resolved_topological_boundaries,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &topological_closed_plate_polygon_features_collection,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const reconstruction_tree_non_null_ptr_to_const_type &reconstruction_tree,
				boost::optional<const std::vector<ReconstructHandle::type> &> topological_sections_reconstruct_handles = boost::none);


		//! Typedef for a sequence of resolved topological boundaries.
		typedef std::vector<const ResolvedTopologicalGeometry *> resolved_topological_boundary_seq_type;


		/**
		 * A structure for partitioning geometry using the polygon of each @a ResolvedTopologicalGeometry
		 * in a sequence of resolved topological boundaries.
		 */
		class ResolvedBoundariesForGeometryPartitioning :
				public GPlatesUtils::ReferenceCount<ResolvedBoundariesForGeometryPartitioning>
		{
		public:

			typedef GPlatesUtils::non_null_intrusive_ptr<ResolvedBoundariesForGeometryPartitioning> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const ResolvedBoundariesForGeometryPartitioning> non_null_ptr_to_const_type;


			/**
			 * Create a @a ResolvedBoundariesForGeometryPartitioning.
			 */
			static
			non_null_ptr_type
			create(
					const std::vector<resolved_topological_geometry_non_null_ptr_type> &resolved_topological_boundaries)
			{
				return non_null_ptr_type(new ResolvedBoundariesForGeometryPartitioning(resolved_topological_boundaries));
			}


			/**
			 * Searches all @a ResolvedTopologicalGeometry objects to see which ones contain
			 * @a point and returns any found in @a resolved_topological_boundary_seq.
			 *
			 * Returns true if any are found.
			 */
			bool
			find_resolved_topology_boundaries_containing_point(
					resolved_topological_boundary_seq_type &resolved_topological_boundary_seq,
					const GPlatesMaths::PointOnSphere &point) const;


			//! Typedef for a sequence of 'GeometryOnSphere::non_null_ptr_to_const_type'.
			typedef GPlatesMaths::PolygonIntersections::partitioned_geometry_seq_type partitioned_geometry_seq_type;

			class ResolvedBoundaryPartitionedGeometries
			{
			public:
				ResolvedBoundaryPartitionedGeometries(
						const ResolvedTopologicalGeometry *resolved_topological_boundary_) :
					resolved_topological_boundary(resolved_topological_boundary_)
				{  }

				//! The resolved topological boundary that partitioned the polylines.
				const ResolvedTopologicalGeometry *resolved_topological_boundary;

				//! The partitioned geometries.
				partitioned_geometry_seq_type partitioned_inside_geometries;
			};

			/**
			 * Typedef for a sequence of associations of partitioned geometries and
			 * the @a ResolvedTopologicalGeometry that they were partitioned into.
			 */
			typedef std::list<ResolvedBoundaryPartitionedGeometries> resolved_boundary_partitioned_geometries_seq_type;


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
			partition_geometry(
					resolved_boundary_partitioned_geometries_seq_type &
							resolved_boundary_partitioned_geometries_seq,
					partitioned_geometry_seq_type &partitioned_outside_geometries,
					const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry) const;

		private:

			/**
			 * An association between a resolved topological boundary and its
			 * polygon structure (used for partitioning).
			 */
			struct GeometryPartitioning
			{

				GeometryPartitioning(
						const ResolvedTopologicalGeometry *resolved_topological_boundary_,
						const GPlatesMaths::PolygonIntersections::non_null_ptr_type &polygon_intersections_) :
					resolved_topological_boundary(resolved_topological_boundary_),
					polygon_intersections(polygon_intersections_)
				{  }

				const ResolvedTopologicalGeometry *resolved_topological_boundary;
				GPlatesMaths::PolygonIntersections::non_null_ptr_type polygon_intersections;
			};

			typedef std::vector<GeometryPartitioning> resolved_boundary_seq_type;

			resolved_boundary_seq_type d_resolved_boundaries;


			ResolvedBoundariesForGeometryPartitioning(
					const std::vector<resolved_topological_geometry_non_null_ptr_type> &resolved_topological_boundaries);
		};


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
				const ResolvedTopologicalGeometry * > >
		find_reconstruction_plate_id_furthest_from_anchor_in_plate_circuit(
				const resolved_topological_boundary_seq_type &resolved_boundaries);


		//
		// Topological networks
		//


		/**
		 * Returns true if @a feature is a topological network feature.
		 */
		bool
		is_topological_network_feature(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature);


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
		 * NOTE: The sections are resolved by referencing already reconstructed topological section
		 * features which in turn must have already have been reconstructed.
		 * This includes any resolved topological lines that form sections.
		 *
		 * @param topological_geometry_reconstruct_handles is a list of reconstruct handles that
		 *        identifies the subset, of all RFGs observing the topological boundary section and/or
		 *        interior features, and all resolved topological lines (@a ResolvedTopologicalGeometry
		 *        containing polylines) observing the topological boundary section features,
		 *        that should be searched when resolving the topological networks.
		 *        This is useful to avoid outdated RFGs and RTGS still in existence (among other scenarios).
		 *
		 * The returned reconstruct handle can be used to identify the resolved topological networks.
		 * This is not currently used though.
		 */
		ReconstructHandle::type
		resolve_topological_networks(
				std::vector<resolved_topological_network_non_null_ptr_type> &resolved_topological_networks,
				const double &reconstruction_time,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &topological_network_features_collection,
				boost::optional<const std::vector<ReconstructHandle::type> &> topological_geometry_reconstruct_handles);
	}
}

#endif // GPLATES_APP_LOGIC_TOPOLOGYUTILS_H
