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
#include "PyFeatureCollection.h"
#include "PyPropertyValues.h"
#include "PythonConverterUtils.h"
#include "PythonHashDefVisitor.h"

#include "app-logic/GeometryUtils.h"
#include "app-logic/ScalarCoverageEvolution.h"
#include "app-logic/ReconstructContext.h"
#include "app-logic/ReconstructMethodRegistry.h"
#include "app-logic/ReconstructParams.h"
#include "app-logic/TopologyInternalUtils.h"
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
	namespace
	{
		/**
		 * This is called directly from Python via 'TopologicalModel.__init__()'.
		 */
		TopologicalModel::non_null_ptr_type
		topological_model_create(
				const FeatureCollectionSequenceFunctionArgument &topological_features,
				const RotationModelFunctionArgument::function_argument_type &rotation_model_argument,
				const GPlatesPropertyValues::GeoTimeInstant &oldest_time,
				const GPlatesPropertyValues::GeoTimeInstant &youngest_time,
				const double &time_increment)
		{
			return TopologicalModel::create(
					topological_features,
					rotation_model_argument,
					oldest_time, youngest_time, time_increment);
		}
	}
}


GPlatesApi::TopologicalModel::non_null_ptr_type
GPlatesApi::TopologicalModel::create(
		const FeatureCollectionSequenceFunctionArgument &topological_features,
		// Note we're using 'RotationModelFunctionArgument::function_argument_type' instead of
		// just 'RotationModelFunctionArgument' since we want to know if it's an existing RotationModel...
		const RotationModelFunctionArgument::function_argument_type &rotation_model_argument,
		const GPlatesPropertyValues::GeoTimeInstant &oldest_time_arg,
		const GPlatesPropertyValues::GeoTimeInstant &youngest_time_arg,
		const double &time_increment_arg)
{
	if (!oldest_time_arg.is_real() ||
		!youngest_time_arg.is_real())
	{
		PyErr_SetString(PyExc_ValueError,
				"Oldest/youngest times cannot be distant-past (float('inf')) or distant-future (float('-inf')).");
		bp::throw_error_already_set();
	}

	// We are expecting these to have integral values.
	const double oldest_time = std::round(oldest_time_arg.value());
	const double youngest_time = std::round(youngest_time_arg.value());
	const double time_increment = std::round(time_increment_arg);

	if (!GPlatesMaths::are_almost_exactly_equal(oldest_time, oldest_time_arg.value()) ||
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

	const unsigned int num_time_slots = time_range.get_num_time_slots();

	//
	// Adapt an existing rotation model, or create a new rotation model.
	//
	// In either case we want to have a suitably large cache size in our rotation model to avoid
	// slowing down our reconstruct-by-topologies (which happens if reconstruction trees are
	// continually evicted and re-populated as we reconstruct different geometries through time).
	//
	// The +1 accounts for the extra time step used to generate deformed geometries (and velocities).
	const unsigned int reconstruction_tree_cache_size = num_time_slots + 1;

	boost::optional<RotationModel::non_null_ptr_type> rotation_model_opt;

	if (const RotationModel::non_null_ptr_type *existing_rotation_model =
		boost::get<RotationModel::non_null_ptr_type>(&rotation_model_argument))
	{
		// Adapt the existing rotation model.
		rotation_model_opt = RotationModel::create(
				*existing_rotation_model,
				reconstruction_tree_cache_size,
				0/*default_anchor_plate_id*/);
	}
	else
	{
		const FeatureCollectionSequenceFunctionArgument rotation_feature_collections_function_argument =
				boost::get<FeatureCollectionSequenceFunctionArgument>(rotation_model_argument);

		// Create a new rotation model (from rotation features).
		rotation_model_opt = RotationModel::create(
				rotation_feature_collections_function_argument,
				reconstruction_tree_cache_size,
				false/*extend_total_reconstruction_poles_to_distant_past*/,
				0/*default_anchor_plate_id*/);
	}

	RotationModel::non_null_ptr_type rotation_model = rotation_model_opt.get();
	GPlatesAppLogic::ReconstructionTreeCreator reconstruction_tree_creator = rotation_model->get_reconstruction_tree_creator();

	std::vector<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> topological_feature_collections;
	topological_features.get_feature_collections(topological_feature_collections);

	// Separate the topological features into topological line, boundary and network features, and
	// regular features (used as topological sections).
	std::vector<GPlatesModel::FeatureHandle::weak_ref> topological_line_features;
	std::vector<GPlatesModel::FeatureHandle::weak_ref> topological_boundary_features;
	std::vector<GPlatesModel::FeatureHandle::weak_ref> topological_network_features;
	std::vector<GPlatesModel::FeatureHandle::weak_ref> regular_features;
	for (auto feature_collection : topological_feature_collections)
	{
		for (auto feature : *feature_collection)
		{
			const GPlatesModel::FeatureHandle::weak_ref feature_ref = feature->reference();

			// Determine the topology geometry type.
			boost::optional<GPlatesAppLogic::TopologyGeometry::Type> topology_geometry_type =
					GPlatesAppLogic::TopologyUtils::get_topological_geometry_type(feature_ref);

			if (topology_geometry_type == GPlatesAppLogic::TopologyGeometry::LINE)
			{
				topological_line_features.push_back(feature_ref);
			}
			else if (topology_geometry_type == GPlatesAppLogic::TopologyGeometry::BOUNDARY)
			{
				topological_boundary_features.push_back(feature_ref);
			}
			else if (topology_geometry_type == GPlatesAppLogic::TopologyGeometry::NETWORK)
			{
				topological_network_features.push_back(feature_ref);
			}
			else
			{
				regular_features.push_back(feature_ref);
			}
		}
	}

	// Create our resolved topology (boundary/network) time spans.
	GPlatesAppLogic::TopologyReconstruct::resolved_boundary_time_span_type::non_null_ptr_type resolved_boundary_time_span =
			GPlatesAppLogic::TopologyReconstruct::resolved_boundary_time_span_type::create(time_range);
	GPlatesAppLogic::TopologyReconstruct::resolved_network_time_span_type::non_null_ptr_type resolved_network_time_span =
			GPlatesAppLogic::TopologyReconstruct::resolved_network_time_span_type::create(time_range);

	// ReconstructContext used for reconstructing the regular features.
	GPlatesAppLogic::ReconstructMethodRegistry reconstruct_method_registry;
	GPlatesAppLogic::ReconstructContext reconstruct_context(reconstruct_method_registry);
	GPlatesAppLogic::ReconstructContext::context_state_reference_type reconstruct_context_state =
			reconstruct_context.create_context_state(
					GPlatesAppLogic::ReconstructMethodInterface::Context(
							GPlatesAppLogic::ReconstructParams(),
							reconstruction_tree_creator));
	reconstruct_context.set_features(regular_features);

	// Iterate over the time slots and fill in the resolved topological boundaries/networks.
	for (unsigned int time_slot = 0; time_slot < num_time_slots; ++time_slot)
	{
		const double reconstruction_time = time_range.get_time(time_slot);

		// Find the topological section feature IDs referenced by any topological features at current reconstruction time.
		//
		// This is an optimisation that avoids unnecessary reconstructions. Only those topological sections referenced
		// by topologies that exist at the reconstruction time are reconstructed.
		std::set<GPlatesModel::FeatureId> topological_sections_referenced;
		GPlatesAppLogic::TopologyInternalUtils::find_topological_sections_referenced(
				topological_sections_referenced,
				topological_line_features,
				GPlatesAppLogic::TopologyGeometry::LINE,
				reconstruction_time);
		GPlatesAppLogic::TopologyInternalUtils::find_topological_sections_referenced(
				topological_sections_referenced,
				topological_boundary_features,
				GPlatesAppLogic::TopologyGeometry::BOUNDARY,
				reconstruction_time);
		GPlatesAppLogic::TopologyInternalUtils::find_topological_sections_referenced(
				topological_sections_referenced,
				topological_network_features,
				GPlatesAppLogic::TopologyGeometry::NETWORK,
				reconstruction_time);

		// Contains the topological section regular geometries referenced by topologies.
		std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type> reconstructed_feature_geometries;

		// Generate RFGs only for the referenced topological sections.
		const GPlatesAppLogic::ReconstructHandle::type reconstruct_handle =
				reconstruct_context.get_reconstructed_topological_sections(
						reconstructed_feature_geometries,
						topological_sections_referenced,
						reconstruct_context_state,
						reconstruction_time);

		// All reconstruct handles used to find topological sections (referenced by topological boundaries/networks).
		std::vector<GPlatesAppLogic::ReconstructHandle::type> topological_sections_reconstruct_handles(1, reconstruct_handle);

		// Contains the resolved topological line sections referenced by topological boundaries and networks.
		std::vector<GPlatesAppLogic::ResolvedTopologicalLine::non_null_ptr_type> resolved_topological_lines;

		// Resolving topological lines generates its own reconstruct handle that will be used by
		// topological boundaries and networks to find this group of resolved lines.
		//
		// So we always resolve topological *lines* regardless of whether the user requested it or not.
		const GPlatesAppLogic::ReconstructHandle::type resolved_topological_lines_handle =
				GPlatesAppLogic::TopologyUtils::resolve_topological_lines(
						resolved_topological_lines,
						topological_line_features,
						reconstruction_tree_creator, 
						reconstruction_time,
						// Resolved topo lines use the reconstructed non-topo geometries...
						topological_sections_reconstruct_handles,
						// Only those topo lines references by resolved boundaries/networks...
						topological_sections_referenced);

		topological_sections_reconstruct_handles.push_back(resolved_topological_lines_handle);

		// Resolve topological boundaries.
		std::vector<GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_type> resolved_topological_boundaries;
		GPlatesAppLogic::TopologyUtils::resolve_topological_boundaries(
				resolved_topological_boundaries,
				topological_boundary_features,
				reconstruction_tree_creator, 
				reconstruction_time,
				// Resolved topo boundaries use the resolved topo lines *and* the reconstructed non-topo geometries...
				topological_sections_reconstruct_handles);

		// Resolve topological networks.
		std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type> resolved_topological_networks;
		GPlatesAppLogic::TopologyUtils::resolve_topological_networks(
				resolved_topological_networks,
				reconstruction_time,
				topological_network_features,
				// Resolved topo networks use the resolved topo lines *and* the reconstructed non-topo geometries...
				topological_sections_reconstruct_handles);

		resolved_boundary_time_span->set_sample_in_time_slot(resolved_topological_boundaries, time_slot);
		resolved_network_time_span->set_sample_in_time_slot(resolved_topological_networks, time_slot);
	}

	GPlatesAppLogic::TopologyReconstruct::non_null_ptr_type topology_reconstruct =
			GPlatesAppLogic::TopologyReconstruct::create(
					time_range,
					resolved_boundary_time_span,
					resolved_network_time_span,
					reconstruction_tree_creator);

	return non_null_ptr_type(
			new TopologicalModel(
					topological_features,
					rotation_model,
					topology_reconstruct));
}


GPlatesApi::ReconstructedGeometryTimeSpan::non_null_ptr_type
GPlatesApi::TopologicalModel::reconstruct_geometry(
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry,
		boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id,
		bp::object scalar_type_to_values_mapping_object) const
{
	// Create time span of reconstructed geometry.
	GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span =
			d_topology_reconstruct->create_geometry_time_span(
					geometry,
					// TODO: Change 'create_geometry_time_span()' to use default anchor plate ID of
					// RotationModel if plate ID not specified...
					reconstruction_plate_id ? reconstruction_plate_id.get() : 0);

	// Extract the optional scalar values.
	ReconstructedGeometryTimeSpan::scalar_type_to_time_span_map_type scalar_type_to_time_span_map;
	if (scalar_type_to_values_mapping_object != bp::object()/*Py_None*/)
	{
		// Extract the mapping of scalar types to scalar values.
		const std::map<
				GPlatesPropertyValues::ValueObjectType/*scalar type*/,
				std::vector<double>/*scalars*/> scalar_type_to_values_map =
						create_scalar_type_to_values_map(scalar_type_to_values_mapping_object);

		// Number of points in domain must match number of scalar values in range.
		const unsigned int num_domain_geometry_points =
				GPlatesAppLogic::GeometryUtils::get_num_geometry_points(*geometry);
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				!scalar_type_to_values_map.empty(),
				GPLATES_ASSERTION_SOURCE);
		// Just test the scalar values length for the first scalar type (all types should already have the same length).
		if (num_domain_geometry_points != scalar_type_to_values_map.begin()->second.size())
		{
			PyErr_SetString(PyExc_ValueError, "Number of scalar values must match number of points in geometry");
			bp::throw_error_already_set();
		}

		// Create optional time spans of reconstructed scalars (one time span per scalar type).
		for (auto scalar_type_to_values : scalar_type_to_values_map)
		{
			const GPlatesPropertyValues::ValueObjectType &scalar_type = scalar_type_to_values.first;
			const std::vector<double> &scalar_values = scalar_type_to_values.second;

			// Select function to evolve scalar values with (based on the scalar type).
			boost::optional<GPlatesAppLogic::scalar_evolution_function_type> scalar_evolution_function =
					get_scalar_evolution_function(scalar_type, GPlatesAppLogic::ReconstructScalarCoverageParams());

			// Reconstruct scalar values over time span.
			GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::non_null_ptr_type scalar_coverage_time_span =
					GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::create(
							geometry_time_span,
							scalar_values,
							scalar_evolution_function);

			scalar_type_to_time_span_map.insert(std::make_pair(scalar_type, scalar_coverage_time_span));
		}
	}

	return ReconstructedGeometryTimeSpan::create(geometry_time_span, scalar_type_to_time_span_map);
}


void
export_topological_model()
{
	//
	// ReconstructedGeometryTimeSpan - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesApi::ReconstructedGeometryTimeSpan,
			GPlatesApi::ReconstructedGeometryTimeSpan::non_null_ptr_type,
			boost::noncopyable>(
					"ReconstructedGeometryTimeSpan",
					"A history of topologies over geological time.\n",
					// Don't allow creation from python side...
					// (Also there is no publicly-accessible default constructor).
					bp::no_init)
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesApi::ReconstructedGeometryTimeSpan>();


	//
	// TopologicalModel - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesApi::TopologicalModel,
			GPlatesApi::TopologicalModel::non_null_ptr_type,
			boost::noncopyable>(
					"TopologicalModel",
					"A history of topologies over geological time.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::topological_model_create,
						bp::default_call_policies(),
						(bp::arg("topological_features"), bp::arg("rotation_model"),
							bp::arg("oldest_time"),
							bp::arg("youngest_time") = GPlatesPropertyValues::GeoTimeInstant(0),
							bp::arg("time_increment") = GPlatesMaths::real_t(1))),
			"__init__(topological_features, rotation_model, oldest_time, [youngest_time=0], [time_increment=1])\n"
			"  Create from topological features, a rotation model and a time span.\n"
			"\n"
			"  :param topological_features: the topological boundary and/or network features and the "
			"topological section features they reference (regular and topological lines) as a feature collection, "
			"or filename, or feature, or sequence of features, or a sequence (eg, ``list`` or ``tuple``) "
			"of any combination of those four types\n"
			"  :type topological_features: :class:`FeatureCollection`, or string, or :class:`Feature`, "
			"or sequence of :class:`Feature`, or sequence of any combination of those four types\n"
			"  :param rotation_model: A rotation model or a rotation feature collection or a rotation "
			"filename or a sequence of rotation feature collections and/or rotation filenames\n"
			"  :type rotation_model: :class:`RotationModel` or :class:`FeatureCollection` or string "
			"or sequence of :class:`FeatureCollection` instances and/or strings\n"
			"  :param oldest_time: oldest time in the history of topologies (must have an integral value)\n"
			"  :type oldest_time: float or :class:`GeoTimeInstant`\n"
			"  :param youngest_time: youngest time in the history of topologies (must have an integral value)\n"
			"  :type youngest_time: float or :class:`GeoTimeInstant`\n"
			"  :param time_increment: time step in the history of topologies (must have an integral value, "
			"and ``oldest_time - youngest_time`` must be an integer multiple of ``time_increment``)\n"
			"  :type time_increment: float\n"
			"  :raises: ValueError if oldest or youngest time is distant-past (``float('inf')``) or "
			"distant-future (``float('-inf')``), or if oldest time is later than (or same as) youngest time, or if "
			"time increment is not positive, or if oldest to youngest time period is not an integer multiple "
			"of the time increment, or if oldest time or youngest time or time increment are not integral values.\n"
			"\n"
			"  Load a topological model (and its associated rotation model) from 100Ma to present day in 1My time increments:\n"
			"  ::\n"
			"\n"
			"    rotation_model = pygplates.RotationModel('rotations.rot')\n"
			"    topological_model = pygplates.TopologicalModel('topologies.gpml', rotation_model, 100)\n"
			"\n"
			"  ...or alternatively just:"
			"  ::\n"
			"\n"
			"    topological_model = pygplates.TopologicalModel('topologies.gpml', 'rotations.rot', 100)\n")
		.def("reconstruct_geometry",
				&GPlatesApi::TopologicalModel::reconstruct_geometry,
				(bp::arg("geometry"),
					bp::arg("reconstruction_plate_id") = boost::optional<GPlatesModel::integer_plate_id_type>(),
					bp::arg("scalars") = bp::object()/*Py_None*/),
				"reconstruct_geometry(geometry, [reconstruction_plate_id], [scalars])\n"
				"  Reconstruct a geometry (and optional scalars) over a time span.\n"
				"\n"
				"  :param geometry: the geometry to reconstruct\n"
				"  :type geometry: :class:`GeometryOnSphere`\n"
				"  :param scalars: optional mapping of scalar types to sequences of scalar values\n"
				"  :type scalars: ``dict`` mapping each :class:`ScalarType` to a sequence "
				"of float, or a sequence of (:class:`ScalarType`, sequence of float) tuples\n"
				"  :raises: ValueError if *scalars* is specified but: is empty, or each :class:`scalar type<ScalarType>` "
				"is not mapped to the same number of scalar values, or the number of scalars is not equal to the "
				"number of points in *geometry*\n"
				"\n"
				"  :rtype: :class:`ReconstructedGeometryTimeSpan`\n")
		.def("get_rotation_model",
				&GPlatesApi::TopologicalModel::get_rotation_model,
				"get_rotation_model()\n"
				"  Return the rotation model used internally.\n"
				"\n"
				"  :rtype: :class:`RotationModel`\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesApi::TopologicalModel>();
}
