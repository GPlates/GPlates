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
				boost::optional<GPlatesModel::integer_plate_id_type> anchor_plate_id)
		{
			return TopologicalModel::create(
					topological_features,
					rotation_model_argument,
					anchor_plate_id);
		}

		/**
		 * Returns the anchor plate ID.
		 */
		GPlatesModel::integer_plate_id_type
		topological_model_get_anchor_plate_id(
				TopologicalModel::non_null_ptr_type topological_model)
		{
			return topological_model->get_rotation_model()->get_reconstruction_tree_creator().get_default_anchor_plate_id();
		}
	}
}


GPlatesApi::TopologicalModel::non_null_ptr_type
GPlatesApi::TopologicalModel::create(
		const FeatureCollectionSequenceFunctionArgument &topological_features,
		// Note we're using 'RotationModelFunctionArgument::function_argument_type' instead of
		// just 'RotationModelFunctionArgument' since we want to know if it's an existing RotationModel...
		const RotationModelFunctionArgument::function_argument_type &rotation_model_argument,
		boost::optional<GPlatesModel::integer_plate_id_type> anchor_plate_id)
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

	return non_null_ptr_type(
			new TopologicalModel(topological_features, rotation_model.get()));
}


GPlatesApi::TopologicalModel::TopologicalModel(
		const FeatureCollectionSequenceFunctionArgument &topological_features,
		const RotationModel::non_null_ptr_type &rotation_model) :
	d_rotation_model(rotation_model),
	d_topological_features(topological_features),
	d_topological_section_reconstruct_context(d_reconstruct_method_registry),
	d_topological_section_reconstruct_context_state(
			d_topological_section_reconstruct_context.create_context_state(
					GPlatesAppLogic::ReconstructMethodInterface::Context(
							GPlatesAppLogic::ReconstructParams(),
							d_rotation_model->get_reconstruction_tree_creator())))
{
	// Get the topological feature collections.
	std::vector<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> topological_feature_collections;
	topological_features.get_feature_collections(topological_feature_collections);

	// Separate into regular features (used as topological sections for topological lines/boundaries/networks),
	// topological lines (can also be used as topological sections for topological boundaries/networks),
	// topological boundaries and topological networks.
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
				d_topological_line_features.push_back(feature_ref);
			}
			else if (topology_geometry_type == GPlatesAppLogic::TopologyGeometry::BOUNDARY)
			{
				d_topological_boundary_features.push_back(feature_ref);
			}
			else if (topology_geometry_type == GPlatesAppLogic::TopologyGeometry::NETWORK)
			{
				d_topological_network_features.push_back(feature_ref);
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


const GPlatesApi::TopologicalModel::ResolvedTopologiesTimeSlot &
GPlatesApi::TopologicalModel::get_resolved_topologies(
		const double &reconstruction_time_arg)
{
	const GPlatesMaths::real_t reconstruction_time = std::round(reconstruction_time_arg);
	if (!GPlatesMaths::are_almost_exactly_equal(reconstruction_time.dval(), reconstruction_time_arg))
	{
		PyErr_SetString(PyExc_ValueError, "Reconstruction time should be an integral value.");
		bp::throw_error_already_set();
	}

	// Insert an empty ResolvedTopologiesTimeSlot. If it's successfully inserted then fill it with
	// resolved topologies, otherwise it has already been filled/cached.
	auto resolved_topologies_insert_result = d_cached_resolved_topologies.insert(
			{reconstruction_time, ResolvedTopologiesTimeSlot()});

	ResolvedTopologiesTimeSlot &resolved_topologies = resolved_topologies_insert_result.first->second;
	if (resolved_topologies_insert_result.second)
	{
		// We want to have a suitably large reconstruction tree cache size in our rotation model to avoid
		// slowing down our reconstruct-by-topologies (which happens if reconstruction trees are
		// continually evicted and re-populated as we reconstruct different geometries through time).
		//
		// The +1 accounts for the extra time step used to generate deformed geometries (and velocities).
		const unsigned int reconstruction_tree_cache_size = d_cached_resolved_topologies.size() + 1;
		d_rotation_model->get_cached_reconstruction_tree_creator_impl()->set_maximum_cache_size(
				reconstruction_tree_cache_size);

		// Fill the resolved topologies for the requested reconstruction time.
		resolve_topologies(resolved_topologies, reconstruction_time.dval());
	}

	return resolved_topologies;
}


void
GPlatesApi::TopologicalModel::resolve_topologies(
		GPlatesApi::TopologicalModel::ResolvedTopologiesTimeSlot &resolved_topologies,
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
	GPlatesAppLogic::TopologyInternalUtils::find_topological_sections_referenced(
			topological_sections_referenced,
			d_topological_network_features,
			GPlatesAppLogic::TopologyGeometry::NETWORK,
			reconstruction_time);

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
	const GPlatesAppLogic::ReconstructHandle::type resolved_topological_lines_handle =
			GPlatesAppLogic::TopologyUtils::resolve_topological_lines(
					// Contains the resolved topological line sections referenced by topological boundaries and networks...
					resolved_topologies.resolved_lines,
					d_topological_line_features,
					d_rotation_model->get_reconstruction_tree_creator(), 
					reconstruction_time,
					// Resolved topo lines use the reconstructed non-topo geometries...
					topological_sections_reconstruct_handles,
					// Only those topo lines referenced by resolved boundaries/networks...
					topological_sections_referenced);

	topological_sections_reconstruct_handles.push_back(resolved_topological_lines_handle);

	// Resolve topological boundaries.
	GPlatesAppLogic::TopologyUtils::resolve_topological_boundaries(
			resolved_topologies.resolved_boundaries,
			d_topological_boundary_features,
			d_rotation_model->get_reconstruction_tree_creator(), 
			reconstruction_time,
			// Resolved topo boundaries use the resolved topo lines *and* the reconstructed non-topo geometries...
			topological_sections_reconstruct_handles);

	// Resolve topological networks.
	GPlatesAppLogic::TopologyUtils::resolve_topological_networks(
			resolved_topologies.resolved_networks,
			reconstruction_time,
			d_topological_network_features,
			// Resolved topo networks use the resolved topo lines *and* the reconstructed non-topo geometries...
			topological_sections_reconstruct_handles);
}


GPlatesApi::ReconstructedGeometryTimeSpan::non_null_ptr_type
GPlatesApi::TopologicalModel::reconstruct_geometry(
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry,
		const GPlatesPropertyValues::GeoTimeInstant &initial_time,
		boost::optional<GPlatesPropertyValues::GeoTimeInstant> oldest_time_arg,
		const GPlatesPropertyValues::GeoTimeInstant &youngest_time_arg,
		const double &time_increment_arg,
		boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id,
		bp::object scalar_type_to_values_mapping_object)
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

		// Get resolved topologies (they'll either be cached or generated on demand).
		const ResolvedTopologiesTimeSlot &resolved_topologies = get_resolved_topologies(time);

		resolved_boundary_time_span->set_sample_in_time_slot(resolved_topologies.resolved_boundaries, time_slot);
		resolved_network_time_span->set_sample_in_time_slot(resolved_topologies.resolved_networks, time_slot);
	}

	GPlatesAppLogic::TopologyReconstruct::non_null_ptr_type topology_reconstruct =
			GPlatesAppLogic::TopologyReconstruct::create(
					time_range,
					resolved_boundary_time_span,
					resolved_network_time_span,
					d_rotation_model->get_reconstruction_tree_creator());

	// Create time span of reconstructed geometry.
	GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span =
			topology_reconstruct->create_geometry_time_span(
					geometry,
					// If a reconstruction plate ID is not specified then use the default anchor plate ID
					// of our rotation model...
					reconstruction_plate_id
							? reconstruction_plate_id.get()
							: d_rotation_model->get_reconstruction_tree_creator().get_default_anchor_plate_id(),
					initial_time.value());

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
					"A history of geometries reconstructed using topologies over geological time.\n"
					"\n"
					"  .. versionadded:: 29\n",
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
					"A history of topologies over geological time.\n"
					"\n"
					"  .. versionadded:: 29\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::topological_model_create,
						bp::default_call_policies(),
						(bp::arg("topological_features"),
							bp::arg("rotation_model"),
							bp::arg("anchor_plate_id") = boost::optional<GPlatesModel::integer_plate_id_type>())),
			"__init__(topological_features, rotation_model, [anchor_plate_id])\n"
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
			"  :param anchor_plate_id: The anchored plate id used for all reconstructions "
			"(resolving topologies, and reconstructing regular features and :meth:`geometries<reconstruct_geometry>`). "
			"Defaults to the default anchor plate of *rotation_model*.\n"
			"  :type anchor_plate_id: int\n"
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
			"CPU and memory (since it caches resolved topologies and reconstructed geometries over geological time).\n")
		.def("reconstruct_geometry",
				&GPlatesApi::TopologicalModel::reconstruct_geometry,
				(bp::arg("geometry"),
					bp::arg("initial_time"),
					bp::arg("oldest_time") = boost::optional<GPlatesPropertyValues::GeoTimeInstant>(),
					bp::arg("youngest_time") = GPlatesPropertyValues::GeoTimeInstant(0),
					bp::arg("time_increment") = GPlatesMaths::real_t(1),
					bp::arg("reconstruction_plate_id") = boost::optional<GPlatesModel::integer_plate_id_type>(),
					bp::arg("scalars") = bp::object()/*Py_None*/),
			"reconstruct_geometry(geometry, initial_time, [oldest_time], [youngest_time=0], [time_increment=1], [reconstruction_plate_id], [scalars])\n"
				"  Reconstruct a geometry (and optional scalars) over a time span.\n"
				"\n"
				"  :param geometry: the geometry to reconstruct using topologies\n"
				"  :type geometry: :class:`GeometryOnSphere`\n"
				"  :param initial_time: The time that reconstruction by topologies starts at.\n"
				"  :type initial_time: float or :class:`GeoTimeInstant`\n"
				"  :param oldest_time: Oldest time in the history of topologies (must have an integral value). "
				"Defaults to *initial_time*.\n"
				"  :type oldest_time: float or :class:`GeoTimeInstant`\n"
				"  :param youngest_time: Youngest time in the history of topologies (must have an integral value). "
				"Defaults to present day.\n"
				"  :type youngest_time: float or :class:`GeoTimeInstant`.\n"
				"  :param time_increment: Time step in the history of topologies (must have an integral value, "
				"and ``oldest_time - youngest_time`` must be an integer multiple of ``time_increment``). "
				"Defaults to 1My.\n"
				"  :type time_increment: float\n"
				"  :param reconstruction_plate_id: Used to rotate *geometry* (assumed to be in its present day position) "
				"to its initial position at time *initial_time*. Defaults to the anchored plate "
				"(specified in :meth:`constructor<__init__>`).\n"
				"  :type reconstruction_plate_id: int\n"
				"  :param scalars: optional mapping of scalar types to sequences of scalar values\n"
				"  :type scalars: ``dict`` mapping each :class:`ScalarType` to a sequence "
				"of float, or a sequence of (:class:`ScalarType`, sequence of float) tuples\n"
				"  :rtype: :class:`ReconstructedGeometryTimeSpan`\n"
				"  :raises: ValueError if *initial_time* is "
				":meth:`distant past<GeoTimeInstant.is_distant_past>` or "
				":meth:`distant future<GeoTimeInstant.is_distant_future>`\n"
				"  :raises: ValueError if *scalars* is specified but: is empty, or each :class:`scalar type<ScalarType>` "
				"is not mapped to the same number of scalar values, or the number of scalars is not equal to the "
				"number of points in *geometry*\n"
				"  :raises: ValueError if oldest or youngest time is distant-past (``float('inf')``) or "
				"distant-future (``float('-inf')``), or if oldest time is later than (or same as) youngest time, or if "
				"time increment is not positive, or if oldest to youngest time period is not an integer multiple "
				"of the time increment, or if oldest time or youngest time or time increment are not integral values.\n"
				"\n"
				"  The *reconstruction_plate_id* is used for any **rigid** reconstructions of *geometry*. This includes "
				"the initial rigid rotation of *geometry* (assumed to be in its present day position) to its initial position "
				"at time *initial_time*. If a reconstruction plate ID is not specified, then *geometry* is assumed to "
				"already be at its initial position at time *initial_time*. "
				"In addition, the reconstruction plate ID is also used when incrementally reconstructing from the initial time "
				"to other times for any geometry points that fail to intersect topologies (dynamic plates and deforming networks). "
				"This can happen either due to small gaps/cracks in a global topological model or when using a topological model that "
				"does not cover the entire globe.\n")
		.def("get_rotation_model",
				&GPlatesApi::TopologicalModel::get_rotation_model,
				"get_rotation_model()\n"
				"  Return the rotation model used internally.\n"
				"\n"
				"  :rtype: :class:`RotationModel`\n")
		.def("get_anchor_plate_id",
				&GPlatesApi::topological_model_get_anchor_plate_id,
				"get_anchor_plate_id()\n"
				"  Return the anchor plate ID (see :meth:`constructor<__init__>`).\n"
				"\n"
				"  :rtype: int\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesApi::TopologicalModel>();
}
