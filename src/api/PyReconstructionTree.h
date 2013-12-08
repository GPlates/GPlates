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

#include <boost/optional.hpp>

#include "app-logic/ReconstructionTree.h"

#include "maths/FiniteRotation.h"

#include "model/types.h"

#include "global/PreconditionViolationError.h"


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
	 * Get equivalent total rotation.
	 *
	 * If @a use_identity_for_missing_plate_ids is true then returns identity rotation if
	 * @a moving_plate_id is not found in the reconstruction tree.
	 * When @a use_identity_for_missing_plate_ids is true then a finite rotation is always returned.
	 *
	 * This interface is exposed so other API functions can use it in their implementation.
	 */
	boost::optional<GPlatesMaths::FiniteRotation>
	get_equivalent_total_rotation(
			const GPlatesAppLogic::ReconstructionTree &reconstruction_tree,
			GPlatesModel::integer_plate_id_type moving_plate_id,
			bool use_identity_for_missing_plate_ids);

	/**
	 * Get relative total rotation.
	 *
	 * If @a use_identity_for_missing_plate_ids is true then uses identity rotation for those plate ids
	 * not found in the reconstruction tree.
	 * When @a use_identity_for_missing_plate_ids is true then a finite rotation is always returned.
	 *
	 * This interface is exposed so other API functions can use it in their implementation.
	 */
	boost::optional<GPlatesMaths::FiniteRotation>
	get_relative_total_rotation(
			const GPlatesAppLogic::ReconstructionTree &reconstruction_tree,
			GPlatesModel::integer_plate_id_type fixed_plate_id,
			GPlatesModel::integer_plate_id_type moving_plate_id,
			bool use_identity_for_missing_plate_ids);

	/**
	 * Get equivalent stage rotation.
	 *
	 * If @a use_identity_for_missing_plate_ids is true then uses identity rotation for those plate ids
	 * not found in the reconstruction trees.
	 * When @a use_identity_for_missing_plate_ids is true then a finite rotation is always returned.
	 *
	 * This interface is exposed so other API functions can use it in their implementation.
	 */
	boost::optional<GPlatesMaths::FiniteRotation>
	get_equivalent_stage_rotation(
			const GPlatesAppLogic::ReconstructionTree &from_reconstruction_tree,
			const GPlatesAppLogic::ReconstructionTree &to_reconstruction_tree,
			GPlatesModel::integer_plate_id_type plate_id,
			bool use_identity_for_missing_plate_ids);

	/**
	 * Get relative stage rotation.
	 *
	 * If @a use_identity_for_missing_plate_ids is true then uses identity rotation for those plate ids
	 * not found in the reconstruction trees.
	 * When @a use_identity_for_missing_plate_ids is true then a finite rotation is always returned.
	 *
	 * This interface is exposed so other API functions can use it in their implementation.
	 */
	boost::optional<GPlatesMaths::FiniteRotation>
	get_relative_stage_rotation(
			const GPlatesAppLogic::ReconstructionTree &from_reconstruction_tree,
			const GPlatesAppLogic::ReconstructionTree &to_reconstruction_tree,
			GPlatesModel::integer_plate_id_type fixed_plate_id,
			GPlatesModel::integer_plate_id_type moving_plate_id,
			bool use_identity_for_missing_plate_ids);
}

#endif   // GPLATES_NO_PYTHON

#endif // GPLATES_API_PYRECONSTRUCTIONTREE_H
