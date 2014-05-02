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

#ifndef GPLATES_API_PYROTATIONMODEL_H
#define GPLATES_API_PYROTATIONMODEL_H

#include <vector>
#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include "PyFeatureCollection.h"

#include "app-logic/ReconstructionTreeCreator.h"

#include "file-io/File.h"

#include "global/python.h"

#include "maths/FiniteRotation.h"

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"

#include "utils/ReferenceCount.h"


#if !defined(GPLATES_NO_PYTHON)

namespace GPlatesApi
{
	/**
	 * Creates reconstruction trees and provides easy way to query equivalent/relative,
	 * total/stage rotations.
	 */
	class RotationModel :
			public GPlatesUtils::ReferenceCount<RotationModel>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<RotationModel> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const RotationModel> non_null_ptr_to_const_type;


		static const unsigned int DEFAULT_RECONSTRUCTION_TREE_CACHE_SIZE;


		/**
		 * Create a rotation model (from rotations features) that will cache reconstruction trees
		 * up to a cache size of @a reconstruction_tree_cache_size.
		 *
		 * @a rotation_features must satisfy FeatureCollectionSequenceFunctionArgument::is_convertible().
		 *
		 * Alternatively you can just use boost::python::extract<RotationModel::non_null_ptr_type>()
		 * on any python object satisfying FeatureCollectionSequenceFunctionArgument::is_convertible().
		 */
		static
		non_null_ptr_type
		create(
				boost::python::object rotation_features,
				unsigned int reconstruction_tree_cache_size = DEFAULT_RECONSTRUCTION_TREE_CACHE_SIZE);


		/**
		 * Create a rotation model (from a sequence of rotation feature collection files) that will cache
		 * reconstruction trees up to a cache size of @a reconstruction_tree_cache_size.
		 */
		static
		non_null_ptr_type
		create(
				const std::vector<GPlatesFileIO::File::non_null_ptr_type> &rotation_features,
				unsigned int reconstruction_tree_cache_size = DEFAULT_RECONSTRUCTION_TREE_CACHE_SIZE);


		/**
		 * Create a rotation model (from a sequence of rotation feature collections) that will cache
		 * reconstruction trees up to a cache size of @a reconstruction_tree_cache_size.
		 */
		static
		non_null_ptr_type
		create(
				const std::vector<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> &rotation_features,
				unsigned int reconstruction_tree_cache_size = DEFAULT_RECONSTRUCTION_TREE_CACHE_SIZE);
	

		GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type
		get_reconstruction_tree(
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type anchor_plate_id);


		/**
		 * Handle the four combinations of total/stage and equivalent/relative rotations in one place.
		 *
		 * If @a fixed_plate_id is none then it is set to @a anchor_plate_id.
		 */
		boost::optional<GPlatesMaths::FiniteRotation>
		get_rotation(
				const double &to_time,
				GPlatesModel::integer_plate_id_type moving_plate_id,
				const double &from_time,
				boost::optional<GPlatesModel::integer_plate_id_type> fixed_plate_id,
				GPlatesModel::integer_plate_id_type anchor_plate_id,
				bool use_identity_for_missing_plate_ids);


		/**
		 * Returns reconstruction tree creator.
		 *
		 * NOTE: Not accessible from python - only used in C++ when RotationModel passed from python.
		 */
		GPlatesAppLogic::ReconstructionTreeCreator
		get_reconstruction_tree_creator() const
		{
			return d_reconstruction_tree_creator;
		}


		/**
		 * Return the feature collections used to create this rotation model.
		 */
		void
		get_feature_collections(
				std::vector<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> &feature_collections) const;


		/**
		 * Return the feature collection files used to create this rotation model.
		 *
		 * NOTE: Any feature collections that did not come from files will have empty filenames.
		 */
		void
		get_files(
				std::vector<GPlatesFileIO::File::non_null_ptr_type> &feature_collection_files) const;

	private:

		RotationModel(
				const std::vector<GPlatesFileIO::File::non_null_ptr_type> &feature_collection_files,
				const GPlatesAppLogic::ReconstructionTreeCreator &reconstruction_tree_creator) :
			d_feature_collection_files(feature_collection_files),
			d_reconstruction_tree_creator(reconstruction_tree_creator)
		{  }


		/**
		 * Keep the feature collections alive (by using intrusive pointers instead of weak refs)
		 * since @a ReconstructionTreeCreator only stores weak references.
		 *
		 * We now also keep track of the files the feature collections came from.
		 * If any feature collection did not come from a file then it will have an empty filename.
		 */
		std::vector<GPlatesFileIO::File::non_null_ptr_type> d_feature_collection_files;

		/**
		 * Cached reconstruction tree creator.
		 */
		GPlatesAppLogic::ReconstructionTreeCreator d_reconstruction_tree_creator;

	};


	/**
	 * A convenience class for receiving a rotation model function argument as either:
	 *  (1) a rotation model, or
	 *  (2) a @a FeatureCollectionSequenceFunctionArgument.
	 *
	 * To get an instance of @a RotationModelFunctionArgument you can either:
	 *  (1) specify RotationModelFunctionArgument directly as a function argument type
	 *      (in the C++ function being wrapped), or
	 *  (2) use boost::python::extract<RotationModelFunctionArgument>().
	 */
	class RotationModelFunctionArgument
	{
	public:

		/**
		 * Types of function argument.
		 */
		typedef boost::variant<
				RotationModel::non_null_ptr_type,
				FeatureCollectionSequenceFunctionArgument> function_argument_type;


		/**
		 * Returns true if @a python_function_argument is convertible to an instance of this class.
		 */
		static
		bool
		is_convertible(
				boost::python::object python_function_argument);


		explicit
		RotationModelFunctionArgument(
				boost::python::object python_function_argument);


		explicit
		RotationModelFunctionArgument(
				const function_argument_type &function_argument);

		/**
		 * Returns function argument as a rotation model (for passing to python).
		 */
		boost::python::object
		to_python() const;

		/**
		 * Return the function argument as a rotation model.
		 */
		RotationModel::non_null_ptr_type
		get_rotation_model() const;

	private:

		static
		RotationModel::non_null_ptr_type
		initialise_rotation_model(
				const function_argument_type &function_argument);


		RotationModel::non_null_ptr_type d_rotation_model;
	};
}

#endif   // GPLATES_NO_PYTHON

#endif // GPLATES_API_PYROTATIONMODEL_H
