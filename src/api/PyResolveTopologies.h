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

#ifndef GPLATES_API_PYRESOLVETOPOLOGIES_H
#define GPLATES_API_PYRESOLVETOPOLOGIES_H


#include <vector>
#include <map>
#include <boost/optional.hpp>

#include "PyFeatureCollection.h"
#include "PyRotationModel.h"

#include "app-logic/ResolvedTopologicalBoundary.h"
#include "app-logic/ResolvedTopologicalLine.h"
#include "app-logic/ResolvedTopologicalNetwork.h"

#include "global/python.h"

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
		constexpr flags_type RESOLVE_TOPOLOGY_TYPE_MASK = (LINE | BOUNDARY | NETWORK);

		// All resolved topology types.
		constexpr flags_type ALL_RESOLVE_TOPOLOGY_TYPES = (LINE | BOUNDARY | NETWORK);
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
				const FeatureCollectionSequenceFunctionArgument &topological_features_argument,
				const RotationModelFunctionArgument &rotation_model_argument,
				const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time,
				boost::optional<GPlatesModel::integer_plate_id_type> anchor_plate_id);

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
				const RotationModel::non_null_ptr_type &rotation_model)
		{
			return non_null_ptr_type(
					new TopologicalSnapshot(
							resolved_topological_lines, resolved_topological_boundaries, resolved_topological_networks,
							topological_files, rotation_model));
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
		 * Get resolved lines, boundaries, networks.
		 */
		std::vector<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type>
		get_resolved_topologies(
				ResolveTopologyType::flags_type resolve_topology_types = ResolveTopologyType::ALL_RESOLVE_TOPOLOGY_TYPES) const;


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

		// Cached resolved topologies created on demand.
		std::vector<GPlatesAppLogic::ResolvedTopologicalLine::non_null_ptr_type> d_resolved_topological_lines;
		std::vector<GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_type> d_resolved_topological_boundaries;
		std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type> d_resolved_topological_networks;

		std::vector<GPlatesFileIO::File::non_null_ptr_type> d_topological_files;
		RotationModel::non_null_ptr_type d_rotation_model;


		TopologicalSnapshot(
				const FeatureCollectionSequenceFunctionArgument &topological_features_argument,
				const RotationModel::non_null_ptr_type &rotation_model,
				const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time);

		TopologicalSnapshot(
				const std::vector<GPlatesAppLogic::ResolvedTopologicalLine::non_null_ptr_type> &resolved_topological_lines,
				const std::vector<GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_type> &resolved_topological_boundaries,
				const std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type> &resolved_topological_networks,
				const std::vector<GPlatesFileIO::File::non_null_ptr_type> &topological_files,
				const RotationModel::non_null_ptr_type &rotation_model) :
			d_resolved_topological_lines(resolved_topological_lines),
			d_resolved_topological_boundaries(resolved_topological_boundaries),
			d_resolved_topological_networks(resolved_topological_networks),
			d_topological_files(topological_files),
			d_rotation_model(rotation_model)
		{  }
	};
}

#endif // GPLATES_API_PYRESOLVETOPOLOGIES_H
