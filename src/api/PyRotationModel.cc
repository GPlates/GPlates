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
#include <vector>
#include <boost/cast.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "PyRotationModel.h"

#include "PyFeatureCollectionFileFormatRegistry.h"
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


GPlatesApi::RotationModel::non_null_ptr_type
GPlatesApi::RotationModel::create_from_feature_collections(
		bp::object feature_collection_seq, // Any python iterable (eg, list, tuple).
		unsigned int reconstruction_tree_cache_size)
{
	// Begin/end iterators over the python feature collections iterable.
	bp::stl_input_iterator<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type>
			feature_collection_seq_iter(feature_collection_seq),
			feature_collection_seq_end;

	// Copy feature collections into a vector.
	std::vector<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> feature_collections;
	for ( ; feature_collection_seq_iter != feature_collection_seq_end; ++feature_collection_seq_iter)
	{
		feature_collections.push_back(*feature_collection_seq_iter);
	}

	return create(feature_collections, reconstruction_tree_cache_size);
}


GPlatesApi::RotationModel::non_null_ptr_type
GPlatesApi::RotationModel::create_from_files(
		bp::object filename_seq, // Any python iterable (eg, list, tuple).
		unsigned int reconstruction_tree_cache_size)
{
	GPlatesFileIO::FeatureCollectionFileFormat::Registry file_registry;

	// Begin/end iterators over the python filenames iterable.
	bp::stl_input_iterator<QString> filename_seq_iter(filename_seq);
	bp::stl_input_iterator<QString> filename_seq_end;

	// Read the feature collections from the files.
	std::vector<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> feature_collections;
	for ( ; filename_seq_iter != filename_seq_end; ++filename_seq_iter)
	{
		const QString filename = *filename_seq_iter;

		// Use the API function in "PyFeatureCollectionFileFormatRegistry.h" to read the file.
		const GPlatesModel::FeatureCollectionHandle::non_null_ptr_type feature_collection =
				GPlatesApi::read_feature_collection(file_registry, filename);

		feature_collections.push_back(feature_collection);
	}

	return create(feature_collections, reconstruction_tree_cache_size);
}


GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type
GPlatesApi::RotationModel::get_reconstruction_tree(
		const double &reconstruction_time)
{
	return d_reconstruction_tree_creator.get_reconstruction_tree(reconstruction_time);
}


boost::optional<GPlatesMaths::FiniteRotation>
GPlatesApi::RotationModel::get_rotation(
		const double &to_time,
		GPlatesModel::integer_plate_id_type moving_plate_id,
		const double &from_time,
		GPlatesModel::integer_plate_id_type fixed_plate_id)
{
	GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type to_reconstruction_tree =
			get_reconstruction_tree(to_time);

	if (GPlatesMaths::are_almost_exactly_equal(from_time, 0))
	{
		if (fixed_plate_id == 0)
		{
			// Use the API function in "PyReconstructionTree.h".
			return GPlatesApi::get_equivalent_total_rotation(
					*to_reconstruction_tree,
					moving_plate_id);
		}

		// Use the API function in "PyReconstructionTree.h".
		return GPlatesApi::get_relative_total_rotation(
				*to_reconstruction_tree,
				fixed_plate_id,
				moving_plate_id);
	}

	const GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type from_reconstruction_tree =
			get_reconstruction_tree(from_time);

	if (fixed_plate_id == 0)
	{
		// Use the API function in "PyReconstructionTree.h".
		return GPlatesApi::get_equivalent_stage_rotation(
				*from_reconstruction_tree,
				*to_reconstruction_tree,
				moving_plate_id);
	}

	// Use the API function in "PyReconstructionTree.h".
	return GPlatesApi::get_relative_stage_rotation(
			*from_reconstruction_tree,
			*to_reconstruction_tree,
			fixed_plate_id,
			moving_plate_id);
}


GPlatesApi::RotationModel::non_null_ptr_type
GPlatesApi::RotationModel::create(
		const std::vector<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> &feature_collections,
		unsigned int reconstruction_tree_cache_size)
{
	// Convert the feature collections to weak refs.
	std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> feature_collection_refs;
	std::vector<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type>::const_iterator
			feature_collections_iter = feature_collections.begin(),
			feature_collections_end = feature_collections.end();
	for ( ; feature_collections_iter != feature_collections_end; ++feature_collections_iter)
	{
		GPlatesModel::FeatureCollectionHandle::non_null_ptr_type feature_collection =
				*feature_collections_iter;

		feature_collection_refs.push_back(feature_collection->reference());
	}

	// Create a cached reconstruction tree creator.
	const GPlatesAppLogic::ReconstructionTreeCreator reconstruction_tree_creator =
			GPlatesAppLogic::create_cached_reconstruction_tree_creator(
					feature_collection_refs,
					0/*default_anchor_plate_id*/,
					reconstruction_tree_cache_size);

	return non_null_ptr_type(new RotationModel(feature_collections, reconstruction_tree_creator));
}


void
export_rotation_model()
{
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
					"cache size parameter pass to :meth:`__init__` or :meth:`create_from_files`. "
					"The *reconstruction_tree_cache_size* parameter of those "
					"methods controls the size of an internal least-recently-used cache of reconstruction "
					"trees (evicts least recently requested reconstruction tree when a new reconstruction "
					"time is requested that does not currently exist in the cache). This enables "
					"reconstruction trees associated with different reconstruction times to be re-used "
					"instead of re-creating them, provided they have not been evicted from the cache. "
					"This benefit also applies when querying rotations with :meth:`get_rotation` since "
					"it, in turn, requests reconstruction trees. The default cache size is one.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::RotationModel::create_from_feature_collections,
						bp::default_call_policies(),
						(bp::arg("feature_collections"),
							bp::arg("reconstruction_tree_cache_size") = 1)),
				"__init__(feature_collections[, reconstruction_tree_cache_size=1])\n"
				"  Create from a sequence of rotation feature collections and an optional "
				"reconstruction tree cache size.\n"
				"\n"
				"  :param feature_collections: A sequence of :class:`FeatureCollection` instances\n"
				"  :type feature_collections: Any sequence such as a ``list`` or a ``tuple``\n"
				"  :param reconstruction_tree_cache_size: number of reconstruction trees to cache internally\n"
				"  :type reconstruction_tree_cache_size: int\n"
				"  :rtype: :class:`RotationModel`\n"
				"\n"
				"  ::\n"
				"\n"
				"    rotation_model = pygplates.RotationModel(feature_collections)\n")
		.def("create_from_files",
				&GPlatesApi::RotationModel::create_from_files,
				(bp::arg("filenames"), bp::arg("reconstruction_tree_cache_size") = 1),
				"create_from_files(filenames[, reconstruction_tree_cache_size=1]) -> RotationModel\n"
				"  Create from a sequence of rotation files and an optional reconstruction tree cache size.\n"
				"\n"
				"  :param filenames: A sequence of filename strings of the rotation files\n"
				"  :type filenames: Any sequence such as a ``list`` or a ``tuple``\n"
				"  :param reconstruction_tree_cache_size: number of reconstruction trees to cache internally\n"
				"  :type reconstruction_tree_cache_size: int\n"
				"  :rtype: :class:`RotationModel`\n"
				"  :raises: OpenFileForReadingError if any file is not readable\n"
				"  :raises: FileFormatNotSupportedError if any file format (identified by the filename "
				"extensions) does not support reading\n"
				"\n"
				"  Internally this method uses :class:`FeatureCollectionFileFormatRegistry` to read "
				"the rotation files.\n"
				"\n"
				"  ::\n"
				"\n"
				"    rotation_model = pygplates.RotationModel.create_from_files(filenames)\n")
		.staticmethod("create_from_files")
		.def("get_rotation",
				&GPlatesApi::RotationModel::get_rotation,
				(bp::arg("to_time"),
					bp::arg("moving_plate_id"),
					bp::arg("from_time") = 0,
					bp::arg("fixed_plate_id") = 0),
				"get_rotation(to_time, moving_plate_id[, from_time=0[, fixed_plate_id=0]]) -> FiniteRotation or None\n"
				"  Return the finite rotation that rotates from the *fixed_plate_id* plate to the *moving_plate_id* "
				"plate and from the time *from_time* to the time *to_time*.\n"
				"\n"
				"  Returns ``None`` if *fixed_plate_id* and *moving_plate_id* do not exist at both "
				"reconstruction times (do not exist in the reconstruction trees associated with those times).\n"
				"\n"
				"  :param to_time: time at which the moving plate is being rotated *to* (in Ma)\n"
				"  :type to_time: float\n"
				"  :param moving_plate_id: the plate id of the moving plate\n"
				"  :type moving_plate_id: int\n"
				"  :param from_time: time at which the moving plate is being rotated *from* (in Ma)\n"
				"  :type from_time: float\n"
				"  :param fixed_plate_id: the plate id of the fixed plate\n"
				"  :type fixed_plate_id: int\n"
				"  :rtype: FiniteRotation or None\n"
				"\n"
				"  This method conveniently handles all four combinations of total/stage and "
				"equivalent/relative rotations (see :class:`ReconstructionTree` for details) normally handled by "
				":meth:`ReconstructionTree.get_equivalent_total_rotation`, "
				":meth:`ReconstructionTree.get_relative_total_rotation`, "
				":func:`get_equivalent_stage_rotation` and "
				":func:`get_relative_stage_rotation`.\n"
				"\n"
				"  This method essentially does the following:\n"
				"  ::\n"
				"\n"
				"    def get_rotation(self, to_time, moving_plate_id[, from_time=0[, fixed_plate_id=0]]):\n"
				"        \n"
				"        if from_time == 0:\n"
				"            if fixed_plate_id == 0:\n"
				"                return self.get_reconstruction_tree(to_time).get_equivalent_total_rotation(moving_plate_id)\n"
				"            \n"
				"            return self.get_reconstruction_tree(to_time).get_relative_total_rotation(fixed_plate_id, moving_plate_id)\n"
				"        \n"
				"        if fixed_plate_id == 0:\n"
				"            return pygplates.get_equivalent_stage_rotation(\n"
				"                self.get_reconstruction_tree(from_time),\n"
				"                self.get_reconstruction_tree(to_time),\n"
				"                moving_plate_id)\n"
				"        \n"
				"        return pygplates.get_relative_stage_rotation(\n"
				"            self.get_reconstruction_tree(from_time),\n"
				"            self.get_reconstruction_tree(to_time),\n"
				"            fixed_plate_id,\n"
				"            moving_plate_id)\n")
		.def("get_reconstruction_tree",
				&GPlatesApi::RotationModel::get_reconstruction_tree,
				(bp::arg("reconstruction_time")),
				"get_reconstruction_tree(reconstruction_time) -> ReconstructionTree\n"
				"  Return the reconstruction tree associated with the specified instant of geological time.\n"
				"\n"
				"  :param reconstruction_time: time at which to create a reconstruction tree (in Ma)\n"
				"  :type reconstruction_time: float\n"
				"  :rtype: :class:`ReconstructionTree`\n"
				"\n"
				"  If the reconstruction tree for the specified reconstruction time is currently in "
				"the internal cache then it is returned, otherwise a new reconstruction tree is created "
				"and stored in the cache (after evicting the reconstruction tree associated with the "
				"least recently requested reconstruction time if necessary).\n")
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
}

#endif // GPLATES_NO_PYTHON
