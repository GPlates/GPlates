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
#include <map>
#include <set>
#include <utility>
#include <vector>
#include <boost/optional.hpp>

#include "ReconstructHandle.h"
#include "ReconstructionTreeCreator.h"
#include "ResolvedTopologicalBoundary.h"
#include "ResolvedTopologicalLine.h"
#include "ResolvedTopologicalNetwork.h"
#include "ResolvedTopologicalSection.h"
#include "ResolvedTopologicalSharedSubSegment.h"
#include "TopologyGeometryType.h"
#include "TopologyNetworkParams.h"

#include "maths/AzimuthalEqualAreaProjection.h"
#include "maths/LatLonPoint.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolygonPartitioner.h"
#include "maths/PolylineOnSphere.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureHandle.h"
#include "model/FeatureId.h"

#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	class ResolvedTopologicalBoundary;
	class ResolvedTopologicalLine;

	/**
	 * This namespace contains utilities that clients of topology-related functionality use.
	 */
	namespace TopologyUtils
	{
		/**
		 * Returns true if @a feature is topological.
		 *
		 * Currently this includes a topological boundary (polygon), line (polyline) or network.
		 */
		bool
		is_topological_feature(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature);

		/**
		 * Returns true if any feature in @a feature_collection is topological.
		 *
		 * Currently this includes a topological boundary (polygon), line (polyline) or network.
		 */
		bool
		has_topological_features(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection);

		/**
		 * Returns the type of topological geometry represented in the specified feature.
		 *
		 * Returns none if feature does not contain a topological geometry.
		 */
		boost::optional<TopologyGeometry::Type>
		get_topological_geometry_type(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature);


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
		 * Create and return a sequence of @a ResolvedTopologicalLine objects by resolving
		 * topological lines in @a topological_line_features_collection.
		 *
		 * NOTE: The sections are resolved by referencing already reconstructed topological section
		 * features which in turn must have already have been reconstructed.
		 *
		 * @param topological_sections_reconstruct_handles is a list of reconstruct handles that
		 *        identifies the subset, of all RFGs observing the topological section features,
		 *        that should be searched when resolving the topological lines.
		 *        This is useful to avoid outdated RFGs still in existence (among other scenarios).
		 * @param topological_lines_referenced Only resolved those topological line features matching
		 *        the specified feature IDs. This is useful when subsequently resolving boundaries/networks
		 *        that reference a subset of the topological line features specified.
		 *
		 * The returned reconstruct handle can be used to identify the resolved topological lines
		 * when resolving topological *boundaries* (since they can reference resolved *lines*).
		 */
		ReconstructHandle::type
		resolve_topological_lines(
				std::vector<ResolvedTopologicalLine::non_null_ptr_type> &resolved_topological_lines,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &topological_line_features_collection,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const double &reconstruction_time,
				boost::optional<const std::vector<ReconstructHandle::type> &> topological_sections_reconstruct_handles = boost::none,
				boost::optional<const std::set<GPlatesModel::FeatureId> &> topological_lines_referenced = boost::none);

		/**
		 * An overload of @a resolve_topological_lines accepting a vector of features instead of a feature collection.
		 */
		ReconstructHandle::type
		resolve_topological_lines(
				std::vector<ResolvedTopologicalLine::non_null_ptr_type> &resolved_topological_lines,
				const std::vector<GPlatesModel::FeatureHandle::weak_ref> &topological_line_features,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const double &reconstruction_time,
				boost::optional<const std::vector<ReconstructHandle::type> &> topological_sections_reconstruct_handles = boost::none,
				boost::optional<const std::set<GPlatesModel::FeatureId> &> topological_lines_referenced = boost::none);


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
		 * Create and return a sequence of @a ResolvedTopologicalBoundary objects by resolving
		 * topological closed plate boundaries in @a topological_closed_plate_polygon_features_collection.
		 *
		 * NOTE: The sections are resolved by referencing already reconstructed topological section
		 * features which in turn must have already have been reconstructed.
		 * This includes any resolved topological lines that form sections.
		 *
		 * @param topological_sections_reconstruct_handles is a list of reconstruct handles that
		 *        identifies the subset, of all RFGs observing the topological section features,
		 *        and all resolved topological lines (@a ResolvedTopologicalLine)
		 *        observing the topological section features,
		 *        that should be searched when resolving the topological boundaries.
		 *        This is useful to avoid outdated RFGs and RTGS still in existence (among other scenarios).
		 *
		 * The returned reconstruct handle can be used to identify the resolved topological boundaries.
		 * This is not currently used though.
		 */
		ReconstructHandle::type
		resolve_topological_boundaries(
				std::vector<ResolvedTopologicalBoundary::non_null_ptr_type> &resolved_topological_boundaries,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &topological_closed_plate_polygon_features_collection,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const double &reconstruction_time,
				boost::optional<const std::vector<ReconstructHandle::type> &> topological_sections_reconstruct_handles = boost::none);

		/**
		 * An overload of @a resolve_topological_boundaries accepting a vector of features instead of a feature collection.
		 */
		ReconstructHandle::type
		resolve_topological_boundaries(
				std::vector<ResolvedTopologicalBoundary::non_null_ptr_type> &resolved_topological_boundaries,
				const std::vector<GPlatesModel::FeatureHandle::weak_ref> &topological_closed_plate_polygon_features,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const double &reconstruction_time,
				boost::optional<const std::vector<ReconstructHandle::type> &> topological_sections_reconstruct_handles = boost::none);


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
		 *        interior features, and all resolved topological lines (@a ResolvedTopologicalLine)
		 *        observing the topological boundary section features,
		 *        that should be searched when resolving the topological networks.
		 *        This is useful to avoid outdated RFGs and RTGS still in existence (among other scenarios).
		 * @param topology_network_params parameters used when creating the resolved networks.
		 *
		 * The returned reconstruct handle can be used to identify the resolved topological networks.
		 * This is not currently used though.
		 */
		ReconstructHandle::type
		resolve_topological_networks(
				std::vector<ResolvedTopologicalNetwork::non_null_ptr_type> &resolved_topological_networks,
				const double &reconstruction_time,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &topological_network_features_collection,
				boost::optional<const std::vector<ReconstructHandle::type> &> topological_geometry_reconstruct_handles,
				const TopologyNetworkParams &topology_network_params = TopologyNetworkParams());

		/**
		 * An overload of @a resolve_topological_networks accepting a vector of features instead of a feature collection.
		 */
		ReconstructHandle::type
		resolve_topological_networks(
				std::vector<ResolvedTopologicalNetwork::non_null_ptr_type> &resolved_topological_networks,
				const double &reconstruction_time,
				const std::vector<GPlatesModel::FeatureHandle::weak_ref> &topological_network_features,
				boost::optional<const std::vector<ReconstructHandle::type> &> topological_geometry_reconstruct_handles,
				const TopologyNetworkParams &topology_network_params = TopologyNetworkParams());


		/**
		 * Finds all sub-segments shared by resolved topology boundaries and network boundaries.
		 *
		 * These sub-segments are separated into non-overlapping sections.
		 * This is in contrast to simply gathering all sub-segments of these resolved boundaries/networks
		 * which will overlap each other (eg, two plate polygons share parts of their boundary
		 * leading to duplication).
		 *
		 * Each @a ResolvedTopologicalSection gathers the shared sub-segments associated with a
		 * single topological section.
		 */
		void
		find_resolved_topological_sections(
				std::vector<ResolvedTopologicalSection::non_null_ptr_type> &resolved_topological_sections,
				const std::vector<ResolvedTopologicalBoundary::non_null_ptr_to_const_type> &resolved_topological_boundaries,
				const std::vector<ResolvedTopologicalNetwork::non_null_ptr_to_const_type> &resolved_topological_networks);


		/**
		 * Typedef for a mapping from resolved topological boundaries and networks to their shared boundary sub-segments.
		 *
		 * Note: The map key is a feature property iterator instead of a resolved topology (ReconstructionGeometry) because
		 *       it's possible to have many reconstruction geometries referencing the same feature property since the
		 *       topological features could have been resolved again (generating different reconstruction geometries) between
		 *       when this map was created and when it is used to look up shared sub-segments.
		 */
		typedef std::map<
				// The geometry property referenced by a ResolvedTopologicalBoundary or ResolvedTopologicalNetwork...
				GPlatesModel::FeatureHandle::iterator,
				// The shared sub-segment and whether the sub-segment geometry is reversed with respect to the section geometry
				// (when it contributes to the ResolvedTopologicalBoundary or ResolvedTopologicalNetwork being mapped)...
				std::vector<
						std::pair<GPlatesAppLogic::ResolvedTopologicalSharedSubSegment::non_null_ptr_type, bool/*is_sub_segment_geometry_reversed*/>>>
								resolved_topological_boundaries_networks_to_shared_sub_segments_map_type;
		
		/**
		 * Takes a sequence of resolved topological sections and creates a mapping from
		 * resolved topological boundaries and networks to their shared boundary sub-segments.
		 *
		 * This is just a different representation of @a find_resolved_topological_sections.
		 * Instead of shared sub-segments referencing resolved topological boundaries and networks,
		 * it's the other way around.
		 */
		void
		map_resolved_topological_boundaries_networks_to_shared_sub_segments(
				resolved_topological_boundaries_networks_to_shared_sub_segments_map_type &resolved_topological_shared_sub_segments_map,
				const std::vector<ResolvedTopologicalSection::non_null_ptr_type> &resolved_topological_sections);
	}
}

#endif // GPLATES_APP_LOGIC_TOPOLOGYUTILS_H
