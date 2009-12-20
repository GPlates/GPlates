/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTUTILS_H
#define GPLATES_APP_LOGIC_RECONSTRUCTUTILS_H

#include <utility>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include "maths/FiniteRotation.h"
#include "maths/GeometryOnSphere.h"

#include "model/FeatureCollectionHandle.h"
#include "model/Reconstruction.h"
#include "model/ReconstructionTree.h"
#include "model/types.h"


namespace GPlatesAppLogic
{
	namespace ReconstructUtils
	{
		/**
		 * Create and return a reconstruction tree for the reconstruction time @a time,
		 * with root @a root.
		 *
		 * The feature collections in @a reconstruction_features_collection are expected to
		 * contain reconstruction features (ie, total reconstruction sequences and absolute
		 * reference frames).
		 *
		 * Question:  Do any of those other functions actually throw exceptions when
		 * they're passed invalid weak_refs?  They should.
		 */
		const GPlatesModel::ReconstructionTree::non_null_ptr_type
		create_reconstruction_tree(
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
						reconstruction_features_collection,
				const double &time,
				GPlatesModel::integer_plate_id_type root);


		/**
		 * Create and return a reconstruction for the reconstruction time @a time, with
		 * root @a root.
		 *
		 * The feature collections in @a reconstruction_features_collection are expected to
		 * contain reconstruction features (ie, total reconstruction sequences and absolute
		 * reference frames).
		 *
		 * Question:  Do any of those other functions actually throw exceptions when
		 * they're passed invalid weak_refs?  They should.
		 */
		const GPlatesModel::Reconstruction::non_null_ptr_type
		create_reconstruction(
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
						reconstructable_features_collection,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
						reconstruction_features_collection,
				const double &time,
				GPlatesModel::integer_plate_id_type root);


		/**
		 * Create and return an empty reconstruction for the reconstruction time @a time,
		 * with root @a root.
		 *
		 * The reconstruction tree contained within the reconstruction will also be empty.
		 *
		 * FIXME:  Remove this function once it is possible to create empty reconstructions
		 * by simply passing empty lists of feature-collections into the prev function.
		 */
		const GPlatesModel::Reconstruction::non_null_ptr_type
		create_empty_reconstruction(
				const double &time,
				GPlatesModel::integer_plate_id_type root);


		/**
		 * Reconstructs a present day @a geometry using @a reconstruction_tree
		 * that rotates from present day to the reconstruction time for which
		 * @a reconstruction_tree was generated.
		 * 
		 * If @a reverse_reconstruct is true then @a geometry is assumed to be
		 * at a non-present-day reconstruction time (the time at which
		 * @a reconstruction_tree was generated to rotate to) and @a geometry
		 * is then reverse rotated back to present day.
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		reconstruct(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
				const GPlatesModel::integer_plate_id_type reconstruction_plate_id,
				const GPlatesModel::ReconstructionTree &reconstruction_tree,
				bool reverse_reconstruct = false);


		/**
		 * This is the same as the other overload of @a reconstruct (for @a GeometryOnSphere)
		 * but this works with any type supported by GPlatesMaths::FiniteRotation as in:
		 *    operator*(GPlatesMaths::FiniteRotation, GeometryType)
		 */
		template <class GeometryType>
		GeometryType
		reconstruct(
				const GeometryType &geometry,
				GPlatesModel::integer_plate_id_type reconstruction_plate_id,
				const GPlatesModel::ReconstructionTree &reconstruction_tree,
				bool reverse_reconstruct = false);
	}


	//
	// Implementation
	//
	namespace ReconstructUtils
	{
		template <class GeometryType>
		GeometryType
		reconstruct(
				const GeometryType &geometry,
				GPlatesModel::integer_plate_id_type reconstruction_plate_id,
				const GPlatesModel::ReconstructionTree &reconstruction_tree,
				bool reverse_reconstruct)
		{
			// Get the composed absolute rotation needed to bring a thing on that plate
			// in the present day to this time.
			GPlatesMaths::FiniteRotation rotation =
					reconstruction_tree.get_composed_absolute_rotation(
							reconstruction_plate_id).first;

			// Are we reversing reconstruction back to present day ?
			if (reverse_reconstruct)
			{
				rotation = GPlatesMaths::get_reverse(rotation);
			}
			
			// Apply the rotation.
			return rotation * geometry;
		}
	}
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTUTILS_H
