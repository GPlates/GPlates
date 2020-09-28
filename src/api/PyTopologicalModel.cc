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

#include "PyFeatureCollection.h"
#include "PythonConverterUtils.h"
#include "PythonHashDefVisitor.h"

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

	// Copy topological feature collection files into a vector.
	std::vector<GPlatesFileIO::File::non_null_ptr_type> topological_feature_collection_files;
	topological_features.get_files(topological_feature_collection_files);

	//
	// Adapt an existing rotation model, or create a new rotation model.
	//
	// In either case we want to have a suitably large cache size in our rotation model to avoid
	// slowing down our reconstruct-by-topologies (which happens if reconstruction trees are
	// continually evicted and re-populated as we reconstruct different geometries through time).
	//
	// The +1 accounts for the extra time step used to generate deformed geometries (and velocities).
	const unsigned int reconstruction_tree_cache_size = time_range.get_num_time_slots() + 1;

	boost::optional<RotationModel::non_null_ptr_type> rotation_model;

	if (const RotationModel::non_null_ptr_type *existing_rotation_model =
		boost::get<RotationModel::non_null_ptr_type>(&rotation_model_argument))
	{
		// Adapt the existing rotation model.
		rotation_model = RotationModel::create(
				*existing_rotation_model,
				reconstruction_tree_cache_size,
				0/*default_anchor_plate_id*/);
	}
	else
	{
		const FeatureCollectionSequenceFunctionArgument rotation_feature_collections_function_argument =
				boost::get<FeatureCollectionSequenceFunctionArgument>(rotation_model_argument);

		// Create a new rotation model (from rotation features).
		rotation_model = RotationModel::create(
				rotation_feature_collections_function_argument,
				reconstruction_tree_cache_size,
				false/*extend_total_reconstruction_poles_to_distant_past*/,
				0/*default_anchor_plate_id*/);
	}

	return non_null_ptr_type(
			new TopologicalModel(
					topological_feature_collection_files,
					rotation_model.get()));
}


void
GPlatesApi::TopologicalModel::get_topological_feature_collections(
		std::vector<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> &topological_feature_collections) const
{
	BOOST_FOREACH(GPlatesFileIO::File::non_null_ptr_type feature_collection_file, d_topological_feature_collection_files)
	{
		GPlatesModel::FeatureCollectionHandle::non_null_ptr_type feature_collection =
				GPlatesUtils::get_non_null_pointer(
						feature_collection_file->get_reference().get_feature_collection().handle_ptr());

		topological_feature_collections.push_back(feature_collection);
	}
}


void
GPlatesApi::TopologicalModel::get_files(
		std::vector<GPlatesFileIO::File::non_null_ptr_type> &topological_feature_collection_files) const
{
	topological_feature_collection_files.insert(
			topological_feature_collection_files.end(),
			d_topological_feature_collection_files.begin(),
			d_topological_feature_collection_files.end());
}


void
export_topological_model()
{
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
			"  Create from topological features and a rotation model.\n"
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
			"of the time increment, or if oldest time or youngest time or time increment are not integral values.\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesApi::TopologicalModel>();
}
