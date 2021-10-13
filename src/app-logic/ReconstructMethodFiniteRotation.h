/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTMETHODFINITEROTATION_H
#define GPLATES_APP_LOGIC_RECONSTRUCTMETHODFINITEROTATION_H

#include <boost/operators.hpp>
#include <boost/optional.hpp>

#include "ReconstructMethodType.h"

#include "maths/FiniteRotation.h"
#include "maths/GeometryOnSphere.h"
#include "maths/types.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	/**
	 * Base class for representing a finite rotation reconstruction for a particular
	 * 'ReconstructMethod::Type' reconstruct method type.
	 *
	 * It serves two purposes:
	 * 1) To transport a @a FiniteRotation around,
	 * 2) To efficiently compare @a FiniteRotation objects by comparing parameters used to derive
	 *    the finite rotation (such as plate id) instead of comparing the finite rotations directly.
	 *
	 * NOTE: Not all reconstruct methods will necessarily generate a finite rotation.
	 * This call is only for those that do - those that don't can just store the final
	 * reconstructed geometry in a @a ReconstructedFeatureGeometry object.
	 */
	class ReconstructMethodFiniteRotation :
			public GPlatesUtils::ReferenceCount<ReconstructMethodFiniteRotation>,
			public boost::equivalent<ReconstructMethodFiniteRotation>
	{
	public:
		// Convenience typedefs for a shared pointer to a @a ReconstructMethodFiniteRotation.
		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructMethodFiniteRotation> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructMethodFiniteRotation> non_null_ptr_to_const_type;


		virtual
		~ReconstructMethodFiniteRotation()
		{  }


		/**
		 * Returns the finite rotation transform.
		 *
		 * This is currently used for transforming on the graphics hardware (in the globe view).
		 *
		 * NOTE: If two @a ReconstructMethodFiniteRotation objects are equal, as determined by
		 * 'operator==', then they will have the same finite rotation. This is a more efficient
		 * way to compare finite rotations (such as sorting before batching to the graphics hardware).
		 */
		const GPlatesMaths::FiniteRotation &
		get_finite_rotation() const
		{
			return d_finite_rotation;
		}


		/**
		 * Transforms (reconstructs) the specified geometry.
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		transform(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry) const
		{
			return d_finite_rotation * geometry;
		}


		/**
		 * Less than comparison operator used to sort transforms.
		 *
		 * Can be used to compared two objects of different types derived from @a ReconstructionMethodTransform.
		 *
		 * 'operator==()' provided by boost::equivalent.
		 */
		friend
		bool
		operator<(
				const ReconstructMethodFiniteRotation &lhs,
				const ReconstructMethodFiniteRotation &rhs)
		{
			if (lhs.d_reconstruct_method_type < rhs.d_reconstruct_method_type)
			{
				return true;
			}

			if (lhs.d_reconstruct_method_type == rhs.d_reconstruct_method_type)
			{
				// NOTE: We get the derived class to compare parameters instead of comparing 'd_finite_rotation'.
				return lhs.less_than_compare_finite_rotation_parameters(rhs);
			}

			return false;
		}

	protected:
		/**
		 * Constructor instantiated by derived class.
		 */
		ReconstructMethodFiniteRotation(
				ReconstructMethod::Type reconstruct_method_type,
				const GPlatesMaths::FiniteRotation &finite_rotation) :
			d_reconstruct_method_type(reconstruct_method_type),
			d_finite_rotation(finite_rotation)
		{  }

		/**
		 * Derived classes implement this method to compare parameters used to generate
		 * the finite rotation - these parameters are compared instead of the finite rotation
		 * because it is cheaper (eg, comparing a plate id versus comparing each double in
		 * a finite rotation against a small epsilon).
		 *
		 * The derived type of @a rhs can be cast a type representative of @a d_reconstruct_method_type.
		 *
		 * This is because 'operator<' has ensured @a rhs has the same 'ReconstructMethod::Type'
		 * as 'this' object.
		 */
		virtual
		bool
		less_than_compare_finite_rotation_parameters(
				const ReconstructMethodFiniteRotation &rhs) const = 0;

	private:
		ReconstructMethod::Type d_reconstruct_method_type;

		//! The finite rotation - note that it is *not* used in the comparison.
		GPlatesMaths::FiniteRotation d_finite_rotation;
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTMETHODFINITEROTATION_H
