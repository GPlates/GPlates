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

#include "ReconstructContext.h"
#include "ReconstructedFeatureGeometry.h"
#include "ReconstructParams.h"
#include "ReconstructionTree.h"
#include "ReconstructionTreeCreator.h"
#include "ReconstructMethodInterface.h"
#include "ReconstructMethodRegistry.h"
#include "RotationUtils.h"

#include "maths/FiniteRotation.h"
#include "maths/GeometryOnSphere.h"
#include "maths/types.h"

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"

#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	/**
	 * Utilities that reconstruct geometry(s) to palaeo times.
	 *
	 * Pure rotation utilities (ie, not dealing with geometries) can go in "RotationUtils.h".
	 */
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

			return is_reconstructable_feature(feature_ref, reconstruct_method_registry);
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

			return has_reconstructable_features(feature_collection, reconstruct_method_registry);
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
		 *
		 * This function will get the next (incremented) global reconstruct handle and store it
		 * in each @a ReconstructedFeatureGeometry instance created (and return the handle).
		 */
		ReconstructHandle::type
		reconstruct(
				std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
				const double &reconstruction_time,
				const ReconstructMethodRegistry &reconstruct_method_registry,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstructable_features_collection,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const ReconstructParams &reconstruct_params = ReconstructParams());

		/**
		 * Same as @a reconstruct overload for @a ReconstructedFeatureGeometry except generates
		 * ReconstructContext::Reconstruction instances instead.
		 */
		ReconstructHandle::type
		reconstruct(
				std::vector<ReconstructContext::Reconstruction> &reconstructions,
				const double &reconstruction_time,
				const ReconstructMethodRegistry &reconstruct_method_registry,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstructable_features_collection,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const ReconstructParams &reconstruct_params = ReconstructParams());

		/**
		 * Same as @a reconstruct overload for @a ReconstructedFeatureGeometry except generates
		 * ReconstructContext::ReconstructedFeature instances instead.
		 */
		ReconstructHandle::type
		reconstruct(
				std::vector<ReconstructContext::ReconstructedFeature> &reconstructed_features,
				const double &reconstruction_time,
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
		 *
		 * This function will get the next (incremented) global reconstruct handle and store it
		 * in each @a ReconstructedFeatureGeometry instance created (and return the handle).
		 */
		ReconstructHandle::type
		reconstruct(
				std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type anchor_plate_id,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstructable_features_collection,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_features_collection =
						std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>(),
				const ReconstructParams &reconstruct_params = ReconstructParams(),
				unsigned int reconstruction_tree_cache_size = 1);

		/**
		 * Same as @a reconstruct overload for @a ReconstructedFeatureGeometry except generates
		 * ReconstructContext::Reconstruction instances instead.
		 */
		ReconstructHandle::type
		reconstruct(
				std::vector<ReconstructContext::Reconstruction> &reconstructions,
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type anchor_plate_id,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstructable_features_collection,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_features_collection =
						std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>(),
				const ReconstructParams &reconstruct_params = ReconstructParams(),
				unsigned int reconstruction_tree_cache_size = 1);

		/**
		 * Same as @a reconstruct overload for @a ReconstructedFeatureGeometry except generates
		 * ReconstructContext::ReconstructedFeature instances instead.
		 */
		ReconstructHandle::type
		reconstruct(
				std::vector<ReconstructContext::ReconstructedFeature> &reconstructed_features,
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type anchor_plate_id,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstructable_features_collection,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_features_collection =
						std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>(),
				const ReconstructParams &reconstruct_params = ReconstructParams(),
				unsigned int reconstruction_tree_cache_size = 1);


		/**
		 * Reconstructs the specified geometry from present day to the specified reconstruction time -
		 * unless @a reverse_reconstruct is true in which case the geometry is assumed to be
		 * the reconstructed geometry (at the reconstruction time) and the returned geometry will
		 * then be the present day geometry.
		 *
		 * NOTE: The specified feature is called @a reconstruction_properties since its geometry(s)
		 * is not reconstructed - it is only used as a source of properties that determine how
		 * to perform the reconstruction (for example, a reconstruction plate ID).
		 *
		 * This is mainly useful when you have a feature and are modifying its geometry at some
		 * reconstruction time (not present day). After each modification the geometry needs to be
		 * reverse reconstructed to present day before it can be attached back onto the feature
		 * because feature's typically store present day geometry in their geometry properties.
		 *
		 * Note that @a reconstruct_method_context contains a @a ReconstructionTreeCreator that can
		 * be used to get reconstruction trees at times other than @a reconstruction_time.
		 * This is useful for reconstructing flowlines since the function might be hooked up
		 * to a reconstruction tree cache.
		 *
		 * Note that @a reconstruct_method_context can also contain deformation information used
		 * to deform the specified geometry.
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		reconstruct_geometry(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
				const ReconstructMethodRegistry &reconstruct_method_registry,
				const GPlatesModel::FeatureHandle::weak_ref &reconstruction_properties,
				const double &reconstruction_time,
				const ReconstructMethodInterface::Context &reconstruct_method_context,
				bool reverse_reconstruct = false);


		/**
		 * Same as other overload of @a reconstruct_geometry but creates a temporary
		 * @a ReconstructMethodInterface::Context internally using @a reconstruction_tree_creator
		 * and @a reconstruct_params.
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		reconstruct_geometry(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
				const ReconstructMethodRegistry &reconstruct_method_registry,
				const GPlatesModel::FeatureHandle::weak_ref &reconstruction_properties,
				const double &reconstruction_time,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const ReconstructParams &reconstruct_params,
				bool reverse_reconstruct = false);


		/**
		 * Same as other overload of @a reconstruct_geometry but creates temporary
		 * @a ReconstructMethodRegistry and cached reconstruction tree creator objects internally.
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		reconstruct_geometry(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
				const GPlatesModel::FeatureHandle::weak_ref &reconstruction_properties,
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type anchor_plate_id,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_features_collection,
				const ReconstructParams &reconstruct_params,
				bool reverse_reconstruct = false,
				unsigned int reconstruction_tree_cache_size = 1);


		/**
		 * Same as other overload of @a reconstruct_geometry but generates a @a ReconstructionTreeCreator
		 * from @a reconstruction_tree (using its reconstruction time, anchor plate ID and reconstruction features).
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		reconstruct_geometry(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
				const GPlatesModel::FeatureHandle::weak_ref &reconstruction_properties,
				const ReconstructionTree &reconstruction_tree,
				const ReconstructParams &reconstruct_params,
				bool reverse_reconstruct = false);


		/**
		 * Reconstructs a present day @a geometry using @a reconstruction_tree
		 * that rotates from present day to the reconstruction time for which
		 * @a reconstruction_tree was generated.
		 * 
		 * @a geometry can be any type supported by GPlatesMaths::FiniteRotation as in:
		 *    operator*(GPlatesMaths::FiniteRotation, GeometryType)
		 *
		 * If @a reverse_reconstruct is true then @a geometry is assumed to be
		 * at a non-present-day reconstruction time (the time at which
		 * @a reconstruction_tree was generated to rotate to) and @a geometry
		 * is then reverse rotated back to present day.
		 */
		template <class GeometryType>
		GeometryType
		reconstruct_by_plate_id(
				const GeometryType &geometry,
				const GPlatesModel::integer_plate_id_type reconstruction_plate_id,
				const ReconstructionTree &reconstruction_tree,
				bool reverse_reconstruct = false)
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


		/**
		 * Reconstruct a present day @a geometry to the specified reconstruction time
		 * using the specified reconstruction properties.
		 * 
		 * @a geometry can be any type supported by GPlatesMaths::FiniteRotation as in:
		 *    operator*(GPlatesMaths::FiniteRotation, GeometryType)
		 *
		 * Also selects appropriate version of half-stage rotation calculation to use:
		 *
		 *   version 1: a single time interval, symmetric spreading that starts at present day.
		 *   version 2: introduced multiple time intervals (10my each) and spreading asymmetry.
		 *   version 3: introduced spreading start time (which is the geometry import time).
		 */
		template <class GeometryType>
		GeometryType
		reconstruct_as_half_stage(
				const GeometryType &geometry,
				const double &reconstruction_time,
				const ReconstructionFeatureProperties &reconstruction_params,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				bool reverse_reconstruct = false)
		{
			// Get the composed absolute rotation needed to bring a thing on that plate
			// in the present day to this time.
			GPlatesMaths::FiniteRotation rotation =
					RotationUtils::get_half_stage_rotation(
							reconstruction_time,
							reconstruction_params,
							reconstruction_tree_creator);

			// Are we reversing reconstruction back to present day ?
			if (reverse_reconstruct)
			{
				rotation = GPlatesMaths::get_reverse(rotation);
			}

			// Apply the rotation.
			return rotation * geometry;
		}

		/**
		 * Reconstructs a present day @a geometry using @a reconstruction_tree
		 * that rotates from present day to the reconstruction time for which
		 * @a reconstruction_tree was generated, using the half-stage rotation 
		 * reconstruction method.
		 * 
		 * @a geometry can be any type supported by GPlatesMaths::FiniteRotation as in:
		 *    operator*(GPlatesMaths::FiniteRotation, GeometryType)
		 *
		 * @a spreading_asymmetry is in the range [-1,1] where the value 0 represents half-stage
		 * rotation, the value 1 represents full-stage rotation (right plate) and the value -1
		 * represents zero stage rotation (left plate).
		 *
		 * Spreading starts at @a spreading_start_time and finishes at @a reconstruction_time.
		 * However rotation by the left plate still happens from present day to @a reconstruction_time
		 * (spreading is relative to the left plate).
		 *
		 * If present day to reconstruction time is greater than @a half_stage_rotation_interval
		 * then it will be divided into multiple half-stage intervals of this size (except for
		 * the last interval that ends at the reconstruction time).
		 * 
		 * If @a reverse_reconstruct is true then @a geometry is assumed to be
		 * at a non-present-day reconstruction time (the time at which
		 * @a reconstruction_tree was generated to rotate to) and @a geometry
		 * is then reverse rotated back to present day.
		 */
		template <class GeometryType>
		GeometryType
		reconstruct_as_half_stage(
				const GeometryType &geometry,
				const GPlatesModel::integer_plate_id_type left_plate_id,
				const GPlatesModel::integer_plate_id_type right_plate_id,
				const double &reconstruction_time,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const double &spreading_asymmetry = 0.0,
				const double &spreading_start_time = 0.0,
				const double &half_stage_rotation_interval = RotationUtils::DEFAULT_TIME_INTERVAL_HALF_STAGE_ROTATION,
				bool reverse_reconstruct = false)
		{
			// Get the composed absolute rotation needed to bring a thing on that plate
			// in the present day to this time.
			GPlatesMaths::FiniteRotation rotation =
					RotationUtils::get_half_stage_rotation(
							reconstruction_tree_creator,
							reconstruction_time,
							left_plate_id,
							right_plate_id,
							spreading_asymmetry,
							spreading_start_time,
							half_stage_rotation_interval);

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
