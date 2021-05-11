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

#include <cmath>
#include <cstddef>
#include <iterator>
#include <sstream>
#include <vector>
#include <boost/cast.hpp>
#include <boost/foreach.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <boost/variant.hpp>

#include "PyTopologicalModel.h"

#include "PyFeature.h"
#include "PyFeatureCollectionFunctionArgument.h"
#include "PyPropertyValues.h"
#include "PythonConverterUtils.h"
#include "PythonExtractUtils.h"
#include "PythonHashDefVisitor.h"

#include "app-logic/GeometryUtils.h"
#include "app-logic/ScalarCoverageEvolution.h"
#include "app-logic/ReconstructParams.h"
#include "app-logic/TopologyInternalUtils.h"
#include "app-logic/TopologyPointLocation.h"
#include "app-logic/TopologyUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/python.h"

#include "maths/MathsUtils.h"
#include "maths/types.h"

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"


namespace bp = boost::python;


namespace GPlatesApi
{
	/**
	 * This is called directly from Python via 'TopologicalModel.__init__()'.
	 */
	TopologicalModel::non_null_ptr_type
	topological_model_create(
			const TopologicalFeatureCollectionSequenceFunctionArgument &topological_features,
			const RotationModelFunctionArgument::function_argument_type &rotation_model_argument,
			boost::optional<GPlatesModel::integer_plate_id_type> anchor_plate_id,
			boost::optional<ResolveTopologyParameters::non_null_ptr_to_const_type> default_resolve_topology_parameters)
	{
		return TopologicalModel::create(
				topological_features,
				rotation_model_argument,
				anchor_plate_id,
				default_resolve_topology_parameters);
	}

	/**
	 * This is called directly from Python via 'TopologicalModel.get_topological_snapshot()'.
	 */
	TopologicalSnapshot::non_null_ptr_type
	topological_model_get_topological_snapshot(
			TopologicalModel::non_null_ptr_type topological_model,
			const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time)
	{
		// Time must not be distant past/future.
		if (!reconstruction_time.is_real())
		{
			PyErr_SetString(PyExc_ValueError,
					"Time values cannot be distant-past (float('inf')) or distant-future (float('-inf')).");
			bp::throw_error_already_set();
		}

		// TopologicalModel::get_topological_snapshot() checks that the reconstruction time is an integral value.
		return topological_model->get_topological_snapshot(reconstruction_time.value());
	}


	/**
	 * This is called directly from Python via 'ReconstructedGeometryTimeSpan.DefaultDeactivatePoints.__init__()'.
	 */
	GPlatesAppLogic::TopologyReconstruct::DefaultDeactivatePoint::non_null_ptr_type
	reconstructed_geometry_time_span_default_deactivate_points_create(
			const double &threshold_velocity_delta,
			const double &threshold_distance_to_boundary_in_kms_per_my,
			bool deactivate_points_that_fall_outside_a_network)
	{
		return GPlatesAppLogic::TopologyReconstruct::DefaultDeactivatePoint::create(
				threshold_velocity_delta,
				threshold_distance_to_boundary_in_kms_per_my,
				deactivate_points_that_fall_outside_a_network);
	}


	namespace
	{
		/**
		 * Extract the geometry.
		 *
		 * @a geometry_object can be either:
		 * (1) a PointOnSphere, or
		 * (2) a MultiPointOnSphere, or
		 * (3) a sequence of PointOnSphere (or anything convertible to PointOnSphere), returned as a MultiPointOnSphere.
		 *
		 * NOTE: Currently @a geometry_object is limited to a PointOnSphere, MultiPointOnSphere or sequence of points.
		 *       In future this will be extended to include polylines and polygons (with interior holes).
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		get_geometry(
				bp::object geometry_object)
		{
			// See if it's a MultiPointOnSphere.
			bp::extract<GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::MultiPointOnSphere>> extract_multi_point(geometry_object);
			if (extract_multi_point.check())
			{
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point = extract_multi_point();
				return multi_point;
			}

			// See if it's a PointOnSphere.
			bp::extract<GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PointGeometryOnSphere>> extract_point(geometry_object);
			if (extract_point.check())
			{
				GPlatesMaths::PointGeometryOnSphere::non_null_ptr_to_const_type point = extract_point();
				return point;
			}

			// Attempt to extract a sequence of points.
			std::vector<GPlatesMaths::PointOnSphere> points;
			PythonExtractUtils::extract_iterable(points, geometry_object,
					"Expected a point or a multipoint or a sequence of points");

			return GPlatesMaths::MultiPointOnSphere::create(points);
		}

		/**
		 * Extract reconstructed geometry points (at @a reconstruction_time) from geometry time span and
		 * return as a Python list.
		 */
		bp::list
		add_geometry_points_to_list(
				GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span,
				const double &reconstruction_time,
				bool return_inactive_points)
		{
			// Put the geometry points in a Python list object.
			boost::python::list geometry_points_list_object;

			// Get the geometry points at the reconstruction time.
			if (return_inactive_points)
			{
				std::vector<boost::optional<GPlatesMaths::PointOnSphere>> all_geometry_points;
				geometry_time_span->get_all_geometry_data(reconstruction_time, all_geometry_points);

				for (auto geometry_point : all_geometry_points)
				{
					// Note that boost::none gets translated to Python 'None'.
					geometry_points_list_object.append(geometry_point);
				}
			}
			else // only active points...
			{
				std::vector<GPlatesMaths::PointOnSphere> geometry_points;
				geometry_time_span->get_geometry_data(reconstruction_time, geometry_points);

				for (auto geometry_point : geometry_points)
				{
					geometry_points_list_object.append(geometry_point);
				}
			}

			return geometry_points_list_object;
		}

		/**
		 * Extract the location in topologies of reconstructed geometry points (at @a reconstruction_time)
		 * from geometry time span and return as a Python list.
		 */
		bp::list
		add_topology_point_locations_to_list(
				GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span,
				const double &reconstruction_time,
				bool return_inactive_points)
		{
			// Put the topology point locations in a Python list object.
			boost::python::list topology_point_locations_list_object;

			// Get the topology point locations at the reconstruction time.
			if (return_inactive_points)
			{
				std::vector<boost::optional<GPlatesAppLogic::TopologyPointLocation>> all_topology_point_locations;
				geometry_time_span->get_all_geometry_data(
						reconstruction_time,
						boost::none/*points*/,
						all_topology_point_locations);

				for (auto topology_point_location : all_topology_point_locations)
				{
					// Note that boost::none gets translated to Python 'None'.
					topology_point_locations_list_object.append(topology_point_location);
				}
			}
			else // only active points...
			{
				std::vector<GPlatesAppLogic::TopologyPointLocation> topology_point_locations;
				geometry_time_span->get_geometry_data(
						reconstruction_time,
						boost::none/*points*/,
						topology_point_locations);

				for (auto topology_point_location : topology_point_locations)
				{
					topology_point_locations_list_object.append(topology_point_location);
				}
			}

			return topology_point_locations_list_object;
		}

		/**
		 * Extract reconstructed scalar values (at @a reconstruction_time and associated with @a scalar_type)
		 * from a scalar coverage time span and return as a Python list.
		 */
		bp::list
		add_scalar_values_to_list(
				GPlatesAppLogic::ScalarCoverageTimeSpan::non_null_ptr_type scalar_coverage_time_span,
				const GPlatesPropertyValues::ValueObjectType &scalar_type,
				const double &reconstruction_time,
				bool return_inactive_points)
		{
			// Put the scalar values in a Python list object.
			boost::python::list scalar_values_list_object;

			// Get the scalar values at the reconstruction time.
			if (return_inactive_points)
			{
				std::vector<double> all_scalar_values;
				std::vector<bool> all_scalar_values_are_active;
				if (scalar_coverage_time_span->get_all_scalar_values(
						scalar_type, reconstruction_time, all_scalar_values, all_scalar_values_are_active))
				{
					const unsigned int num_all_scalar_values = scalar_coverage_time_span->get_num_all_scalar_values();
					for (unsigned int scalar_value_index = 0; scalar_value_index < num_all_scalar_values; ++scalar_value_index)
					{
						boost::optional<double> scalar_value;
						if (all_scalar_values_are_active[scalar_value_index])
						{
							scalar_value = all_scalar_values[scalar_value_index];
						}

						// Note that boost::none gets translated to Python 'None'.
						scalar_values_list_object.append(scalar_value);
					}
				}
			}
			else // only active points...
			{
				std::vector<double> scalar_values;
				if (scalar_coverage_time_span->get_scalar_values(scalar_type, reconstruction_time, scalar_values))
				{
					for (auto scalar_value : scalar_values)
					{
						scalar_values_list_object.append(scalar_value);
					}
				}
			}

			return scalar_values_list_object;
		}
	}

	/**
	 * Returns the list of reconstructed geometry points (at reconstruction time).
	 */
	bp::object
	reconstructed_geometry_time_span_get_geometry_points(
			ReconstructedGeometryTimeSpan::non_null_ptr_type reconstructed_geometry_time_span,
			const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time,
			bool return_inactive_points)
	{
		// Reconstruction time must not be distant past/future.
		if (!reconstruction_time.is_real())
		{
			PyErr_SetString(PyExc_ValueError,
					"Reconstruction time cannot be distant-past (float('inf')) or distant-future (float('-inf')).");
			bp::throw_error_already_set();
		}

		// Return None if there are no active points at the reconstruction time.
		if (!reconstructed_geometry_time_span->get_geometry_time_span()->is_valid(reconstruction_time.value()))
		{
			return bp::object()/*Py_None*/;
		}

		GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span =
				reconstructed_geometry_time_span->get_geometry_time_span();

		return add_geometry_points_to_list(geometry_time_span, reconstruction_time.value(), return_inactive_points);
	}

	/**
	 * Returns the list of reconstructed geometry points (at reconstruction time).
	 */
	bp::object
	reconstructed_geometry_time_span_get_topology_point_locations(
			ReconstructedGeometryTimeSpan::non_null_ptr_type reconstructed_geometry_time_span,
			const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time,
			bool return_inactive_points)
	{
		// Reconstruction time must not be distant past/future.
		if (!reconstruction_time.is_real())
		{
			PyErr_SetString(PyExc_ValueError,
					"Reconstruction time cannot be distant-past (float('inf')) or distant-future (float('-inf')).");
			bp::throw_error_already_set();
		}

		// Return None if there are no active points at the reconstruction time.
		if (!reconstructed_geometry_time_span->get_geometry_time_span()->is_valid(reconstruction_time.value()))
		{
			return bp::object()/*Py_None*/;
		}

		GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span =
				reconstructed_geometry_time_span->get_geometry_time_span();

		return add_topology_point_locations_to_list(geometry_time_span, reconstruction_time.value(), return_inactive_points);
	}

	/**
	 * Returns the list of reconstructed scalar values (at reconstruction time) associated with the specified scalar type (if specified),
	 * otherwise returns a dict mapping available scalar types to their reconstructed scalar values (at reconstruction time).
	 */
	bp::object
	reconstructed_geometry_time_span_get_scalar_values(
			ReconstructedGeometryTimeSpan::non_null_ptr_type reconstructed_geometry_time_span,
			const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time,
			boost::optional<GPlatesPropertyValues::ValueObjectType> scalar_type,
			bool return_inactive_points)
	{
		// Reconstruction time must not be distant past/future.
		if (!reconstruction_time.is_real())
		{
			PyErr_SetString(PyExc_ValueError,
					"Reconstruction time cannot be distant-past (float('inf')) or distant-future (float('-inf')).");
			bp::throw_error_already_set();
		}

		// Return None if there are no active points at the reconstruction time.
		if (!reconstructed_geometry_time_span->get_geometry_time_span()->is_valid(reconstruction_time.value()))
		{
			return bp::object()/*Py_None*/;
		}

		GPlatesAppLogic::ScalarCoverageTimeSpan::non_null_ptr_type scalar_coverage_time_span =
				reconstructed_geometry_time_span->get_scalar_coverage_time_span();

		if (scalar_type)
		{
			// Look up the scalar type.
			if (!scalar_coverage_time_span->contains_scalar_type(scalar_type.get()))
			{
				// Not found.
				return bp::object()/*Py_None*/;
			}

			return add_scalar_values_to_list(
					scalar_coverage_time_span,
					scalar_type.get(),
					reconstruction_time.value(),
					return_inactive_points);
		}

		bp::dict scalar_values_dict;

		// Find all available scalar types contained in the scalar coverage.
		std::vector<GPlatesPropertyValues::ValueObjectType> available_scalar_types;
		scalar_coverage_time_span->get_scalar_types(available_scalar_types);

		// Iterate over scalar types.
		for (const auto &available_scalar_type : available_scalar_types)
		{
			boost::python::list curr_scalar_values_list_object = add_scalar_values_to_list(
					scalar_coverage_time_span,
					available_scalar_type,
					reconstruction_time.value(),
					return_inactive_points);

			// Map the current scalar type to its scalar values.
			scalar_values_dict[available_scalar_type] = curr_scalar_values_list_object;
		}

		return scalar_values_dict;
	}

	/**
	 * Returns true if point is not located in any resolved topologies.
	 */
	bool
	topology_point_not_located_in_resolved_topology(
			const GPlatesAppLogic::TopologyPointLocation &topology_point_location)
	{
		return topology_point_location.not_located();
	}

	/**
	 * Returns resolved topological boundary containing point, otherwise boost::none.
	 */
	boost::optional<GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_type>
	topology_point_located_in_resolved_boundary(
			const GPlatesAppLogic::TopologyPointLocation &topology_point_location)
	{
		return topology_point_location.located_in_resolved_boundary();
	}

	/**
	 * Returns resolved topological network if it contains point, otherwise None.
	 */
	boost::optional<GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type>
	topology_point_located_in_resolved_network(
			const GPlatesAppLogic::TopologyPointLocation &topology_point_location)
	{
		boost::optional<GPlatesAppLogic::TopologyPointLocation::network_location_type>
				network_location = topology_point_location.located_in_resolved_network();
		if (network_location)
		{
			GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type resolved_network = network_location->first;
			return resolved_network;
		}

		return boost::none;
	}

	/**
	 * Returns resolved topological network if its deforming region (excludes rigid blocks) contains point, otherwise None.
	 */
	boost::optional<GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type>
	topology_point_located_in_resolved_network_deforming_region(
			const GPlatesAppLogic::TopologyPointLocation &topology_point_location)
	{
		boost::optional<GPlatesAppLogic::TopologyPointLocation::network_location_type>
				network_location = topology_point_location.located_in_resolved_network();
		if (network_location)
		{
			GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type resolved_network = network_location->first;
			const GPlatesAppLogic::ResolvedTriangulation::Network::PointLocation &point_location = network_location->second;

			if (point_location.located_in_deforming_region())
			{
				return resolved_network;
			}
		}

		return boost::none;
	}

	/**
	 * Returns tuple (resolved topological network, rigid block RFG) containing point, otherwise None.
	 */
	bp::object
	topology_point_located_in_resolved_network_rigid_block(
			const GPlatesAppLogic::TopologyPointLocation &topology_point_location)
	{
		// Is located in a resolved network?
		boost::optional<GPlatesAppLogic::TopologyPointLocation::network_location_type>
				network_location = topology_point_location.located_in_resolved_network();
		if (network_location)
		{
			GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type resolved_network = network_location->first;
			const GPlatesAppLogic::ResolvedTriangulation::Network::PointLocation &point_location = network_location->second;

			// Is located in one of resolved network's rigid blocks?
			if (boost::optional<const GPlatesAppLogic::ResolvedTriangulation::Network::RigidBlock &> rigid_block =
				point_location.located_in_rigid_block())
			{
				return bp::make_tuple(resolved_network, rigid_block->get_reconstructed_feature_geometry());
			}
		}

		return bp::object()/*Py_None*/;
	}
}


GPlatesApi::TopologicalModel::non_null_ptr_type
GPlatesApi::TopologicalModel::create(
		const TopologicalFeatureCollectionSequenceFunctionArgument &topological_features,
		// Note we're using 'RotationModelFunctionArgument::function_argument_type' instead of
		// just 'RotationModelFunctionArgument' since we want to know if it's an existing RotationModel...
		const RotationModelFunctionArgument::function_argument_type &rotation_model_argument,
		boost::optional<GPlatesModel::integer_plate_id_type> anchor_plate_id,
		boost::optional<ResolveTopologyParameters::non_null_ptr_to_const_type> default_resolve_topology_parameters)
{
	boost::optional<RotationModel::non_null_ptr_type> rotation_model;

	//
	// Adapt an existing rotation model, or create a new rotation model.
	//
	if (const RotationModel::non_null_ptr_type *existing_rotation_model =
		boost::get<RotationModel::non_null_ptr_type>(&rotation_model_argument))
	{
		// Adapt the existing rotation model.
		rotation_model = RotationModel::create(
				*existing_rotation_model,
				// Start off with a cache size of 1 (later we'll increase it as needed)...
				1/*reconstruction_tree_cache_size*/,
				// If anchor plate ID is none then defaults to the default anchor plate of existing rotation model...
				anchor_plate_id);
	}
	else
	{
		const FeatureCollectionSequenceFunctionArgument rotation_feature_collections_function_argument =
				boost::get<FeatureCollectionSequenceFunctionArgument>(rotation_model_argument);

		// Create a new rotation model (from rotation features).
		rotation_model = RotationModel::create(
				rotation_feature_collections_function_argument,
				// Start off with a cache size of 1 (later we'll increase it as needed)...
				1/*reconstruction_tree_cache_size*/,
				false/*extend_total_reconstruction_poles_to_distant_past*/,
				// We're creating a new RotationModel from scratch (as opposed to adapting an existing one)
				// so the anchor plate ID defaults to zero if not specified...
				anchor_plate_id ? anchor_plate_id.get() : 0);
	}

	// If no resolve topology parameters specified then use default values.
	if (!default_resolve_topology_parameters)
	{
		default_resolve_topology_parameters = ResolveTopologyParameters::create();
	}

	return non_null_ptr_type(
			new TopologicalModel(topological_features, rotation_model.get(), default_resolve_topology_parameters.get()));
}


GPlatesApi::TopologicalModel::TopologicalModel(
		const TopologicalFeatureCollectionSequenceFunctionArgument &topological_features,
		const RotationModel::non_null_ptr_type &rotation_model,
		ResolveTopologyParameters::non_null_ptr_to_const_type default_resolve_topology_parameters) :
	d_rotation_model(rotation_model),
	d_topological_section_reconstruct_context(d_reconstruct_method_registry),
	d_topological_section_reconstruct_context_state(
			d_topological_section_reconstruct_context.create_context_state(
					GPlatesAppLogic::ReconstructMethodInterface::Context(
							GPlatesAppLogic::ReconstructParams(),
							d_rotation_model->get_reconstruction_tree_creator())))
{
	// Get the topological feature collections / files.
	topological_features.get_feature_collections(d_topological_feature_collections);
	topological_features.get_files(d_topological_files);
	// Get the associated resolved topology parameters.
	std::vector<boost::optional<ResolveTopologyParameters::non_null_ptr_to_const_type>> resolve_topology_parameters_list;
	topological_features.get_resolve_topology_parameters(resolve_topology_parameters_list);
	// Each feature collection has an optional associated resolve topology parameters.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			resolve_topology_parameters_list.size() == d_topological_feature_collections.size(),
			GPLATES_ASSERTION_SOURCE);

	// Separate into regular features (used as topological sections for topological lines/boundaries/networks),
	// topological lines (can also be used as topological sections for topological boundaries/networks),
	// topological boundaries and topological networks.
	//
	// This makes it faster to resolve topologies since resolving topological lines/boundaries/networks visits
	// only those topological features actually containing topological lines/boundaries/networks respectively
	// (because visiting, eg, a network feature when resolving boundary features requires visiting all the feature
	// properties of that network only to discard the network feature since it's not a topological boundary).
	const unsigned int num_feature_collections = d_topological_feature_collections.size();
	for (unsigned int feature_collection_index = 0; feature_collection_index < num_feature_collections; ++feature_collection_index)
	{
		auto feature_collection = d_topological_feature_collections[feature_collection_index];
		auto resolve_topology_parameters = resolve_topology_parameters_list[feature_collection_index];

		// If current feature collection did not specify resolve topology parameters then use the default parameters.
		if (!resolve_topology_parameters)
		{
			resolve_topology_parameters = default_resolve_topology_parameters;
		}

		for (auto feature : *feature_collection)
		{
			const GPlatesModel::FeatureHandle::weak_ref feature_ref = feature->reference();

			// Determine the topology geometry type.
			boost::optional<GPlatesAppLogic::TopologyGeometry::Type> topology_geometry_type =
					GPlatesAppLogic::TopologyUtils::get_topological_geometry_type(feature_ref);

			if (topology_geometry_type == GPlatesAppLogic::TopologyGeometry::LINE)
			{
				d_topological_line_features.push_back(feature_ref);
			}
			else if (topology_geometry_type == GPlatesAppLogic::TopologyGeometry::BOUNDARY)
			{
				d_topological_boundary_features.push_back(feature_ref);
			}
			else if (topology_geometry_type == GPlatesAppLogic::TopologyGeometry::NETWORK)
			{
				// Add the network feature to the group of network features associated with the
				// topology network params belonging to the current feature collection.
				//
				// If multiple feature collections have the same network parameters then all their
				// network features will end up in the same group.
				const auto &topology_network_params = resolve_topology_parameters.get()->get_topology_network_params();
				d_topological_network_features_map[topology_network_params].push_back(feature_ref);
			}
			else
			{
				d_topological_section_regular_features.push_back(feature_ref);
			}
		}
	}

	// Set the topological section regular features in the reconstruct context.
	d_topological_section_reconstruct_context.set_features(d_topological_section_regular_features);
}


GPlatesApi::TopologicalSnapshot::non_null_ptr_type
GPlatesApi::TopologicalModel::get_topological_snapshot(
		const double &reconstruction_time_arg)
{
	const GPlatesMaths::real_t reconstruction_time = std::round(reconstruction_time_arg);
	if (!GPlatesMaths::are_almost_exactly_equal(reconstruction_time.dval(), reconstruction_time_arg))
	{
		PyErr_SetString(PyExc_ValueError, "Reconstruction time should be an integral value.");
		bp::throw_error_already_set();
	}

	// Return existing snapshot if we've already cached one for the specified reconstruction time.
	auto topological_snapshot_find_result = d_cached_topological_snapshots.find(reconstruction_time);
	if (topological_snapshot_find_result != d_cached_topological_snapshots.end())
	{
		return topological_snapshot_find_result->second;
	}

	//
	// Create a new snapshot.
	//

	// First we want to have a suitably large reconstruction tree cache size in our rotation model to
	// avoid slowing down our reconstruct-by-topologies (which happens if reconstruction trees are
	// continually evicted and re-populated as we reconstruct different geometries through time).
	//
	// The +1 accounts for the extra time step used to generate deformed geometries (and velocities).
	const unsigned int reconstruction_tree_cache_size = d_cached_topological_snapshots.size() + 1;
	d_rotation_model->get_cached_reconstruction_tree_creator_impl()->set_maximum_cache_size(
			reconstruction_tree_cache_size);

	// Create snapshot.
	TopologicalSnapshot::non_null_ptr_type topological_snapshot =
			create_topological_snapshot(reconstruction_time.dval());

	// Cache snapshot.
	d_cached_topological_snapshots.insert(
			topological_snapshots_type::value_type(reconstruction_time, topological_snapshot));

	return topological_snapshot;
}


GPlatesApi::TopologicalSnapshot::non_null_ptr_type
GPlatesApi::TopologicalModel::create_topological_snapshot(
		const double &reconstruction_time)
{
	// Find the topological section feature IDs referenced by any topological features at current reconstruction time.
	//
	// This is an optimisation that avoids unnecessary reconstructions. Only those topological sections referenced
	// by topologies that exist at the reconstruction time are reconstructed.
	std::set<GPlatesModel::FeatureId> topological_sections_referenced;
	GPlatesAppLogic::TopologyInternalUtils::find_topological_sections_referenced(
			topological_sections_referenced,
			d_topological_line_features,
			GPlatesAppLogic::TopologyGeometry::LINE,
			reconstruction_time);
	GPlatesAppLogic::TopologyInternalUtils::find_topological_sections_referenced(
			topological_sections_referenced,
			d_topological_boundary_features,
			GPlatesAppLogic::TopologyGeometry::BOUNDARY,
			reconstruction_time);
	for (const auto &topological_network_features_map_entry : d_topological_network_features_map)
	{
		const topological_features_seq_type &topological_network_features = topological_network_features_map_entry.second;

		GPlatesAppLogic::TopologyInternalUtils::find_topological_sections_referenced(
				topological_sections_referenced,
				topological_network_features,
				GPlatesAppLogic::TopologyGeometry::NETWORK,
				reconstruction_time);
	}

	// Contains the topological section regular geometries referenced by topologies.
	std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type> reconstructed_feature_geometries;

	// Generate RFGs only for the referenced topological sections.
	const GPlatesAppLogic::ReconstructHandle::type reconstruct_handle =
			d_topological_section_reconstruct_context.get_reconstructed_topological_sections(
					reconstructed_feature_geometries,
					topological_sections_referenced,
					d_topological_section_reconstruct_context_state,
					reconstruction_time);

	// All reconstruct handles used to find topological sections (referenced by topological boundaries/networks).
	std::vector<GPlatesAppLogic::ReconstructHandle::type> topological_sections_reconstruct_handles(1, reconstruct_handle);

	// Resolving topological lines generates its own reconstruct handle that will be used by
	// topological boundaries and networks to find this group of resolved lines.
	//
	// So we always resolve topological *lines* regardless of whether the user requested it or not.
	std::vector<GPlatesAppLogic::ResolvedTopologicalLine::non_null_ptr_type> resolved_lines;
	const GPlatesAppLogic::ReconstructHandle::type resolved_topological_lines_handle =
			GPlatesAppLogic::TopologyUtils::resolve_topological_lines(
					// Contains the resolved topological line sections referenced by topological boundaries and networks...
					resolved_lines,
					d_topological_line_features,
					d_rotation_model->get_reconstruction_tree_creator(), 
					reconstruction_time,
					// Resolved topo lines use the reconstructed non-topo geometries...
					topological_sections_reconstruct_handles
	// NOTE: We need to generate all resolved topological lines, not just those referenced by resolved boundaries/networks,
	//       because the user may later explicitly request the resolved topological lines (or explicitly export them)...
#if 0
					// Only those topo lines referenced by resolved boundaries/networks...
					, topological_sections_referenced
#endif
			);

	topological_sections_reconstruct_handles.push_back(resolved_topological_lines_handle);

	// Resolve topological boundaries.
	std::vector<GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_type> resolved_boundaries;
	GPlatesAppLogic::TopologyUtils::resolve_topological_boundaries(
			resolved_boundaries,
			d_topological_boundary_features,
			d_rotation_model->get_reconstruction_tree_creator(), 
			reconstruction_time,
			// Resolved topo boundaries use the resolved topo lines *and* the reconstructed non-topo geometries...
			topological_sections_reconstruct_handles);

	// Resolve topological networks.
	//
	// Different network features can have a different resolve topology parameters so resolve them separately.
	std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type> resolved_networks;
	for (const auto &topological_network_features_map_entry : d_topological_network_features_map)
	{
		const GPlatesAppLogic::TopologyNetworkParams &topology_network_params = topological_network_features_map_entry.first;
		const topological_features_seq_type &topological_network_features = topological_network_features_map_entry.second;

		GPlatesAppLogic::TopologyUtils::resolve_topological_networks(
				resolved_networks,
				reconstruction_time,
				topological_network_features,
				// Resolved topo networks use the resolved topo lines *and* the reconstructed non-topo geometries...
				topological_sections_reconstruct_handles,
				topology_network_params);
	}

	return TopologicalSnapshot::create(
			resolved_lines, resolved_boundaries, resolved_networks,
			d_topological_files, d_rotation_model, reconstruction_time);
}


GPlatesApi::ReconstructedGeometryTimeSpan::non_null_ptr_type
GPlatesApi::TopologicalModel::reconstruct_geometry(
		bp::object geometry_object,
		const GPlatesPropertyValues::GeoTimeInstant &initial_time,
		boost::optional<GPlatesPropertyValues::GeoTimeInstant> oldest_time_arg,
		const GPlatesPropertyValues::GeoTimeInstant &youngest_time_arg,
		const double &time_increment_arg,
		boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id,
		bp::object scalar_type_to_initial_scalar_values_mapping_object,
		boost::optional<GPlatesAppLogic::TopologyReconstruct::DeactivatePoint::non_null_ptr_to_const_type> deactivate_points)
{
	// Initial reconstruction time must not be distant past/future.
	if (!initial_time.is_real())
	{
		PyErr_SetString(PyExc_ValueError,
				"Initial reconstruction time cannot be distant-past (float('inf')) or distant-future (float('-inf')).");
		bp::throw_error_already_set();
	}

	// Oldest time defaults to initial reconstruction time if not specified.
	if (!oldest_time_arg)
	{
		oldest_time_arg = initial_time;
	}

	if (!oldest_time_arg->is_real() ||
		!youngest_time_arg.is_real())
	{
		PyErr_SetString(PyExc_ValueError,
				"Oldest/youngest times cannot be distant-past (float('inf')) or distant-future (float('-inf')).");
		bp::throw_error_already_set();
	}

	// We are expecting these to have integral values.
	const double oldest_time = std::round(oldest_time_arg->value());
	const double youngest_time = std::round(youngest_time_arg.value());
	const double time_increment = std::round(time_increment_arg);

	if (!GPlatesMaths::are_almost_exactly_equal(oldest_time, oldest_time_arg->value()) ||
		!GPlatesMaths::are_almost_exactly_equal(youngest_time, youngest_time_arg.value()) ||
		!GPlatesMaths::are_almost_exactly_equal(time_increment, time_increment_arg))
	{
		PyErr_SetString(PyExc_ValueError, "Oldest/youngest times and time increment must have integral values.");
		bp::throw_error_already_set();
	}

	if (oldest_time <= youngest_time)
	{
		PyErr_SetString(PyExc_ValueError, "Oldest time cannot be later than (or same as) youngest time.");
		bp::throw_error_already_set();
	}

	if (time_increment <= 0)
	{
		PyErr_SetString(PyExc_ValueError, "Time increment must be positive.");
		bp::throw_error_already_set();
	}

	const GPlatesAppLogic::TimeSpanUtils::TimeRange time_range(
			oldest_time/*begin_time*/, youngest_time/*end_time*/, time_increment,
			// If time increment was specified correctly then it shouldn't need to be adjusted...
			GPlatesAppLogic::TimeSpanUtils::TimeRange::ADJUST_TIME_INCREMENT);
	if (!GPlatesMaths::are_almost_exactly_equal(time_range.get_time_increment(), time_increment))
	{
		PyErr_SetString(PyExc_ValueError,
				"Oldest to youngest time period must be an integer multiple of the time increment.");
		bp::throw_error_already_set();
	}

	// Create our resolved topology (boundary/network) time spans.
	GPlatesAppLogic::TopologyReconstruct::resolved_boundary_time_span_type::non_null_ptr_type resolved_boundary_time_span =
			GPlatesAppLogic::TopologyReconstruct::resolved_boundary_time_span_type::create(time_range);
	GPlatesAppLogic::TopologyReconstruct::resolved_network_time_span_type::non_null_ptr_type resolved_network_time_span =
			GPlatesAppLogic::TopologyReconstruct::resolved_network_time_span_type::create(time_range);

	const unsigned int num_time_slots = time_range.get_num_time_slots();

	// Iterate over the time slots and fill in the resolved topological boundaries/networks.
	for (unsigned int time_slot = 0; time_slot < num_time_slots; ++time_slot)
	{
		const double time = time_range.get_time(time_slot);

		// Get topological snapshot (it'll either be cached or generated on demand).
		TopologicalSnapshot::non_null_ptr_type topological_snapshot = get_topological_snapshot(time);

		resolved_boundary_time_span->set_sample_in_time_slot(topological_snapshot->get_resolved_topological_boundaries(), time_slot);
		resolved_network_time_span->set_sample_in_time_slot(topological_snapshot->get_resolved_topological_networks(), time_slot);
	}

	GPlatesAppLogic::TopologyReconstruct::non_null_ptr_type topology_reconstruct =
			GPlatesAppLogic::TopologyReconstruct::create(
					time_range,
					resolved_boundary_time_span,
					resolved_network_time_span,
					d_rotation_model->get_reconstruction_tree_creator());

	// Extract the geometry.
	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry = get_geometry(geometry_object);

	// Create time span of reconstructed geometry.
	GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span =
			topology_reconstruct->create_geometry_time_span(
					geometry,
					// If a reconstruction plate ID is not specified then use the default anchor plate ID
					// of our rotation model...
					reconstruction_plate_id
							? reconstruction_plate_id.get()
							: d_rotation_model->get_reconstruction_tree_creator().get_default_anchor_plate_id(),
					initial_time.value(),
					deactivate_points);

	// Extract the optional initial scalar values.
	GPlatesAppLogic::ScalarCoverageTimeSpan::initial_scalar_coverage_type initial_scalar_coverage;
	if (scalar_type_to_initial_scalar_values_mapping_object != bp::object()/*Py_None*/)
	{
		// Extract the mapping of scalar types to scalar values.
		initial_scalar_coverage = create_scalar_type_to_values_map(
				scalar_type_to_initial_scalar_values_mapping_object);

		// Number of points in domain must match number of scalar values in range.
		const unsigned int num_domain_geometry_points =
				GPlatesAppLogic::GeometryUtils::get_num_geometry_points(*geometry);
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				!initial_scalar_coverage.empty(),
				GPLATES_ASSERTION_SOURCE);
		// Just test the scalar values length for the first scalar type (all types should already have the same length).
		if (num_domain_geometry_points != initial_scalar_coverage.begin()->second.size())
		{
			PyErr_SetString(PyExc_ValueError, "Number of scalar values must match number of points in geometry");
			bp::throw_error_already_set();
		}
	}

	// Create the scalar coverage from the initial scalar values and the geometry time span.
	GPlatesAppLogic::ScalarCoverageTimeSpan::non_null_ptr_type scalar_coverage_time_span =
			GPlatesAppLogic::ScalarCoverageTimeSpan::create(initial_scalar_coverage, geometry_time_span);

	return ReconstructedGeometryTimeSpan::create(geometry_time_span, scalar_coverage_time_span);
}


void
export_topological_model()
{
	//
	// TopologyPointLocation - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<GPlatesAppLogic::TopologyPointLocation>(
					"TopologyPointLocation",
					"Locates a point in a specific resolved topological boundary or network (deforming region or interior rigid block).\n"
					"\n"
					"  .. versionadded:: 29\n",
					// Don't allow creation from python side...
					bp::no_init)
		.def("not_located_in_resolved_topology",
				&GPlatesApi::topology_point_not_located_in_resolved_topology,
				"not_located_in_resolved_topology()\n"
				"  Query if point is not located in any resolved topological boundaries or networks.\n"
				"\n"
				"  :returns: ``True`` if point is not located in any resolved topologies\n"
				"  :rtype: bool\n")
		.def("located_in_resolved_boundary",
				&GPlatesApi::topology_point_located_in_resolved_boundary,
				"located_in_resolved_boundary()\n"
				"  Query if point is located in a :class:`resolved topological boundary<ResolvedTopologicalBoundary>`.\n"
				"\n"
				"  :returns: the resolved topological boundary that contains the point, otherwise ``None``\n"
				"  :rtype: :class:`ResolvedTopologicalBoundary` or ``None``\n")
		.def("located_in_resolved_network",
				&GPlatesApi::topology_point_located_in_resolved_network,
				"located_in_resolved_network()\n"
				"  Query if point is located in a :class:`resolved topological network<ResolvedTopologicalNetwork>`.\n"
				"\n"
				"  :returns: the resolved topological network that contains the point, otherwise ``None``\n"
				"  :rtype: :class:`ResolvedTopologicalNetwork` or ``None``\n"
				"\n"
				"  .. note:: The point can be anywhere inside a resolved topological network - inside its deforming region "
				"or inside any one of its interior rigid blocks (if it has any).\n")
		.def("located_in_resolved_network_deforming_region",
				&GPlatesApi::topology_point_located_in_resolved_network_deforming_region,
				"located_in_resolved_network_deforming_region()\n"
				"  Query if point is located in the deforming region of a :class:`resolved topological network<ResolvedTopologicalNetwork>`.\n"
				"\n"
				"  :returns: the resolved topological network whose deforming region contains the point, otherwise ``None``\n"
				"  :rtype: :class:`ResolvedTopologicalNetwork` or ``None``\n"
				"\n"
				"  .. note:: Returns ``None`` if point is inside a resolved topological network but is also inside one of "
				"its interior rigid blocks (and hence not inside its deforming region).\n")
		.def("located_in_resolved_network_rigid_block",
				&GPlatesApi::topology_point_located_in_resolved_network_rigid_block,
				"located_in_resolved_network_rigid_block()\n"
				"  Query if point is located in an interior rigid block of a :class:`resolved topological network<ResolvedTopologicalNetwork>`.\n"
				"\n"
				"  :returns: tuple of resolved topological network and its interior rigid block (that contains the point), otherwise ``None``\n"
				"  :rtype: 2-tuple (:class:`ResolvedTopologicalNetwork`, :class:`ReconstructedFeatureGeometry`),  or ``None``\n"
				"\n"
				"  .. note:: Returns ``None`` if point is inside a resolved topological network but is *not* inside one of "
				"its interior rigid blocks.\n")
		// Make unhashable, with no comparison operators...
		.def(GPlatesApi::NoHashDefVisitor(false, false))
	;

	// Enable boost::optional<TopologyPointLocation> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesAppLogic::TopologyPointLocation>();

	{
		//
		// ReconstructedGeometryTimeSpan - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
		//
		bp::scope reconstructed_geometry_time_span_class = bp::class_<
				GPlatesApi::ReconstructedGeometryTimeSpan,
				GPlatesApi::ReconstructedGeometryTimeSpan::non_null_ptr_type,
				boost::noncopyable>(
						"ReconstructedGeometryTimeSpan",
						"A history of geometries reconstructed using topologies over geological time.\n"
						"\n"
						"  .. versionadded:: 29\n",
						// Don't allow creation from python side...
						// (Also there is no publicly-accessible default constructor).
						bp::no_init)
			.def("get_geometry_points",
					&GPlatesApi::reconstructed_geometry_time_span_get_geometry_points,
					(bp::arg("reconstruction_time"),
						bp::arg("return_inactive_points") = false),
					"get_geometry_points(reconstruction_time, [return_inactive_points=False])\n"
					"  Returns geometry points at a specific reconstruction time.\n"
					"\n"
					"  :param reconstruction_time: Time to extract reconstructed geometry points. Can be any non-negative time.\n"
					"  :type reconstruction_time: float or :class:`GeoTimeInstant`\n"
					"  :param return_inactive_points: Whether to return inactive geometry points. "
					"If ``True`` then each inactive point stores ``None`` instead of a point and hence the size of each ``list`` "
					"of points is equal to the number of points in the initial geometry (which are all initially active). "
					"By default only active points are returned.\n"
					"  :returns: list of :class:`PointOnSphere`, or ``None`` if no points are active at *reconstruction_time*\n"
					"  :rtype: ``list`` or ``None``\n"
					"  :raises: ValueError if *reconstruction_time* is "
					":meth:`distant past<GeoTimeInstant.is_distant_past>` or "
					":meth:`distant future<GeoTimeInstant.is_distant_future>`\n")
			.def("get_topology_point_locations",
					&GPlatesApi::reconstructed_geometry_time_span_get_topology_point_locations,
					(bp::arg("reconstruction_time"),
						bp::arg("return_inactive_points") = false),
					"get_topology_point_locations(reconstruction_time, [return_inactive_points=False])\n"
					"  Returns the locations of geometry points in resolved topologies at a specific reconstruction time.\n"
					"\n"
					"  :param reconstruction_time: Time to extract topology point locations. Can be any non-negative time.\n"
					"  :type reconstruction_time: float or :class:`GeoTimeInstant`\n"
					"  :param return_inactive_points: Whether to return topology locations associated with inactive points. "
					"If ``True`` then each topology location corresponding to an inactive point stores ``None`` instead of a "
					"topology location and hence the size of each ``list`` of topology locations is equal to the number of points "
					"in the initial geometry (which are all initially active). "
					"By default only topology locations for active points are returned.\n"
					"  :returns: list of :class:`TopologyPointLocation`, or ``None`` if no points are active at *reconstruction_time*\n"
					"  :rtype: ``list`` or ``None``\n"
					"  :raises: ValueError if *reconstruction_time* is "
					":meth:`distant past<GeoTimeInstant.is_distant_past>` or "
					":meth:`distant future<GeoTimeInstant.is_distant_future>`\n")
			.def("get_scalar_values",
					&GPlatesApi::reconstructed_geometry_time_span_get_scalar_values,
					(bp::arg("reconstruction_time"),
						bp::arg("scalar_type") = boost::optional<GPlatesPropertyValues::ValueObjectType>(),
						bp::arg("return_inactive_points") = false),
					"get_scalar_values(reconstruction_time, [scalar_type], [return_inactive_points=False])\n"
					"  Returns scalar values at a specific reconstruction time either for a single scalar type (as a ``list``) or "
					"for all scalar types (as a ``dict``).\n"
					"\n"
					"  :param reconstruction_time: Time to extract reconstructed scalar values. Can be any non-negative time.\n"
					"  :type reconstruction_time: float or :class:`GeoTimeInstant`\n"
					"  :param scalar_type: Optional scalar type to retrieve scalar values for (returned as a ``list``). "
					"If not specified then all scalar values for all scalar types are returned (returned as a ``dict``).\n"
					"  :type scalar_type: :class:`ScalarType`\n"
					"  :param return_inactive_points: Whether to return scalars associated with inactive points. "
					"If ``True`` then each scalar corresponding to an inactive point stores ``None`` instead of a scalar and hence "
					"the size of each ``list`` of scalars is equal to the number of points (and scalars) in the initial geometry "
					"(which are all initially active). "
					"By default only scalars for active points are returned.\n"
					"  :returns: If *scalar_type* is specified then a ``list`` of scalar values associated with *scalar_type* "
					"at *reconstruction_time* (or ``None`` if no matching scalar type), otherwise a ``dict`` mapping available "
					"scalar types with their associated scalar values ``list`` at *reconstruction_time* (or ``None`` if no scalar types "
					"are available). Returns ``None`` if no points are active at *reconstruction_time*.\n"
					"  :rtype: ``list`` or ``dict`` or ``None``\n"
					"  :raises: ValueError if *reconstruction_time* is "
					":meth:`distant past<GeoTimeInstant.is_distant_past>` or "
					":meth:`distant future<GeoTimeInstant.is_distant_future>`\n")
			// Make hash and comparisons based on C++ object identity (not python object identity)...
			.def(GPlatesApi::ObjectIdentityHashDefVisitor())
		;

		// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
		GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesApi::ReconstructedGeometryTimeSpan>();


		//
		// ReconstructedGeometryTimeSpan.DeactivatePoints - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
		//
		// A class nested within python class ReconstructedGeometryTimeSpan (due to above 'bp::scope').
		bp::class_<
				GPlatesAppLogic::TopologyReconstruct::DeactivatePoint,
				GPlatesApi::ReconstructedGeometryTimeSpan::DeactivatePointWrapper::non_null_ptr_type,
				boost::noncopyable>(
						"DeactivatePoints",
						// NOTE: It seems Sphinx does not document '__init__' for nested classes (tested with Sphinx 3.4.3).
						//       Instead we'll document it in this *class* docstring.
						"The base class interface for deactivating geometry points as they are reconstructed forward and/or backward in time.\n"
						"\n"
						"To create your own class that inherits this base class and overrides its "
						":meth:`deactivate method <ReconstructedGeometryTimeSpan.DeactivatePoints.deactivate>` and then use that "
						"when :meth:`reconstructing a geometry using topologies <TopologicalModel.reconstruct_geometry>`:\n"
						"::\n"
						"\n"
						"  class MyDeactivatePoints(pygplates.ReconstructedGeometryTimeSpan.DeactivatePoints):\n"
						"      def __init__(self):\n"
						"          super(MyDeactivatePoints, self).__init__()\n"
						"          # Other initialisation you may want...\n"
						"          ...\n"
						"      def deactivate(self, prev_point, prev_location, prev_time, current_point, current_location, current_time):\n"
						"          # Implement your deactivation algorithm here...\n"
						"          ...\n"
						"          return ...\n"
						"  \n"
						"  # Reconstruct points in 'geometry' from 100Ma to present day using class MyDeactivatePoints to deactivate them (in this case subduct).\n"
						"  topological_model.reconstruct_geometry(geometry, 100, deactivate_points=MyDeactivatePoints())\n"
						"\n"
						".. warning:: If you create your own Python class that inherits this base class then you must call the base class *__init__* "
						"method otherwise you will get a *Boost.Python.ArgumentError* exception. Note that if you do not define an *__init__* method "
						"in your derived class then Python will call the base class *__init__* (so you don't have to do anything). "
						"However if you do define *__init__* in your derived class then it must explicitly call the base class *__init__*.\n"
						"\n"
						".. versionadded:: 31\n"
						"\n"
						"__init__()\n"
						"  Default constructor - must be explicitly called by derived class.\n"
						// NOTE: Must not define 'bp::no_init' because this base class is meant to be inherited by a python class.
					)
			.def("deactivate",
					bp::pure_virtual(&GPlatesAppLogic::TopologyReconstruct::DeactivatePoint::deactivate),
					(bp::arg("prev_point"),
						bp::arg("prev_location"),
						bp::arg("prev_time"),
						bp::arg("current_point"),
						bp::arg("current_location"),
						bp::arg("current_time")),
					// NOTE: It seems Sphinx does properly document parameters of methods of nested classes (tested with Sphinx 3.4.3).
					//       Instead we'll document the parameters using a list.
					"deactivate(prev_point, prev_location, current_point, current_location, current_time, reverse_reconstruct)\n"
					"  Return true if the point should be deactivated.\n"
					"\n"
					"  * **prev_point** (:class:`PointOnSphere`): the previous position of the point\n"
					"\n"
					"  * **prev_location** (:class:`TopologyPointLocation`): the previous location of the point in the topologies\n"
					"\n"
					"  * **prev_time** (float or :class:`GeoTimeInstant`): the time associated with the previous position of the point\n"
					"\n"
					"  * **current_point** (:class:`PointOnSphere`): the current position of the point\n"
					"\n"
					"  * **current_location** (:class:`TopologyPointLocation`): the current location of the point in the topologies\n"
					"\n"
					"  * **current_time** (float or :class:`GeoTimeInstant`): the time associated with the current position of the point\n"
					"\n"
					"  * **Return type**: bool\n"
					"\n"
					"  The above parameters represent the previous and current position/location-in-topologies/time of a single point in the "
					":meth:`geometry being reconstructed <TopologicalModel.reconstruct_geometry>`. If you return ``True`` then the point will be "
					"deactivated and will not have a position at the *next* time (where ``next_time = current_time + (current_time - prev_time)``).\n"
					"\n"
					".. note:: If the current time is *younger* than the previous time (``current_time < prev_time``) then we are reconstructing "
					"*forward* in time and the next time will be *younger* than the current time (``next_time < current_time``). Conversely, if "
					"the current time is *older* than the previous time (``current_time > prev_time``) then we are reconstructing "
					"*backward* in time and the next time will be *older* than the current time (``next_time > current_time``).\n"
					"\n"
					".. note:: This function is called for each point that is reconstructed using :meth:`TopologicalModel.reconstruct_geometry` "
					"at each time step.\n"
					"\n"
					// For some reason Sphinx (tested version 3.4.3) seems to repeat this docstring twice (not sure why, so we'll let users know)...
					".. note:: This function might be inadvertently documented twice.\n")
		;

		// Enable GPlatesAppLogic::TopologyReconstruct::DeactivatePoint::non_null_ptr_type to be stored in a Python object.
		// Normally the HeldType GPlatesApi::ReconstructedGeometryTimeSpan::DeactivatePointWrapper::non_null_ptr_type is stored.
		//
		// For example, this enables:
		//
		//   bp::arg("deactivate_points") = boost::optional<GPlatesAppLogic::TopologyReconstruct::DeactivatePoint::non_null_ptr_to_const_type>()
		//
		// ...as a default argument in 'TopologicalModel.reconstruct_geometry()' because bp::arg stores its default value in a Python object.
		//
		// Without 'bp::register_ptr_to_python<GPlatesAppLogic::TopologyReconstruct::DeactivatePoint::non_null_ptr_type>()'
		// (and also 'PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesAppLogic::TopologyReconstruct::DeactivatePoint>()')
		// the above bp::arg statement would fail.
		// 
		bp::register_ptr_to_python<GPlatesAppLogic::TopologyReconstruct::DeactivatePoint::non_null_ptr_type>();

		GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<
				GPlatesAppLogic::TopologyReconstruct::DeactivatePoint>();


		// Docstring for class pygplates.ReconstructedGeometryTimeSpan.DefaultDeactivatePoints.
		//
		// NOTE: It seems Sphinx does not document '__init__' for nested classes (tested with Sphinx 3.4.3).
		//       Instead we'll document it in this *class* docstring.
		std::stringstream default_deactivate_points_class_docstring_stream;
		default_deactivate_points_class_docstring_stream <<
				"The default algorithm for deactivating geometry points as they are reconstructed forward and/or backward in time.\n"
				"\n"
				".. versionadded:: 31\n"
				"\n"
				"__init__([threshold_velocity_delta="
				<< GPlatesAppLogic::TopologyReconstruct::DefaultDeactivatePoint::DEFAULT_THRESHOLD_VELOCITY_DELTA
				<< "], [threshold_distance_to_boundary="
				<< GPlatesAppLogic::TopologyReconstruct::DefaultDeactivatePoint::DEFAULT_THRESHOLD_DISTANCE_TO_BOUNDARY_IN_KMS_PER_MY
				<< "], [deactivate_points_that_fall_outside_a_network="
				<< (GPlatesAppLogic::TopologyReconstruct::DefaultDeactivatePoint::DEFAULT_DEACTIVATE_POINTS_THAT_FALL_OUTSIDE_A_NETWORK ? "True" : "False")
				<< "])\n"
				"  Create default algorithm for deactivating points using the specified parameters.\n"
				"\n"
				"  * **threshold_velocity_delta** (float): A point that transitions from one plate/network to another can "
				"disappear if the change in velocity exceeds this threshold (in units of cms/yr). Defaults to ``"
				<< GPlatesAppLogic::TopologyReconstruct::DefaultDeactivatePoint::DEFAULT_THRESHOLD_VELOCITY_DELTA
				<< "`` cms/yr.\n"
				"\n"
				"  * **threshold_distance_to_boundary** (float): Only those transitioning points exceeding the "
				"*threshold velocity delta* **and** that are close enough to a plate/network boundary can disappear. "
				"The distance is proportional to the relative velocity (change in velocity), plus a constant offset based on the "
				"*threshold distance to boundary* (in units of kms/myr) to account for plate boundaries that change shape significantly "
				"from one time step to the next (note that some boundaries are meant to do this and others are a result of digitisation). "
				"The actual distance threshold used is ``(threshold_distance_to_boundary + relative_velocity) * time_increment``. Defaults to ``"
				<< GPlatesAppLogic::TopologyReconstruct::DefaultDeactivatePoint::DEFAULT_THRESHOLD_DISTANCE_TO_BOUNDARY_IN_KMS_PER_MY
				<< "`` kms/myr.\n"
				"\n"
				"  * **deactivate_points_that_fall_outside_a_network** (bool): Whether to have points inside a deforming network "
				"disappear as soon as they fall outside all deforming networks. This is useful for initial crustal thickness points that have "
				"been generated inside a deforming network and where subsequently deformed points should be limited to the deformed network regions. "
				"In this case sudden large changes to the deforming network boundary can progressively exclude points over time. "
				"However in the case where the topologies (deforming networks and rigid plates) have global coverage this option should "
				"generally be left disabled so that points falling outside deforming networks can then be reconstructed using rigid plates. "
				"And these rigidly reconstructed points may even re-enter a subsequent deforming network. Defaults to ``"
				<< (GPlatesAppLogic::TopologyReconstruct::DefaultDeactivatePoint::DEFAULT_DEACTIVATE_POINTS_THAT_FALL_OUTSIDE_A_NETWORK ? "True" : "False")
				<< "``.\n"
				"\n"
				".. note:: This is the default algorithm used internally.\n"
				"\n"
				"To use the default deactivation algorithm (this class) but with some non-default parameters, and then use that "
				"when :meth:`reconstructing a geometry using topologies <TopologicalModel.reconstruct_geometry>`:\n"
				"::\n"
				"\n"
				"  # Reconstruct points in 'geometry' from 100Ma to present day using this class to deactivate them (in this case subduct).\n"
				"  topological_model.reconstruct_geometry(\n"
				"      geometry,\n"
				"      100,\n"
				"      deactivate_points = pygplates.ReconstructedGeometryTimeSpan.DefaultDeactivatePoints(\n"
				"          # Choose our own parameters that are different than the defaults.\n"
				"          threshold_velocity_delta = 0.9, # cms/yr\n"
				"          threshold_distance_to_boundary = 15, # kms/myr\n"
				"          deactivate_points_that_fall_outside_a_network = True))\n"
				;

		//
		// ReconstructedGeometryTimeSpan.DeactivatePoints - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
		//
		// A class nested within python class ReconstructedGeometryTimeSpan (due to above 'bp::scope').
		bp::class_<
				GPlatesAppLogic::TopologyReconstruct::DefaultDeactivatePoint,
				GPlatesAppLogic::TopologyReconstruct::DefaultDeactivatePoint::non_null_ptr_type,
				bp::bases<GPlatesAppLogic::TopologyReconstruct::DeactivatePoint>,
				boost::noncopyable>(
						"DefaultDeactivatePoints",
						default_deactivate_points_class_docstring_stream.str().c_str(),
						// There is no publicly-accessible default constructor...
						bp::no_init)
			.def("__init__",
					bp::make_constructor(
							&GPlatesApi::reconstructed_geometry_time_span_default_deactivate_points_create,
							bp::default_call_policies(),
							(bp::arg("threshold_velocity_delta") =
									GPlatesAppLogic::TopologyReconstruct::DefaultDeactivatePoint::DEFAULT_THRESHOLD_VELOCITY_DELTA,
								bp::arg("threshold_distance_to_boundary") =
									GPlatesAppLogic::TopologyReconstruct::DefaultDeactivatePoint::DEFAULT_THRESHOLD_DISTANCE_TO_BOUNDARY_IN_KMS_PER_MY,
								bp::arg("deactivate_points_that_fall_outside_a_network") =
									GPlatesAppLogic::TopologyReconstruct::DefaultDeactivatePoint::DEFAULT_DEACTIVATE_POINTS_THAT_FALL_OUTSIDE_A_NETWORK)))
		;

		// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
		GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<
				GPlatesAppLogic::TopologyReconstruct::DefaultDeactivatePoint>();
	}


	//
	// TopologicalModel - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesApi::TopologicalModel,
			GPlatesApi::TopologicalModel::non_null_ptr_type,
			boost::noncopyable>(
					"TopologicalModel",
					"A history of topologies over geological time.\n"
					"\n"
					"  .. versionadded:: 30\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::topological_model_create,
						bp::default_call_policies(),
						(bp::arg("topological_features"),
							bp::arg("rotation_model"),
							bp::arg("anchor_plate_id") = boost::optional<GPlatesModel::integer_plate_id_type>(),
							bp::arg("default_resolve_topology_parameters") =
								boost::optional<GPlatesApi::ResolveTopologyParameters::non_null_ptr_to_const_type>())),
			"__init__(topological_features, rotation_model, [anchor_plate_id], [default_resolve_topology_parameters])\n"
			"  Create from topological features, a rotation model and a time span.\n"
			"\n"
			"  :param topological_features: The topological boundary and/or network features and the "
			"topological section features they reference (regular and topological lines) as a feature collection, "
			"or filename, or feature, or sequence of features, or a sequence (eg, ``list`` or ``tuple``) "
			"of any combination of those four types. Note: Each sequence entry can optionally be a 2-tuple "
			"(entry, :class:`ResolveTopologyParameters`) to override *default_resolve_topology_parameters* for that entry.\n"
			"  :type topological_features: :class:`FeatureCollection`, or string, or :class:`Feature`, "
			"or sequence of :class:`Feature`, or sequence of any combination of those four types\n"
			"  :param rotation_model: A rotation model or a rotation feature collection or a rotation "
			"filename or a sequence of rotation feature collections and/or rotation filenames\n"
			"  :type rotation_model: :class:`RotationModel` or :class:`FeatureCollection` or string "
			"or sequence of :class:`FeatureCollection` instances and/or strings\n"
			"  :param anchor_plate_id: The anchored plate id used for all reconstructions "
			"(resolving topologies, and reconstructing regular features and :meth:`geometries<reconstruct_geometry>`). "
			"Defaults to the default anchor plate of *rotation_model*.\n"
			"  :type anchor_plate_id: int\n"
			"  :param default_resolve_topology_parameters: Default parameters used to resolve topologies. "
			"Note that these can optionally be overridden in *topological_features*. "
			"Defaults to :meth:`default-constructed ResolveTopologyParameters<ResolveTopologyParameters.__init__>`).\n"
			"  :type default_resolve_topology_parameters: :class:`ResolveTopologyParameters`\n"
			"\n"
			"  Load a topological model (and its associated rotation model):\n"
			"  ::\n"
			"\n"
			"    rotation_model = pygplates.RotationModel('rotations.rot')\n"
			"    topological_model = pygplates.TopologicalModel('topologies.gpml', rotation_model)\n"
			"\n"
			"  ...or alternatively just:"
			"  ::\n"
			"\n"
			"    topological_model = pygplates.TopologicalModel('topologies.gpml', 'rotations.rot')\n"
			"\n"
			"  .. note:: All reconstructions (including resolving topologies and reconstructing regular features and "
			":meth:`geometries<reconstruct_geometry>`) use *anchor_plate_id*. So if you need to use a different "
			"anchor plate ID then you'll need to create a new :class:`TopologicalModel<__init__>`. However this should "
			"only be done if necessary since each :class:`TopologicalModel` created can consume a reasonable amount of "
			"CPU and memory (since it caches resolved topologies and reconstructed geometries over geological time).\n"
			"\n"
			"  .. versionchanged:: 31\n"
			"     Added *default_resolve_topology_parameters* argument.\n")
		.def("topological_snapshot",
				&GPlatesApi::topological_model_get_topological_snapshot,
				(bp::arg("reconstruction_time")),
				"topological_snapshot(reconstruction_time)\n"
				"  Returns a snapshot of resolved topologies at the requested reconstruction time.\n"
				"\n"
				"  :param reconstruction_time: the geological time of the snapshot (must have an *integral* value)\n"
				"  :type reconstruction_time: float or :class:`GeoTimeInstant`\n"
				"  :rtype: :class:`TopologicalSnapshot`\n"
				"  :raises: ValueError if *reconstruction_time* is not an *integral* value\n")
		.def("reconstruct_geometry",
				&GPlatesApi::TopologicalModel::reconstruct_geometry,
				(bp::arg("geometry"),
					bp::arg("initial_time"),
					bp::arg("oldest_time") = boost::optional<GPlatesPropertyValues::GeoTimeInstant>(),
					bp::arg("youngest_time") = GPlatesPropertyValues::GeoTimeInstant(0),
					bp::arg("time_increment") = GPlatesMaths::real_t(1),
					bp::arg("reconstruction_plate_id") = boost::optional<GPlatesModel::integer_plate_id_type>(),
					bp::arg("initial_scalars") = bp::object()/*Py_None*/,
					bp::arg("deactivate_points") = boost::optional<GPlatesAppLogic::TopologyReconstruct::DeactivatePoint::non_null_ptr_to_const_type>(
							GPlatesUtils::static_pointer_cast<const GPlatesAppLogic::TopologyReconstruct::DeactivatePoint>(
									GPlatesAppLogic::TopologyReconstruct::DefaultDeactivatePoint::create()))),
				"reconstruct_geometry(geometry, initial_time, [oldest_time], [youngest_time=0], [time_increment=1], "
				"[reconstruction_plate_id], [initial_scalars], [deactivate_points=ReconstructedGeometryTimeSpan.DefaultDeactivatePoints()])\n"
				"  Reconstruct a geometry (and optional scalars) over a time span.\n"
				"\n"
				"  :param geometry: The geometry to reconstruct (using topologies). Currently limited to a "
				"multipoint, or a point or sequence of points. Polylines and polygons to be introduced in future.\n"
				"  :type geometry: :class:`MultiPointOnSphere`, or :class:`PointOnSphere`, or sequence of points "
				"(where a point can be :class:`PointOnSphere` or (x,y,z) tuple or (latitude,longitude) tuple in degrees)\n"
				"  :param initial_time: The time that reconstruction by topologies starts at.\n"
				"  :type initial_time: float or :class:`GeoTimeInstant`\n"
				"  :param oldest_time: Oldest time in the history of topologies (must have an *integral* value). "
				"Defaults to *initial_time*.\n"
				"  :type oldest_time: float or :class:`GeoTimeInstant`\n"
				"  :param youngest_time: Youngest time in the history of topologies (must have an *integral* value). "
				"Defaults to present day.\n"
				"  :type youngest_time: float or :class:`GeoTimeInstant`.\n"
				"  :param time_increment: Time step in the history of topologies (must have an *integral* value, "
				"and ``oldest_time - youngest_time`` must be an integer multiple of ``time_increment``). "
				"Defaults to 1My.\n"
				"  :type time_increment: float\n"
				"  :param reconstruction_plate_id: Used to rotate *geometry* (assumed to be in its present day position) "
				"to its initial position at time *initial_time*. Defaults to the anchored plate "
				"(specified in :meth:`constructor<__init__>`).\n"
				"  :type reconstruction_plate_id: int\n"
				"  :param initial_scalars: optional mapping of scalar types to sequences of initial scalar values\n"
				"  :type initial_scalars: ``dict`` mapping each :class:`ScalarType` to a sequence "
				"of float, or a sequence of (:class:`ScalarType`, sequence of float) tuples\n"
				"  :param deactivate_points: Specify how points are deactivated when reconstructed forward and/or backward in time, or "
				"specify ``None`` to disable deactivation of points (which is useful if you know your points are on continental crust where "
				"they're typically always active, as opposed to oceanic crust that is produced at mid-ocean ridges and consumed at subduction zones). "
				"Note that you can use your own class derived from :class:`ReconstructedGeometryTimeSpan.DeactivatePoints` or "
				"use the provided class :class:`ReconstructedGeometryTimeSpan.DefaultDeactivatePoints`. "
				"Defaults to a default-constructed :class:`ReconstructedGeometryTimeSpan.DefaultDeactivatePoints`.\n"
				"  :type deactivate_points: :class:`ReconstructedGeometryTimeSpan.DeactivatePoints` or None\n"
				"  :rtype: :class:`ReconstructedGeometryTimeSpan`\n"
				"  :raises: ValueError if *initial_time* is "
				":meth:`distant past<GeoTimeInstant.is_distant_past>` or "
				":meth:`distant future<GeoTimeInstant.is_distant_future>`\n"
				"  :raises: ValueError if *initial_scalars* is specified but: is empty, or each :class:`scalar type<ScalarType>` "
				"is not mapped to the same number of scalar values, or the number of scalars is not equal to the "
				"number of points in *geometry*\n"
				"  :raises: ValueError if oldest or youngest time is distant-past (``float('inf')``) or "
				"distant-future (``float('-inf')``), or if oldest time is later than (or same as) youngest time, or if "
				"time increment is not positive, or if oldest to youngest time period is not an integer multiple "
				"of the time increment, or if oldest time or youngest time or time increment are not *integral* values.\n"
				"\n"
				"  The *reconstruction_plate_id* is used for any **rigid** reconstructions of *geometry*. This includes "
				"the initial rigid rotation of *geometry* (assumed to be in its present day position) to its initial position "
				"at time *initial_time*. If a reconstruction plate ID is not specified, then *geometry* is assumed to "
				"already be at its initial position at time *initial_time*. "
				"In addition, the reconstruction plate ID is also used when incrementally reconstructing from the initial time "
				"to other times for any geometry points that fail to intersect topologies (dynamic plates and deforming networks). "
				"This can happen either due to small gaps/cracks in a global topological model or when using a topological model that "
				"does not cover the entire globe.\n"
				"\n"
				"  To reconstruct points in a geometry from 100Ma to present day in increments of 1 Myr using default deactivation "
				"(in this case subduction of oceanic points):\n"
				"  ::\n"
				"\n"
				"    topological_model.reconstruct_geometry(geometry, 100)\n"
				"\n"
				"  To do the same but with no deactivation (in this case continental points):\n"
				"  ::\n"
				"\n"
				"    topological_model.reconstruct_geometry(geometry, 100, deactivate_points=None)\n"
				"\n"
				"  .. versionchanged:: 31\n"
				"     Added *deactivate_points* argument.\n")
		.def("get_rotation_model",
				&GPlatesApi::TopologicalModel::get_rotation_model,
				"get_rotation_model()\n"
				"  Return the rotation model used internally.\n"
				"\n"
				"  :rtype: :class:`RotationModel`\n"
				"\n"
				"  .. note:: The :meth:`default anchor plate ID<RotationModel.get_default_anchor_plate_id>` of the returned rotation model "
				"may be different to that of the rotation model passed into the :meth:`constructor<__init__>` if an anchor plate ID was specified "
				"in the :meth:`constructor<__init__>`.\n")
		.def("get_anchor_plate_id",
				&GPlatesApi::TopologicalModel::get_anchor_plate_id,
				"get_anchor_plate_id()\n"
				"  Return the anchor plate ID (see :meth:`constructor<__init__>`).\n"
				"\n"
				"  :rtype: int\n"
				"\n"
				"  .. note:: This is the same as the :meth:`default anchor plate ID<RotationModel.get_default_anchor_plate_id>` "
				"of :meth:`get_rotation_model`.\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesApi::TopologicalModel>();
}
