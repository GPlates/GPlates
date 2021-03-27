/* $Id$ */

/**
 * \file
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2020 The University of Sydney, Australia
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

#ifndef GPLATES_API_PYTOPOLOGICALSNAPSHOT_H
#define GPLATES_API_PYTOPOLOGICALSNAPSHOT_H


#include <vector>
#include <map>
#include <boost/optional.hpp>
#include <QString>

#include "PyResolveTopologyParameters.h"
#include "PyRotationModel.h"
#include "PyTopologicalFeatureCollectionFunctionArgument.h"

#include "app-logic/ResolvedTopologicalBoundary.h"
#include "app-logic/ResolvedTopologicalLine.h"
#include "app-logic/ResolvedTopologicalNetwork.h"
#include "app-logic/ResolvedTopologicalSection.h"

#include "global/python.h"

#include "maths/PolygonOrientation.h"
#include "maths/types.h"

#include "model/types.h"

#include "property-values/GeoTimeInstant.h"

#include "utils/ReferenceCount.h"


namespace GPlatesApi
{
	/**
	 * Enumeration to determine which resolved topology types to output.
	 *
	 * This can be used as a bitwise combination of flags.
	 */
	namespace ResolveTopologyType
	{
		enum Value
		{
			LINE = (1 << 0),
			BOUNDARY = (1 << 1),
			NETWORK = (1 << 2)
		};

		// Use this (integer) type when extracting flags (of resolved topology types).
		typedef unsigned int flags_type;

		// Mask of allowed bit flags for 'ResolveTopologyType'.
		constexpr flags_type ALL_RESOLVE_TOPOLOGY_TYPES = (LINE | BOUNDARY | NETWORK);

		// Bit flags for BOUNDARY and NETWORK.
		constexpr flags_type BOUNDARY_AND_NETWORK_RESOLVE_TOPOLOGY_TYPES = (BOUNDARY | NETWORK);

		// Default resolved topology types includes only those with boundaries
		// (hence topological lines are excluded).
		constexpr flags_type DEFAULT_RESOLVE_TOPOLOGY_TYPES = BOUNDARY_AND_NETWORK_RESOLVE_TOPOLOGY_TYPES;

		// Default resolved topological section types includes only those with boundaries
		// (hence topological lines are excluded).
		constexpr flags_type DEFAULT_RESOLVE_TOPOLOGICAL_SECTION_TYPES = BOUNDARY_AND_NETWORK_RESOLVE_TOPOLOGY_TYPES;
	};


	/**
	 * Snapshot, at a specific reconstruction time, of dynamic plates and deforming networks.
	 */
	class TopologicalSnapshot :
			public GPlatesUtils::ReferenceCount<TopologicalSnapshot>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<TopologicalSnapshot> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const TopologicalSnapshot> non_null_ptr_to_const_type;


		/**
		 * Create a topological snapshot, at specified reconstruction time, from topological features and associated rotation model.
		 *
		 * Note that this @a create overload resolves topologies (whereas the other overload does not).
		 */
		static
		non_null_ptr_type
		create(
				const TopologicalFeatureCollectionSequenceFunctionArgument &topological_features_argument,
				const RotationModelFunctionArgument &rotation_model_argument,
				const double &reconstruction_time,
				boost::optional<GPlatesModel::integer_plate_id_type> anchor_plate_id,
				boost::optional<ResolveTopologyParameters::non_null_ptr_to_const_type> default_resolve_topology_parameters);

		/**
		 * Create a topological snapshot, at specified reconstruction time, from the previously resolved topologies.
		 */
		static
		non_null_ptr_type
		create(
				const std::vector<GPlatesAppLogic::ResolvedTopologicalLine::non_null_ptr_type> &resolved_topological_lines,
				const std::vector<GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_type> &resolved_topological_boundaries,
				const std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type> &resolved_topological_networks,
				const std::vector<GPlatesFileIO::File::non_null_ptr_type> &topological_files,
				const RotationModel::non_null_ptr_type &rotation_model,
				const double &reconstruction_time)
		{
			return non_null_ptr_type(
					new TopologicalSnapshot(
							resolved_topological_lines, resolved_topological_boundaries, resolved_topological_networks,
							topological_files, rotation_model, reconstruction_time));
		}


		/**
		 * Get resolved topological lines.
		 */
		const std::vector<GPlatesAppLogic::ResolvedTopologicalLine::non_null_ptr_type> &
		get_resolved_topological_lines() const
		{
			return d_resolved_topological_lines;
		}

		/**
		 * Get resolved topological boundaries.
		 */
		const std::vector<GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_type> &
		get_resolved_topological_boundaries() const
		{
			return d_resolved_topological_boundaries;
		}

		/**
		 * Get resolved topological networks.
		 */
		const std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type> &
		get_resolved_topological_networks() const
		{
			return d_resolved_topological_networks;
		}

		/**
		 * Get resolved topologies (lines, boundaries, networks).
		 *
		 * If @a same_order_as_topological_features is true then the resolved topologies are
		 * sorted in the order of the features in the topological files (and the order across files).
		 *
		 * By default returns only resolved boundaries and networks (excludes resolved lines).
		 */
		std::vector<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type>
		get_resolved_topologies(
				ResolveTopologyType::flags_type resolve_topology_types = ResolveTopologyType::DEFAULT_RESOLVE_TOPOLOGY_TYPES,
				bool same_order_as_topological_features = false) const;

		/**
		 * Export resolved topologies (lines, boundaries, networks) to a file.
		 *
		 * If @a wrap_to_dateline is true then wrap/clip resolved topologies to the dateline
		 * (currently ignored unless exporting to an ESRI Shapefile format file). Defaults to true.
		 *
		 * If @a force_boundary_orientation is not none then force boundary orientation (clockwise or counter-clockwise)
		 * of resolved boundaries and networks. Currently ignored by ESRI Shapefile which always uses clockwise.
		 *
		 * By default exports only resolved boundaries and networks (excludes resolved lines).
		 */
		void
		export_resolved_topologies(
				const QString &export_file_name,
				ResolveTopologyType::flags_type resolve_topology_types = ResolveTopologyType::DEFAULT_RESOLVE_TOPOLOGY_TYPES,
				bool wrap_to_dateline = true,
				boost::optional<GPlatesMaths::PolygonOrientation::Orientation> force_boundary_orientation = boost::none) const;

		/**
		 * Get resolved topological sections (each contains sub-segments of boundaries shared by topological boundaries and networks).
		 *
		 * If @a same_order_as_topological_features is true then the resolved topological sections are
		 * sorted in the order of the features in the topological files (and the order across files).
		 *
		 * @a resolve_topological_section_types specifies which resolved topologies the returned sections reference.
		 * For example, if only ResolveTopologyType::BOUNDARY is specified then deforming networks
		 * (ResolveTopologyType::NETWORK) are not considered and the return sections contain sub-segments
		 * that only reference resolved topological *boundaries*.
		 * Note that ResolveTopologyType::LINE is ignored (if specified) since only boundary and network
		 * topologies contribute to resolved topological sections.
		 * By default resolved boundaries and networks are considered.
		 */
		std::vector<GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type>
		get_resolved_topological_sections(
				ResolveTopologyType::flags_type resolve_topological_section_types = ResolveTopologyType::DEFAULT_RESOLVE_TOPOLOGICAL_SECTION_TYPES,
				bool same_order_as_topological_features = false) const;

		/**
		 * Export resolved topological sections (each contains sub-segments of boundaries shared by topological boundaries and networks).
		 *
		 * If @a wrap_to_dateline is true then wrap/clip resolved topological sections to the dateline
		 * (currently ignored unless exporting to an ESRI Shapefile format file). Defaults to true.
		 *
		 * By default exports resolved boundaries and networks.
		 */
		void
		export_resolved_topological_sections(
				const QString &export_file_name,
				ResolveTopologyType::flags_type resolve_topological_section_types = ResolveTopologyType::DEFAULT_RESOLVE_TOPOLOGICAL_SECTION_TYPES,
				bool wrap_to_dateline = true) const;


		/**
		 * Get the topological files.
		 */
		const std::vector<GPlatesFileIO::File::non_null_ptr_type> &
		get_topological_files() const
		{
			return d_topological_files;
		}

		/**
		 * Get the rotation model.
		 */
		RotationModel::non_null_ptr_type
		get_rotation_model() const
		{
			return d_rotation_model;
		}

		/**
		 * Returns the anchor plate ID.
		 */
		GPlatesModel::integer_plate_id_type
		get_anchor_plate_id() const
		{
			return get_rotation_model()->get_reconstruction_tree_creator().get_default_anchor_plate_id();
		}

	private:

		std::vector<GPlatesFileIO::File::non_null_ptr_type> d_topological_files;
		RotationModel::non_null_ptr_type d_rotation_model;
		double d_reconstruction_time;

		std::vector<GPlatesAppLogic::ResolvedTopologicalLine::non_null_ptr_type> d_resolved_topological_lines;
		std::vector<GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_type> d_resolved_topological_boundaries;
		std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type> d_resolved_topological_networks;

		/**
		 * Cached resolved topological sections created on demand.
		 *
		 * The four arrays correspond to finding resolved topological sections considering topologies of:
		 * - Neither ResolveTopologyType::BOUNDARY nor ResolveTopologyType::NETWORK,
		 * - Both ResolveTopologyType::BOUNDARY and ResolveTopologyType::NETWORK,
		 * - Only ResolveTopologyType::BOUNDARY,
		 * - Only ResolveTopologyType::NETWORK.
		 */
		mutable boost::optional<std::vector<GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type>> d_resolved_topological_sections[4];


		TopologicalSnapshot(
				const TopologicalFeatureCollectionSequenceFunctionArgument &topological_features_argument,
				const RotationModel::non_null_ptr_type &rotation_model,
				const double &reconstruction_time,
				ResolveTopologyParameters::non_null_ptr_to_const_type default_resolve_topology_parameters);

		TopologicalSnapshot(
				const std::vector<GPlatesAppLogic::ResolvedTopologicalLine::non_null_ptr_type> &resolved_topological_lines,
				const std::vector<GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_type> &resolved_topological_boundaries,
				const std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type> &resolved_topological_networks,
				const std::vector<GPlatesFileIO::File::non_null_ptr_type> &topological_files,
				const RotationModel::non_null_ptr_type &rotation_model,
				const double &reconstruction_time) :
			d_topological_files(topological_files),
			d_rotation_model(rotation_model),
			d_reconstruction_time(reconstruction_time),
			d_resolved_topological_lines(resolved_topological_lines),
			d_resolved_topological_boundaries(resolved_topological_boundaries),
			d_resolved_topological_networks(resolved_topological_networks)
		{  }

		/**
		 * Finds all sub-segments shared by resolved topology boundaries and/or network boundaries
		 * (depending on @a resolve_topological_section_types).
		 */
		std::vector<GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type>
		find_resolved_topological_sections(
				ResolveTopologyType::flags_type resolve_topological_section_types) const;

		/**
		 * Sort the resolved topologies in the order of the features in the topological files (and the order across files).
		 */
		std::vector<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type>
		sort_resolved_topologies(
				const std::vector<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type> &resolved_topologies) const;

		/**
		 * Sort the resolved topological sections in the order of the features in the topological files (and the order across files).
		 */
		std::vector<GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type>
		sort_resolved_topological_sections(
				const std::vector<GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type> &resolved_topological_sections) const;
	};
}

#endif // GPLATES_API_PYTOPOLOGICALSNAPSHOT_H
