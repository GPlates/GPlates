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

#ifndef GPLATES_API_PYTOPOLOGICALMODEL_H
#define GPLATES_API_PYTOPOLOGICALMODEL_H

#include <vector>
#include <map>
#include <boost/optional.hpp>

#include "PyFeatureCollection.h"
#include "PyRotationModel.h"

#include "app-logic/ReconstructContext.h"
#include "app-logic/ReconstructMethodRegistry.h"
#include "app-logic/ResolvedTopologicalBoundary.h"
#include "app-logic/ResolvedTopologicalLine.h"
#include "app-logic/ResolvedTopologicalNetwork.h"
#include "app-logic/ScalarCoverageDeformation.h"
#include "app-logic/TimeSpanUtils.h"
#include "app-logic/TopologyReconstruct.h"

#include "file-io/File.h"

#include "global/python.h"

#include "maths/types.h"

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"

#include "property-values/GeoTimeInstant.h"
#include "property-values/ValueObjectType.h"

#include "utils/ReferenceCount.h"


namespace GPlatesApi
{
	/**
	 * Contains topology-reconstructed history of a geometry (and optional scalars) over a time span.
	 */
	class ReconstructedGeometryTimeSpan :
			public GPlatesUtils::ReferenceCount<ReconstructedGeometryTimeSpan>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructedGeometryTimeSpan> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructedGeometryTimeSpan> non_null_ptr_to_const_type;

		typedef std::map<
				GPlatesPropertyValues::ValueObjectType/*scalar type*/,
				GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::non_null_ptr_type/*scalars*/>
						scalar_type_to_time_span_map_type;


		/**
		 * Create a reconstructed geometry time span (with optional reconstructed scalars).
		 */
		static
		non_null_ptr_type
		create(
				GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span,
				const scalar_type_to_time_span_map_type &scalar_type_to_time_span_map)
		{
			return non_null_ptr_type(new ReconstructedGeometryTimeSpan(geometry_time_span, scalar_type_to_time_span_map));
		}

	private:

		ReconstructedGeometryTimeSpan(
				GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span,
				const scalar_type_to_time_span_map_type &scalar_type_to_time_span_map) :
			d_geometry_time_span(geometry_time_span),
			d_scalar_type_to_time_span_map(scalar_type_to_time_span_map)
		{  }


		/**
		 * Reconstructed geometry time span.
		 */
		GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type d_geometry_time_span;

		/**
		 * Mapping of scalar types to their reconstructed scalar time spans.
		 */
		scalar_type_to_time_span_map_type d_scalar_type_to_time_span_map;
	};


	/**
	 * Dynamic plates and deforming networks, and their associated rotation model.
	 */
	class TopologicalModel :
			public GPlatesUtils::ReferenceCount<TopologicalModel>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<TopologicalModel> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const TopologicalModel> non_null_ptr_to_const_type;


		/**
		 * Create a topological model from topological features and associated rotation model.
		 */
		static
		non_null_ptr_type
		create(
				const FeatureCollectionSequenceFunctionArgument &topological_features,
				// Note we're using 'RotationModelFunctionArgument::function_argument_type' instead of
				// just 'RotationModelFunctionArgument' since we want to know if it's an existing RotationModel...
				const RotationModelFunctionArgument::function_argument_type &rotation_model_argument,
				boost::optional<GPlatesModel::integer_plate_id_type> anchor_plate_id);


		/**
		 * Reconstruct/deform a geometry or a coverage (geometry + scalars) over a time range.
		 *
		 * The time range is from @a oldest_time (defaults to @a initial_time) to
		 * @a youngest_time (defaults to present day) in increments of @a time_increment (defaults to 1My).
		 *
		 * Returns a reconstructed time span.
		 *
		 * @a scalar_type_to_values_mapping_object can be any of the following:
		 *   (1) a Python 'dict' mapping each 'ScalarType' to a sequence of float, or
		 *   (2) a Python sequence of ('ScalarType', sequence of float) tuples.
		 *   (3) boost::python::object()  // Py_None
		 *
		 * This will raise Python ValueError if @a scalar_type_to_values_mapping_object is not None but:
		 * is empty, or if each scalar type is not mapped to the same number of scalar values, or
		 * number of scalar values is not equal to number of points in @a geometry.
		 */
		ReconstructedGeometryTimeSpan::non_null_ptr_type
		reconstruct_geometry(
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry,
				const GPlatesPropertyValues::GeoTimeInstant &initial_time,
				boost::optional<GPlatesPropertyValues::GeoTimeInstant> oldest_time = boost::none,
				const GPlatesPropertyValues::GeoTimeInstant &youngest_time = GPlatesPropertyValues::GeoTimeInstant(0),
				const double &time_increment = 1,
				boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id = boost::none,
				boost::python::object scalar_type_to_values_mapping_object = boost::python::object()/*Py_None*/);


		/**
		 * Return the topological feature collections used to create this topological model.
		 */
		void
		get_topological_feature_collections(
				std::vector<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> &topological_feature_collections) const
		{
			d_topological_features.get_feature_collections(topological_feature_collections);
		}


		/**
		 * Return the topological feature collection files used to create this topological model.
		 *
		 * NOTE: Any feature collections that did not come from files will have empty filenames.
		 */
		void
		get_files(
				std::vector<GPlatesFileIO::File::non_null_ptr_type> &topological_feature_collections_files) const
		{
			d_topological_features.get_files(topological_feature_collections_files);
		}


		/**
		 * Get the rotation model.
		 */
		RotationModel::non_null_ptr_type
		get_rotation_model() const
		{
			return d_rotation_model;
		}

	private:

		/**
		 * Resolved topologies at a time instant.
		 */
		struct ResolvedTopologiesTimeSlot
		{
			std::vector<GPlatesAppLogic::ResolvedTopologicalLine::non_null_ptr_type> resolved_lines;
			std::vector<GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_type> resolved_boundaries;
			std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type> resolved_networks;
		};

		//! Typedef for a mapping of (integral) times to resolved topologies.
		typedef std::map<GPlatesMaths::real_t/*time*/, ResolvedTopologiesTimeSlot> resolved_topology_time_slots_type;


		/**
		 * Rotation model associated with topological features.
		 */
		RotationModel::non_null_ptr_type d_rotation_model;

		/**
		 * Topological feature collections/files.
		 */
		FeatureCollectionSequenceFunctionArgument d_topological_features;

		// Separate @a d_topological_features into regular features (used as topological sections for
		// topological lines/boundaries/networks), topological lines (can also be used as topological
		// sections for topological boundaries/networks), topological boundaries and topological networks.
		std::vector<GPlatesModel::FeatureHandle::weak_ref> d_topological_section_regular_features;
		std::vector<GPlatesModel::FeatureHandle::weak_ref> d_topological_line_features;
		std::vector<GPlatesModel::FeatureHandle::weak_ref> d_topological_boundary_features;
		std::vector<GPlatesModel::FeatureHandle::weak_ref> d_topological_network_features;

		GPlatesAppLogic::ReconstructMethodRegistry d_reconstruct_method_registry;

		/**
		 * Used to reconstruct @a d_topological_section_regular_features so they can be used as
		 * topological sections for the topologies.
		 */
		GPlatesAppLogic::ReconstructContext d_topological_section_reconstruct_context;
		GPlatesAppLogic::ReconstructContext::context_state_reference_type d_topological_section_reconstruct_context_state;

		/**
		 * Cache of resolved topologies at various (integer) time instants.
		 */
		resolved_topology_time_slots_type d_cached_resolved_topologies;


		TopologicalModel(
				const FeatureCollectionSequenceFunctionArgument &topological_features,
				const RotationModel::non_null_ptr_type &rotation_model);

		/**
		 * Returns the resolved topologies for the specified time (creating and caching them if necessary).
		 *
		 * @a reconstruction_time should be an integral value.
		 */
		const ResolvedTopologiesTimeSlot &
		get_resolved_topologies(
				const double &reconstruction_time);

		/**
		 * Resolves topologies for the specified time.
		 *
		 * @a reconstruction_timereconstruction_time should be an integral value.
		 */
		void
		resolve_topologies(
				GPlatesApi::TopologicalModel::ResolvedTopologiesTimeSlot &resolved_topologies,
				const double &reconstruction_time);
	};
}

#endif // GPLATES_API_PYTOPOLOGICALMODEL_H
