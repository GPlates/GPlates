/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#include "PyRotationModel.h"

#include "PyFeatureCollection.h"
#include "PyInterpolationException.h"
#include "PyReconstructionTree.h"
#include "PythonConverterUtils.h"
#include "PythonHashDefVisitor.h"

#include "global/GPlatesAssert.h"
#include "global/python.h"

#include "maths/MathsUtils.h"

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"

#include "property-values/GeoTimeInstant.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	namespace
	{
		/**
		 * A from-python converter from a rotation model or a sequence of feature collections to a
		 * @a RotationModelFunctionArgument.
		 */
		struct ConversionRotationModelFunctionArgument :
				private boost::noncopyable
		{
			static
			void *
			convertible(
					PyObject *obj)
			{
				namespace bp = boost::python;

				if (RotationModelFunctionArgument::is_convertible(
					bp::object(bp::handle<>(bp::borrowed(obj)))))
				{
					return obj;
				}

				return NULL;
			}

			static
			void
			construct(
					PyObject *obj,
					boost::python::converter::rvalue_from_python_stage1_data *data)
			{
				namespace bp = boost::python;

				void *const storage = reinterpret_cast<
						bp::converter::rvalue_from_python_storage<
								RotationModelFunctionArgument> *>(
										data)->storage.bytes;

				new (storage) RotationModelFunctionArgument(
						bp::object(bp::handle<>(bp::borrowed(obj))));

				data->convertible = storage;
			}
		};


		/**
		 * Registers converter from a rotation model or a sequence of feature collections to a @a RotationModelFunctionArgument.
		 */
		void
		register_rotation_model_function_argument_conversion()
		{
			// Register function argument types variant.
			PythonConverterUtils::register_variant_conversion<
					RotationModelFunctionArgument::function_argument_type>();

			// NOTE: We don't define a to-python conversion.
			// 
			// From python conversion.
			bp::converter::registry::push_back(
					&ConversionRotationModelFunctionArgument::convertible,
					&ConversionRotationModelFunctionArgument::construct,
					bp::type_id<RotationModelFunctionArgument>());
		}


		/**
		 * This is called directly from Python via 'RotationModel.__init__()'.
		 */
		RotationModel::non_null_ptr_type
		rotation_model_create_from_features(
				const FeatureCollectionSequenceFunctionArgument &rotation_features,
				unsigned int reconstruction_tree_cache_size,
				bool extend_total_reconstruction_poles_to_distant_past)
		{
			return RotationModel::create(
					rotation_features,
					reconstruction_tree_cache_size,
					extend_total_reconstruction_poles_to_distant_past);
		}

		/**
		 * This is called directly from Python via 'RotationModel.__init__()'.
		 *
		 * This is a deprecated overload of 'RotationModel.__init__()' that accepts the
		 * deprecated argument 'clone_rotation_features' (which is no longer needed, but we
		 * still accept for any clients still using it, and then just ignore it).
		 *
		 * Note that this overload has the same type and order of function arguments as
		 * @a rotation_model_create_from_features, and so it is only distinguishable if the
		 * caller explicitly specified the 'clone_rotation_features' argument name, as in:
		 *
		 *   rotation_model = RotationModel(rotation_features, clone_rotation_features=False)
		 *
		 * ...however, if they specified:
		 *
		 *   rotation_model = RotationModel(rotation_features, False)
		 *
		 * ...then we incorrectly accept the @a rotation_model_create_from_features overload, and
		 * the 'clone_rotation_features' argument will be interpreted as a
		 * 'extend_total_reconstruction_poles_to_distant_past' argument.
		 */
		RotationModel::non_null_ptr_type
		rotation_model_create_from_features_deprecated(
				const FeatureCollectionSequenceFunctionArgument &rotation_features,
				unsigned int reconstruction_tree_cache_size,
				bool clone_rotation_features)
		{
			// Ignore deprecated 'clone_rotation_features' argument.
			return RotationModel::create(
					rotation_features,
					reconstruction_tree_cache_size);
		}

		/**
		 * This is called directly from Python via 'RotationModel.__init__()'.
		 *
		 * This enables Python code such as:
		 *
		 *   def my_func(rotation_features_or_model, ...):
		 *     rotation_model = pygplates.RotationModel(rotation_features_or_model)
		 *
		 * ...which will work if 'rotation_features_or_model' is already a 'RotationModel'.
		 */
		RotationModel::non_null_ptr_type
		rotation_model_create_from_rotation_model(
				RotationModel::non_null_ptr_type rotation_model)
		{
			return rotation_model;
		}
	}
}


// We don't want this excessively large because it uses memory, but make it large enough so
// that all reconstruction trees (times) used to reconstruct a mid-ocean ridge fit in the cache.
// The default half-stage time interval is 10My (see RotationUtils::get_half_stage_rotation()')
// so a caching 100 entries will support mid-ocean ridges as old as 1,000 Ma.
// An example of such caching is reconstructing (or reverse reconstructing) a group of
// mid-ocean ridges with the same time-of-appearance. They need to have the same time of appearance
// because version 3 half-stage rotations start spreading at the time-of-appearance
// (ie, the 10My intervals are 'begin_time', 'begin_time-10', begin_time-20', ..., reconstruction_time).
// Because the mid-ocean ridges have the same time intervals they'll reuse the cache entries.
// Whereas version 2 starts at present day, which means mid-ocean ridges with different
// appearances times will still share 10My intervals (ie, 0Ma, 10Ma, 20Ma, ..., reconstruction_time).
// So version 2 is less restrictive in its ability to share cache entries.
//
// Each cache entry (reconstruction tree) consumes ~0.5Mb.
const unsigned int GPlatesApi::RotationModel::DEFAULT_RECONSTRUCTION_TREE_CACHE_SIZE = 150;


GPlatesApi::RotationModel::non_null_ptr_type
GPlatesApi::RotationModel::create(
		const FeatureCollectionSequenceFunctionArgument &rotation_features,
		unsigned int reconstruction_tree_cache_size,
		bool extend_total_reconstruction_poles_to_distant_past)
{
	// Copy feature collection files into a vector.
	std::vector<GPlatesFileIO::File::non_null_ptr_type> feature_collection_files;
	rotation_features.get_files(feature_collection_files);

	return create(
			feature_collection_files,
			reconstruction_tree_cache_size,
			extend_total_reconstruction_poles_to_distant_past);
}


GPlatesApi::RotationModel::non_null_ptr_type
GPlatesApi::RotationModel::create(
		const std::vector<GPlatesFileIO::File::non_null_ptr_type> &feature_collection_files,
		unsigned int reconstruction_tree_cache_size,
		bool extend_total_reconstruction_poles_to_distant_past)
{
	// Convert the feature collections (in the files) to weak refs (for ReconstructionTreeCreator).
	std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> feature_collection_refs;
	BOOST_FOREACH(
			GPlatesFileIO::File::non_null_ptr_type feature_collection_file,
			feature_collection_files)
	{
		GPlatesModel::FeatureCollectionHandle::non_null_ptr_type feature_collection =
				GPlatesUtils::get_non_null_pointer(
						feature_collection_file->get_reference().get_feature_collection().handle_ptr());

		feature_collection_refs.push_back(feature_collection->reference());
	}

	// Create a cached reconstruction tree creator.
	const GPlatesAppLogic::ReconstructionTreeCreator reconstruction_tree_creator =
			GPlatesAppLogic::create_cached_reconstruction_tree_creator(
					feature_collection_refs,
					extend_total_reconstruction_poles_to_distant_past,
					0/*default_anchor_plate_id*/,
					reconstruction_tree_cache_size);

	return non_null_ptr_type(new RotationModel(feature_collection_files, reconstruction_tree_creator));
}


GPlatesApi::RotationModel::non_null_ptr_type
GPlatesApi::RotationModel::create(
		const std::vector<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> &feature_collections,
		unsigned int reconstruction_tree_cache_size,
		bool extend_total_reconstruction_poles_to_distant_past)
{
	// Create feature collection files with empty filenames.
	std::vector<GPlatesFileIO::File::non_null_ptr_type> feature_collection_files;
	BOOST_FOREACH(
			GPlatesModel::FeatureCollectionHandle::non_null_ptr_type feature_collection,
			feature_collections)
	{
		// Create a file with an empty filename - since we don't know if feature collection
		// came from a file or not.
		GPlatesFileIO::File::non_null_ptr_type feature_collection_file =
				GPlatesFileIO::File::create_file(GPlatesFileIO::FileInfo(), feature_collection);

		feature_collection_files.push_back(feature_collection_file);
	}

	return create(feature_collection_files, reconstruction_tree_cache_size, extend_total_reconstruction_poles_to_distant_past);
}


GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type
GPlatesApi::RotationModel::get_reconstruction_tree(
		const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time,
		GPlatesModel::integer_plate_id_type anchor_plate_id)
{
	// Time must not be distant past/future.
	GPlatesGlobal::Assert<InterpolationException>(
			reconstruction_time.is_real(),
			GPLATES_ASSERTION_SOURCE,
			"Time values cannot be distant-past (float('inf')) or distant-future (float('-inf')).");

	return d_reconstruction_tree_creator.get_reconstruction_tree(reconstruction_time.value(), anchor_plate_id);
}


boost::optional<GPlatesMaths::FiniteRotation>
GPlatesApi::RotationModel::get_rotation(
		const GPlatesPropertyValues::GeoTimeInstant &to_time,
		GPlatesModel::integer_plate_id_type moving_plate_id,
		const GPlatesPropertyValues::GeoTimeInstant &from_time,
		boost::optional<GPlatesModel::integer_plate_id_type> fixed_plate_id,
		GPlatesModel::integer_plate_id_type anchor_plate_id,
		bool use_identity_for_missing_plate_ids)
{
	// Times must not be distant past/future.
	GPlatesGlobal::Assert<InterpolationException>(
			to_time.is_real() && from_time.is_real(),
			GPLATES_ASSERTION_SOURCE,
			"Time values cannot be distant-past (float('inf')) or distant-future (float('-inf')).");

	GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type to_reconstruction_tree =
			get_reconstruction_tree(to_time, anchor_plate_id);

	if (from_time == GPlatesPropertyValues::GeoTimeInstant(0))
	{
		if (!fixed_plate_id)
		{
			// Use the API function in "PyReconstructionTree.h".
			return GPlatesApi::get_equivalent_total_rotation(
					*to_reconstruction_tree,
					moving_plate_id,
					use_identity_for_missing_plate_ids);
		}

		// Use the API function in "PyReconstructionTree.h".
		return GPlatesApi::get_relative_total_rotation(
				*to_reconstruction_tree,
				moving_plate_id,
				fixed_plate_id.get(),
				use_identity_for_missing_plate_ids);
	}

	const GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type from_reconstruction_tree =
			get_reconstruction_tree(from_time, anchor_plate_id);

	if (!fixed_plate_id)
	{
		// Use the API function in "PyReconstructionTree.h".
		return GPlatesApi::get_equivalent_stage_rotation(
				*from_reconstruction_tree,
				*to_reconstruction_tree,
				moving_plate_id,
				use_identity_for_missing_plate_ids);
	}

	// Use the API function in "PyReconstructionTree.h".
	return GPlatesApi::get_relative_stage_rotation(
			*from_reconstruction_tree,
			*to_reconstruction_tree,
			moving_plate_id,
			fixed_plate_id.get(),
			use_identity_for_missing_plate_ids);
}


void
GPlatesApi::RotationModel::get_feature_collections(
		std::vector<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> &feature_collections) const
{
	BOOST_FOREACH(GPlatesFileIO::File::non_null_ptr_type feature_collection_file, d_feature_collection_files)
	{
		GPlatesModel::FeatureCollectionHandle::non_null_ptr_type feature_collection =
				GPlatesUtils::get_non_null_pointer(
						feature_collection_file->get_reference().get_feature_collection().handle_ptr());

		feature_collections.push_back(feature_collection);
	}
}


void
GPlatesApi::RotationModel::get_files(
		std::vector<GPlatesFileIO::File::non_null_ptr_type> &feature_collection_files) const
{
	feature_collection_files.insert(
			feature_collection_files.end(),
			d_feature_collection_files.begin(),
			d_feature_collection_files.end());
}


bool
GPlatesApi::RotationModelFunctionArgument::is_convertible(
		bp::object python_function_argument)
{
	// It needs to be either a rotation model or a sequence of feature collections.
	//
	// Note: We avoid actually extracting the feature collections since we don't want to read
	// them from files (ie, we only want to check the argument type).
	return bp::extract<RotationModel::non_null_ptr_type>(python_function_argument).check() ||
		bp::extract<FeatureCollectionSequenceFunctionArgument>(python_function_argument).check();
}


GPlatesApi::RotationModelFunctionArgument::RotationModelFunctionArgument(
		bp::object python_function_argument) :
	d_rotation_model(initialise_rotation_model(bp::extract<function_argument_type>(python_function_argument)))
{
}


GPlatesApi::RotationModelFunctionArgument::RotationModelFunctionArgument(
		const function_argument_type &function_argument) :
	d_rotation_model(initialise_rotation_model(function_argument))
{
}


GPlatesApi::RotationModel::non_null_ptr_type
GPlatesApi::RotationModelFunctionArgument::initialise_rotation_model(
		const function_argument_type &function_argument)
{
	if (const RotationModel::non_null_ptr_type *rotation_model =
		boost::get<RotationModel::non_null_ptr_type>(&function_argument))
	{
		return *rotation_model;
	}

	const FeatureCollectionSequenceFunctionArgument feature_collections_function_argument =
			boost::get<FeatureCollectionSequenceFunctionArgument>(function_argument);

	return RotationModel::create(feature_collections_function_argument);
}


GPlatesApi::RotationModel::non_null_ptr_type
GPlatesApi::RotationModelFunctionArgument::get_rotation_model() const
{
	return d_rotation_model;
}


void
export_rotation_model()
{
	std::stringstream rotation_model_from_features_constructor_docstring_stream;
	rotation_model_from_features_constructor_docstring_stream <<
			// General overloaded signature (must be in first overloaded 'def' - used by Sphinx)...
			// Specific overload signature...
			"__init__(...)\n"
			"A *RotationModel* object can be constructed in more than one way...\n"
			"\n"
			// Specific overload signature...
			"__init__(rotation_features, [reconstruction_tree_cache_size="
			<< GPlatesApi::RotationModel::DEFAULT_RECONSTRUCTION_TREE_CACHE_SIZE
			<< "], [extend_total_reconstruction_poles_to_distant_past=False])\n"
			"  Create from rotation feature collection(s) and/or rotation filename(s), and "
			"an optional reconstruction tree cache size.\n"
			"\n"
			"  :param rotation_features: A rotation feature collection, or rotation filename, or "
			"rotation feature, or sequence of rotation features, or a sequence (eg, ``list`` or ``tuple``) "
			"of any combination of those four types\n"
			"  :type rotation_features: :class:`FeatureCollection`, or string, or :class:`Feature`, "
			"or sequence of :class:`Feature`, or sequence of any combination of those four types\n"
			"  :param reconstruction_tree_cache_size: number of reconstruction trees to cache internally\n"
			"  :type reconstruction_tree_cache_size: int\n"
			"  :param extend_total_reconstruction_poles_to_distant_past: extend each moving plate "
			"sequence back infinitely far into the distant past such that reconstructed geometries will "
			"not snap back to their present day positions when the reconstruction time is older than "
			"the oldest times specified in the rotation features (defaults to ``False``)\n"
			"  :type extend_total_reconstruction_poles_to_distant_past: bool\n"
			"  :raises: OpenFileForReadingError if any file is not readable (when filenames specified)\n"
			"  :raises: FileFormatNotSupportedError if any file format (identified by the filename "
			"extensions) does not support reading (when filenames specified)\n"
			"\n"
			"  Note that *rotation_features* can be a rotation :class:`FeatureCollection` or a "
			"rotation filename or a rotation :class:`Feature` or a sequence of rotation :class:`features<Feature>`, "
			"or a sequence (eg, ``list`` or ``tuple``) of any combination of those four types.\n"
			"\n"
			"  If any rotation filenames are specified then this method uses "
			":class:`FeatureCollectionFileFormatRegistry` internally to read the rotation files.\n"
			"\n"
			"  Load a rotation file and some rotation adjustments (as a collection of rotation features) "
			"into a rotation model:\n"
			"  ::\n"
			"\n"
			"    rotation_adjustments = pygplates.FeatureCollection()\n"
			"    ...\n"
			"    rotation_model = pygplates.RotationModel(['rotations.rot', rotation_adjustments])\n"
			;

	//
	// RotationModel - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesApi::RotationModel,
			GPlatesApi::RotationModel::non_null_ptr_type,
			boost::noncopyable>(
					"RotationModel",
					"Query a finite rotation of a moving plate relative to any other plate, optionally "
					"between two instants in geological time.\n"
					"\n"
					"See :ref:`pygplates_foundations_plate_reconstruction_hierarchy`.\n"
					"\n"
					"This class provides an easy way to query rotations in any of the four combinations of "
					"total/stage and equivalent/relative rotations using :meth:`get_rotation`. "
					":class:`Reconstruction trees<ReconstructionTree>` can also be created at any instant "
					"of geological time and these are cached internally depending on a user-specified "
					"cache size parameter pass to :meth:`__init__`. "
					"The *reconstruction_tree_cache_size* parameter of those "
					"methods controls the size of an internal least-recently-used cache of reconstruction "
					"trees (evicts least recently requested reconstruction tree when a new reconstruction "
					"time is requested that does not currently exist in the cache). This enables "
					"reconstruction trees associated with different reconstruction times to be re-used "
					"instead of re-creating them, provided they have not been evicted from the cache. "
					"This benefit also applies when querying rotations with :meth:`get_rotation` since "
					"it, in turn, requests reconstruction trees.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		// Define deprecated '__init__' first since later definitions get higher priority.
		// Also exclude a docstring for it, since we don't want it documented anymore...
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::rotation_model_create_from_features_deprecated,
						bp::default_call_policies(),
						(bp::arg("rotation_features"),
							bp::arg("reconstruction_tree_cache_size") =
								GPlatesApi::RotationModel::DEFAULT_RECONSTRUCTION_TREE_CACHE_SIZE,
							bp::arg("clone_rotation_features") = true)))
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::rotation_model_create_from_features,
						bp::default_call_policies(),
						(bp::arg("rotation_features"),
							bp::arg("reconstruction_tree_cache_size") =
								GPlatesApi::RotationModel::DEFAULT_RECONSTRUCTION_TREE_CACHE_SIZE,
							bp::arg("extend_total_reconstruction_poles_to_distant_past") = false)),
				rotation_model_from_features_constructor_docstring_stream.str().c_str())
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::rotation_model_create_from_rotation_model,
						bp::default_call_policies(),
						(bp::arg("rotation_model"))),
			// Specific overload signature...
			"__init__(rotation_model)\n"
			"  Create from an existing rotation model as a convenience.\n"
			"\n"
			"  :param rotation_model: an existing rotation model\n"
			"  :type rotation_model: :class:`RotationModel`\n"
			"\n"
			"  This is useful when defining your own function that accepts rotation features or a rotation model. "
			"It avoids the hassle of having to explicitly test for each source type:\n"
			"  ::\n"
			"\n"
			"    def my_function(rotation_features_or_model):\n"
			"        # The appropriate constructor (__init__) overload is chosen depending on argument type.\n"
			"        rotation_model = pygplates.RotationModel(rotation_features_or_model)\n"
			"        ...\n"
			"\n"
			"  .. note:: This :meth:`constructor<__init__>` just returns a reference to *rotation_model* because a "
			"*RotationModel* object is immutable (contains no operations or methods that modify its state) and "
			"hence a deep copy of *rotation_model* is not needed.\n")
		.def("get_rotation",
				&GPlatesApi::RotationModel::get_rotation,
				(bp::arg("to_time"),
					bp::arg("moving_plate_id"),
					bp::arg("from_time") = 0,
					bp::arg("fixed_plate_id") = boost::optional<GPlatesModel::integer_plate_id_type>(),
					bp::arg("anchor_plate_id") = 0,
					bp::arg("use_identity_for_missing_plate_ids") = true),
				"get_rotation(to_time, moving_plate_id, [from_time=0], [fixed_plate_id], [anchor_plate_id=0]"
				", [use_identity_for_missing_plate_ids=True])\n"
				"  Return the finite rotation that rotates from the *fixed_plate_id* plate to the *moving_plate_id* "
				"plate and from the time *from_time* to the time *to_time*.\n"
				"\n"
				"  :param to_time: time at which the moving plate is being rotated *to* (in Ma)\n"
				"  :type to_time: float or :class:`GeoTimeInstant`\n"
				"  :param moving_plate_id: the plate id of the moving plate\n"
				"  :type moving_plate_id: int\n"
				"  :param from_time: time at which the moving plate is being rotated *from* (in Ma)\n"
				"  :type from_time: float or :class:`GeoTimeInstant`\n"
				"  :param fixed_plate_id: the plate id of the fixed plate (defaults to *anchor_plate_id* "
				"if not specified)\n"
				"  :type fixed_plate_id: int\n"
				"  :param anchor_plate_id: the id of the anchored plate\n"
				"  :type anchor_plate_id: int\n"
				"  :param use_identity_for_missing_plate_ids: whether to return an "
				":meth:`identity rotation<FiniteRotation.create_identity_rotation>` or return ``None`` "
				"for missing plate ids (default is to use identity rotation)\n"
				"  :type use_identity_for_missing_plate_ids: bool\n"
				"  :rtype: :class:`FiniteRotation`, or None (if *use_identity_for_missing_plate_ids* is ``False``)\n"
				"  :raises: InterpolationError if any time value is "
				":meth:`distant past<GeoTimeInstant.is_distant_past>` or "
				":meth:`distant future<GeoTimeInstant.is_distant_future>`\n"
				"\n"
				"  This method conveniently handles all four combinations of total/stage and "
				"equivalent/relative rotations normally handled by:\n"
				"\n"
				"  * :meth:`ReconstructionTree.get_equivalent_total_rotation` - "
				"see :ref:`pygplates_foundations_equivalent_total_rotation` for rotation math derivation\n"
				"  * :meth:`ReconstructionTree.get_relative_total_rotation` - "
				"see :ref:`pygplates_foundations_relative_total_rotation` for rotation math derivation\n"
				"  * :meth:`ReconstructionTree.get_equivalent_stage_rotation` - "
				"see :ref:`pygplates_foundations_equivalent_stage_rotation` for rotation math derivation\n"
				"  * :meth:`ReconstructionTree.get_relative_stage_rotation` - "
				"see :ref:`pygplates_foundations_relative_stage_rotation` for rotation math derivation\n"
				"\n"
				"  If *fixed_plate_id* is not specified then it defaults to *anchor_plate_id* (which "
				"itself defaults to zero). Normally it is sufficient to specify *fixed_plate_id* "
				"(for a relative rotation) and leave *anchor_plate_id* as its default (zero). "
				"However if there is no plate circuit path from the default anchor plate (zero) to either "
				"*moving_plate_id* or *fixed_plate_id*, but there is a path from *fixed_plate_id* to "
				"*moving_plate_id*, then the correct result will require setting *anchor_plate_id* to "
				"*fixed_plate_id*. See :ref:`pygplates_foundations_plate_reconstruction_hierarchy` for "
				"an overview of plate circuit paths.\n"
				"\n"
				"  If there is no plate circuit path from *moving_plate_id* (and optionally *fixed_plate_id*) "
				"to the anchor plate (at times *to_time* and optionally *from_time*) then an "
				":meth:`identity rotation<FiniteRotation.create_identity_rotation>` is returned if "
				"*use_identity_for_missing_plate_ids* is ``True``, otherwise ``None`` is returned. "
				"See :ref:`pygplates_foundations_plate_reconstruction_hierarchy` for details on how a "
				"plate id can go missing and how to work around it.\n"
				"\n"
				"  This method essentially does the following:\n"
				"  ::\n"
				"\n"
				"    def get_rotation(rotation_model, to_time, moving_plate_id, from_time=0, "
				"fixed_plate_id=None, anchor_plate_id=0):\n"
				"        \n"
				"        if from_time == 0:\n"
				"            if fixed_plate_id is None:\n"
				"                return rotation_model.get_reconstruction_tree(to_time, anchor_plate_id)"
				".get_equivalent_total_rotation(moving_plate_id)\n"
				"            \n"
				"            return rotation_model.get_reconstruction_tree(to_time, anchor_plate_id)"
				".get_relative_total_rotation(moving_plate_id, fixed_plate_id)\n"
				"        \n"
				"        if fixed_plate_id is None:\n"
				"            return pygplates.ReconstructionTree.get_equivalent_stage_rotation(\n"
				"                rotation_model.get_reconstruction_tree(from_time, anchor_plate_id),\n"
				"                rotation_model.get_reconstruction_tree(to_time, anchor_plate_id),\n"
				"                moving_plate_id)\n"
				"        \n"
				"        return pygplates.ReconstructionTree.get_relative_stage_rotation(\n"
				"            rotation_model.get_reconstruction_tree(from_time, anchor_plate_id),\n"
				"            rotation_model.get_reconstruction_tree(to_time, anchor_plate_id),\n"
				"            moving_plate_id,\n"
				"            fixed_plate_id)\n")
		.def("get_reconstruction_tree",
				&GPlatesApi::RotationModel::get_reconstruction_tree,
				(bp::arg("reconstruction_time"),
					bp::arg("anchor_plate_id")=0),
				"get_reconstruction_tree(reconstruction_time, [anchor_plate_id=0])\n"
				"  Return the reconstruction tree associated with the specified instant of "
				"geological time and anchored plate id.\n"
				"\n"
				"  :param reconstruction_time: time at which to create a reconstruction tree (in Ma)\n"
				"  :type reconstruction_time: float or :class:`GeoTimeInstant`\n"
				"  :param anchor_plate_id: the id of the anchored plate that *equivalent* rotations "
				"are calculated with respect to\n"
				"  :type anchor_plate_id: int\n"
				"  :rtype: :class:`ReconstructionTree`\n"
				"  :raises: InterpolationError if *reconstruction_time* is "
				":meth:`distant past<GeoTimeInstant.is_distant_past>` or "
				":meth:`distant future<GeoTimeInstant.is_distant_future>`\n"
				"\n"
				"  If the reconstruction tree for the specified reconstruction time and anchored plate id "
				"is currently in the internal cache then it is returned, otherwise a new reconstruction "
				"tree is created and stored in the cache (after evicting the reconstruction tree associated "
				"with the least recently requested reconstruction time and anchored plate id if necessary).\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesApi::RotationModel>();

	// Register converter from a rotation model or a sequence of feature collections to a @a RotationModelFunctionArgument.
	GPlatesApi::register_rotation_model_function_argument_conversion();
}

#endif // GPLATES_NO_PYTHON
