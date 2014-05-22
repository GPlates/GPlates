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

#include "PyReconstructionTree.h"
#include "PythonConverterUtils.h"

#include "global/GPlatesAssert.h"
#include "global/python.h"
// This is not included by <boost/python.hpp>.
// Also we must include this after <boost/python.hpp> which means after "global/python.h".
#include <boost/python/stl_iterator.hpp>

#include "maths/MathsUtils.h"

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	namespace
	{
		/**
		 * A from-python converter from a rotation model or a sequence of feature collections to a
		 * @a RotationModelFunctionArgument, and a to-python convert back to a rotation model.
		 */
		struct python_RotationModelFunctionArgument :
				private boost::noncopyable
		{
			struct Conversion
			{
				static
				PyObject *
				convert(
						const RotationModelFunctionArgument &function_arg)
				{
					namespace bp = boost::python;

					return bp::incref(function_arg.to_python().ptr());
				}
			};

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

			// To python conversion.
			bp::to_python_converter<
					RotationModelFunctionArgument,
					python_RotationModelFunctionArgument::Conversion>();

			// From python conversion.
			bp::converter::registry::push_back(
					&python_RotationModelFunctionArgument::convertible,
					&python_RotationModelFunctionArgument::construct,
					bp::type_id<RotationModelFunctionArgument>());
		}
	}
}


const unsigned int GPlatesApi::RotationModel::DEFAULT_RECONSTRUCTION_TREE_CACHE_SIZE = 32;


GPlatesApi::RotationModel::non_null_ptr_type
GPlatesApi::RotationModel::create(
		bp::object rotation_features_python_object,
		unsigned int reconstruction_tree_cache_size)
{
	// Use convenience class 'FeatureCollectionSequenceFunctionArgument' to extract the
	// rotation features allowing a variety of ways for the user to specify the features.
	const FeatureCollectionSequenceFunctionArgument feature_collections_function_argument =
			bp::extract<FeatureCollectionSequenceFunctionArgument>(rotation_features_python_object);

	// Copy feature collection files into a vector.
	std::vector<GPlatesFileIO::File::non_null_ptr_type> feature_collection_files;
	feature_collections_function_argument.get_files(feature_collection_files);

	return create(feature_collection_files, reconstruction_tree_cache_size);
}


GPlatesApi::RotationModel::non_null_ptr_type
GPlatesApi::RotationModel::create(
		const std::vector<GPlatesFileIO::File::non_null_ptr_type> &feature_collection_files,
		unsigned int reconstruction_tree_cache_size)
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
					0/*default_anchor_plate_id*/,
					reconstruction_tree_cache_size);

	return non_null_ptr_type(new RotationModel(feature_collection_files, reconstruction_tree_creator));
}


GPlatesApi::RotationModel::non_null_ptr_type
GPlatesApi::RotationModel::create(
		const std::vector<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> &feature_collections,
		unsigned int reconstruction_tree_cache_size)
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

	return create(feature_collection_files, reconstruction_tree_cache_size);
}


GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type
GPlatesApi::RotationModel::get_reconstruction_tree(
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type anchor_plate_id)
{
	return d_reconstruction_tree_creator.get_reconstruction_tree(reconstruction_time, anchor_plate_id);
}


boost::optional<GPlatesMaths::FiniteRotation>
GPlatesApi::RotationModel::get_rotation(
		const double &to_time,
		GPlatesModel::integer_plate_id_type moving_plate_id,
		const double &from_time,
		boost::optional<GPlatesModel::integer_plate_id_type> fixed_plate_id,
		GPlatesModel::integer_plate_id_type anchor_plate_id,
		bool use_identity_for_missing_plate_ids)
{
	GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type to_reconstruction_tree =
			get_reconstruction_tree(to_time, anchor_plate_id);

	if (GPlatesMaths::are_almost_exactly_equal(from_time, 0))
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

	// Copy feature collections into a vector.
	std::vector<GPlatesFileIO::File::non_null_ptr_type> feature_collection_files;
	feature_collections_function_argument.get_files(feature_collection_files);

	return RotationModel::create(feature_collection_files);
}


bp::object
GPlatesApi::RotationModelFunctionArgument::to_python() const
{
	// Wrap rotation model in a python object.
	return bp::object(get_rotation_model());
}


GPlatesApi::RotationModel::non_null_ptr_type
GPlatesApi::RotationModelFunctionArgument::get_rotation_model() const
{
	return d_rotation_model;
}


void
export_rotation_model()
{
	// Select the desired overload...
	GPlatesApi::RotationModel::non_null_ptr_type (*rotation_model_create)(
			boost::python::object,
			unsigned int) =
					&GPlatesApi::RotationModel::create;

	std::stringstream rotation_model_constructor_docstring_stream;
	rotation_model_constructor_docstring_stream <<
			"__init__(rotation_features[, reconstruction_tree_cache_size="
			<< GPlatesApi::RotationModel::DEFAULT_RECONSTRUCTION_TREE_CACHE_SIZE
			<< "])\n"
			"  Create from rotation feature collection(s) and/or rotation filename(s), and "
			"an optional reconstruction tree cache size.\n"
			"\n"
			"  :param rotation_features: A rotation feature collection or a rotation  filename "
			"or a sequence of rotation feature collections and/or rotation filenames\n"
			"  :type rotation_features: :class:`FeatureCollection` or string or sequence of "
			":class:`FeatureCollection` and/or strings\n"
			"  :param reconstruction_tree_cache_size: number of reconstruction trees to cache internally\n"
			"  :type reconstruction_tree_cache_size: int\n"
			"  :raises: OpenFileForReadingError if any file is not readable (when filenames specified)\n"
			"  :raises: FileFormatNotSupportedError if any file format (identified by the filename "
			"extensions) does not support reading (when filenames specified)\n"
			"\n"
			"  Note that *rotation_features* can be a rotation :class:`FeatureCollection` or a "
			"rotation filename or a sequence (eg, ``list`` or ``tuple``) containing rotation "
			":class:`FeatureCollection` instances or filenames (or a mixture of both).\n"
			"\n"
			"  If any rotation filenames are specified then this method uses "
			":class:`FeatureCollectionFileFormatRegistry` internally to read the rotation files.\n"
			"\n"
			"  ::\n"
			"\n"
			"    rotation_adjustments = pygplates.FeatureCollection()\n"
			"    ...\n"
			"    rotation_model = pygplates.RotationModel(['rotations.rot', rotation_adjustments])\n";

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
		.def("__init__",
				bp::make_constructor(
						rotation_model_create,
						bp::default_call_policies(),
						(bp::arg("rotation_features"),
							bp::arg("reconstruction_tree_cache_size") =
								GPlatesApi::RotationModel::DEFAULT_RECONSTRUCTION_TREE_CACHE_SIZE)),
				rotation_model_constructor_docstring_stream.str().c_str())
		.def("get_rotation",
				&GPlatesApi::RotationModel::get_rotation,
				(bp::arg("to_time"),
					bp::arg("moving_plate_id"),
					bp::arg("from_time") = 0,
					bp::arg("fixed_plate_id") = boost::optional<GPlatesModel::integer_plate_id_type>(),
					bp::arg("anchor_plate_id") = 0,
					bp::arg("use_identity_for_missing_plate_ids") = true),
				"get_rotation(to_time, moving_plate_id[, from_time=0][, fixed_plate_id][, anchor_plate_id=0]"
				"[, use_identity_for_missing_plate_ids=True]) -> FiniteRotation or None\n"
				"  Return the finite rotation that rotates from the *fixed_plate_id* plate to the *moving_plate_id* "
				"plate and from the time *from_time* to the time *to_time*.\n"
				"\n"
				"  :param to_time: time at which the moving plate is being rotated *to* (in Ma)\n"
				"  :type to_time: float\n"
				"  :param moving_plate_id: the plate id of the moving plate\n"
				"  :type moving_plate_id: int\n"
				"  :param from_time: time at which the moving plate is being rotated *from* (in Ma)\n"
				"  :type from_time: float\n"
				"  :param fixed_plate_id: the plate id of the fixed plate (defaults to *anchor_plate_id* "
				"if not specified)\n"
				"  :type fixed_plate_id: int\n"
				"  :param anchor_plate_id: the id of the anchored plate\n"
				"  :type anchor_plate_id: int\n"
				"  :param use_identity_for_missing_plate_ids: whether to use an "
				":meth:`identity rotation<FiniteRotation.create_identity_rotation>` or return ``None`` "
				"for missing plate ids (default is to use identity rotation)\n"
				"  :type use_identity_for_missing_plate_ids: bool\n"
				"  :rtype: :class:`FiniteRotation`, or None (if *use_identity_for_missing_plate_ids* is ``False``)\n"
				"\n"
				"  This method conveniently handles all four combinations of total/stage and "
				"equivalent/relative rotations (see :class:`ReconstructionTree` for details) normally handled by "
				":meth:`ReconstructionTree.get_equivalent_total_rotation`, "
				":meth:`ReconstructionTree.get_relative_total_rotation`, "
				":func:`get_equivalent_stage_rotation` and "
				":func:`get_relative_stage_rotation`.\n"
				"\n"
				"  If *fixed_plate_id* is not specified then it defaults to *anchor_plate_id* (which "
				"itself defaults to zero). Normally it is sufficient to specify *fixed_plate_id* "
				"(for a relative rotation) and leave *anchor_plate_id* as its default (zero). "
				"However if there is no plate circuit path from the default anchor plate (zero) to either "
				"*moving_plate_id* or *fixed_plate_id*, but there is a path from *fixed_plate_id* to "
				"*moving_plate_id*, then the correct result will require setting *anchor_plate_id* to "
				"*fixed_plate_id*. See :class:`ReconstructionTree` for an overview of plate circuit paths.\n"
				"\n"
				"  If there is no plate circuit path from *moving_plate_id* (and optionally *fixed_plate_id*) "
				"to the anchor plate (at times *to_time* and optionally *from_time*) then an "
				":meth:`identity rotation<FiniteRotation.create_identity_rotation>` is used for that "
				"plate's rotation if *use_identity_for_missing_plate_ids* is ``True``, "
				"otherwise this method returns immediately with ``None``. See :class:`ReconstructionTree` "
				"for details on how a plate id can go missing and how to work around it.\n"
				"\n"
				"  This method essentially does the following:\n"
				"  ::\n"
				"\n"
				"    def get_rotation(rotation_model, to_time, moving_plate_id, from_time=0, "
				"fixed_plate_id=None, anchor_plate_id=0):\n"
				"        \n"
				"        if from_time == 0:\n"
				"            if not fixed_plate_id:\n"
				"                return rotation_model.get_reconstruction_tree(to_time, anchor_plate_id)"
				".get_equivalent_total_rotation(moving_plate_id)\n"
				"            \n"
				"            return rotation_model.get_reconstruction_tree(to_time, anchor_plate_id)"
				".get_relative_total_rotation(moving_plate_id, fixed_plate_id)\n"
				"        \n"
				"        if not fixed_plate_id:\n"
				"            return pygplates.get_equivalent_stage_rotation(\n"
				"                rotation_model.get_reconstruction_tree(from_time, anchor_plate_id),\n"
				"                rotation_model.get_reconstruction_tree(to_time, anchor_plate_id),\n"
				"                moving_plate_id)\n"
				"        \n"
				"        return pygplates.get_relative_stage_rotation(\n"
				"            rotation_model.get_reconstruction_tree(from_time, anchor_plate_id),\n"
				"            rotation_model.get_reconstruction_tree(to_time, anchor_plate_id),\n"
				"            moving_plate_id,\n"
				"            fixed_plate_id)\n")
		.def("get_reconstruction_tree",
				&GPlatesApi::RotationModel::get_reconstruction_tree,
				(bp::arg("reconstruction_time"),
					bp::arg("anchor_plate_id")=0),
				"get_reconstruction_tree(reconstruction_time[, anchor_plate_id=0]) -> ReconstructionTree\n"
				"  Return the reconstruction tree associated with the specified instant of "
				"geological time and anchored plate id.\n"
				"\n"
				"  :param reconstruction_time: time at which to create a reconstruction tree (in Ma)\n"
				"  :type reconstruction_time: float\n"
				"  :param anchor_plate_id: the id of the anchored plate that *equivalent* rotations "
				"are calculated with respect to\n"
				"  :type anchor_plate_id: int\n"
				"  :rtype: :class:`ReconstructionTree`\n"
				"\n"
				"  If the reconstruction tree for the specified reconstruction time and anchored plate id "
				"is currently in the internal cache then it is returned, otherwise a new reconstruction "
				"tree is created and stored in the cache (after evicting the reconstruction tree associated "
				"with the least recently requested reconstruction time and anchored plate id if necessary).\n")
	;

	// Enable boost::optional<RotationModel::non_null_ptr_type> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesApi::RotationModel::non_null_ptr_type>();

	// Registers 'non-const' to 'const' conversions.
	boost::python::implicitly_convertible<
			GPlatesApi::RotationModel::non_null_ptr_type,
			GPlatesApi::RotationModel::non_null_ptr_to_const_type>();
	boost::python::implicitly_convertible<
			boost::optional<GPlatesApi::RotationModel::non_null_ptr_type>,
			boost::optional<GPlatesApi::RotationModel::non_null_ptr_to_const_type> >();

	// Register converter from a rotation model or a sequence of feature collections to a @a RotationModelFunctionArgument.
	GPlatesApi::register_rotation_model_function_argument_conversion();
}

#endif // GPLATES_NO_PYTHON
