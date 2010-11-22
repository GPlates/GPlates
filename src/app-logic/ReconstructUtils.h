/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
 * Copyright (C) 2010 Geological Survey of Norway
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

#include "Reconstruction.h"
#include "ReconstructionGeometryCollection.h"
#include "ReconstructionTree.h"

#include "maths/FiniteRotation.h"
#include "maths/GeometryOnSphere.h"

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"


namespace GPlatesAppLogic
{
	namespace ReconstructUtils
	{
		/**
		 * Returns true if @a feature_ref is a reconstruction feature.
		 *
		 * This is total reconstruction sequences and absolute reference frames.
		 */
		bool
		is_reconstruction_feature(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref);


		/**
		 * Returns true if @a feature_collection contains any features that pass the
		 * @a is_reconstruction_feature test.
		 */
		bool
		has_reconstruction_features(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection);


		/**
		 * Create and return a reconstruction tree for the reconstruction time @a time,
		 * with root @a root.
		 *
		 * The feature collections in @a reconstruction_features_collection are expected to
		 * contain reconstruction features (ie, total reconstruction sequences and absolute
		 * reference frames).
		 *
		 * If @a reconstruction_features_collection is empty then the returned @a ReconstructionTree
		 * will always give an identity rotation when queried for a composed absolute rotation.
		 *
		 * Question:  Do any of those other functions actually throw exceptions when
		 * they're passed invalid weak_refs?  They should.
		 */
		const ReconstructionTree::non_null_ptr_type
		create_reconstruction_tree(
				const double &time,
				GPlatesModel::integer_plate_id_type root,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
						reconstruction_features_collection =
								std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>());

#if 0
		/**
		 * Same as the above overload of @a create_reconstruction_tree except that it accepts
		 * a sequence of feature handles instead of a sequence of feature collections.
		 */
		const ReconstructionTree::non_null_ptr_type
		create_reconstruction_tree(
				const double &time,
				GPlatesModel::integer_plate_id_type root,
				const std::vector<GPlatesModel::FeatureHandle::weak_ref> &
						reconstruction_features);
#endif


		/**
		 * Returns true if @a feature_ref is reconstructable.
		 *
		 * This is any feature that can generate a @a ReconstructedFeatureGeometry when
		 * @a reconstruct processes it.
		 */
		bool
		is_reconstructable_feature(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref);


		/**
		 * Returns true if @a feature_collection contains any features that pass the
		 * @a is_reconstructable_feature test.
		 */
		bool
		has_reconstructable_features(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection);


		/**
		 * Create and return a reconstruction geometry collection, containing
		 * @a ReconstructionGeometry objects, by rotating feature geometries in
		 * @a reconstructable_features_collection using @a reconstruction_tree.
		 *
		 * Only features that exist at the reconstruction time of @a reconstruction_tree
		 * are rotated and generate reconstruction geometries.
		 *
		 * If @a reconstructable_features_collection is empty then the returned
		 * @a ReconstructionGeometryCollection will contain no @a ReconstructionGeometry objects.
		 *
		 * Question:  Do any of those other functions actually throw exceptions when
		 * they're passed invalid weak_refs?  They should.
		 */
		ReconstructionGeometryCollection::non_null_ptr_type
		reconstruct(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
						reconstructable_features_collection =
								std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>());


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
				const ReconstructionTree &reconstruction_tree,
				bool reverse_reconstruct = false);

		/**
		 * Reconstructs a present day @a geometry using @a reconstruction_tree
		 * that rotates from present day to the reconstruction time for which
		 * @a reconstruction_tree was generated, using the half-stage rotation 
		 * reconstruction method.
		 * 
		 * If @a reverse_reconstruct is true then @a geometry is assumed to be
		 * at a non-present-day reconstruction time (the time at which
		 * @a reconstruction_tree was generated to rotate to) and @a geometry
		 * is then reverse rotated back to present day.
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		reconstruct_as_half_stage(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
				const GPlatesModel::integer_plate_id_type left_plate_id,
				const GPlatesModel::integer_plate_id_type right_plate_id,
				const ReconstructionTree &reconstruction_tree,
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
				const ReconstructionTree &reconstruction_tree,
				bool reverse_reconstruct = false);

		/**
		 * This is the same as the other overload of @a reconstruct (for @a GeometryOnSphere)
		 * but this works with any type supported by GPlatesMaths::FiniteRotation as in:
		 *    operator*(GPlatesMaths::FiniteRotation, GeometryType)
		 */
		template <class GeometryType>
		GeometryType
		reconstruct_as_half_stage(
				const GeometryType &geometry,
				GPlatesModel::integer_plate_id_type left_plate_id,
				GPlatesModel::integer_plate_id_type right_plate_id,
				const ReconstructionTree &reconstruction_tree,
				bool reverse_reconstruct = false);


		boost::optional<GPlatesMaths::FiniteRotation>
		get_half_stage_rotation(
				const ReconstructionTree &reconstruction_tree,
				GPlatesModel::integer_plate_id_type left_plate_id,
				GPlatesModel::integer_plate_id_type right_plate_id
				);

		/**
		 * Returns the stage-pole for @a moving_plate_id wrt @a fixed_plate_id, between
		 * the times represented by @a reconstruction_tree_ptr_1 and 
		 * @a reconstruction_tree_ptr_2
		 */
		GPlatesMaths::FiniteRotation
			get_stage_pole(
			const GPlatesAppLogic::ReconstructionTree &reconstruction_tree_ptr_1, 
			const GPlatesAppLogic::ReconstructionTree &reconstruction_tree_ptr_2, 
			const GPlatesModel::integer_plate_id_type &moving_plate_id, 
			const GPlatesModel::integer_plate_id_type &fixed_plate_id);	
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
				const ReconstructionTree &reconstruction_tree,
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

		template <class GeometryType>
		GeometryType
		reconstruct_as_half_stage(
			const GeometryType &geometry,
			GPlatesModel::integer_plate_id_type left_plate_id,
			GPlatesModel::integer_plate_id_type right_plate_id,
			const ReconstructionTree &reconstruction_tree,
			bool reverse_reconstruct)
		{
			// Get the composed absolute rotation needed to bring a thing on that plate
			// in the present day to this time.
			boost::optional<GPlatesMaths::FiniteRotation> rotation =
				get_half_stage_rotation(reconstruction_tree,left_plate_id,right_plate_id);

			if (!rotation)
			{
				return geometry;
			}

			// Are we reversing reconstruction back to present day ?
			if (reverse_reconstruct)
			{
				*rotation = GPlatesMaths::get_reverse(*rotation);
			}

			// Apply the rotation.
			return (*rotation) * geometry;
		}

	}

}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTUTILS_H
