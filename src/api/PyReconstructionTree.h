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

#ifndef GPLATES_API_PYRECONSTRUCTIONTREE_H
#define GPLATES_API_PYRECONSTRUCTIONTREE_H

#include <vector>
#include <boost/optional.hpp>

#include "app-logic/ReconstructionTree.h"
#include "app-logic/ReconstructionTreeCreator.h"

#include "global/PreconditionViolationError.h"
#include "global/python.h"

#include "maths/FiniteRotation.h"

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"

#include "utils/ReferenceCount.h"


#if !defined(GPLATES_NO_PYTHON)

namespace GPlatesApi
{
	/**
	 * The anchored plates of two reconstruction trees are not the same.
	 */
	class DifferentAnchoredPlatesInReconstructionTreesException :
			public GPlatesGlobal::PreconditionViolationError
	{
	public:
		explicit
		DifferentAnchoredPlatesInReconstructionTreesException(
				const GPlatesUtils::CallStack::Trace &exception_source) :
			GPlatesGlobal::PreconditionViolationError(exception_source)
		{  }

		~DifferentAnchoredPlatesInReconstructionTreesException() throw()
		{  }

	protected:

		virtual
		const char *
		exception_name() const
		{
			return "DifferentAnchoredPlatesInReconstructionTreesException";
		}

	};


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


		static
		non_null_ptr_type
		create_from_feature_collections(
				boost::python::object feature_collection_seq, // Any python iterable (eg, list, tuple).
				unsigned int reconstruction_tree_cache_size);


		static
		non_null_ptr_type
		create_from_files(
				boost::python::object filename_seq, // Any python iterable (eg, list, tuple).
				unsigned int reconstruction_tree_cache_size);


		GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type
		get_reconstruction_tree(
				const double &reconstruction_time);


		/**
		 * Handle the four combinations of total/stage and equivalent/relative rotations in one place.
		 */
		boost::optional<GPlatesMaths::FiniteRotation>
		get_rotation(
				const double &to_time,
				GPlatesModel::integer_plate_id_type moving_plate_id,
				const double &from_time,
				GPlatesModel::integer_plate_id_type fixed_plate_id);

	private:

		// Common create method.
		static
		non_null_ptr_type
		create(
				const std::vector<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> &feature_collections,
				unsigned int reconstruction_tree_cache_size);

		RotationModel(
				const std::vector<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> &feature_collections,
				const GPlatesAppLogic::ReconstructionTreeCreator &reconstruction_tree_creator) :
			d_feature_collections(feature_collections),
			d_reconstruction_tree_creator(reconstruction_tree_creator)
		{  }


		/**
		 * Keep the feature collections alive (by using intrusive pointers instead of weak refs)
		 * since @a ReconstructionTreeCreator only stores weak references.
		 */
		std::vector<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> d_feature_collections;

		/**
		 * Cached reconstruction tree creator.
		 */
		GPlatesAppLogic::ReconstructionTreeCreator d_reconstruction_tree_creator;

	};
}

#endif   // GPLATES_NO_PYTHON

#endif // GPLATES_API_PYRECONSTRUCTIONTREE_H
