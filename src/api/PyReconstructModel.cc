/**
 * Copyright (C) 2024 The University of Sydney, Australia
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
#include <limits>
#include <sstream>
#include <vector>
#include <boost/bind/bind.hpp>
#include <boost/cast.hpp>
#include <boost/foreach.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <boost/variant.hpp>

#include "PyReconstructModel.h"

#include "PyFeature.h"
#include "PyFeatureCollectionFunctionArgument.h"
#include "PyPropertyValues.h"
#include "PythonConverterUtils.h"
#include "PythonExtractUtils.h"
#include "PythonHashDefVisitor.h"
#include "PythonPickle.h"

#include "app-logic/GeometryUtils.h"
#include "app-logic/ReconstructParams.h"
#include "app-logic/VelocityDeltaTime.h"
#include "app-logic/VelocityUnits.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/python.h"

#include "maths/MathsUtils.h"
#include "maths/types.h"

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"

#include "scribe/Scribe.h"

#include "utils/Earth.h"


namespace bp = boost::python;


namespace GPlatesApi
{
	/**
	 * This is called directly from Python via 'ReconstructModel.__init__()'.
	 */
	ReconstructModel::non_null_ptr_type
	reconstruct_model_create(
			const FeatureCollectionSequenceFunctionArgument &reconstructable_features,
			const RotationModelFunctionArgument::function_argument_type &rotation_model_argument,
			boost::optional<GPlatesModel::integer_plate_id_type> anchor_plate_id,
			boost::optional<unsigned int> reconstruct_snapshot_cache_size)
	{
		return ReconstructModel::create(
				reconstructable_features,
				rotation_model_argument,
				anchor_plate_id,
				reconstruct_snapshot_cache_size);
	}

	/**
	 * This is called directly from Python via 'ReconstructModel.get_reconstruct_snapshot()'.
	 */
	ReconstructSnapshot::non_null_ptr_type
	reconstruct_model_get_reconstruct_snapshot(
			ReconstructModel::non_null_ptr_type reconstruct_model,
			const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time)
	{
		// Time must not be distant past/future.
		if (!reconstruction_time.is_real())
		{
			PyErr_SetString(PyExc_ValueError,
					"Time values cannot be distant-past (float('inf')) or distant-future (float('-inf')).");
			bp::throw_error_already_set();
		}

		return reconstruct_model->get_reconstruct_snapshot(reconstruction_time.value());
	}
}


GPlatesApi::ReconstructModel::non_null_ptr_type
GPlatesApi::ReconstructModel::create(
		const FeatureCollectionSequenceFunctionArgument &reconstructable_features,
		// Note we're using 'RotationModelFunctionArgument::function_argument_type' instead of
		// just 'RotationModelFunctionArgument' since we want to know if it's an existing RotationModel...
		const RotationModelFunctionArgument::function_argument_type &rotation_model_argument,
		boost::optional<GPlatesModel::integer_plate_id_type> anchor_plate_id,
		boost::optional<unsigned int> reconstruct_snapshot_cache_size)
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
		//
		// Note: We're creating our own RotationModel from scratch (as opposed to adapting an existing one)
		//       to avoid having two rotation models (each with their own cache) thus doubling the cache memory usage.
		rotation_model = RotationModel::create(
				rotation_feature_collections_function_argument,
				// Start off with a cache size of 1 (later we'll increase it as needed)...
				1/*reconstruction_tree_cache_size*/,
				false/*extend_total_reconstruction_poles_to_distant_past*/,
				// We're creating a new RotationModel from scratch (as opposed to adapting an existing one)
				// so the anchor plate ID defaults to zero if not specified...
				anchor_plate_id ? anchor_plate_id.get() : 0);
	}

	// Get the reconstructable files.
	std::vector<GPlatesFileIO::File::non_null_ptr_type> reconstructable_files;
	reconstructable_features.get_files(reconstructable_files);

	return non_null_ptr_type(
			new ReconstructModel(
					rotation_model.get(),
					reconstructable_files,
					reconstruct_snapshot_cache_size));
}


GPlatesApi::ReconstructModel::ReconstructModel(
		const RotationModel::non_null_ptr_type &rotation_model,
		const std::vector<GPlatesFileIO::File::non_null_ptr_type> &reconstructable_files,
		boost::optional<unsigned int> reconstruct_snapshot_cache_size) :
	d_rotation_model(rotation_model),
	d_reconstructable_files(reconstructable_files),
	d_reconstruct_snapshot_cache_size(reconstruct_snapshot_cache_size),
	d_reconstruct_snapshot_cache(
			// Function to create a reconstruct snapshot given a reconstruction time...
			boost::bind(&ReconstructModel::create_reconstruct_snapshot, this, boost::placeholders::_1),
			// Initially set cache size to 1 - we'll set it properly in 'initialise_reconstruction()'...
			1)
{
	initialise();
}


void
GPlatesApi::ReconstructModel::initialise()
{
	// Clear the data members we're about to initialise in case this function called during transcribing.
	d_reconstructable_feature_collections.clear();
	// Also clear any cached reconstruct snapshots.
	d_reconstruct_snapshot_cache.clear();

	// Size of topological snapshot cache.
	const unsigned int reconstruct_snapshot_cache_size = d_reconstruct_snapshot_cache_size
			? d_reconstruct_snapshot_cache_size.get()
			// If not specified then default to unlimited - set to a very large value.
			// But should be *less* than max value so that max value can compare greater than it...
			: (std::numeric_limits<unsigned int>::max)() - 2;
	// Set size of reconstruct snapshot cache.
	d_reconstruct_snapshot_cache.set_maximum_num_values_in_cache(reconstruct_snapshot_cache_size);

	// Size of reconstruction tree cache.
	//
	// The +1 accounts for the extra time step used to calculate velocities.
	const unsigned int reconstruction_tree_cache_size = reconstruct_snapshot_cache_size + 1;
	d_rotation_model->get_cached_reconstruction_tree_creator_impl()->set_maximum_cache_size(reconstruction_tree_cache_size);

	// Extract a feature collection from each reconstructable file.
	for (auto reconstructable_file : d_reconstructable_files)
	{
		const GPlatesModel::FeatureCollectionHandle::non_null_ptr_type reconstructable_feature_collection(
				reconstructable_file->get_reference().get_feature_collection().handle_ptr());

		d_reconstructable_feature_collections.push_back(reconstructable_feature_collection);
	}
}


GPlatesApi::ReconstructSnapshot::non_null_ptr_type
GPlatesApi::ReconstructModel::get_reconstruct_snapshot(
		const double &reconstruction_time)
{
	return d_reconstruct_snapshot_cache.get_value(reconstruction_time);
}


GPlatesApi::ReconstructSnapshot::non_null_ptr_type
GPlatesApi::ReconstructModel::create_reconstruct_snapshot(
		const GPlatesMaths::real_t &reconstruction_time)
{
	return ReconstructSnapshot::create(
			d_reconstructable_files,
			d_rotation_model,
			reconstruction_time.dval());
}


GPlatesScribe::TranscribeResult
GPlatesApi::ReconstructModel::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<ReconstructModel> &reconstruct_model)
{
	if (scribe.is_saving())
	{
		save_construct_data(scribe, reconstruct_model.get_object());
	}
	else // loading
	{
		GPlatesScribe::LoadRef<RotationModel::non_null_ptr_type> rotation_model;
		std::vector<GPlatesFileIO::File::non_null_ptr_type> reconstructable_files;
		boost::optional<unsigned int> reconstruct_snapshot_cache_size;
		if (!load_construct_data(
				scribe,
				rotation_model,
				reconstructable_files,
				reconstruct_snapshot_cache_size))
		{
			return scribe.get_transcribe_result();
		}

		// Create the topological model.
		reconstruct_model.construct_object(
				rotation_model,
				reconstructable_files,
				reconstruct_snapshot_cache_size);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesApi::ReconstructModel::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (scribe.is_saving())
		{
			save_construct_data(scribe, *this);
		}
		else // loading
		{
			GPlatesScribe::LoadRef<RotationModel::non_null_ptr_type> rotation_model;
			boost::optional<unsigned int> reconstruct_snapshot_cache_size;
			if (!load_construct_data(
					scribe,
					rotation_model,
					d_reconstructable_files,
					reconstruct_snapshot_cache_size))
			{
				return scribe.get_transcribe_result();
			}
			d_rotation_model = rotation_model.get();
			d_reconstruct_snapshot_cache_size = reconstruct_snapshot_cache_size;

			// Initialise reconstruction (based on the construct parameters we just loaded).
			//
			// Note: The existing reconstruction in 'this' reconstruct model must be old data
			//       because 'transcribed_construct_data' is false (ie, it was not transcribed) and so 'this'
			//       object must've been created first (using unknown constructor arguments) and *then* transcribed.
			initialise();
		}
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


void
GPlatesApi::ReconstructModel::save_construct_data(
		GPlatesScribe::Scribe &scribe,
		const ReconstructModel &reconstruct_model)
{
	// Save the rotation model.
	scribe.save(TRANSCRIBE_SOURCE, reconstruct_model.d_rotation_model, "rotation_model");

	const GPlatesScribe::ObjectTag files_tag("files");

	// Save number of reconstructable files.
	const unsigned int num_files = reconstruct_model.d_reconstructable_files.size();
	scribe.save(TRANSCRIBE_SOURCE, num_files, files_tag.sequence_size());

	// Save the topological files (feature collections and their filenames).
	for (unsigned int file_index = 0; file_index < num_files; ++file_index)
	{
		const auto feature_collection_file = reconstruct_model.d_reconstructable_files[file_index];

		const GPlatesModel::FeatureCollectionHandle::non_null_ptr_type feature_collection(
				feature_collection_file->get_reference().get_feature_collection().handle_ptr());
		const QString filename =
				feature_collection_file->get_reference().get_file_info().get_qfileinfo().absoluteFilePath();

		scribe.save(TRANSCRIBE_SOURCE, feature_collection, files_tag[file_index]("feature_collection"));
		scribe.save(TRANSCRIBE_SOURCE, filename, files_tag[file_index]("filename"));
	}

	// Save the reconstruct snapshot cache size.
	scribe.save(TRANSCRIBE_SOURCE, reconstruct_model.d_reconstruct_snapshot_cache_size, "reconstruct_snapshot_cache_size");
}


bool
GPlatesApi::ReconstructModel::load_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::LoadRef<RotationModel::non_null_ptr_type> &rotation_model,
		std::vector<GPlatesFileIO::File::non_null_ptr_type> &reconstructable_files,
		boost::optional<unsigned int> &reconstruct_snapshot_cache_size)
{
	// Load the rotation model.
	rotation_model = scribe.load<RotationModel::non_null_ptr_type>(TRANSCRIBE_SOURCE, "rotation_model");
	if (!rotation_model.is_valid())
	{
		return false;
	}

	const GPlatesScribe::ObjectTag files_tag("files");

	// Number of topological files.
	unsigned int num_files;
	if (!scribe.transcribe(TRANSCRIBE_SOURCE, num_files, files_tag.sequence_size()))
	{
		return false;
	}

	// Load the reconstructable files (feature collections and their filenames).
	for (unsigned int file_index = 0; file_index < num_files; ++file_index)
	{
		GPlatesScribe::LoadRef<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> feature_collection =
				scribe.load<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type>(
						TRANSCRIBE_SOURCE,
						files_tag[file_index]("feature_collection"));
		if (!feature_collection.is_valid())
		{
			return false;
		}

		QString filename;
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, filename, files_tag[file_index]("filename")))
		{
			return false;
		}

		reconstructable_files.push_back(
				GPlatesFileIO::File::create_file(GPlatesFileIO::FileInfo(filename), feature_collection));
	}

	// Load the reconstruct snapshot cache size.
	if (!scribe.transcribe(TRANSCRIBE_SOURCE, reconstruct_snapshot_cache_size, "reconstruct_snapshot_cache_size"))
	{
		return false;
	}

	return true;
}


void
export_reconstruct_model()
{
	//
	// ReconstructModel - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesApi::ReconstructModel,
			GPlatesApi::ReconstructModel::non_null_ptr_type,
			boost::noncopyable>(
					"ReconstructModel",
					"A history of reconstructed geometries over geological time.\n"
					"\n"
					"A *ReconstructModel* can also be `pickled <https://docs.python.org/3/library/pickle.html>`_.\n"
					"\n"
					".. versionadded:: 0.48\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::reconstruct_model_create,
						bp::default_call_policies(),
						(bp::arg("reconstructable_features"),
							bp::arg("rotation_model"),
							bp::arg("anchor_plate_id") = boost::optional<GPlatesModel::integer_plate_id_type>(),
							bp::arg("reconstruct_snapshot_cache_size") = boost::optional<unsigned int>())),
			"__init__(reconstructable_features, rotation_model, [anchor_plate_id], [reconstruct_snapshot_cache_size])\n"
			"  Create from reconstructable features and a rotation model.\n"
			"\n"
			"  :param reconstructable_features: The features to reconstruct as a feature collection, "
			"or filename, or feature, or sequence of features, or a sequence (eg, ``list`` or ``tuple``) "
			"of any combination of those four types.\n"
			"  :type reconstructable_features: FeatureCollection, or str, or os.PathLike, or Feature, "
			"or sequence of Feature, or sequence of any combination of those four types\n"
			"  :param rotation_model: A rotation model. Or a rotation feature collection, or a rotation filename, "
			"or a rotation feature, or a sequence of rotation features, or a sequence of any combination of those four types.\n"
			"  :type rotation_model: RotationModel. Or FeatureCollection, or str, or os.PathLike, "
			"or Feature, or sequence of Feature, or sequence of any combination of those four types\n"
			"  :param anchor_plate_id: The anchored plate id used for reconstructions. "
			"Defaults to the default anchor plate of *rotation_model* (or zero if *rotation_model* is not a :class:`RotationModel`).\n"
			"  :type anchor_plate_id: int\n"
			"  :param reconstruct_snapshot_cache_size: Number of reconstruct snapshots to cache internally. Defaults to unlimited.\n"
			"  :type reconstruct_snapshot_cache_size: int\n"
			"\n"
			"  Load a reconstruct model (and its associated rotation model):\n"
			"  ::\n"
			"\n"
			"    rotation_model = pygplates.RotationModel('rotations.rot')\n"
			"    reconstruct_model = pygplates.ReconstructModel('reconstructable_features.gpml', rotation_model)\n"
			"\n"
			"  ...or alternatively just:"
			"  ::\n"
			"\n"
			"    reconstruct_model = pygplates.ReconstructModel('reconstructable_features.gpml', 'rotations.rot')\n"
			"\n"
			"  .. note:: All reconstructions use *anchor_plate_id*. So if you need to use a different "
			"anchor plate ID then you'll need to create a new :class:`ReconstructModel<__init__>`. However this should "
			"only be done if necessary since each :class:`ReconstructModel` created can consume a reasonable amount of "
			"CPU and memory (since it caches reconstructed geometries over geological time).\n"
			"\n"
			"  .. note:: The *reconstruct_snapshot_cache_size* parameter controls "
			"the size of an internal least-recently-used cache of reconstruct snapshots "
			"(evicts least recently requested reconstruct snapshot when a new reconstruction "
			"time is requested that does not currently exist in the cache). This enables "
			"reconstruct snapshots associated with different reconstruction times to be re-used "
			"instead of re-creating them, provided they have not been evicted from the cache.\n")
		// Pickle support...
		//
		// Note: This adds an __init__ method accepting a single argument (of type 'bytes') that supports pickling.
		//       So we define this *after* (higher priority) the other __init__ methods in case one of them accepts a single argument
		//       of type bp::object (which, being more general, would otherwise obscure the __init__ that supports pickling).
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesApi::ReconstructModel::non_null_ptr_type>())
		.def("reconstruct_snapshot",
				&GPlatesApi::reconstruct_model_get_reconstruct_snapshot,
				(bp::arg("reconstruction_time")),
				"reconstruct_snapshot(reconstruction_time)\n"
				"  Returns a snapshot of reconstructed geometries at the requested reconstruction time.\n"
				"\n"
				"  :param reconstruction_time: the geological time of the snapshot\n"
				"  :type reconstruction_time: float or GeoTimeInstant\n"
				"  :rtype: ReconstructSnapshot\n"
				"  :raises: ValueError if *reconstruction_time* is distant-past (``float('inf')``) or distant-future (``float('-inf')``).\n")
		.def("get_rotation_model",
				&GPlatesApi::ReconstructModel::get_rotation_model,
				"get_rotation_model()\n"
				"  Return the rotation model used internally.\n"
				"\n"
				"  :rtype: RotationModel\n"
				"\n"
				"  .. note:: The :meth:`default anchor plate ID<RotationModel.get_default_anchor_plate_id>` of the returned rotation model "
				"may be different to that of the rotation model passed into the :meth:`constructor<__init__>` if an anchor plate ID was specified "
				"in the :meth:`constructor<__init__>`.\n"
				"\n"
				"  .. note:: The reconstruction tree cache size of the returned rotation model is equal to the *reconstruct_snapshot_cache_size* "
				"argument specified in the :meth:`constructor<__init__>` plus one (or unlimited if not specified).\n")
		.def("get_anchor_plate_id",
				&GPlatesApi::ReconstructModel::get_anchor_plate_id,
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
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesApi::ReconstructModel>();
}
