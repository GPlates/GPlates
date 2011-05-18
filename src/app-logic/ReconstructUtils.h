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

#include <list>
#include <map>
#include <utility>
#include <vector>
#include <QDebug>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include "AppLogicFwd.h"
#include "ReconstructParams.h"
#include "ReconstructionTree.h"
#include "ReconstructionTreeCreator.h"
#include "ReconstructMethodRegistry.h"

#include "maths/FiniteRotation.h"
#include "maths/GeometryOnSphere.h"
#include "maths/types.h"

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"

#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	namespace ReconstructUtils
	{

        void
        display_rotation(
            const GPlatesMaths::FiniteRotation &rotation);


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
		 * Returns true if @a feature_ref is reconstructable.
		 *
		 * This is any feature that can generate a @a ReconstructedFeatureGeometry when
		 * @a reconstruct processes it.
		 *
		 * @a reconstruct_method_registry used to determined if the feature is reconstructable.
		 */
		bool
		is_reconstructable_feature(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref,
				const ReconstructMethodRegistry &reconstruct_method_registry);


		/**
		 * Same as other overload of @a is_reconstructable_feature but creates
		 * a temporary @a ReconstructMethodRegistry object internally.
		 */
		inline
		bool
		is_reconstructable_feature(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref)
		{
			ReconstructMethodRegistry reconstruct_method_registry;
			register_default_reconstruct_method_types(reconstruct_method_registry);

			return is_reconstructable_feature(feature_ref);
		}


		/**
		 * Returns true if @a feature_collection contains any features that pass the
		 * @a is_reconstructable_feature test.
		 *
		 * @a reconstruct_method_registry used to determined if the features are reconstructable.
		 */
		bool
		has_reconstructable_features(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection,
				const ReconstructMethodRegistry &reconstruct_method_registry);


		/**
		 * Same as other overload of @a is_reconstructable_feature but creates
		 * a temporary @a ReconstructMethodRegistry object internally.
		 */
		inline
		bool
		has_reconstructable_features(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
		{
			ReconstructMethodRegistry reconstruct_method_registry;
			register_default_reconstruct_method_types(reconstruct_method_registry);

			return has_reconstructable_features(feature_collection);
		}


		/**
		 * Generate @a ReconstructedFeatureGeometry objects by reconstructing feature geometries in
		 * @a reconstructable_features_collection using reconstruction trees obtained from
		 * @a reconstruction_tree_creator.
		 *
		 * Note that a @a ReconstructionTreeCreator is passed in instead of a reconstruction tree.
		 * This is because some reconstructable features require reconstruction trees at times
		 * other than the specified @a reconstruction_time (eg, flowlines).
		 *
		 * Only features that exist at the reconstruction time @a reconstruction_time
		 * are reconstructed and generate @a ReconstructedFeatureGeometry objects.
		 *
		 * @a reconstruct_method_registry used to determine which reconstruct methods should
		 * be used for which reconstructable features.
		 *
		 * @a reconstruct_params are various parameters used for reconstructing - note that
		 * different reconstruct methods will be interested in differents parameters.
		 */
		void
		reconstruct(
				std::vector<reconstructed_feature_geometry_non_null_ptr_type> &reconstructed_feature_geometries,
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type anchor_plate_id,
				const ReconstructMethodRegistry &reconstruct_method_registry,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstructable_features_collection,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const ReconstructParams &reconstruct_params = ReconstructParams());


		/**
		 * Same as other overload of @a reconstruct but creates temporary
		 * @a ReconstructMethodRegistry and cached reconstruction tree creator objects internally.
		 *
		 * The internally created reconstruction tree cache is used to cache reconstruction trees if the
		 * reconstructable features use reconstruction trees for times other than @a reconstruction_time.
		 *
		 * @a reconstruction_tree_cache_size is used to determine the maximum number of
		 * reconstruction trees to cache if the reconstructable features use reconstruction
		 * trees for reconstruction times other than @a reconstruction_time.
		 */
		inline
		void
		reconstruct(
				std::vector<reconstructed_feature_geometry_non_null_ptr_type> &reconstructed_feature_geometries,
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type anchor_plate_id,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstructable_features_collection,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_features_collection =
						std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>(),
				const ReconstructParams &reconstruct_params = ReconstructParams(),
				unsigned int reconstruction_tree_cache_size = 100)
		{
			ReconstructMethodRegistry reconstruct_method_registry;
			register_default_reconstruct_method_types(reconstruct_method_registry);

			ReconstructionTreeCreator reconstruction_tree_creator =
					get_cached_reconstruction_tree_creator(
							reconstruction_features_collection,
							reconstruction_time,
							anchor_plate_id,
							reconstruction_tree_cache_size);

			reconstruct(
					reconstructed_feature_geometries,
					reconstruction_time,
					anchor_plate_id,
					reconstruct_method_registry,
					reconstructable_features_collection,
					reconstruction_tree_creator,
					reconstruct_params);
		}


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
